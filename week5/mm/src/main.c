#include <stdint.h>

#include <buddy.h>
#include <balloc.h>
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

	const unsigned perm = VMA_PERM_READ | VMA_PERM_WRITE;
	struct thread *me = thread_current();


	/* NULL ptr magic */
	if (mmap(me->mm, 0, PAGE_SIZE, perm)) {
		printf("mmap [0x0, 0x1000) failed\n");
		while (1);
	}

	if (mmap(me->mm, PAGE_SIZE, 2 * PAGE_SIZE, perm)) {
		printf("mmap [0x1000, 0x2000) failed\n");
		while (1);
	}

	/**
	 * TLB is not transparent, so we need to explicitly flush it
	 * after page table update
	 **/
	flush_tlb_addr(0);
	flush_tlb_addr(PAGE_SIZE);

	const char msg[] = "Hello, World!";

	printf("Before NULL access\n");
	memcpy(0, msg, sizeof(msg));
	printf("After NULL access\n");

	if (mset(me->mm, 0, 0, PAGE_SIZE)) {
		printf("zeroing [0x0, 0x1000) failed\n");
		while (1);
	}

	if (mset(me->mm, PAGE_SIZE, 0, PAGE_SIZE)) {
		printf("zeroing [0x1000, 0x2000) failed\n");
		while (1);
	}

	munmap(me->mm, 0, PAGE_SIZE);
	flush_tlb_addr(0);

	if (mset(me->mm, PAGE_SIZE, 0, PAGE_SIZE)) {
		printf("zeroing [0x1000, 0x2000) 2 failed\n");
		while (1);
	}

	munmap(me->mm, PAGE_SIZE, 2 * PAGE_SIZE);
	flush_tlb_addr(PAGE_SIZE);

	/* Page Fault */
	printf("Before NULL access\n");
	memcpy(0, msg, sizeof(msg));
	printf("After NULL access\n");

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
