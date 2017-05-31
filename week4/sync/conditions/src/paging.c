#include <paging.h>
#include <balloc.h>
#include <memory.h>
#include <print.h>
#include <string.h>


static pte_t pt_alloc(void)
{
	/**
	 * bootstrap.S maps only lower 4GB of the physical memory, so
	 * all allocations during setup of the initial page table should
	 * take free memory from there, otherwise we wouldn't have a
	 * logical address to use.
	 **/
	const uintptr_t from = 0;
	const uintptr_t to = (uintptr_t)1 << 32;

	const uintptr_t phys = __balloc_alloc(from, to, PAGE_SIZE, PAGE_SIZE);

	if (!phys) {
		printf("Failed to allocate a page for initial page table\n");
		while (1);
	}

	void *vaddr = va(phys);

	memset(vaddr, 0, PAGE_SIZE);
	return phys;
}


static int pt_shift(int lvl)
{
	static const int offs[] = {0, 12, 21, 30, 39};

	return offs[lvl];
}

static int pt_offs(uintptr_t addr, int lvl)
{
	static const int mask[] = {0xfff, 0x1ff, 0x1ff, 0x1ff, 0x1ff};

	return (addr >> pt_shift(lvl)) & mask[lvl];
}

static uint64_t pt_size(int lvl)
{
	return (uint64_t)1 << pt_shift(lvl);
}

/**
 * This function tries to use large pages when possible. We need to
 * create mapping for the whole physical memory and kernel only once
 * and then we are never going to change that. Thus we can write a
 * simple function that don't need to consider unmapping, failuers
 * and so on. When we go to user space we will need to create more
 * elaborated API that will be able not only map but also unmap pages,
 * count references to pages and so on.
 **/
static void __pt_map(pte_t *pt, uint64_t virt, uint64_t size,
			uint64_t phys, pte_t pte_flags, int lvl)
{
	/* For the internal pages we always use most permissive flags. */
	const pte_t pde_flags = PTE_PRESENT | PTE_WRITE;
	const pte_t pte_large = (lvl == 3 || lvl == 2) ? PTE_LARGE : 0;

	const uint64_t esize = pt_size(lvl);
	const uint64_t emask = esize - 1;

	const int from = pt_offs(virt, lvl);
	const int to = pt_offs(virt + size - 1, lvl) + 1;

	#define MIN(a, b) ((a) < (b) ? (a) : (b))
	for (int i = from; i != to; ++i) {
		const uint64_t eend = (virt + esize) & ~emask;
		const uint64_t tomap = MIN(size, eend - virt);
		pte_t pte = pt[i];

		if (!(pte & PTE_PRESENT)) {
			const int notaligned = (virt & emask) || (phys & emask);
			const int leaf = (lvl == 1) || (pte_large &&
						tomap == esize && !notaligned);

			if (leaf) {
				pt[i] = (pte_t)phys | pte_flags | pte_large;
				virt += tomap;
				phys += tomap;
				size -= tomap;
				continue;
			}

			pte = pt_alloc() | pde_flags;
			pt[i] = pte;
		}

		__pt_map(va(pte & PTE_PHYS_MASK), virt, tomap, phys,
					pte_flags, lvl - 1);
		virt += tomap;
		phys += tomap;
		size -= tomap;
	}
	#undef MIN
}

static void pt_map(pte_t *pt, uintptr_t virt, uintptr_t size,
			uintptr_t phys, pte_t flags)
{
	__pt_map(pt, virt, size, phys, flags | PTE_PRESENT, 4);
}

static void cr3_write(uintptr_t phys)
{ __asm__ ("movq %0, %%cr3" : : "r"(phys)); }


void paging_setup(void)
{
	const uintptr_t mask = ~(uintptr_t)PAGE_MASK;
	const uintptr_t phys = pt_alloc();
	const uintptr_t gb = (uintptr_t)1024 * 1024 * 1024;

	pte_t *pt = va(phys);

	const uintptr_t e = balloc_phys_mem() & mask;
	const uintptr_t b = 0;

	printf("map [0x%llx-0x%llx]\n", (unsigned long long)b,
				(unsigned long long)e);
	pt_map(pt, HIGHER_BASE + b, e - b, b, PTE_WRITE);

	pt_map(pt, VIRTUAL_BASE, 2 * gb, 0, PTE_WRITE);
	cr3_write(phys);
}
