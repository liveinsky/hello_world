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
	char buf[1];
	
	system("mknod /dev/hello c 121 3");
	fd = open("/dev/hello", O_RDWR);
	
	ioctl(fd, HELLO_CLEAR, &i);
	//ioctl(fd, HELLO_BLUE, NULL);
	
	close(fd);

	return 0;
}
