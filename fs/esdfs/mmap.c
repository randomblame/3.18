/*
 * Copyright (c) 1998-2014 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2014 Stony Brook University
 * Copyright (c) 2003-2014 The Research Foundation of SUNY
 * Copyright (C) 2013-2014, 2016 Motorola Mobility, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "esdfs.h"

static int esdfs_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	int err;
	struct file *file;
	const struct vm_operations_struct *lower_vm_ops;
	struct esdfs_sb_info *sbi;
	const struct cred *creds;

	file = (struct file *)vma->vm_private_data;
	sbi = ESDFS_SB(file->f_path.dentry->d_sb);
	creds = esdfs_override_creds(sbi, NULL);
	if (!creds)
		return -ENOMEM;

	lower_vm_ops = ESDFS_F(file)->lower_vm_ops;
	BUG_ON(!lower_vm_ops);
	err = lower_vm_ops->fault(vma, vmf);
	esdfs_revert_creds(creds, NULL);
	return err;
}

static int esdfs_page_mkwrite(struct vm_area_struct *vma,
			       struct vm_fault *vmf)
{
	int err = 0;
	struct file *file;
	const struct vm_operations_struct *lower_vm_ops;
	struct esdfs_sb_info *sbi;
	const struct cred *creds;

	file = (struct file *)vma->vm_private_data;
	sbi = ESDFS_SB(file->f_path.dentry->d_sb);
	creds = esdfs_override_creds(sbi, NULL);
	if (!creds)
		return -ENOMEM;

	lower_vm_ops = ESDFS_F(file)->lower_vm_ops;
	BUG_ON(!lower_vm_ops);
	if (!lower_vm_ops->page_mkwrite)
		goto out;

	err = lower_vm_ops->page_mkwrite(vma, vmf);
out:
	esdfs_revert_creds(creds, NULL);
	return err;
}

static ssize_t esdfs_direct_IO(int rw, struct kiocb *iocb,
				struct iov_iter *iter, loff_t pos)
{
	/*
	 * This function should never be called directly.  We need it
	 * to exist, to get past a check in open_check_o_direct(),
	 * which is called from do_last().
	 */
	return -EINVAL;
}

const struct address_space_operations esdfs_aops = {
	.direct_IO = esdfs_direct_IO,
};

const struct vm_operations_struct esdfs_vm_ops = {
	.fault		= esdfs_fault,
	.page_mkwrite	= esdfs_page_mkwrite,
};
