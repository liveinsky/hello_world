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


void hello_ts_handler(int irq, void *priv, struct pt_regs *reg)
{
	printk(KERN_INFO "data_ts: down...\n");
}

static int hello_ts_open(struct inode *inode, struct file *filp)
{
	set_gpio_ctrl(GPIO_YPON);
	set_gpio_ctrl(GPIO_YMON);
	set_gpio_ctrl(GPIO_XPON);
	set_gpio_ctrl(GPIO_XMON);

	ADCTSC = DOWN_INT | XP_PULL_UP_EN | \
				 XP_AIN | XM_HIZ | YP_AIN | YM_GND | \
				 XP_PST(WAIT_INT_MODE);


	/* Request touch panel IRQ */
	if (request_irq(IRQ_TC, hello_ts_handler, 0, 
	"hello-ts", 0)) {
		printk(KERN_ALERT "hello: request irq failed.\n");
	  	return -1;
	}

	return 0;

}

static ssize_t hello_ts_read(struct file *filp, char *buf, size_t size, loff_t *off)
{
	return 0;
}

static ssize_t hello_ts_write(struct file *filp, const char *buf, size_t size, 
      loff_t *off)
{
	return 0;
}

static int hello_ts_close(struct inode *inode, struct file *filp)
{
  	return 0;
}

static int hello_ts_ioctl(struct inode *inode, struct file *filp, 
    unsigned int cmd, unsigned long arg)
{
  	return -ENOTTY;
}

static struct file_operations hello_ts_fops = {  
  	owner:    THIS_MODULE,
  	open:     hello_ts_open,
  	release:  hello_ts_close,
  	read:     hello_ts_read,
  	write:    hello_ts_write,
  	ioctl:    hello_ts_ioctl,
};

static struct miscdevice hello_ts_misc = {
  	minor:    CDATA_TS_MINOR,
  	name:    "hello-ts",
  	fops:    &hello_ts_fops,
};

int hello_ts_init_module(void)
{
	if (misc_register(&hello_ts_misc) < 0) {
	  	printk(KERN_INFO "CDATA-TS: can't register driver\n");
	  	return -1;
	}

	printk(KERN_INFO "CDATA-TS: hello_ts_init_module\n");

	return 0;
}

void hello_ts_cleanup_module(void)

{
	misc_register(&hello_ts_misc);
}

module_init(hello_ts_init_module);
module_exit(hello_ts_cleanup_module);

MODULE_LICENSE("GPL");
