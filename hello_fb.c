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
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "hello_ioctl.h"
#include <linux/semaphore.h>
#include <linux/platform_device.h>

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

/* should be defined in miscdevice.h */
#define HELLO_FB_MINOR 123

struct hello_t {
	/* for frame buffer */
	unsigned char *fb;
	unsigned char *buf;
	unsigned int index;
	unsigned int offset;

	//DECLARE_WAIT_QUEUE_HEAD(wq);
	wait_queue_head_t wq;

	/* semaphore */
	struct semaphore sem;

	/* spinlock */
	spinlock_t	lock;

	/* work queue */
	struct work_struct work;	// kernel 2.6
};

static struct workqueue_struct *hello_wq;

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

static void flush_lcd(struct work_struct *work)
{
	struct hello_t *hello;
	unsigned char *fb;
	unsigned char *buf;
	unsigned int index;
	unsigned int offset;
	unsigned int i=0;
	wait_queue_head_t *wq;

	//printk(KERN_INFO "Hello World: flush_lcd()\n");
	
	hello = container_of(work, struct hello_t, work);

	spin_lock(&hello->lock);
	
	fb = hello->fb;
	buf = hello->buf;
	index = hello->index;
	offset = hello->offset;
	wq = &hello->wq;
	
	spin_unlock(&hello->lock);
	
	for(i=0; i < index; i++)
	{
		if(offset >= LCD_SIZE)
			offset = 0;
		writeb(buf[i], &fb[offset++]);

#if 0
		/* for debug */
		if(delay == 1)
		{
			schedule();
			for(j=0; j < 1000; j++)
				;
		}
#endif
	}

	hello->index = 0;
	hello->offset = offset;

	wake_up(wq);
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
	unsigned long flags;
	struct work_struct *work;
	wait_queue_head_t *wq;
	DECLARE_WAITQUEUE(wait, current);

	//printk(KERN_INFO "Hello World: write (size = %d, buf=%x:%x:%x:%x)\n", size, buf[0], buf[1], buf[2], buf[3]);

	hello = (struct hello_t *)filp->private_data;
	
	down_interruptible(&hello->sem); 	/* semaphore */
	spin_lock_irqsave(&hello->lock, flags);			/* spinlock */
	fb = hello->fb;
	pixel = hello->buf;
	index = hello->index;
	spin_unlock_irqrestore(&hello->lock, flags);	/* spinlock */
	wq = &hello->wq;
	work = &hello->work;	// kernel 2.6
	up(&hello->sem);					/* semaphore */

	for(i=0; i < size; i++)
	{
		if(index >= LCD_BUF_SIZE)
		{
			down_interruptible(&hello->sem);
			hello->index = index;
			up(&hello->sem);
			
			queue_work(hello_wq, work);

			/* wait queue */
			/*
			wait.flags = 0;
			wait.task = current; */
			init_waitqueue_entry(&wait, current);
			add_wait_queue(wq, &wait);
repeat:
			//current->state = TASK_INTERRUPTIBLE;
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();

			down_interruptible(&hello->sem);
			index = hello->index;
			up(&hello->sem);
			
			if(index != 0)
				goto repeat;

			/* remove process scheduling resource */
			remove_wait_queue(wq, &wait);
		}
		copy_from_user(&pixel[index++], &buf[i], 1);
	}

	down_interruptible(&hello->sem);
	hello->index = index;
	up(&hello->sem);

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
	/* wait queue for process scheduling */
	init_waitqueue_head(&hello->wq);
	/* semaphore */
	sema_init(&hello->sem, 1);
	/* spinlock */
	spin_lock_init(&hello->lock);
	/* work queue */
	INIT_WORK(&hello->work, flush_lcd);

	filp->private_data = (void *)hello;

	return 0;
}

static int hello_release(struct inode *inode, struct file *filp)
{
	struct hello_t *hello = (struct hello_t *)filp->private_data;
	printk(KERN_INFO "Hello World: release\n");

	/* not sure !? */
	flush_work(&hello->work);

	kfree(hello);
	kfree(hello->buf);

	return 0;
}

static int hello_mmap(struct file *filp, struct vm_area_struct *vma)
{
	/* If not implement the page table remap, the vma->vm_start & vma->vm_end
		are not in the /proc/<PID>/maps.										*/

	unsigned long from;
	unsigned long to;
	unsigned long size;

	printk(KERN_INFO "Hello World: mmap\n");

	from = vma->vm_start;
	to = 0x33f00000;	// frame buffer physical address
	size = vma->vm_end - vma->vm_start;

	while(size > 0)
	{
		/* kernel 2.6 have handle user page table */
		remap_pfn_range(vma, from, to, PAGE_SIZE, vma->vm_page_prot);

		from += PAGE_SIZE;
		to += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

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

struct file_operations hello_fb_fops = {
	owner:		THIS_MODULE,
	open:		hello_open,
	release:	hello_release,
	read:		hello_read,
	write:		hello_write,
	ioctl:		hello_ioctl,
	mmap:		hello_mmap,
};

static struct miscdevice hello_fb_misc = {
	minor: HELLO_FB_MINOR,
	name: "hello_fb",
	fops: &hello_fb_fops,
};

static int hello_fb_probe(struct platform_device *pdev)
{
    if (misc_register(&hello_fb_misc) < 0) {
		printk(KERN_INFO "hello_fb: platform prob failed.\n");
		return -1;
	}
		
	printk(KERN_INFO "hello_fb: platform probe ok.\n");

	return 0;
}

static int hello_fb_remove(struct platform_device *pdev)
{
	printk(KERN_INFO "hello_fb: platform remove.\n");
	misc_deregister(&hello_fb_misc);
	
	return 0;	
}

static struct platform_driver hello_fb_driver = {
	.probe = hello_fb_probe,
	.remove = hello_fb_remove,
//	.suspend = hello_fb_suspend,
//	.resume = hello_fb_resume,
	.driver = {
				.name = "hello-lcd",
				.owner = THIS_MODULE,
			},
};

static int hello_fb_module_init(void)
{
	printk(KERN_INFO "Hello fb: init module\n");
	
	hello_wq = create_workqueue("hello_fb");

	if(hello_wq == NULL)
	{
		printk(KERN_INFO "hello: work queue init failed.\n");
		return -1;
	}

    return platform_driver_register(&hello_fb_driver);	
}

static void hello_fb_module_exit(void)
{
	printk(KERN_INFO "Hello fb: exit module\n");
	
	destroy_workqueue(hello_wq);
}

module_init(hello_fb_module_init);
module_exit(hello_fb_module_exit);

MODULE_LICENSE("GPL");
