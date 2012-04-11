#ifndef	__HELLO_DEV_CLASS_H__
#define	__HELLO_DEV_CLASS_H__

#define MAX_MINOR	15

struct hello_dev {
	int minor;
	int (*connect)(struct hello_dev *);
	int (*disconnect)(struct hello_dev *);
	void *private;
};

int hello_device_register(struct hello_dev *ops);
int hello_device_unregister(struct hello_dev *ops);

#endif	//__HELLO_DEV_CLASS_H__
