#ifndef __BOOTSTRAP_ALLOCATOR_H__
#define __BOOTSTRAP_ALLOCATOR_H__

#include <stdint.h>
#include <stddef.h>

#include <memory.h>

/**
 * Bootstrap allocator is a very-very simple allocator that we are
 * going to use during OS kernel setup process. In fact we only need
 * it to setup initial convinient paging and buddy allocator, after
 * that we can drop the allocator and use buddy allocator + SLAB.
 *
 * So this allocator isn't supposed to be general purporse in fact
 * we don't even need to support freeing of allocated memory - so
 * the most straightforward and stupid approach will work.
 **/

struct multiboot_info;

/**
 * Bootstrap allocator uses physical map from multiboot compatible
 * to find free space in the physical address space. This function
 * must be called before allocating any memory using bootstrap
 * allocator.
 **/
void balloc_setup(const struct multiboot_info *info);


/**
 * Finds free "align" bytes aligned segment of "size" bytes in the
 * range ["from"; "to") of the physical address space.
 **/
uintptr_t __balloc_alloc(uintptr_t from, uintptr_t to,
			size_t size, size_t align);

/**
 * The same as above but doesn't limit range of the physical address
 * space.
 **/
uintptr_t balloc_alloc(size_t size, size_t align);



/**
 * Bootstrap allocator information also serves us as a memory map.
 **/

struct balloc_range {
	uintptr_t begin;
	uintptr_t end;
};

size_t balloc_ranges(void);
void balloc_get_range(size_t i, struct balloc_range *range);

size_t balloc_free_ranges(void);
void balloc_get_free_range(size_t i, struct balloc_range *range);

uintptr_t balloc_phys_mem(void);

#endif /*__BOOTSTRAP_ALLOCATOR_H__*/
