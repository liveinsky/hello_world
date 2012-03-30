#ifndef _HELLO_IOCTL_H_
#define _HELLO_IOCTO_H_

#include <linux/ioctl.h>

#define HELLO_CLEAR _IOW(0xCE, 1, int)

#define HELLO_RED 	_IO(0xCE, 2)
#define HELLO_GREEN _IO(0xCE, 3)
#define HELLO_BLUE 	_IO(0xCE, 4)
#define HELLO_BLACK _IO(0xCE, 5)
#define HELLO_WHITE _IO(0xCE, 6)

#endif
