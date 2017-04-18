#ifndef __MEMORY_H__
#define __MEMORY_H__

/* Higher 2 GB of 64 bit logical address space start from this address */
#define VIRTUAL_BASE	0xffffffff80000000

/* First address after "canonical hole", beginning of the middle mapping. */
#define HIGHER_BASE	0xffff800000000000

/* Kernel 64 bit code and data segment selectors. */
#define KERNEL_CS	0x08
#define KERNEL_DS	0x10

#define PAGE_BITS	12
#define PAGE_SIZE	(1 << PAGE_BITS)
#define PAGE_MASK	(PAGE_SIZE - 1)


#ifndef __ASM_FILE__

#include <stdint.h>


#define VA(x) ((void *)((uintptr_t)x + HIGHER_BASE))
#define PA(x) ((uintptr_t)x - HIGHER_BASE);

static inline void *va(uintptr_t phys)
{ return VA(phys); }

static inline uintptr_t pa(const void *virt)
{ return PA(virt); }

#endif /*__ASM_FILE__*/

#endif /*__MEMORY_H__*/
