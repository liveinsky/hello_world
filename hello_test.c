#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "hello_ioctl.h"

int main(void)
{
	int fd;
	int i=200*100;
	unsigned char buf[4] = {0x00, 0xff, 0x00, 0xff};
	
	system("mknod /dev/hello c 121 3");
	fd = open("/dev/hello", O_RDWR);

	while(i--)
	{
		write(fd, buf, 4);
	}
	
	close(fd);

	return 0;
}
