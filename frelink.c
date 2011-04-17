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



/*
 * Our main data structure.
 */
struct frelink_data {
	struct proc_dir_entry *proc_entry;
};
static struct frelink_data frelink;


/* IOCTL CODE ---------------------------------------------------------*/
static long frelink_ioctl(struct file *file, unsigned int cmd,
                                            unsigned long arg)
{
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
		ret = 0;
		break;
	case FRELINK_IOCRECLOOP:
		ret = 0;
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

	IPRINTK ("frelink module loaded\n");

	entry = create_proc_entry(MODULE_NAME, 0400, NULL);
	if (!entry) {
		printk (KERN_ALERT "Error creating /proc/"MODULE_NAME
		                                          " file\n");
		return -EBADF;
	}
	entry->proc_fops = &frelink_ops;
	frelink.proc_entry = entry;

	return 0;
}
module_init(frelink_init);


static void __exit frelink_exit(void)
{
	IPRINTK ("unloading module...\n");

	remove_proc_entry(MODULE_NAME, NULL);
}
module_exit(frelink_exit);
