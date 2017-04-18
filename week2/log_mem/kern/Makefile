ifneq ($(KERNELRELEASE),)

obj-m := getseg.o
CFLAGS_getseg.o += -DDEBUG

else

KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -rf *.o *.ko *.order *.symvers *.mod.c

endif
