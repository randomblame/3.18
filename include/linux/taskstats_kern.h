/* taskstats_kern.h - kernel header for per-task statistics interface
 *
 * Copyright (C) Shailabh Nagar, IBM Corp. 2006
 *           (C) Balbir Singh,   IBM Corp. 2006
 */

#ifndef _LINUX_TASKSTATS_KERN_H
#define _LINUX_TASKSTATS_KERN_H

#include <linux/taskstats.h>
#include <linux/sched.h>
#include <net/genetlink.h>

enum {
	TASKSTATS_MSG_UNICAST,		/* send data only to requester */
	TASKSTATS_MSG_MULTICAST,	/* send data to a group */
};

#ifdef CONFIG_TASKSTATS
extern kmem_cache_t *taskstats_cache;
extern struct mutex taskstats_exit_mutex;

static inline int taskstats_has_listeners(void)
{
	if (!genl_sock)
		return 0;
	return netlink_has_listeners(genl_sock, TASKSTATS_LISTEN_GROUP);
}


static inline void taskstats_exit_alloc(struct taskstats **ptidstats)
{
	*ptidstats = NULL;
	if (taskstats_has_listeners())
		*ptidstats = kmem_cache_zalloc(taskstats_cache, SLAB_KERNEL);
}

static inline void taskstats_exit_free(struct taskstats *tidstats)
{
	if (tidstats)
		kmem_cache_free(taskstats_cache, tidstats);
}

static inline void taskstats_tgid_init(struct signal_struct *sig)
{
	spin_lock_init(&sig->stats_lock);
	sig->stats = NULL;
}

static inline void taskstats_tgid_alloc(struct signal_struct *sig)
{
	struct taskstats *stats;
	unsigned long flags;

	stats = kmem_cache_zalloc(taskstats_cache, SLAB_KERNEL);
	if (!stats)
		return;

	spin_lock_irqsave(&sig->stats_lock, flags);
	if (!sig->stats) {
		sig->stats = stats;
		stats = NULL;
	}
	spin_unlock_irqrestore(&sig->stats_lock, flags);

	if (stats)
		kmem_cache_free(taskstats_cache, stats);
}

static inline void taskstats_tgid_free(struct signal_struct *sig)
{
	struct taskstats *stats = NULL;
	unsigned long flags;

	spin_lock_irqsave(&sig->stats_lock, flags);
	if (sig->stats) {
		stats = sig->stats;
		sig->stats = NULL;
	}
	spin_unlock_irqrestore(&sig->stats_lock, flags);
	if (stats)
		kmem_cache_free(taskstats_cache, stats);
}

extern void taskstats_exit_send(struct task_struct *, struct taskstats *, int);
extern void taskstats_init_early(void);
extern void taskstats_tgid_alloc(struct signal_struct *);
#else
static inline void taskstats_exit_alloc(struct taskstats **ptidstats)
{}
static inline void taskstats_exit_free(struct taskstats *ptidstats)
{}
static inline void taskstats_exit_send(struct task_struct *tsk,
				       struct taskstats *tidstats,
				       int group_dead)
{}
static inline void taskstats_tgid_init(struct signal_struct *sig)
{}
static inline void taskstats_tgid_alloc(struct signal_struct *sig)
{}
static inline void taskstats_tgid_free(struct signal_struct *sig)
{}
static inline void taskstats_init_early(void)
{}
#endif /* CONFIG_TASKSTATS */

#endif

