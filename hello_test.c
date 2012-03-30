#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

int main(void)
{
	int fd;
	char buf[16];
	
	system("mknod /dev/hello c 121 3");
	fd = open("/dev/hello", O_RDWR);
	read(fd, buf, 16);
	close(fd);

	return 0;
}
