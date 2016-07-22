/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/pagemap.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/hash.h>
#include <linux/msm_ion.h>
#include <soc/qcom/smd.h>
#include <soc/qcom/subsystem_notif.h>
#include <linux/msm_iommu_domains.h>
#include <linux/scatterlist.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/dma-contiguous.h>
#include <linux/iommu.h>
#include <linux/kref.h>
#include <linux/debugfs.h>

#include "smd_ts.h"

static struct smd_ts_apps ts_diff;
/* called in smd_irq, as irq is off, it doesnot need spinlock */
static void ts_smdl_transfer(struct smd_ts_apps *ts_app)
{
	int read_len;
	int avail_len;

	avail_len = smd_read_avail(ts_app->chan);
	/* avail_len should be aligned to sizeof(uint64_t) */
	if (avail_len <= 0 || avail_len & (sizeof(uint64_t) - 1)) {
		pr_err("invalid avail length %d\n", avail_len);
		return;
	}

	while (avail_len > 0) {
		/* buf_ptr always point to the latest data to write */
		ts_app->buf_ptr--;
		if (ts_app->buf_ptr < ts_app->ts_buf)
			ts_app->buf_ptr = ts_app->ts_buf + ts_app->buf_len - 1;

		read_len = smd_read_from_cb(ts_app->chan, ts_app->buf_ptr,
					sizeof(uint64_t));
		if (read_len != sizeof(uint64_t)) {
			pr_err("invaild read\n");
			return;
		}
		ts_app->ready_buf_len++;

		if (ts_app->ready_buf_len > ts_app->buf_len)
			ts_app->ready_buf_len = ts_app->buf_len;

		avail_len -= sizeof(uint64_t);
	}
	/* complete who wait for me */
	if (!completion_done(&ts_app->work))
		complete(&ts_app->work);
}

/* event handler of timestamp_diff, called in smd_irq */
static void ts_diff_handler(void *priv, unsigned event)
{
	struct completion *work = &ts_diff.work;

	switch (event) {
	case SMD_EVENT_OPEN:
		complete(work);
		break;

	case SMD_EVENT_CLOSE:
		break;

	case SMD_EVENT_DATA:
		ts_smdl_transfer(&ts_diff);
		break;
	}
}

/* open channel and wait for completion */
static int ts_smdl_open(struct smd_ts_apps *ts_app)
{
	int ret;

	if (ts_app->chan)
		return 0;

	/* open timestamp's smd channel */
	ret = smd_named_open_on_edge(ts_app->dev_name, TS_CHANNEL_TYPE,
		&ts_app->chan, ts_app, ts_app->event_handler);
	if (ret) {
		pr_err("open channel %s failed\n", ts_app->dev_name);
		return ret;
	}

	/* wait for DSP---AP finish communication */
	ret = wait_for_completion_timeout(&ts_app->work, 5*HZ);
	if (ret == 0) {
		pr_err("%s wait for completion failed\n", ts_app->dev_name);
		smd_close(ts_app->chan);
		return ret;
	}

	return 0;
}

/* read ts to ts_buf, and copy ts_buf to user_buf */
static int ts_buf_read(struct smd_ts_apps *ts_app, char *user_buf, size_t count)
{
	int ret = 0;
	unsigned int length;
	unsigned long flags;
	uint64_t *buf_tmp;

	if (count == 0)
		return 0;

	spin_lock_irqsave(&ts_app->lock, flags);

	/* if none data ready, wait until at least one ready;
	 * it should release the lock, and get the lock after completion. */
	if (ts_app->ready_buf_len == 0) {
		init_completion(&ts_app->work);
		spin_unlock_irqrestore(&ts_app->lock, flags);
		wait_for_completion(&ts_app->work);
		spin_lock_irqsave(&ts_app->lock, flags);
	}

	if (count > (ts_app->ready_buf_len * sizeof(uint64_t)))
		count = ts_app->ready_buf_len * sizeof(uint64_t);

	ts_app->ready_buf_len -= count / sizeof(uint64_t);

	buf_tmp = ts_app->buf_ptr;
	/* buf structure
	 * ts_buf******buf_ptr******ts_buf+buf_len */
	length = (ts_app->ts_buf + ts_app->buf_len - ts_app->buf_ptr)
			* sizeof(uint64_t);
	if (count <= length) {
		ts_app->buf_ptr += count / sizeof(uint64_t) - 1;
	} else {
		ts_app->buf_ptr = ts_app->ts_buf +
				(count - length) / sizeof(uint64_t) - 1;
	}

	spin_unlock_irqrestore(&ts_app->lock, flags);

	/* do memcpy outof spinlock_irq */
	if (count <= length) {
		ret = copy_to_user(user_buf, buf_tmp, count);
	} else {
		/* copy from buf_ptr */
		ret = copy_to_user(user_buf, ts_app->buf_ptr, length);
		/* copy from the begin of ts_buf */
		ret += copy_to_user(user_buf + length, ts_app->ts_buf,
				(count - length));
	}

	if (ret)
		return -EFAULT;
	return count;
}

