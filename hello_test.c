#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

void main(void)
{
	int fd;

	fd = open("/dev/hello", O_RDWR);
	close(fd);
}
