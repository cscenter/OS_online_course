#include <balloc.h>
#include <multiboot.h>
#include <print.h>
#include <string.h>



/**
 * Bootstrap allocator uses super straightforward approach
 * - it just stores all free ranges in the array of fixed
 * size. If for some reason the arrays size is not enough
 * just report an error and hang.
 **/
#define BALLOC_MAX_RANGE_SIZE	256
struct balloc_ranges {
	struct balloc_range range[BALLOC_MAX_RANGE_SIZE];
	size_t size;
};
static struct balloc_ranges free, all;



static void balloc_add_range(struct balloc_ranges *rs,
			uintptr_t begin, uintptr_t end)
{
	size_t from = 0, to;

	while (from != rs->size && begin > rs->range[from].end)
		++from;

	to = from;
	while (to != rs->size && end >= rs->range[to].begin)
		++to;

	/**
	 * Now from and to semi-closed maybe closed interval of
	 * ranges we need to replace with new one (from inclusive
	 * and to exclusive). So we can check wether there is
	 * enough space in the array of free ranges and report
	 * an error if there is no enough space.
	 **/
	if (rs->size + 1 - (to - from) > BALLOC_MAX_RANGE_SIZE) {
		printf("There is no enought space,"
			" increase BALLOC_MAX_RANGE_SIZE\n");
		while (1);
	}

	if (from != to) {
		if (begin > rs->range[from].begin)
			begin = rs->range[from].begin;

		if (end < rs->range[to - 1].end)
			end = rs->range[to - 1].end;
	}

	/**
	 * Move free ranges starting from "to" position to "from + 1"
	 * position. At position from will place our newly added range.
	 **/
	memmove(&rs->range[from + 1], &rs->range[to],
				(rs->size - to) * sizeof(rs->range[0]));
	rs->size += 1 - (to - from);
	rs->range[from].begin = begin;
	rs->range[from].end = end;
}

static void balloc_remove_range(struct balloc_ranges *rs,
			uintptr_t begin, uintptr_t end)
{
	size_t from = 0, to;

	while (from != rs->size && begin > rs->range[from].end)
		++from;

	to = from;
	while (to != rs->size && end >= rs->range[to].begin)
		++to;

	if (from == to)
		return;

	const uintptr_t before = rs->range[from].begin;
	const uintptr_t after = rs->range[to - 1].end;

	memmove(&rs->range[from], &rs->range[to],
				(rs->size - to) * sizeof(rs->range[0]));
	rs->size -= to - from;

	if (before < begin) balloc_add_range(rs, before, begin);
	if (after > end) balloc_add_range(rs, end, after);
}

static uintptr_t balloc_align_down(uintptr_t x, size_t align)
{
	return x - x % align;
}

static uintptr_t balloc_align_up(uintptr_t x, size_t align)
{
	return balloc_align_down(x + align - 1, align);
}

static uintptr_t balloc_find_range(const struct balloc_ranges *rs,
			uintptr_t from, uintptr_t to,
			size_t size, size_t align)
{
	for (size_t i = 0; i != rs->size; ++i) {
		if (rs->range[i].begin >= to)
			break;
		if (rs->range[i].end <= from)
			continue;

		const uintptr_t addr = balloc_align_up(rs->range[i].begin,
					align);

		if (addr + size <= rs->range[i].end)
			return addr;
	}
	return to;
}


uintptr_t __balloc_alloc(uintptr_t from, uintptr_t to,
			size_t size, size_t align)
{
	const uintptr_t addr = balloc_find_range(&free, from, to, size, align);

	if (addr == to)
		return 0;

	balloc_remove_range(&free, addr, addr + size);
	return addr;
}

uintptr_t balloc_alloc(size_t size, size_t align)
{
	return __balloc_alloc(0, UINTPTR_MAX, size, align);
}

void balloc_setup(const struct multiboot_info *info)
{
	if ((info->flags & MULTIBOOT_INFO_MEM_MAP) == 0) {
		printf("No memory map provided\n");
		return;
	}

	const uintptr_t begin = info->mmap_addr;
	const uintptr_t end = begin + info->mmap_length;

	uintptr_t ptr = begin;

	while (ptr + sizeof(struct multiboot_mmap_entry) <= end) {
		const struct multiboot_mmap_entry *e = va(ptr);
		const unsigned long long from = e->addr;
		const unsigned long long to = from + e->len;

		printf("range 0x%llx-0x%llx\n", from, to);

		balloc_add_range(&all, from, to);
		if (e->type == MULTIBOOT_MEMORY_AVAILABLE)
			balloc_add_range(&free, from, to);
		ptr += e->size + sizeof(e->size);
	}

	/**
	 * Just in case memory map is strange and contains overlapped
	 * ranges we at first add all free ranges and then remove all
	 * busy ranges. And we also must explicitly delete range occupied
	 * by the kernel.
	 **/
	ptr = begin;
	while (ptr + sizeof(struct multiboot_mmap_entry) <= end) {
		const struct multiboot_mmap_entry *e = va(ptr);
		const unsigned long long from = e->addr;
		const unsigned long long to = from + e->len;

		if (e->type != MULTIBOOT_MEMORY_AVAILABLE)
			balloc_remove_range(&free, from, to);
		ptr += e->size + sizeof(e->size);
	}

	/* Both are defined in the linker script (kernel.ld) */
	extern char text_phys_begin[];
	extern char bss_phys_end[];

	const uintptr_t kernel_begin = (uintptr_t)text_phys_begin;
	const uintptr_t kernel_end = (uintptr_t)bss_phys_end;

	balloc_remove_range(&free, kernel_begin, kernel_end);

	/**
	 * Drop the first 1MB of physical memory all together - it's
	 * BIOS's area and known to be tricky. So mark it as a reserved
	 * area. Also we can interpret zero physical address as an invalid.
	 **/
	balloc_add_range(&all, 0, 1024 * 1024);
	balloc_remove_range(&free, 0, 1024 * 1024);

	/* Print free physical ranges as a "proof" we did everything well. */
	printf("Free memory ranges:\n");
	for (size_t i = 0; i != free.size; ++i)
		printf("range: 0x%llx-0x%llx\n",
				(unsigned long long)free.range[i].begin,
				(unsigned long long)free.range[i].end);
}

size_t balloc_ranges(void)
{ return all.size; }

void balloc_get_range(size_t i, struct balloc_range *range)
{ *range = all.range[i]; }


size_t balloc_free_ranges(void)
{ return free.size; }

void balloc_get_free_range(size_t i, struct balloc_range *range)
{ *range = free.range[i]; }


uintptr_t balloc_phys_mem(void)
{ return all.range[all.size - 1].end; }
