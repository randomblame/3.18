/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2005  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#include "fuse_i.h"

#include <linux/pagemap.h>
#include <linux/file.h>
#include <linux/gfp.h>
#include <linux/sched.h>
#include <linux/namei.h>

static inline unsigned long time_to_jiffies(unsigned long sec,
					    unsigned long nsec)
{
	struct timespec ts = {sec, nsec};
	return jiffies + timespec_to_jiffies(&ts);
}

static void fuse_lookup_init(struct fuse_req *req, struct inode *dir,
			     struct dentry *entry,
			     struct fuse_entry_out *outarg)
{
	req->in.h.opcode = FUSE_LOOKUP;
	req->in.h.nodeid = get_node_id(dir);
	req->inode = dir;
	req->in.numargs = 1;
	req->in.args[0].size = entry->d_name.len + 1;
	req->in.args[0].value = entry->d_name.name;
	req->out.numargs = 1;
	req->out.args[0].size = sizeof(struct fuse_entry_out);
	req->out.args[0].value = outarg;
}

static int fuse_dentry_revalidate(struct dentry *entry, struct nameidata *nd)
{
	if (!entry->d_inode || is_bad_inode(entry->d_inode))
		return 0;
	else if (time_after(jiffies, entry->d_time)) {
		int err;
		struct fuse_entry_out outarg;
		struct inode *inode = entry->d_inode;
		struct fuse_inode *fi = get_fuse_inode(inode);
		struct fuse_conn *fc = get_fuse_conn(inode);
		struct fuse_req *req = fuse_get_request_nonint(fc);
		if (!req)
			return 0;

		fuse_lookup_init(req, entry->d_parent->d_inode, entry, &outarg);
		request_send_nonint(fc, req);
		err = req->out.h.error;
		if (!err) {
			if (outarg.nodeid != get_node_id(inode)) {
				fuse_send_forget(fc, req, outarg.nodeid, 1);
				return 0;
			}
			fi->nlookup ++;
		}
		fuse_put_request(fc, req);
		if (err || (outarg.attr.mode ^ inode->i_mode) & S_IFMT)
			return 0;

		fuse_change_attributes(inode, &outarg.attr);
		entry->d_time = time_to_jiffies(outarg.entry_valid,
						outarg.entry_valid_nsec);
		fi->i_time = time_to_jiffies(outarg.attr_valid,
					     outarg.attr_valid_nsec);
	}
	return 1;
}

static struct dentry_operations fuse_dentry_operations = {
	.d_revalidate	= fuse_dentry_revalidate,
};

static int fuse_lookup_iget(struct inode *dir, struct dentry *entry,
			    struct inode **inodep)
{
	int err;
	struct fuse_entry_out outarg;
	struct inode *inode = NULL;
	struct fuse_conn *fc = get_fuse_conn(dir);
	struct fuse_req *req;

	if (entry->d_name.len > FUSE_NAME_MAX)
		return -ENAMETOOLONG;

	req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	fuse_lookup_init(req, dir, entry, &outarg);
	request_send(fc, req);
	err = req->out.h.error;
	if (!err) {
		inode = fuse_iget(dir->i_sb, outarg.nodeid, outarg.generation,
				  &outarg.attr);
		if (!inode) {
			fuse_send_forget(fc, req, outarg.nodeid, 1);
			return -ENOMEM;
		}
	}
	fuse_put_request(fc, req);
	if (err && err != -ENOENT)
		return err;

	if (inode) {
		struct fuse_inode *fi = get_fuse_inode(inode);
		entry->d_time =	time_to_jiffies(outarg.entry_valid,
						outarg.entry_valid_nsec);
		fi->i_time = time_to_jiffies(outarg.attr_valid,
					     outarg.attr_valid_nsec);
	}

	entry->d_op = &fuse_dentry_operations;
	*inodep = inode;
	return 0;
}

