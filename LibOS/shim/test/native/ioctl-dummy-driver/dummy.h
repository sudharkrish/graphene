#ifndef _DUMMY_H
#define _DUMMY_H

#include <linux/ioctl.h>
#include <linux/stddef.h>
#include <linux/types.h>

#define DUMMY_FILE	"/dev/dummy"
#define DUMMY_MINOR	MISC_DYNAMIC_MINOR

#define DUMMY_MAGIC	0x61
#define DUMMY_IOCTL_EFD		_IOWR(DUMMY_MAGIC, 0, struct dummy_efd_info)
#define DUMMY_IOCTL_PRINT	_IOWR(DUMMY_MAGIC, 1, struct dummy_print)

struct dummy_print {
	const char *  str;
	int size;
};

struct dummy_efd_info {
	int pid;//may not be needed..
	int efd;
};

#endif /* _DUMMY_H */
