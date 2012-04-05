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
	int i=256;
	int *addr;
	unsigned char buf[4] = {0x00, 0xff, 0x00, 0xff};
	
	/* manully mknod for mmap() test to check /proc/<PID>/maps */
	//system("mknod /dev/hello c 121 3");
	fd = open("/dev/hello", O_RDWR);

	addr = mmap(0, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	sleep(30);

	while(i--)
	{
		//write(fd, buf, 4);
		memcpy(&addr[i], buf, 4);
	}

	close(fd);

	return 0;
}