void fuse_invalidate_attr(struct inode *inode)
{
	get_fuse_inode(inode)->i_time = jiffies - 1;
}

static void fuse_invalidate_entry(struct dentry *entry)
{
	d_invalidate(entry);
	entry->d_time = jiffies - 1;
}

static int create_new_entry(struct fuse_conn *fc, struct fuse_req *req,
			    struct inode *dir, struct dentry *entry,
			    int mode)
{
	struct fuse_entry_out outarg;
	struct inode *inode;
	struct fuse_inode *fi;
	int err;

	req->in.h.nodeid = get_node_id(dir);
	req->inode = dir;
	req->out.numargs = 1;
	req->out.args[0].size = sizeof(outarg);
	req->out.args[0].value = &outarg;
	request_send(fc, req);
	err = req->out.h.error;
	if (err) {
		fuse_put_request(fc, req);
		return err;
	}
	inode = fuse_iget(dir->i_sb, outarg.nodeid, outarg.generation,
			  &outarg.attr);
	if (!inode) {
		fuse_send_forget(fc, req, outarg.nodeid, 1);
		return -ENOMEM;
	}
	fuse_put_request(fc, req);

	/* Don't allow userspace to do really stupid things... */
	if ((inode->i_mode ^ mode) & S_IFMT) {
		iput(inode);
		return -EIO;
	}

	entry->d_time = time_to_jiffies(outarg.entry_valid,
					outarg.entry_valid_nsec);

	fi = get_fuse_inode(inode);
	fi->i_time = time_to_jiffies(outarg.attr_valid,
				     outarg.attr_valid_nsec);

	d_instantiate(entry, inode);
	fuse_invalidate_attr(dir);
	return 0;
}

static int fuse_mknod(struct inode *dir, struct dentry *entry, int mode,
		      dev_t rdev)
{
	struct fuse_mknod_in inarg;
	struct fuse_conn *fc = get_fuse_conn(dir);
	struct fuse_req *req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	memset(&inarg, 0, sizeof(inarg));
	inarg.mode = mode;
	inarg.rdev = new_encode_dev(rdev);
	req->in.h.opcode = FUSE_MKNOD;
	req->in.numargs = 2;
	req->in.args[0].size = sizeof(inarg);
	req->in.args[0].value = &inarg;
	req->in.args[1].size = entry->d_name.len + 1;
	req->in.args[1].value = entry->d_name.name;
	return create_new_entry(fc, req, dir, entry, mode);
}

static int fuse_create(struct inode *dir, struct dentry *entry, int mode,
		       struct nameidata *nd)
{
	return fuse_mknod(dir, entry, mode, 0);
}

static int fuse_mkdir(struct inode *dir, struct dentry *entry, int mode)
{
	struct fuse_mkdir_in inarg;
	struct fuse_conn *fc = get_fuse_conn(dir);
	struct fuse_req *req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	memset(&inarg, 0, sizeof(inarg));
	inarg.mode = mode;
	req->in.h.opcode = FUSE_MKDIR;
	req->in.numargs = 2;
	req->in.args[0].size = sizeof(inarg);
	req->in.args[0].value = &inarg;
	req->in.args[1].size = entry->d_name.len + 1;
	req->in.args[1].value = entry->d_name.name;
	return create_new_entry(fc, req, dir, entry, S_IFDIR);
}

static int fuse_symlink(struct inode *dir, struct dentry *entry,
			const char *link)
{
	struct fuse_conn *fc = get_fuse_conn(dir);
	unsigned len = strlen(link) + 1;
	struct fuse_req *req;

	if (len > FUSE_SYMLINK_MAX)
		return -ENAMETOOLONG;

	req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	req->in.h.opcode = FUSE_SYMLINK;
	req->in.numargs = 2;
	req->in.args[0].size = entry->d_name.len + 1;
	req->in.args[0].value = entry->d_name.name;
	req->in.args[1].size = len;
	req->in.args[1].value = link;
	return create_new_entry(fc, req, dir, entry, S_IFLNK);
}

