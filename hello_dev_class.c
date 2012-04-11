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
#include <linux/slab.h>
#include "hello_dev_class.h"

struct hello_dev_data {
	struct hello_dev *ops;
	struct miscdevice misc;

	int minor;
};

static struct hello_dev_data *hello_devices[MAX_MINOR];

/*	------------ misc ------------ */

static int hello_dev_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int hello_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int hello_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations hello_dev_fops = {
	owner:		THIS_MODULE,
	open:		hello_dev_open,
	release:	hello_release,
	ioctl:		hello_ioctl,
};


int hello_device_register(struct hello_dev *ops)
{
	int minor;
	struct hello_dev_data *data;
	char name[16];

	if(ops == NULL)
		goto fail1;

	minor = ops->minor;

	if(minor > MAX_MINOR)
		goto fail1;

	data = hello_devices[minor];

	if(data != NULL)
		goto fail1;

	data = (struct hello_dev_data *) kmalloc(sizeof(struct hello_dev_data), GFP_KERNEL);
	memset(data, 0, sizeof(struct hello_dev_data));

	data->ops = ops;
	data->minor = minor;
	hello_devices[minor] = data;

	sprintf(name, "hello%d\n", minor);
	data->misc.minor = 20+minor;
	data->misc.name = name;
	data->misc.fops = &hello_dev_fops;

	misc_register(&data->misc);
	printk(KERN_ALERT "hello_dev: registering %s to misc\n", name);

	return 0;

fail1:
	printk(KERN_ALERT "hello_dev: register failed.\n");
	return -1;
}

int hello_device_unregister(struct hello_dev *ops)
{
	int minor;
	struct hello_dev_data *data;

	minor = ops->minor;
	data = hello_devices[minor];

	misc_deregister(&data->misc);

	kfree(data);

	return 0;
}

/*	------------ sysfs ------------ */

static ssize_t hello_show_version(struct class *class, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "Hello Class V1.0\n");
}

static ssize_t hello_handle_connect(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
	struct hello_dev_data *data;
	struct hello_dev *ops;
	char str[1];
	int v;
	int i;

	memcpy(str, buf, 1);
	
	/* atoi */
	v = str[0];
	v = v - '0';

	if((v!=0) || (v!=1))
		return 0;

	printk(KERN_ALERT "hello_dev: connect_enable = %d\n", v);

	/* callback connect */
	for(i=0; i < MAX_MINOR; i++)
	{
		data = hello_devices[i];

		if(data == NULL)
			continue;

		ops = data->ops;

		if(v == 0)
		{
			if(ops && ops->disconnect)
				ops->disconnect(ops);
		}
		else
		{
			if(ops && ops->connect)
				ops->connect(ops);
		}
	}

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

