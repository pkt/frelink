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

#include "frelink.h"


#define MODULE_NAME "frelink"
MODULE_AUTHOR("Pantelis Koukousoulas <pktoss@gmail.com>");
MODULE_DESCRIPTION("frelink: recover deleted open files");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.5");


#define IPRINTK(_f, _a...) printk(KERN_INFO MODULE_NAME ": " _f, ## _a)


/*
 * Our main data structure.
 */
struct frelink_data {
	struct proc_dir_entry *proc_entry;
};
static struct frelink_data frelink;


static int __init frelink_init(void)
{
  	struct proc_dir_entry *entry;

	IPRINTK ("frelink module loaded\n");

	entry = create_proc_entry(MODULE_NAME, 0400, NULL);
	if (!entry) {
		printk (KERN_ALERT "Error creating /proc/"MODULE_NAME" file\n");
		return -EBADF;
	}
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
