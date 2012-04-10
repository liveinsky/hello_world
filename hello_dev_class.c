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
#include <linux/uaccess.h>

struct hello_dev {
};

int hello_device_register(struct hello_dev *ops)
{
	if(ops == NULL)
		goto fail1;
	return 0;

fail1:
	printk(KERN_ALERT, "hello_dev: register failed.\n");
	return -1;
}

int hello_device_unregister(struct hello_dev *ops)
{
	return 0;
}

static ssize_t hello_show_version(struct class *class, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "Hello Class V1.0\n");
}

static ssize_t hello_handle_connect(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	return 0;
}

static CLASS_ATTR(hello, 0666, hello_show_version, hello_handle_connect);

static struct class *hello_class;

static int __init hello_dev_init(void)
{
	hello_class = class_create(THIS_MODULE, "hello_subsys");
	class_create_file(hello_class, &class_attr_hello);
	
	return 0;
}

static void __exit hello_dev_exit(void)
{
	class_remove_file(hello_class, &class_attr_hello);
	class_destroy(hello_class);
}

module_init(hello_dev_init);
module_exit(hello_dev_exit);

EXPORT_SYMBOL(hello_device_register);
EXPORT_SYMBOL(hello_device_unregister);

MODULE_LICENSE("GPL");

