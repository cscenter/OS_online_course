#include <misc.h>

#include <memory.h>
#include <multiboot.h>
#include <print.h>
#include <stddef.h>
#include <string.h>


uintptr_t initrd_begin;
uintptr_t initrd_end;

uintptr_t mmap_begin;
uintptr_t mmap_end;


static void mmap_find(const struct multiboot_info *info)
{
	if ((info->flags & MULTIBOOT_INFO_MEM_MAP) == 0) {
		printf("No memory map provided\n");
		while (1);
	}

	mmap_begin = info->mmap_addr;
	mmap_end = mmap_begin + info->mmap_length;
}

static void initrd_find(const struct multiboot_info *info)
{
	if (!(info->flags & MULTIBOOT_INFO_MODS)) {
		printf("no initramfs provided\n");
		while (1);
	}

	const struct multiboot_mod_list *mods = va(info->mods_addr);
	const size_t count = info->mods_count;

	for (size_t i = 0; i != count; ++i) {
		static const char sign[] = "070701";
		const struct multiboot_mod_list *mod = &mods[i];

		if (mod->mod_end - mod->mod_start < sizeof(sign) - 1)
			continue;

		const void *ptr = va(mod->mod_start);

		if (memcmp(ptr, sign, sizeof(sign) - 1))
			continue;

		initrd_begin = mod->mod_start;
		initrd_end = mod->mod_end;
                return;
        }
	printf("no initramfs provided\n");
	while (1);
}

void misc_setup(const struct multiboot_info *info)
{
	mmap_find(info);
	initrd_find(info);
}