static int fuse_unlink(struct inode *dir, struct dentry *entry)
{
	int err;
	struct fuse_conn *fc = get_fuse_conn(dir);
	struct fuse_req *req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	req->in.h.opcode = FUSE_UNLINK;
	req->in.h.nodeid = get_node_id(dir);
	req->inode = dir;
	req->in.numargs = 1;
	req->in.args[0].size = entry->d_name.len + 1;
	req->in.args[0].value = entry->d_name.name;
	request_send(fc, req);
	err = req->out.h.error;
	fuse_put_request(fc, req);
	if (!err) {
		struct inode *inode = entry->d_inode;

		/* Set nlink to zero so the inode can be cleared, if
                   the inode does have more links this will be
                   discovered at the next lookup/getattr */
		inode->i_nlink = 0;
		fuse_invalidate_attr(inode);
		fuse_invalidate_attr(dir);
	} else if (err == -EINTR)
		fuse_invalidate_entry(entry);
	return err;
}

static int fuse_rmdir(struct inode *dir, struct dentry *entry)
{
	int err;
	struct fuse_conn *fc = get_fuse_conn(dir);
	struct fuse_req *req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	req->in.h.opcode = FUSE_RMDIR;
	req->in.h.nodeid = get_node_id(dir);
	req->inode = dir;
	req->in.numargs = 1;
	req->in.args[0].size = entry->d_name.len + 1;
	req->in.args[0].value = entry->d_name.name;
	request_send(fc, req);
	err = req->out.h.error;
	fuse_put_request(fc, req);
	if (!err) {
		entry->d_inode->i_nlink = 0;
		fuse_invalidate_attr(dir);
	} else if (err == -EINTR)
		fuse_invalidate_entry(entry);
	return err;
}

static int fuse_rename(struct inode *olddir, struct dentry *oldent,
		       struct inode *newdir, struct dentry *newent)
{
	int err;
	struct fuse_rename_in inarg;
	struct fuse_conn *fc = get_fuse_conn(olddir);
	struct fuse_req *req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	memset(&inarg, 0, sizeof(inarg));
	inarg.newdir = get_node_id(newdir);
	req->in.h.opcode = FUSE_RENAME;
	req->in.h.nodeid = get_node_id(olddir);
	req->inode = olddir;
	req->inode2 = newdir;
	req->in.numargs = 3;
	req->in.args[0].size = sizeof(inarg);
	req->in.args[0].value = &inarg;
	req->in.args[1].size = oldent->d_name.len + 1;
	req->in.args[1].value = oldent->d_name.name;
	req->in.args[2].size = newent->d_name.len + 1;
	req->in.args[2].value = newent->d_name.name;
	request_send(fc, req);
	err = req->out.h.error;
	fuse_put_request(fc, req);
	if (!err) {
		fuse_invalidate_attr(olddir);
		if (olddir != newdir)
			fuse_invalidate_attr(newdir);
	} else if (err == -EINTR) {
		/* If request was interrupted, DEITY only knows if the
		   rename actually took place.  If the invalidation
		   fails (e.g. some process has CWD under the renamed
		   directory), then there can be inconsistency between
		   the dcache and the real filesystem.  Tough luck. */
		fuse_invalidate_entry(oldent);
		if (newent->d_inode)
			fuse_invalidate_entry(newent);
	}

	return err;
}

