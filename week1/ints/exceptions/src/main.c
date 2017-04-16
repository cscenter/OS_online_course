#include <stdint.h>
#include <multiboot.h>
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


static void div_by_zero_handler(void)
{
	printf("Ops... Devision By Zero!\n");
	while (1);
}

void main(void)
{
	qemu_gdb_hang();
	vga_clr();
	ints_setup();

	register_exception_handler(INTNO_DIVBYZERO, &div_by_zero_handler);

	static volatile int zero;

	printf("1 / %d = %d\n", zero, 1 / zero);
}
