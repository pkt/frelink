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

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "frelink.h"


static int get_module_fd();
static int tst_ioctl(unsigned int devfd);

static int recover_from_fd(unsigned int devfd, const char *const name);
static int recover_from_lo(unsigned int devfd, const char *const name);


#define starts_with(A,B) (strncmp((A), (B), strlen(B)) == 0)

int main(int argc, char **argv)
{
	int devfd;
	char *arg;

	int ret = 0;


	if (argc != 2) {
		fputs("Usage: frelink /proc/<pid>/fd/<fd>\n"
		          "or frelink /dev/loop0\n", stderr);
		ret = 0; goto exit;
	}

	arg = argv[1];

	devfd = get_module_fd();
	if (devfd < 0) {
		perror("open /proc/frelink");
		ret = 1; goto exit;
	}

	if (starts_with(arg, "--test-ioctl")) {
		ret = tst_ioctl(devfd);
		goto release_devfd;
	}

	if (starts_with(arg, "/proc")) {
		ret = recover_from_fd(devfd, arg);
		goto release_devfd;
	}

	if (starts_with(arg, "/dev/loop")) {
		ret = recover_from_lo(devfd, arg);
		goto release_devfd;
	}

	fputs("illegal argument\n", stderr);
	ret = 1;

release_devfd:
	close(devfd);

exit:
	return ret;
}


static int get_module_fd()
{
	int fd = open("/proc/frelink", O_RDONLY, 0444);

	if (fd < 0) {
		errno = ENODEV;
		return -1;
	}

	return fd;
}

char *recover_name_from_fd(const char *const fdname)
{
	#define BUFSIZE 1024
	static char namebuf[1024];

	ssize_t len = readlink(fdname, namebuf, BUFSIZE-1);

	if (len != -1)
		namebuf[len] = '\0';
	else {
		fprintf(stderr, "invalid /proc file name\n");
		errno = EBADF;
		return NULL;
	}

	if ( len > 10 && !strcmp(namebuf+len-10, " (deleted)")) {
		*(namebuf + len - 10) = '\0';
	}

	return namebuf;
}


static int tst_ioctl(unsigned int devfd)
{
	struct frelink_arg data;

	data.id.fd = 1;
	return ioctl(devfd, FRELINK_IOCRECTEST, &data);
}

static int recover_from_fd(unsigned int devfd, const char *const name)
{
	struct frelink_arg data;
	char * newname = recover_name_from_fd(name);

	int fd = open(name, O_RDONLY, 0666);
	if (fd < 0) {
		fprintf(stderr, "open");
		perror(name);
		return 1;
	}

	if (!name) {
		perror("recover_name_from_fd");
		errno = EBADF;
	}

	data.id.fd = fd;
	data.path  = newname;

	return ioctl(devfd, FRELINK_IOCRECFD, &data);
}

static int recover_from_lo(unsigned int devfd, const char *const name)
{
	struct frelink_arg data;
	int loidx = 0;

	fputs("TODO\n",stderr);
	return 0;
}
