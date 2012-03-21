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

static int hello_module_init(void)
{
	printk(KERN_INFO "Hello World: init module\n");
	return 0;
}

static void hello_module_exit(void)
{
	printk(KERN_INFO "Hellow World: exit module\n");
}

module_init(hello_module_init);
module_exit(hwllo_module_exit);

MODULE_LICENSE("GPL");