static int fuse_link(struct dentry *entry, struct inode *newdir,
		     struct dentry *newent)
{
	int err;
	struct fuse_link_in inarg;
	struct inode *inode = entry->d_inode;
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct fuse_req *req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	memset(&inarg, 0, sizeof(inarg));
	inarg.oldnodeid = get_node_id(inode);
	req->in.h.opcode = FUSE_LINK;
	req->inode2 = inode;
	req->in.numargs = 2;
	req->in.args[0].size = sizeof(inarg);
	req->in.args[0].value = &inarg;
	req->in.args[1].size = newent->d_name.len + 1;
	req->in.args[1].value = newent->d_name.name;
	err = create_new_entry(fc, req, newdir, newent, inode->i_mode);
	/* Contrary to "normal" filesystems it can happen that link
	   makes two "logical" inodes point to the same "physical"
	   inode.  We invalidate the attributes of the old one, so it
	   will reflect changes in the backing inode (link count,
	   etc.)
	*/
	if (!err || err == -EINTR)
		fuse_invalidate_attr(inode);
	return err;
}

int fuse_do_getattr(struct inode *inode)
{
	int err;
	struct fuse_attr_out arg;
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct fuse_req *req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	req->in.h.opcode = FUSE_GETATTR;
	req->in.h.nodeid = get_node_id(inode);
	req->inode = inode;
	req->out.numargs = 1;
	req->out.args[0].size = sizeof(arg);
	req->out.args[0].value = &arg;
	request_send(fc, req);
	err = req->out.h.error;
	fuse_put_request(fc, req);
	if (!err) {
		if ((inode->i_mode ^ arg.attr.mode) & S_IFMT) {
			make_bad_inode(inode);
			err = -EIO;
		} else {
			struct fuse_inode *fi = get_fuse_inode(inode);
			fuse_change_attributes(inode, &arg.attr);
			fi->i_time = time_to_jiffies(arg.attr_valid,
						     arg.attr_valid_nsec);
		}
	}
	return err;
}

/*
 * Calling into a user-controlled filesystem gives the filesystem
 * daemon ptrace-like capabilities over the requester process.  This
 * means, that the filesystem daemon is able to record the exact
 * filesystem operations performed, and can also control the behavior
 * of the requester process in otherwise impossible ways.  For example
 * it can delay the operation for arbitrary length of time allowing
 * DoS against the requester.
 *
 * For this reason only those processes can call into the filesystem,
 * for which the owner of the mount has ptrace privilege.  This
 * excludes processes started by other users, suid or sgid processes.
 */
static int fuse_allow_task(struct fuse_conn *fc, struct task_struct *task)
{
	if (fc->flags & FUSE_ALLOW_OTHER)
		return 1;

	if (task->euid == fc->user_id &&
	    task->suid == fc->user_id &&
	    task->uid == fc->user_id &&
	    task->egid == fc->group_id &&
	    task->sgid == fc->group_id &&
	    task->gid == fc->group_id)
		return 1;

	return 0;
}

static int fuse_revalidate(struct dentry *entry)
{
	struct inode *inode = entry->d_inode;
	struct fuse_inode *fi = get_fuse_inode(inode);
	struct fuse_conn *fc = get_fuse_conn(inode);

	if (!fuse_allow_task(fc, current))
		return -EACCES;
	if (get_node_id(inode) != FUSE_ROOT_ID &&
	    time_before_eq(jiffies, fi->i_time))
		return 0;

	return fuse_do_getattr(inode);
}

static int fuse_permission(struct inode *inode, int mask, struct nameidata *nd)
{
	struct fuse_conn *fc = get_fuse_conn(inode);

	if (!fuse_allow_task(fc, current))
		return -EACCES;
	else if (fc->flags & FUSE_DEFAULT_PERMISSIONS) {
		int err = generic_permission(inode, mask, NULL);

		/* If permission is denied, try to refresh file
		   attributes.  This is also needed, because the root
		   node will at first have no permissions */
		if (err == -EACCES) {
		 	err = fuse_do_getattr(inode);
			if (!err)
				err = generic_permission(inode, mask, NULL);
		}

		/* FIXME: Need some mechanism to revoke permissions:
		   currently if the filesystem suddenly changes the
		   file mode, we will not be informed about it, and
		   continue to allow access to the file/directory.

		   This is actually not so grave, since the user can
		   simply keep access to the file/directory anyway by
		   keeping it open... */

		return err;
	} else {
		int mode = inode->i_mode;
		if ((mask & MAY_WRITE) && IS_RDONLY(inode) &&
                    (S_ISREG(mode) || S_ISDIR(mode) || S_ISLNK(mode)))
                        return -EROFS;
		if ((mask & MAY_EXEC) && !S_ISDIR(mode) && !(mode & S_IXUGO))
			return -EACCES;
		return 0;
	}
}

