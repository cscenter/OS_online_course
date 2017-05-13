#include <stdint.h>

#include <buddy.h>
#include <balloc.h>
#include <exec.h>
#include <initramfs.h>
#include <ints.h>
#include <list.h>
#include <memory.h>
#include <misc.h>
#include <mm.h>
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

	const char *argv[] = { "initramfs/test" };

	if (exec(sizeof(argv)/sizeof(argv[0]), argv)) {
		printf("exec %s failed\n", argv[0]);
		while (1);
	}
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
	mm_setup();
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
