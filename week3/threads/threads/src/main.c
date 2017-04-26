#include <stdint.h>

#include <buddy.h>
#include <balloc.h>
#include <ints.h>
#include <list.h>
#include <memory.h>
#include <paging.h>
#include <print.h>
#include <slab.h>
#include <string.h>
#include <vga.h>
#include <threads.h>
#include <time.h>


static void qemu_gdb_hang(void)
{
#ifdef DEBUG
	static volatile int wait = 1;

	while (wait);
#endif
}


static int threadx(void *arg)
{
	const int id = (intptr_t)arg;

	while (1)
		printf("I'm thread %d\n", id);
	return 0;
}

static int init(void *unused)
{
	(void) unused;

	struct thread *thread1 = thread_create(&threadx, (void *)1);
	struct thread *thread2 = thread_create(&threadx, (void *)2);

	thread_start(thread1);
	thread_start(thread2);

	while (1);
	return 0;
}

void main(uintptr_t mb_info_phys)
{
	const struct multiboot_info *info = va(mb_info_phys);

	qemu_gdb_hang();
	vga_clr();
	ints_setup();
	balloc_setup(info);
	paging_setup();
	buddy_setup();
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
