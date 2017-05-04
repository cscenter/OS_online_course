ifneq ($(KERNELRELEASE),)

obj-m := getseg.o
CFLAGS_getseg.o += -DDEBUG

else

VER = `uname -r`
KDIR ?= "/lib/modules/${VER}/build"

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -rf *.o *.ko *.order *.symvers *.mod.c

endif
