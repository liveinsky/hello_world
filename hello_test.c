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
	int *addr;
	pid_t pid;
	unsigned char buf[4] = {0x00, 0xff, 0x00, 0xff};
	
	/* manully mknod for mmap() test to check /proc/<PID>/maps */
	system("mknod /dev/hello c 121 3");
	fd = open("/dev/hello", O_RDWR);
	/* step.1 open(), step.2 fork() => parent & child use the same struct file
	   step.1 fork(), step.2 open() => parent & child use the differnet struct files */
	pid = fork();
	
	if(pid == 0)
	{
		buf[0] = 0x00;
		buf[1] = 0x00;
		buf[2] = 0x00;

		while(i--)
		{
			write(fd, buf, 4);
		}
	}
	else
	{
		buf[0] = 0xff;
		buf[1] = 0x00;
		buf[2] = 0x00;

		while(i--)
		{
			write(fd, buf, 4);
		}
	}


	close(fd);

	return 0;
}
