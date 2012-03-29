#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define	DEV_MAJOR	121
#define	DEV_NAME	"debug"

#define LCD_SIZE (320*240*4)

ssize_t hello_read(struct file *filp, char *buf, size_t size, loff_t *offset)
{
	printk(KERN_INFO "Hello World: read\n");
	return 0;
}

ssize_t hello_write(struct file *filp, const char *buf, size_t size, loff_t *offset)
{
	printk(KERN_INFO "Hello World: write\n");
	return 0;
}

int hello_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Hello World: open (minor num = %d)\n", MINOR(inode->i_rdev));
	return 0;
}

int hello_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Hello World: release\n");
	return 0;
}

int hello_mmap(struct file *filp, struct vm_area_struct *vma)
{
	printk(KERN_INFO "Hello World: mmap\n");
	return 0;
}

int hello_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk(KERN_INFO "Hello World: ioctl\n");
	return 0;
}

struct file_operations hello_fops = {
	owner:		THIS_MODULE,
	open:		hello_open,
	release:	hello_release,
	read:		hello_read,
	write:		hello_write,
	ioctl:		hello_ioctl,
	mmap:		hello_mmap,
};

static int hello_module_init(void)
{
	printk(KERN_INFO "Hello World: init module\n");
	
	if(register_chrdev(DEV_MAJOR, DEV_NAME, & hello_fops) < 0)
	{
		printk("couldn't register device\n");
		return -1;
	}
	
	return 0;
}

static void hello_module_exit(void)
{
	printk(KERN_INFO "Hello World: exit module\n");
	
	unregister_chrdev(DEV_MAJOR, DEV_NAME);
}

module_init(hello_module_init);
module_exit(hello_module_exit);

MODULE_LICENSE("GPL");
