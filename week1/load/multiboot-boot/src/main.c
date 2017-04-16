#include <stdint.h>
#include <multiboot.h>
#include <memory.h>
#include <print.h>
#include <vga.h>


static void qemu_gdb_hang(void)
{
#ifdef DEBUG
	static volatile int wait = 1;

	while (wait);
#endif
}


void main(void)
{
	qemu_gdb_hang();
	vga_clr();
	printf("Hello, World!\n");
}