static int parse_dirfile(char *buf, size_t nbytes, struct file *file,
			 void *dstbuf, filldir_t filldir)
{
	while (nbytes >= FUSE_NAME_OFFSET) {
		struct fuse_dirent *dirent = (struct fuse_dirent *) buf;
		size_t reclen = FUSE_DIRENT_SIZE(dirent);
		int over;
		if (!dirent->namelen || dirent->namelen > FUSE_NAME_MAX)
			return -EIO;
		if (reclen > nbytes)
			break;

		over = filldir(dstbuf, dirent->name, dirent->namelen,
			       file->f_pos, dirent->ino, dirent->type);
		if (over)
			break;

		buf += reclen;
		nbytes -= reclen;
		file->f_pos = dirent->off;
	}

	return 0;
}

static int fuse_checkdir(struct file *cfile, struct file *file)
{
	struct inode *inode;
	if (!cfile)
		return -EIO;
	inode = cfile->f_dentry->d_inode;
	if (!S_ISREG(inode->i_mode)) {
		fput(cfile);
		return -EIO;
	}

	file->private_data = cfile;
	return 0;
}

static int fuse_getdir(struct file *file)
{
	struct inode *inode = file->f_dentry->d_inode;
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct fuse_req *req = fuse_get_request(fc);
	struct fuse_getdir_out_i outarg;
	int err;

	if (!req)
		return -ERESTARTNOINTR;

	req->in.h.opcode = FUSE_GETDIR;
	req->in.h.nodeid = get_node_id(inode);
	req->inode = inode;
	req->out.numargs = 1;
	req->out.args[0].size = sizeof(struct fuse_getdir_out);
	req->out.args[0].value = &outarg;
	request_send(fc, req);
	err = req->out.h.error;
	fuse_put_request(fc, req);
	if (!err)
		err = fuse_checkdir(outarg.file, file);
	return err;
}

static int fuse_readdir(struct file *file, void *dstbuf, filldir_t filldir)
{
	struct file *cfile = file->private_data;
	char *buf;
	int ret;

	if (!cfile) {
		ret = fuse_getdir(file);
		if (ret)
			return ret;

		cfile = file->private_data;
	}

	buf = (char *) __get_free_page(GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = kernel_read(cfile, file->f_pos, buf, PAGE_SIZE);
	if (ret > 0)
		ret = parse_dirfile(buf, ret, file, dstbuf, filldir);

	free_page((unsigned long) buf);
	return ret;
}

static char *read_link(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct fuse_req *req = fuse_get_request(fc);
	char *link;

	if (!req)
		return ERR_PTR(-ERESTARTNOINTR);

	link = (char *) __get_free_page(GFP_KERNEL);
	if (!link) {
		link = ERR_PTR(-ENOMEM);
		goto out;
	}
	req->in.h.opcode = FUSE_READLINK;
	req->in.h.nodeid = get_node_id(inode);
	req->inode = inode;
	req->out.argvar = 1;
	req->out.numargs = 1;
	req->out.args[0].size = PAGE_SIZE - 1;
	req->out.args[0].value = link;
	request_send(fc, req);
	if (req->out.h.error) {
		free_page((unsigned long) link);
		link = ERR_PTR(req->out.h.error);
	} else
		link[req->out.args[0].size] = '\0';
 out:
	fuse_put_request(fc, req);
	return link;
}

static void free_link(char *link)
{
	if (!IS_ERR(link))
		free_page((unsigned long) link);
}

static void *fuse_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	nd_set_link(nd, read_link(dentry));
	return NULL;
}

