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

static void print_memmap(const struct multiboot_info *info)
{
	if ((info->flags & MULTIBOOT_INFO_MEM_MAP) == 0) {
		printf("No memory map provided\n");
		return;
	}

	const uintptr_t begin = info->mmap_addr;
	const uintptr_t end = begin + info->mmap_length;
	uintptr_t ptr = begin;

	printf("Memory Map:\n");
	while (ptr + sizeof(struct multiboot_mmap_entry) <= end) {
		const struct multiboot_mmap_entry *e = va(ptr);
		const unsigned long long from = e->addr;
		const unsigned long long to = from + e->len;

		printf("range [0x%llx-0x%llx) %s\n", from, to,
				e->type == MULTIBOOT_MEMORY_AVAILABLE
				? "available" : "reserved");
		ptr += e->size + sizeof(e->size);
	}
}

void main(uintptr_t mb_info_phys)
{
	const struct multiboot_info * const info = va(mb_info_phys);

	qemu_gdb_hang();
	vga_clr();
	print_memmap(info);
}
