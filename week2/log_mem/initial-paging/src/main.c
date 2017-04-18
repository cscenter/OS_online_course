#include <stdint.h>

#include <balloc.h>
#include <ints.h>
#include <memory.h>
#include <paging.h>
#include <print.h>
#include <string.h>
#include <vga.h>


static void qemu_gdb_hang(void)
{
#ifdef DEBUG
	static volatile int wait = 1;

	while (wait);
#endif
}


void test_mapping(void)
{
	const size_t count = balloc_free_ranges();

	for (size_t i = 0; i != count; ++i) {
		struct balloc_range range;

		balloc_get_free_range(i, &range);
		printf("fill 0x%llx-0x%llx\n",
					(unsigned long long)range.begin,
					(unsigned long long)range.end);
		memset(va(range.begin), 42, range.end - range.begin);
	}
}

void main(uintptr_t mb_info_phys)
{
	const struct multiboot_info *info = va(mb_info_phys);

	qemu_gdb_hang();
	vga_clr();
	ints_setup();
	balloc_setup(info);
	paging_setup();

	test_mapping();

	while (1);
}
