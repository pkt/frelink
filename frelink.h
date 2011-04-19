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

#ifndef FRELINK_H_
#define FRELINK_H_

#ifdef  __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif


struct frelink_arg
{
  union {
    unsigned int fd;
    unsigned int loidx;
  } id;
  const char *path;
};

/* Use '0xF5' as magic number, ioctl-number.txt indicates it is free */
#define FRELINK_IOC_MAGIC 0xF5

#define FRELINK_IOCRECFD     _IOW(FRELINK_IOC_MAGIC, 1, struct frelink_arg *)
#define FRELINK_IOCRECLOOP   _IOW(FRELINK_IOC_MAGIC, 2, struct frelink_arg *)
#define FRELINK_IOCRECTEST   _IOW(FRELINK_IOC_MAGIC, 3, struct frelink_arg *)

#define FRELINK_IOC_MAXNR    3

#endif /* FRELINK_H_ */
