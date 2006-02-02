/*
 * The "user cache".
 *
 * (C) Copyright 1991-2000 Linus Torvalds
 *
 * We have a per-user structure to keep track of how many
 * processes, files etc the user has claimed, in order to be
 * able to have per-user limits for system resources. 
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/key.h>
#include <linux/interrupt.h>

/*
 * UID task count cache, to get fast user lookup in "alloc_uid"
 * when changing user ID's (ie setuid() and friends).
 */

#define UIDHASH_BITS (CONFIG_BASE_SMALL ? 3 : 8)
#define UIDHASH_SZ		(1 << UIDHASH_BITS)
#define UIDHASH_MASK		(UIDHASH_SZ - 1)
#define __uidhashfn(uid)	(((uid >> UIDHASH_BITS) + uid) & UIDHASH_MASK)
#define uidhashentry(uid)	(uidhash_table + __uidhashfn((uid)))

static kmem_cache_t *uid_cachep;
static struct list_head uidhash_table[UIDHASH_SZ];

/*
 * The uidhash_lock is mostly taken from process context, but it is
 * occasionally also taken from softirq/tasklet context, when
 * task-structs get RCU-freed. Hence all locking must be softirq-safe.
 * But free_uid() is also called with local interrupts disabled, and running
 * local_bh_enable() with local interrupts disabled is an error - we'll run
 * softirq callbacks, and they can unconditionally enable interrupts, and
 * the caller of free_uid() didn't expect that..
 */
static DEFINE_SPINLOCK(uidhash_lock);

struct user_struct root_user = {
	.__count	= ATOMIC_INIT(1),
	.processes	= ATOMIC_INIT(1),
	.files		= ATOMIC_INIT(0),
	.sigpending	= ATOMIC_INIT(0),
	.mq_bytes	= 0,
	.locked_shm     = 0,
#ifdef CONFIG_KEYS
	.uid_keyring	= &root_user_keyring,
	.session_keyring = &root_session_keyring,
#endif
};

/*
 * These routines must be called with the uidhash spinlock held!
 */
static inline void uid_hash_insert(struct user_struct *up, struct list_head *hashent)
{
	list_add(&up->uidhash_list, hashent);
}

static inline void uid_hash_remove(struct user_struct *up)
{
	list_del(&up->uidhash_list);
}

static inline struct user_struct *uid_hash_find(uid_t uid, struct list_head *hashent)
{
	struct list_head *up;

	list_for_each(up, hashent) {
		struct user_struct *user;

		user = list_entry(up, struct user_struct, uidhash_list);

		if(user->uid == uid) {
			atomic_inc(&user->__count);
			return user;
		}
	}

	return NULL;
}

/*
 * Locate the user_struct for the passed UID.  If found, take a ref on it.  The
 * caller must undo that ref with free_uid().
 *
 * If the user_struct could not be found, return NULL.
 */
struct user_struct *find_user(uid_t uid)
{
	struct user_struct *ret;
	unsigned long flags;

	spin_lock_irqsave(&uidhash_lock, flags);
	ret = uid_hash_find(uid, uidhashentry(uid));
	spin_unlock_irqrestore(&uidhash_lock, flags);
	return ret;
}

void free_uid(struct user_struct *up)
{
	unsigned long flags;

	local_irq_save(flags);
	if (up && atomic_dec_and_lock(&up->__count, &uidhash_lock)) {
		uid_hash_remove(up);
		key_put(up->uid_keyring);
		key_put(up->session_keyring);
		kmem_cache_free(uid_cachep, up);
		spin_unlock(&uidhash_lock);
	}
	local_irq_restore(flags);
}

struct user_struct * alloc_uid(uid_t uid)
{
	struct list_head *hashent = uidhashentry(uid);
	struct user_struct *up;

	spin_lock_irq(&uidhash_lock);
	up = uid_hash_find(uid, hashent);
	spin_unlock_irq(&uidhash_lock);

	if (!up) {
		struct user_struct *new;

		new = kmem_cache_alloc(uid_cachep, SLAB_KERNEL);
		if (!new)
			return NULL;
		new->uid = uid;
		atomic_set(&new->__count, 1);
		atomic_set(&new->processes, 0);
		atomic_set(&new->files, 0);
		atomic_set(&new->sigpending, 0);
#ifdef CONFIG_INOTIFY
		atomic_set(&new->inotify_watches, 0);
		atomic_set(&new->inotify_devs, 0);
#endif

		new->mq_bytes = 0;
		new->locked_shm = 0;

		if (alloc_uid_keyring(new) < 0) {
			kmem_cache_free(uid_cachep, new);
			return NULL;
		}

		/*
		 * Before adding this, check whether we raced
		 * on adding the same user already..
		 */
		spin_lock_irq(&uidhash_lock);
		up = uid_hash_find(uid, hashent);
		if (up) {
			key_put(new->uid_keyring);
			key_put(new->session_keyring);
			kmem_cache_free(uid_cachep, new);
		} else {
			uid_hash_insert(new, hashent);
			up = new;
		}
		spin_unlock_irq(&uidhash_lock);

	}
	return up;
}

void switch_uid(struct user_struct *new_user)
{
	struct user_struct *old_user;

	/* What if a process setreuid()'s and this brings the
	 * new uid over his NPROC rlimit?  We can check this now
	 * cheaply with the new uid cache, so if it matters
	 * we should be checking for it.  -DaveM
	 */
	old_user = current->user;
	atomic_inc(&new_user->processes);
	atomic_dec(&old_user->processes);
	switch_uid_keyring(new_user);
	current->user = new_user;
	free_uid(old_user);
	suid_keys(current);
}


static int __init uid_cache_init(void)
{
	int n;

	uid_cachep = kmem_cache_create("uid_cache", sizeof(struct user_struct),
			0, SLAB_HWCACHE_ALIGN|SLAB_PANIC, NULL, NULL);

	for(n = 0; n < UIDHASH_SZ; ++n)
		INIT_LIST_HEAD(uidhash_table + n);

	/* Insert the root user immediately (init already runs as root) */
	spin_lock_irq(&uidhash_lock);
	uid_hash_insert(&root_user, uidhashentry(0));
	spin_unlock_irq(&uidhash_lock);

	return 0;
}

module_init(uid_cache_init);
