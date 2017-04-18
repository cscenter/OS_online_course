#include <stdint.h>

#include <balloc.h>
#include <memory.h>
#include <print.h>
#include <vga.h>
#include <ints.h>


static void qemu_gdb_hang(void)
{
#ifdef DEBUG
	static volatile int wait = 1;

	while (wait);
#endif
}


void main(uintptr_t mb_info_phys)
{
	const struct multiboot_info *info = va(mb_info_phys);

	qemu_gdb_hang();
	vga_clr();
	ints_setup();
	balloc_setup(info);

	while (1);
}
