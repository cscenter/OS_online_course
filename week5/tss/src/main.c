#include <stdint.h>

#include <buddy.h>
#include <balloc.h>
#include <initramfs.h>
#include <ints.h>
#include <list.h>
#include <memory.h>
#include <misc.h>
#include <paging.h>
#include <print.h>
#include <ramfs.h>
#include <slab.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <vga.h>


static void qemu_gdb_hang(void)
{
#ifdef DEBUG
	static volatile int wait = 1;

	while (wait);
#endif
}


static int init(void *unused)
{
	(void) unused;
	struct file *file;

	if (ramfs_open("initramfs/README", &file)) {
		printf("failed to open initramfs/README file\n");
		while (1);
	}

	char data[257];
	long offs = 0, ret;

	while ((ret = ramfs_readat(file, data, sizeof(data) - 1, offs)) > 0) {
		data[ret] = '\0';
		printf("%s", data);
		offs += ret;
	}
	ramfs_close(file);

	while (1);

	return 0;
}

void main(uintptr_t mb_info_phys)
{
	const struct multiboot_info *info = va(mb_info_phys);

	qemu_gdb_hang();
	vga_clr();
	misc_setup(info);
	ints_setup();
	balloc_setup();
	paging_setup();
	buddy_setup();
	ramfs_setup();
	initramfs_setup();
	time_setup();
	scheduler_setup();

	struct thread *thread = thread_create(&init, 0);

	if (!thread) {
		printf("failed to create init thread\n");
		while (1);
	}

	thread_start(thread);
	scheduler_idle();
}
