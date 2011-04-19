/*
 * frelink: Recover deleted files that are still open by a process
 * or a loop mount.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) Pantelis Koukousoulas 2011
 *
 * Author: Pantelis Koukousoulas <pktoss@gmail.com>
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include <linux/namei.h>
#include <linux/file.h>
#include <linux/fs.h>

#include <linux/kprobes.h>
#include <linux/loop.h>

#include <linux/workqueue.h>

#include "frelink.h"


#define MODULE_NAME "frelink"
MODULE_AUTHOR("Pantelis Koukousoulas <pktoss@gmail.com>");
MODULE_DESCRIPTION("frelink: recover deleted open files");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.5");


#define IPRINTK(_f, _a...) printk(KERN_INFO MODULE_NAME ": " _f, ## _a)

/*
 * Forward decls
 */
static long jloop_get_status(struct loop_device *lo, struct loop_info64 *info);
static void do_recover(struct work_struct *work);
static int relink_file(struct file *f, char *path);


/*
 * Our main data structure.
 */
struct frelink_data {
	struct delayed_work work;
	struct proc_dir_entry *proc_entry;
	struct file *file;
	char *path;
	atomic_t busy;
	atomic_t loidx;
};
static struct frelink_data frelink;


/*
 * KProbe structure
 */
static struct jprobe loop_jprobe = {
	.entry                   = jloop_get_status,
	.kp = {
		.symbol_name     = "loop_get_status",
	},
};


static void do_recover(struct work_struct *work)
{
	struct frelink_data *rd = container_of(work,
	                                       struct frelink_data,
	                                       work.work);
	struct file *file = rd->file;
	char *name = rd->path;

	int ret = relink_file(file, name);

	if (ret == 0)
		IPRINTK("File \"%s\" undelete OK!\n", name);
	else
		IPRINTK("Failed to undelete \"%s\": "
                        "ret = \"%d\"\n", name, ret);

	/* We have finished with recovering, no longer busy */
	atomic_set(&rd->busy,0);
}


/*
 * This function is the "juice" of the module and shared by
 * both code paths (fd and loop). Given a "struct file *"
 * and a path, it makes a new hardlink for the deleted
 * file's inode so that it doesn't get actually deleted.
 */
static int relink_file(struct file *f, char *path)
{
	struct dentry *old_d, *new_d;
	struct nameidata nd;
	int ret;

	old_d = f->f_dentry;

	ret = path_lookup(path, LOOKUP_PARENT, &nd);
	if (ret)
		goto exit;

	ret = -EXDEV;

	new_d = lookup_create(&nd, 0);
	ret = PTR_ERR(new_d);
	if (!IS_ERR(new_d)) {
		//If we don't do this, vfs_link() won't work
		old_d->d_inode->i_nlink = 1;

		//This is the essence of what this module does :)
		ret = vfs_link(old_d, nd.path.dentry->d_inode, new_d);

		//This will remove the "(deleted)" suffix from the old dentry
		d_rehash(old_d);

		dput(new_d);
	}
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	path_put(&nd.path);

exit:
	return ret;
}


/*
 * Our jprobe handler. "Steals" the struct file * and path.
 */
static long jloop_get_status(struct loop_device *lo, struct loop_info64 *info)
{
	struct file *lofile;
	char *lopath;
	int nlinks;

	int loidx = lo->lo_number;
	int myloidx = atomic_read(&frelink.loidx);

	/* We only care about a specific loopback device */
	if (loidx != myloidx)
		goto exit;
	
	IPRINTK("\"Stealing\" backing file info from /dev/loop%d\n",loidx);

	lofile = lo->lo_backing_file;
	lopath = lo->lo_file_name;
	nlinks = lofile->f_dentry->d_inode->i_nlink;

	if (nlinks == 0) {
		IPRINTK("Found deleted backing file.. Scheduling undelete ...\n");
		frelink.file = lofile;
		frelink.path = lopath;
		schedule_delayed_work(&frelink.work, msecs_to_jiffies(1000));
	}

exit:
	/* Always end with a call to jprobe_return(). */
	jprobe_return();

	/* Control never actually reaches this point */
	return 0;
}


