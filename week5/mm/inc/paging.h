#ifndef __PAGING_H__
#define __PAGING_H__

#include <memory.h>
#include <stddef.h>
#include <stdint.h>


typedef uint64_t	pte_t;

#define PTE_PHYS_MASK	((pte_t)0xffffffffff000)

#define PTE_PRESENT	((pte_t)1 << 0)
#define PTE_LARGE	((pte_t)1 << 7)
#define PTE_WRITE	((pte_t)1 << 1)
#define PTE_USER	((pte_t)1 << 2)


extern uintptr_t initial_cr3;


size_t pt_index(uintptr_t addr, int lvl);
uintptr_t pt_addr(const pte_t *pml4, uintptr_t virt_addr);

int pt_map(pte_t *pml4, uintptr_t vaddr, size_t size, pte_t flags);
void pt_unmap(pte_t *pml4, uintptr_t vaddr, size_t size);


static inline void cr3_write(uintptr_t phys)
{
	__asm__ volatile ("movq %0, %%cr3" : : "r"(phys) : "memory");
}

static inline void flush_tlb_addr(uintptr_t vaddr)
{
	__asm__ volatile ("invlpg (%0)" : : "r"(vaddr) : "memory");
}

/**
 * All required paging setup was actually done in bootstrap.S,
 * so this function actually just creates a new page table that maps
 * the whole pysical memory into some range of our logical address
 * space. One of conveniences of 64bit architectures is that a
 * logical address space is huge, at least much larger than amount
 * of physical memory one can have in a computer system. So we can
 * just map the phole physical memory so that we have at least one
 * logical address for every physical address (may be not the only one).
 **/
void paging_setup(void);

#endif /*__PAGING_H__*/