static int ts_diff_open(struct inode *inode, struct file *file)
{
	if (inode->i_private)
		file->private_data = inode->i_private;
	return ts_smdl_open(&ts_diff);
}

static ssize_t ts_diff_read(struct file *file, char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	return ts_buf_read(&ts_diff, user_buf, count);
}
static int ts_diff_release(struct inode *inode, struct file *file)
{
	return smd_close(ts_diff.chan);
}

/* alloc dev and open channel */
static int smd_ts_apps_init(struct smd_ts_apps *ts_app)
{
	int ret = -1;
	unsigned int length = sizeof(uint64_t) * ts_app->buf_len;

	/* malloc buf for list_buf */
	ts_app->ts_buf = kmalloc(length, GFP_KERNEL);
	if (!ts_app->ts_buf)
		return -ENOMEM;
	memset(ts_app->ts_buf, 0, sizeof(uint64_t) * ts_app->buf_len);
	ts_app->buf_ptr = ts_app->ts_buf;

	init_completion(&ts_app->work);
	spin_lock_init(&ts_app->lock);

	ret = alloc_chrdev_region(&ts_app->dev_no, 0, 1, ts_app->dev_name);
	if (ret != 0)
		goto alloc_chrdev_fail;
	cdev_init(&ts_app->cdev, ts_app->fops);
	ts_app->cdev.owner = THIS_MODULE;

	ret = cdev_add(&ts_app->cdev, MKDEV(MAJOR(ts_app->dev_no), 0), 1);
	if (ret != 0)
		goto cdev_add_fail;

	ts_app->class = class_create(THIS_MODULE, ts_app->dev_name);
	if (IS_ERR(ts_app->class))
		goto class_create_fail;

	ts_app->dev = device_create(ts_app->class, NULL,
					MKDEV(MAJOR(ts_app->dev_no), 0),
					NULL, ts_app->dev_name);
	if (IS_ERR(ts_app->dev))
		goto device_create_fail;

	return 0;

device_create_fail:
	class_destroy(ts_app->class);
class_create_fail:
	cdev_del(&ts_app->cdev);
cdev_add_fail:
	unregister_chrdev_region(ts_app->dev_no, 1);
alloc_chrdev_fail:
	kfree(ts_app->ts_buf);

	return ret;
}

static const struct file_operations ts_diff_fops = {
	.open = ts_diff_open,
	.read = ts_diff_read,
	.release = ts_diff_release,
};

/* close smd and free device */
static void smd_ts_apps_deinit(struct smd_ts_apps *ts_app)
{
	if (ts_app->chan)
		smd_close(ts_app->chan);
	class_destroy(ts_app->class);
	cdev_del(&ts_app->cdev);
	unregister_chrdev_region(ts_app->dev_no, 1);
	kfree(ts_app->ts_buf);
}

static int __init smd_ts_init(void)
{
	int ret;

	ts_diff.buf_len = TS_DIFF_BUF_NUM,
	ts_diff.dev_name = TS_DIFF_PORT_NAME,
	ts_diff.fops = &ts_diff_fops,
	ts_diff.event_handler = ts_diff_handler,

	ret = smd_ts_apps_init(&ts_diff);
	if (ret != 0) {
		pr_err("failed to init ts_diff\n");
		return ret;
	}

	return 0;
}

static void __exit smd_ts_exit(void)
{
	smd_ts_apps_deinit(&ts_diff);
}

module_init(smd_ts_init);
module_exit(smd_ts_exit);

MODULE_LICENSE("GPL v2");
