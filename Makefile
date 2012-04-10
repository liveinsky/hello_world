# for test

obj-m := hello_2.6.o

KDIR := /lib/modules/${shell uname -r}/build
PWD := ${shell pwd}

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko .*cmd modules.* Module.* .tmp_versions *.mod.c