static void fuse_put_link(struct dentry *dentry, struct nameidata *nd, void *c)
{
	free_link(nd_get_link(nd));
}

static int fuse_dir_open(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int fuse_dir_release(struct inode *inode, struct file *file)
{
	struct file *cfile = file->private_data;

	if (cfile)
		fput(cfile);

	return 0;
}

static unsigned iattr_to_fattr(struct iattr *iattr, struct fuse_attr *fattr)
{
	unsigned ivalid = iattr->ia_valid;
	unsigned fvalid = 0;

	memset(fattr, 0, sizeof(*fattr));

	if (ivalid & ATTR_MODE)
		fvalid |= FATTR_MODE,   fattr->mode = iattr->ia_mode;
	if (ivalid & ATTR_UID)
		fvalid |= FATTR_UID,    fattr->uid = iattr->ia_uid;
	if (ivalid & ATTR_GID)
		fvalid |= FATTR_GID,    fattr->gid = iattr->ia_gid;
	if (ivalid & ATTR_SIZE)
		fvalid |= FATTR_SIZE,   fattr->size = iattr->ia_size;
	/* You can only _set_ these together (they may change by themselves) */
	if ((ivalid & (ATTR_ATIME | ATTR_MTIME)) == (ATTR_ATIME | ATTR_MTIME)) {
		fvalid |= FATTR_ATIME | FATTR_MTIME;
		fattr->atime = iattr->ia_atime.tv_sec;
		fattr->mtime = iattr->ia_mtime.tv_sec;
	}

	return fvalid;
}

static int fuse_setattr(struct dentry *entry, struct iattr *attr)
{
	struct inode *inode = entry->d_inode;
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct fuse_inode *fi = get_fuse_inode(inode);
	struct fuse_req *req;
	struct fuse_setattr_in inarg;
	struct fuse_attr_out outarg;
	int err;
	int is_truncate = 0;

	if (fc->flags & FUSE_DEFAULT_PERMISSIONS) {
		err = inode_change_ok(inode, attr);
		if (err)
			return err;
	}

	if (attr->ia_valid & ATTR_SIZE) {
		unsigned long limit;
		is_truncate = 1;
		limit = current->signal->rlim[RLIMIT_FSIZE].rlim_cur;
		if (limit != RLIM_INFINITY && attr->ia_size > (loff_t) limit) {
			send_sig(SIGXFSZ, current, 0);
			return -EFBIG;
		}
	}

	req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	memset(&inarg, 0, sizeof(inarg));
	inarg.valid = iattr_to_fattr(attr, &inarg.attr);
	req->in.h.opcode = FUSE_SETATTR;
	req->in.h.nodeid = get_node_id(inode);
	req->inode = inode;
	req->in.numargs = 1;
	req->in.args[0].size = sizeof(inarg);
	req->in.args[0].value = &inarg;
	req->out.numargs = 1;
	req->out.args[0].size = sizeof(outarg);
	req->out.args[0].value = &outarg;
	request_send(fc, req);
	err = req->out.h.error;
	fuse_put_request(fc, req);
	if (!err) {
		if ((inode->i_mode ^ outarg.attr.mode) & S_IFMT) {
			make_bad_inode(inode);
			err = -EIO;
		} else {
			if (is_truncate) {
				loff_t origsize = i_size_read(inode);
				i_size_write(inode, outarg.attr.size);
				if (origsize > outarg.attr.size)
					vmtruncate(inode, outarg.attr.size);
			}
			fuse_change_attributes(inode, &outarg.attr);
			fi->i_time = time_to_jiffies(outarg.attr_valid,
						     outarg.attr_valid_nsec);
		}
	} else if (err == -EINTR)
		fuse_invalidate_attr(inode);

	return err;
}

static int fuse_getattr(struct vfsmount *mnt, struct dentry *entry,
			struct kstat *stat)
{
	struct inode *inode = entry->d_inode;
	int err = fuse_revalidate(entry);
	if (!err)
		generic_fillattr(inode, stat);

	return err;
}

static struct dentry *fuse_lookup(struct inode *dir, struct dentry *entry,
				  struct nameidata *nd)
{
	struct inode *inode;
	int err = fuse_lookup_iget(dir, entry, &inode);
	if (err)
		return ERR_PTR(err);
	if (inode && S_ISDIR(inode->i_mode)) {
		/* Don't allow creating an alias to a directory  */
		struct dentry *alias = d_find_alias(inode);
		if (alias && !(alias->d_flags & DCACHE_DISCONNECTED)) {
			dput(alias);
			iput(inode);
			return ERR_PTR(-EIO);
		}
	}
	return d_splice_alias(inode, entry);
}

static int fuse_setxattr(struct dentry *entry, const char *name,
			 const void *value, size_t size, int flags)
{
	struct inode *inode = entry->d_inode;
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct fuse_req *req;
	struct fuse_setxattr_in inarg;
	int err;

	if (size > FUSE_XATTR_SIZE_MAX)
		return -E2BIG;

	if (fc->no_setxattr)
		return -EOPNOTSUPP;

	req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	memset(&inarg, 0, sizeof(inarg));
	inarg.size = size;
	inarg.flags = flags;
	req->in.h.opcode = FUSE_SETXATTR;
	req->in.h.nodeid = get_node_id(inode);
	req->inode = inode;
	req->in.numargs = 3;
	req->in.args[0].size = sizeof(inarg);
	req->in.args[0].value = &inarg;
	req->in.args[1].size = strlen(name) + 1;
	req->in.args[1].value = name;
	req->in.args[2].size = size;
	req->in.args[2].value = value;
	request_send(fc, req);
	err = req->out.h.error;
	fuse_put_request(fc, req);
	if (err == -ENOSYS) {
		fc->no_setxattr = 1;
		err = -EOPNOTSUPP;
	}
	return err;
}

static ssize_t fuse_getxattr(struct dentry *entry, const char *name,
			     void *value, size_t size)
{
	struct inode *inode = entry->d_inode;
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct fuse_req *req;
	struct fuse_getxattr_in inarg;
	struct fuse_getxattr_out outarg;
	ssize_t ret;

	if (fc->no_getxattr)
		return -EOPNOTSUPP;

	req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	memset(&inarg, 0, sizeof(inarg));
	inarg.size = size;
	req->in.h.opcode = FUSE_GETXATTR;
	req->in.h.nodeid = get_node_id(inode);
	req->inode = inode;
	req->in.numargs = 2;
	req->in.args[0].size = sizeof(inarg);
	req->in.args[0].value = &inarg;
	req->in.args[1].size = strlen(name) + 1;
	req->in.args[1].value = name;
	/* This is really two different operations rolled into one */
	req->out.numargs = 1;
	if (size) {
		req->out.argvar = 1;
		req->out.args[0].size = size;
		req->out.args[0].value = value;
	} else {
		req->out.args[0].size = sizeof(outarg);
		req->out.args[0].value = &outarg;
	}
	request_send(fc, req);
	ret = req->out.h.error;
	if (!ret)
		ret = size ? req->out.args[0].size : outarg.size;
	else {
		if (ret == -ENOSYS) {
			fc->no_getxattr = 1;
			ret = -EOPNOTSUPP;
		}
	}
	fuse_put_request(fc, req);
	return ret;
}

static ssize_t fuse_listxattr(struct dentry *entry, char *list, size_t size)
{
	struct inode *inode = entry->d_inode;
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct fuse_req *req;
	struct fuse_getxattr_in inarg;
	struct fuse_getxattr_out outarg;
	ssize_t ret;

	if (fc->no_listxattr)
		return -EOPNOTSUPP;

	req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	memset(&inarg, 0, sizeof(inarg));
	inarg.size = size;
	req->in.h.opcode = FUSE_LISTXATTR;
	req->in.h.nodeid = get_node_id(inode);
	req->inode = inode;
	req->in.numargs = 1;
	req->in.args[0].size = sizeof(inarg);
	req->in.args[0].value = &inarg;
	/* This is really two different operations rolled into one */
	req->out.numargs = 1;
	if (size) {
		req->out.argvar = 1;
		req->out.args[0].size = size;
		req->out.args[0].value = list;
	} else {
		req->out.args[0].size = sizeof(outarg);
		req->out.args[0].value = &outarg;
	}
	request_send(fc, req);
	ret = req->out.h.error;
	if (!ret)
		ret = size ? req->out.args[0].size : outarg.size;
	else {
		if (ret == -ENOSYS) {
			fc->no_listxattr = 1;
			ret = -EOPNOTSUPP;
		}
	}
	fuse_put_request(fc, req);
	return ret;
}

static int fuse_removexattr(struct dentry *entry, const char *name)
{
	struct inode *inode = entry->d_inode;
	struct fuse_conn *fc = get_fuse_conn(inode);
	struct fuse_req *req;
	int err;

	if (fc->no_removexattr)
		return -EOPNOTSUPP;

	req = fuse_get_request(fc);
	if (!req)
		return -ERESTARTNOINTR;

	req->in.h.opcode = FUSE_REMOVEXATTR;
	req->in.h.nodeid = get_node_id(inode);
	req->inode = inode;
	req->in.numargs = 1;
	req->in.args[0].size = strlen(name) + 1;
	req->in.args[0].value = name;
	request_send(fc, req);
	err = req->out.h.error;
	fuse_put_request(fc, req);
	if (err == -ENOSYS) {
		fc->no_removexattr = 1;
		err = -EOPNOTSUPP;
	}
	return err;
}

static struct inode_operations fuse_dir_inode_operations = {
	.lookup		= fuse_lookup,
	.mkdir		= fuse_mkdir,
	.symlink	= fuse_symlink,
	.unlink		= fuse_unlink,
	.rmdir		= fuse_rmdir,
	.rename		= fuse_rename,
	.link		= fuse_link,
	.setattr	= fuse_setattr,
	.create		= fuse_create,
	.mknod		= fuse_mknod,
	.permission	= fuse_permission,
	.getattr	= fuse_getattr,
	.setxattr	= fuse_setxattr,
	.getxattr	= fuse_getxattr,
	.listxattr	= fuse_listxattr,
	.removexattr	= fuse_removexattr,
};

static struct file_operations fuse_dir_operations = {
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.readdir	= fuse_readdir,
	.open		= fuse_dir_open,
	.release	= fuse_dir_release,
};

static struct inode_operations fuse_common_inode_operations = {
	.setattr	= fuse_setattr,
	.permission	= fuse_permission,
	.getattr	= fuse_getattr,
	.setxattr	= fuse_setxattr,
	.getxattr	= fuse_getxattr,
	.listxattr	= fuse_listxattr,
	.removexattr	= fuse_removexattr,
};

static struct inode_operations fuse_symlink_inode_operations = {
	.setattr	= fuse_setattr,
	.follow_link	= fuse_follow_link,
	.put_link	= fuse_put_link,
	.readlink	= generic_readlink,
	.getattr	= fuse_getattr,
	.setxattr	= fuse_setxattr,
	.getxattr	= fuse_getxattr,
	.listxattr	= fuse_listxattr,
	.removexattr	= fuse_removexattr,
};

void fuse_init_common(struct inode *inode)
{
	inode->i_op = &fuse_common_inode_operations;
}

void fuse_init_dir(struct inode *inode)
{
	inode->i_op = &fuse_dir_inode_operations;
	inode->i_fop = &fuse_dir_operations;
}

void fuse_init_symlink(struct inode *inode)
{
	inode->i_op = &fuse_symlink_inode_operations;
}
