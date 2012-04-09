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

struct hello_ts {
	struct input_dev ts_input;
	int x;
	int y;
};

static int ts_input_open(struct input_dev *dev)
{
	return 0;
}

static void ts_input_close(struct input_dev *dev)
{
}

void hello_ts_handler(int irq, void *priv, struct pt_regs *reg)
{
	struct hello_ts *hello = (struct hello_ts *)priv;

	printk(KERN_INFO "data_ts: down...\n");

	/* FIXME: read(x,y) from ADC */
	hello->x = 100;
	hello->y = 100;

	my_tasklet.data = (unsigned long) hello;

	tasklet_schedule(&my_tasklet);
}

void hello_bh(unsigned long prive)
{
	struct hello_ts *hello = (struct hello_ts *)prive;
	struct input_dev *dev = &hello->ts_input;

	printk(KERN_INFO "bh: ...\n");

	input_report_abs(dev, ABS_X, hello->x);
	input_report_abs(dev, ABS_Y, hello->y);
}

static int hello_ts_open(struct inode *inode, struct file *filp)
{
	struct hello_ts *hello;

	hello = kmalloc(sizeof(struct hello_ts), GFP_KERNEL);

	set_gpio_ctrl(GPIO_YPON);
	set_gpio_ctrl(GPIO_YMON);
	set_gpio_ctrl(GPIO_XPON);
	set_gpio_ctrl(GPIO_XMON);

	ADCTSC = DOWN_INT | XP_PULL_UP_EN | \
				 XP_AIN | XM_HIZ | YP_AIN | YM_GND | \
				 XP_PST(WAIT_INT_MODE);


	/* Request touch panel IRQ */
	if (request_irq(IRQ_TC, hello_ts_handler, 0, 
	"hello-ts", (void *)hello)) {
		printk(KERN_ALERT "hello: request irq failed.\n");
	  	return -1;
	}

	/* handling input device */
	hello->ts_input.name = "hello-ts";
	hello->ts_input.open = ts_input_open;
	hello->ts_input.close = ts_input_close;
	// capabilities
	hello->ts_input.absbit[0] = BIT(ABS_X) | BIT(ABS_Y);

	input_register_device(&hello->ts_input);
	
	hello->x = 0;
	hello->y = 0;

	filp->private_data = (void *) hello;

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
	  	printk(KERN_INFO "Hello-TS: can't register driver\n");
	  	return -1;
	}

	printk(KERN_INFO "Hello-TS: hello_ts_init_module\n");

	return 0;
}

void hello_ts_cleanup_module(void)

{
	misc_register(&hello_ts_misc);
}

module_init(hello_ts_init_module);
module_exit(hello_ts_cleanup_module);

MODULE_LICENSE("GPL");
