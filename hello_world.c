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
#include <asm-arm/semaphore.h>
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
	//DECLARE_WAIT_QUEUE_HEAD(wq);
	wait_queue_head_t wq;

	/* semaphore */
	struct semaphore sem;

	/* spinlock */
	spinlock_t	lock;
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

static void flush_lcd(unsigned long priv)
{
	struct hello_t *hello = (struct hello_t *) priv;
	unsigned char *fb;
	unsigned char *buf;
	unsigned int index;
	unsigned int offset;
	unsigned int i=0;

	//printk(KERN_INFO "Hello World: flush_lcd()\n");

	spin_lock(&hello->lock);
	
	fb = hello->fb;
	buf = hello->buf;
	index = hello->index;
	offset = hello->offset;
	
	spin_unlock(&hello->lock);
	
	for(i=0; i < index; i++)
	{
		if(offset >= LCD_SIZE)
			offset = 0;
		writeb(buf[i], &fb[offset++]);
	}

	hello->index = 0;
	hello->offset = offset;

}

void hello_wakeup(unsigned long priv)
{
	struct hello_t *hello = (struct hello_t *) priv;
	struct timer_list *sched = &hello->sched_timer;
	wait_queue_head_t *wq = &hello->wq;
	
	wake_up(wq);

	sched->expires = jiffies + 5*HZ;
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
	unsigned long flags;
	struct timer_list *flush_timer, *sched_timer;
	wait_queue_head_t *wq;
	wait_queue_t wait;

	//printk(KERN_INFO "Hello World: write (size = %d, buf=%x:%x:%x:%x)\n", size, buf[0], buf[1], buf[2], buf[3]);

	hello = (struct hello_t *)filp->private_data;
	
	down_interruptible(&hello->sem); 	/* semaphore */
	spin_lock_irqsave(&hello->lock, flags);		/* spinlock */
	fb = hello->fb;
	pixel = hello->buf;
	index = hello->index;
	spin_unlock_irqsave(&hello->lock, flags);	/* spinlock */
	flush_timer = &hello->flush_timer;
	sched_timer = &hello->sched_timer;
	wq = &hello->wq;
	up(&hello->sem);					/* semaphore */

	for(i=0; i < size; i++)
	{
		if(index >= LCD_BUF_SIZE)
		{
			down_interruptible(&hello->sem);
			hello->index = index;
			up(&hello->sem);
			
			/* kernel scheduling: use kernel timer to defferred exec flush_lcd */
			flush_timer->expires = jiffies + 1*HZ;
			flush_timer->function = flush_lcd;
			flush_timer->data = (unsigned long) hello;
			add_timer(flush_timer);
		
			/* process scheduling: use kernel timer to defferred exec hello_wakeup */
			sched_timer->expires = jiffies + 5*HZ;
			sched_timer->function =  hello_wakeup;
			sched_timer->data = (unsigned long) hello;
			add_timer(sched_timer);

			/* wait queue */
			wait.flags = 0;
			wait.task = current;
			add_wait_queue(wq, &wait);
repeat:
			current->state = TASK_INTERRUPTIBLE;
			schedule();

			down_interruptible(&hello->sem);
			index = hello->index;
			up(&hello->sem);
			
			if(index != 0)
				goto repeat;

			/* remove process scheduling resource 
			   NOTE: sched_timer NEED to del 		*/
			remove_wait_queue(wq, &wait);
			del_timer(sched_timer);
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
	/* kernel timer for kernel scheduling */
	init_timer(&hello->flush_timer);
	/* kernel timer for process scheduling */
	init_timer(&hello->sched_timer);
	/* wait queue for process scheduling */
	init_waitqueue_head(&hello->wq);
	/* semaphore */
	sema_init(&hello->sem, 1);
	/* spinlock */
	spin_lock_init(&hello->lock);

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
	/* If not implement the page table remap, the vma->vm_start & vma->vm_end
		are not in the /proc/<PID>/maps.										*/

	unsigned long from;
	unsigned long to;
	unsigned long size;

	printk(KERN_INFO "Hello World: mmap\n");
	printk("start addr = %p\n", vma->vm_start);
	printk("end addr = %p\n", vma->vm_end);

	from = vma->vm_start;
	to = 0x33f00000;	// frame buffer physical address
	size = vma->vm_end - vma->vm_start;

#if 0
	/* only for the reserved area is continued */
	remap_page_range(from, to, size, PAGE_SHARED);
#else
	/* for the general case (remapped base on PAGE_SIZE) */
	while(size > 0)
	{
#if 1	// kernel 2.4 or kernel 2.6
		/* kernel 2.4, no handle user sapce page table */
		remap_page_range(from, to, PAGE_SIZE, PAGE_SHARED);
#else
		/* kernel 2.6 have handle user page table */
		//remap_pfn_range(vma, vma->vm_start, __pa(kvirt) >> PAGE_SHIFT, PAGE_SIZE, vma->vm_page_prot);
		remap_pfn_range(vma, from, to, PAGE_SIZE, vma->vm_page_prot);
#endif

		from += PAGE_SIZE;
		to += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
#endif

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