/*
 * IOCTL handler 1: recover from fd.
 * Wrapper for recover_from_file()
 */
static int recover_from_fd(struct frelink_arg *p)
{
	struct file *f;
	int fd = p->id.fd;
	char *path = getname(p->path);
	int ret = PTR_ERR(path);
	if (IS_ERR(path))
		return ret;

	IPRINTK("Recovering from fd \"%d\"to path \"%s\"", fd, path);

	atomic_set(&frelink.busy,1);

	f = fget(fd);
	if (!f) {
		ret = -EBADF;
		goto exit;
	}
	ret = relink_file(f, path);

exit:
	fput(f);
	atomic_set(&frelink.busy,0);
	return ret;
}

/*
 * IOCTL handler 2: recover a loop-mounted file
 */
static int recover_from_loop(struct frelink_arg *p)
{
	int ret = 0;
	int loidx = p->id.loidx;

	IPRINTK("Recovering loop-mounted file from /dev/loop%d\n",loidx);

	atomic_set(&frelink.busy,1);
	atomic_set(&frelink.loidx,loidx);

	return 0;
}


/* IOCTL CODE ---------------------------------------------------------*/
static long frelink_ioctl(struct file *file, unsigned int cmd,
                                            unsigned long arg)
{
	int busy = atomic_read(&frelink.busy);
	struct frelink_arg __user *p = (struct frelink_arg __user *)arg;
	int ret = 0;

	/* Basic access checks */
	if ((_IOC_TYPE(cmd) != FRELINK_IOC_MAGIC) ||
	    (_IOC_NR(cmd) > FRELINK_IOC_MAXNR)    ||
	    (_IOC_DIR(cmd) & _IOC_READ)) return -ENOTTY;

	if (!access_ok(VERIFY_READ, p, _IOC_SIZE(cmd)))
		return -EFAULT;

	switch (cmd) {
	case FRELINK_IOCRECFD:
		ret = busy ? -EBUSY : recover_from_fd(p);
		break;
	case FRELINK_IOCRECLOOP:
		ret = busy ? -EBUSY : recover_from_loop(p);
		break;
	case FRELINK_IOCRECTEST:
		ret = p->id.fd == 1 ? 0 : -EFAULT;
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static const struct file_operations frelink_ops = {
  .owner = THIS_MODULE,
  .unlocked_ioctl = frelink_ioctl,
};
/* END IOCTL CODE -----------------------------------------------------*/


static int __init frelink_init(void)
{
  	struct proc_dir_entry *entry;
	int ret = 0;

	IPRINTK ("frelink module loaded\n");

	entry = create_proc_entry(MODULE_NAME, 0400, NULL);
	if (!entry) {
		printk (KERN_ALERT "Error creating /proc/"MODULE_NAME
		                                          " file\n");
		return -EBADF;
	}
	entry->proc_fops = &frelink_ops;
	frelink.proc_entry = entry;
	atomic_set(&frelink.busy,0);
	atomic_set(&frelink.loidx,-1);

	ret = register_jprobe(&loop_jprobe);
	
	if (ret < 0) {
		IPRINTK("register_jprobe failed, returned %d\n", ret);
		return -1;
	}

	IPRINTK("Planted jprobe at %p, handler addr %p\n",
                  loop_jprobe.kp.addr, loop_jprobe.entry);

	INIT_DELAYED_WORK(&frelink.work, do_recover);

	return ret;
}
module_init(frelink_init);


static void __exit frelink_exit(void)
{
	IPRINTK ("unloading module...\n");

	remove_proc_entry(MODULE_NAME, NULL);

	unregister_jprobe(&loop_jprobe);

	IPRINTK("jprobe at \"%p\" unregistered\n", loop_jprobe.kp.addr);
}
module_exit(frelink_exit);
