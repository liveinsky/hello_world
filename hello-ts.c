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

void hello_bh(unsigned long);
DECLARE_TASKLET(my_tasklet, hello_bh, NULL);

struct input_dev ts_input;
int x;
int y;

static int ts_input_open(struct input_dev *dev)
{
	input_report_abs(dev, ABS_X, x);
	input_report_abs(dev, ABS_Y, y);
	return 0;
}

static void ts_input_close(struct input_dev *dev)
{
}

void hello_ts_handler(int irq, void *priv, struct pt_regs *reg)
{
	printk(KERN_INFO "data_ts: down...\n");

	/* FIXME: read(x,y) from ADC */
	x = 100;
	y = 100;

	tasklet_schedule(&my_tasklet);
}

void hello_bh(unsigned long prive)
{
	printk(KERN_INFO "bh: ...\n");
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

	/* handling input device */
	ts_input.name = "hello-ts";
	ts_input.open = ts_input_open;
	ts_input.close = ts_input_close;
	// capabilities
	ts_input.absbit[0] = BIT(ABS_X) | BIT(ABS_Y);

	input_register_device(&ts_input);

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

/* NOTE: HELLO_TS_MINOR should be defined in miscdevice.h */
static struct miscdevice hello_ts_misc = {
  	minor:    HELLO_TS_MINOR,
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
