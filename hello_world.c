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
#include "hello_ioctl.h"

#define	DEV_MAJOR	121
#define	DEV_NAME	"debug"

/* 1 pixel = 4 bytes*/
#define LCD_PIXEL (320*240)
#define LCD_SIZE (LCD_PIXEL*4)
#define LCD_ADDR 0x33F00000
#define LCD_BUF_SIZE	128

#define WHITE	0xFFFFFF
#define RED		0xFF0000
#define GREEN	0x00FF00
#define BLUE	0x0000FF
#define BLACK	0x000000

struct hello_t {
	/* for frame buffer */
	unsigned char *fb;
	unsigned char *buf;
	unsigned int index;
	unsigned int offset;

	/* kernel scheduling */
	struct timer_list	flush_timer;

	/* process scheduling */
	struct timer_list	sched_timer;
};

static int lcd_set(unsigned long *fb, unsigned long color, int pixel)
{
	int i=0;

	if((pixel > LCD_PIXEL) || (pixel == 0))
		pixel = LCD_PIXEL;

	for(i = 0; i < pixel; i++)
	{
		writel(color, fb++);
	}

	return 0;
}

static int flush_lcd(unsigned long priv)
{
	struct hello_t *hello = (struct hello_t *) priv;
	unsigned char *fb = hello->fb;
	unsigned char *buf = hello->buf;
	unsigned int index = hello->index;
	unsigned int offset = hello->offset;
	unsigned int i=0;

	printk(KERN_INFO "Hello World: flush_lcd()\n");
	
	for(i=0; i < index; i++)
	{
		if(offset >= LCD_SIZE)
			offset = 0;
		writeb(buf[i], &fb[offset++]);
	}

	hello->index = 0;
	hello->offset = offset;

	return 0;
}

void hello_wakeup(unsigned long priv)
{
	struct hello_t *hello = (struct hello_t *) priv;
	struct timer_list *sched = &hello->sched_timer;
	
	current->state = TASK_RUNNING;
	schedule();

	sched->expires = jiffies + 10*HZ;
	add_timer(sched);
}

static ssize_t hello_read(struct file *filp, char *buf, size_t size, loff_t *offset)
{
	int i=0;
	printk(KERN_INFO "Hello World: read\n");
	while(i++ < 5000)
	{
		printk(KERN_INFO "Hello World: read %d, before state = %d\n", i, (int)current->state);
		current->state = TASK_INTERRUPTIBLE;
		printk(KERN_INFO "Hello World: read %d, after state = %d\n", i, (int)current->state);
		schedule();
	}
	return 0;
}

static ssize_t hello_write(struct file *filp, const char *buf, size_t size, loff_t *offset)
{
	struct hello_t *hello=NULL;
	unsigned char *fb=NULL;
	unsigned char *pixel=NULL;
	unsigned int index=0, i=0;
	struct timer_list *flush_timer, *sched_timer;

	printk(KERN_INFO "Hello World: write (size = %d, buf=%x:%x:%x:%x)\n", size, buf[0], buf[1], buf[2], buf[3]);

	// FIXME: lock
	hello = (struct hello_t *)filp->private_data;
	fb = hello->fb;
	pixel = hello->buf;
	index = hello->index;
	flush_timer = &hello->flush_timer;
	sched_timer = &hello->sched_timer;
	// FIXME: unlock

	for(i=0; i < size; i++)
	{
		if(index >= LCD_BUF_SIZE)
		{
			hello->index = index;
			
			/* kernel scheduling: use kernel timer to defferred exec flush_lcd */
			flush_timer->expires = jiffies + 1*HZ;
			flush_timer->function = flush_lcd;
			flush_timer->data = (unsigned long) hello;
			add_timer(flush_timer);
		
			/* process scheduling: use kernel timer to defferred exec hello_wakeup */
			sched_timer->expires = jiffies + 10*HZ;
			sched_timer->function = hello_wakeup;
			flush_timer->data = (unsigned long) hello;
			add_timer(sched_timer);
repeat:
			current->state = TASK_INTERRUPTIBLE;
			schedule();

			index = hello->index;
			
			if(index != 0)
				goto repeat;
		}
		copy_from_user(&pixel[index++], &buf[i], 1);
	}

	// FIXME: lock
	hello->index = index;
	// FIXME: unlock

	return 0;
}

static int hello_open(struct inode *inode, struct file *filp)
{
	struct hello_t *hello;
	
	printk(KERN_INFO "Hello World: open (minor num = %d)\n", MINOR(inode->i_rdev));

	hello = kmalloc(GFP_KERNEL, sizeof(struct hello_t));
	/* frame buffer */
	hello->fb = ioremap(LCD_ADDR, LCD_SIZE);
	hello->buf = kmalloc(GFP_KERNEL, LCD_BUF_SIZE);
	hello->index = 0;
	hello->offset = 0;
	/* kernel timer for kernel scheduling */
	init_timer(&hello->flush_timer);
	/* kernel timer for process scheduling */
	init_timer(&hello->sched_timer);

	filp->private_data = (void *)hello;

	return 0;
}

static int hello_release(struct inode *inode, struct file *filp)
{
	struct hello_t *hello = (struct hello_t *)filp->private_data;
	struct timer_list *flush_timer = &hello->flush_timer;
	struct timer_list *sched_timer = &hello->sched_timer;
	printk(KERN_INFO "Hello World: release\n");

	flush_lcd((unsigned long) hello);

	/* kernel timer */
	del_timer(flush_timer);
	del_timer(sched_timer);

	kfree(hello);
	kfree(hello->buf);

	return 0;
}

static int hello_mmap(struct file *filp, struct vm_area_struct *vma)
{
	printk(KERN_INFO "Hello World: mmap\n");
	return 0;
}

static int hello_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long *fb;
	unsigned long color;
	int pixel=0;
	
	printk(KERN_INFO "Hello World: ioctl\n");
	
	// FIXME: lock
	fb = (unsigned long *)((struct hello_t *)filp->private_data)->fb;
	// FIXME: unlock

	switch(cmd)
	{
		case HELLO_CLEAR:
			copy_from_user(&pixel, (void *)arg, sizeof(unsigned long));
			//get_user(pixel, arg); => equal = copy_from_user()
			color = WHITE;
			break;
		case HELLO_RED:
			color = RED;
			break;
		case HELLO_GREEN:
			color = GREEN;
			break;
		case HELLO_BLUE:
			color = BLUE;
			break;
		case HELLO_BLACK:
			color = BLACK;
			break;
		case HELLO_WHITE:
			color = WHITE;
			break;
		default:
			return -ENOTTY;
	}
	
	lcd_set(fb, color, pixel);

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
	unsigned long *fb;

	printk(KERN_INFO "Hello World: init module\n");
	
	/* remapping LCD PHY addr to virtual addr & reset the LCD */
	fb = ioremap(LCD_ADDR, LCD_SIZE);
	lcd_set(fb, WHITE, 0);
	
	if(register_chrdev(DEV_MAJOR, DEV_NAME, & hello_fops) < 0)
	{
		printk("couldn't register hello device\n");
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
