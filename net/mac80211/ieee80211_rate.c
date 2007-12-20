/*
 * Copyright 2002-2005, Instant802 Networks, Inc.
 * Copyright 2005-2006, Devicescape Software, Inc.
 * Copyright (c) 2006 Jiri Benc <jbenc@suse.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/rtnetlink.h>
#include "ieee80211_rate.h"
#include "ieee80211_i.h"

struct rate_control_alg {
	struct list_head list;
	struct rate_control_ops *ops;
};

static LIST_HEAD(rate_ctrl_algs);
static DEFINE_MUTEX(rate_ctrl_mutex);

int ieee80211_rate_control_register(struct rate_control_ops *ops)
{
	struct rate_control_alg *alg;

	if (!ops->name)
		return -EINVAL;

	mutex_lock(&rate_ctrl_mutex);
	list_for_each_entry(alg, &rate_ctrl_algs, list) {
		if (!strcmp(alg->ops->name, ops->name)) {
			/* don't register an algorithm twice */
			WARN_ON(1);
			mutex_unlock(&rate_ctrl_mutex);
			return -EALREADY;
		}
	}

	alg = kzalloc(sizeof(*alg), GFP_KERNEL);
	if (alg == NULL) {
		mutex_unlock(&rate_ctrl_mutex);
		return -ENOMEM;
	}
	alg->ops = ops;

	list_add_tail(&alg->list, &rate_ctrl_algs);
	mutex_unlock(&rate_ctrl_mutex);

	return 0;
}
EXPORT_SYMBOL(ieee80211_rate_control_register);

void ieee80211_rate_control_unregister(struct rate_control_ops *ops)
{
	struct rate_control_alg *alg;

	mutex_lock(&rate_ctrl_mutex);
	list_for_each_entry(alg, &rate_ctrl_algs, list) {
		if (alg->ops == ops) {
			list_del(&alg->list);
			kfree(alg);
			break;
		}
	}
	mutex_unlock(&rate_ctrl_mutex);
}
EXPORT_SYMBOL(ieee80211_rate_control_unregister);

static struct rate_control_ops *
ieee80211_try_rate_control_ops_get(const char *name)
{
	struct rate_control_alg *alg;
	struct rate_control_ops *ops = NULL;

	if (!name)
		return NULL;

	mutex_lock(&rate_ctrl_mutex);
	list_for_each_entry(alg, &rate_ctrl_algs, list) {
		if (!strcmp(alg->ops->name, name))
			if (try_module_get(alg->ops->module)) {
				ops = alg->ops;
				break;
			}
	}
	mutex_unlock(&rate_ctrl_mutex);
	return ops;
}

/* Get the rate control algorithm. If `name' is NULL, get the first
 * available algorithm. */
static struct rate_control_ops *
ieee80211_rate_control_ops_get(const char *name)
{
	struct rate_control_ops *ops;

	if (!name)
		name = "simple";

	ops = ieee80211_try_rate_control_ops_get(name);
	if (!ops) {
		request_module("rc80211_%s", name);
		ops = ieee80211_try_rate_control_ops_get(name);
	}
	return ops;
}

static void ieee80211_rate_control_ops_put(struct rate_control_ops *ops)
{
	module_put(ops->module);
}

struct rate_control_ref *rate_control_alloc(const char *name,
					    struct ieee80211_local *local)
{
	struct rate_control_ref *ref;

	ref = kmalloc(sizeof(struct rate_control_ref), GFP_KERNEL);
	if (!ref)
		goto fail_ref;
	kref_init(&ref->kref);
	ref->ops = ieee80211_rate_control_ops_get(name);
	if (!ref->ops)
		goto fail_ops;
	ref->priv = ref->ops->alloc(local);
	if (!ref->priv)
		goto fail_priv;
	return ref;

fail_priv:
	ieee80211_rate_control_ops_put(ref->ops);
fail_ops:
	kfree(ref);
fail_ref:
	return NULL;
}

static void rate_control_release(struct kref *kref)
{
	struct rate_control_ref *ctrl_ref;

	ctrl_ref = container_of(kref, struct rate_control_ref, kref);
	ctrl_ref->ops->free(ctrl_ref->priv);
	ieee80211_rate_control_ops_put(ctrl_ref->ops);
	kfree(ctrl_ref);
}

void rate_control_get_rate(struct net_device *dev,
			   struct ieee80211_hw_mode *mode, struct sk_buff *skb,
			   struct rate_selection *sel)
{
	struct ieee80211_local *local = wdev_priv(dev->ieee80211_ptr);
	struct rate_control_ref *ref = local->rate_ctrl;
	struct ieee80211_sub_if_data *sdata;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
	struct sta_info *sta = sta_info_get(local, hdr->addr1);
	int i;
	u16 fc;

	memset(sel, 0, sizeof(struct rate_selection));

	/* Send management frames and broadcast/multicast data using lowest
	 * rate. */
	fc = le16_to_cpu(hdr->frame_control);
	if ((fc & IEEE80211_FCTL_FTYPE) != IEEE80211_FTYPE_DATA ||
	    is_multicast_ether_addr(hdr->addr1))
		sel->rate = rate_lowest(local, mode, sta);

	/* If a forced rate is in effect, select it. */
	sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	if (sdata->bss && sdata->bss->force_unicast_rateidx > -1)
		sel->rate = &mode->rates[sdata->bss->force_unicast_rateidx];

	/* If we haven't found the rate yet, ask the rate control algo. */
	if (!sel->rate)
		ref->ops->get_rate(ref->priv, dev, mode, skb, sel);

	/* Select a non-ERP backup rate. */
	if (!sel->nonerp) {
		for (i = 0; i < mode->num_rates - 1; i++) {
			struct ieee80211_rate *rate = &mode->rates[i];
			if (sel->rate->rate < rate->rate)
				break;

			if (rate_supported(sta, mode, i) &&
			    !(rate->flags & IEEE80211_RATE_ERP))
				sel->nonerp = rate;
		}
	}

	if (sta)
		sta_info_put(sta);
}

struct rate_control_ref *rate_control_get(struct rate_control_ref *ref)
{
	kref_get(&ref->kref);
	return ref;
}

void rate_control_put(struct rate_control_ref *ref)
{
	kref_put(&ref->kref, rate_control_release);
}

int ieee80211_init_rate_ctrl_alg(struct ieee80211_local *local,
				 const char *name)
{
	struct rate_control_ref *ref, *old;

	ASSERT_RTNL();
	if (local->open_count || netif_running(local->mdev))
		return -EBUSY;

	ref = rate_control_alloc(name, local);
	if (!ref) {
		printk(KERN_WARNING "%s: Failed to select rate control "
		       "algorithm\n", wiphy_name(local->hw.wiphy));
		return -ENOENT;
	}

	old = local->rate_ctrl;
	local->rate_ctrl = ref;
	if (old) {
		rate_control_put(old);
		sta_info_flush(local, NULL);
	}

	printk(KERN_DEBUG "%s: Selected rate control "
	       "algorithm '%s'\n", wiphy_name(local->hw.wiphy),
	       ref->ops->name);


	return 0;
}

void rate_control_deinitialize(struct ieee80211_local *local)
{
	struct rate_control_ref *ref;

	ref = local->rate_ctrl;
	local->rate_ctrl = NULL;
	rate_control_put(ref);
}
