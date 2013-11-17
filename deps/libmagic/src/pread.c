#include "file.h"
#ifndef lint
FILE_RCSID("@(#)$File: pread.c,v 1.1 2013/02/18 15:40:59 christos Exp $")
#endif  /* lint */
#include <fcntl.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

ssize_t
pread(int fd, void *buf, size_t len, off_t off) {
	if (lseek(fd, off, SEEK_SET) == (off_t)-1)
		return -1;

	return read(fd, buf, len);
}
