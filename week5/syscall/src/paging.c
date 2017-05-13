#include <paging.h>
#include <balloc.h>
#include <buddy.h>
#include <memory.h>
#include <print.h>
#include <string.h>


uintptr_t initial_cr3;


static int pt_shift(int lvl)
{
	static const int offs[] = {0, 12, 21, 30, 39};

	return offs[lvl];
}

size_t pt_index(uintptr_t addr, int lvl)
{
	static const int mask[] = {0xfff, 0x1ff, 0x1ff, 0x1ff, 0x1ff};

	return (addr >> pt_shift(lvl)) & mask[lvl];
}

static uint64_t pt_size(int lvl)
{
	return (uint64_t)1 << pt_shift(lvl);
}

static size_t pt_order(int lvl)
{
	return pt_shift(lvl) - PAGE_BITS;
}

uintptr_t pt_addr(const pte_t *pml4, uintptr_t addr)
{
	uintptr_t mask = 0xffffffffffffull;
	uintptr_t phys = pa(pml4);
	const pte_t *pt = pml4;

	for (int i = 4; i != 0; --i) {
		const pte_t pte = pt[pt_index(addr, i)];

		if (!(pte & PTE_PRESENT))
			return 0;

		phys = pte & PTE_PHYS_MASK;
		pt = va(phys);
		mask >>= 9;
		if (pte & PTE_LARGE)
			break;
	}
	return phys | (addr & mask);
}

static pte_t pt_alloc(void)
{
	const uintptr_t phys = buddy_alloc(0);

	if (!phys) {
		printf("Failed to allocate a page for initial page table\n");
		while (1);
	}

	void *vaddr = va(phys);

	memset(vaddr, 0, PAGE_SIZE);
	return phys;
}

static void pt_free(uintptr_t phys)
{
	buddy_free(phys, 0);
}


static int __pt_map(pte_t *pt, uint64_t virt, uint64_t size, pte_t pte_flags,
			int lvl)
{
	const pte_t pde_flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
	const pte_t pte_large = (lvl == 3 || lvl == 2) ? PTE_LARGE : 0;

	const uint64_t esize = pt_size(lvl);
	const uint64_t emask = esize - 1;
	const size_t order = pt_order(lvl);

	const int from = pt_index(virt, lvl);
	const int to = pt_index(virt + size - 1, lvl) + 1;

	for (int i = from; i != to; ++i) {
		#define MIN(a, b) ((a) < (b) ? (a) : (b))
		const uint64_t eend = (virt + esize) & ~emask;
		const uint64_t tomap = MIN(size, eend - virt);
		#undef MIN

		const int notaligned = virt & emask;
		const int leaf = (lvl == 1) || (pte_large && tomap == esize
					&& !notaligned);
		pte_t pte = pt[i];

		if (!(pte & PTE_PRESENT)) {
			if (leaf) {
				const uintptr_t phys = buddy_alloc(order);
				const pte_t flags = pte_flags | pte_large;

				if (phys) {
					pt[i] = (pte_t)phys | flags;
					virt += tomap;
					size -= tomap;
					continue;
				}

				if (lvl == 1)
					return -1;
			}

			if (!(pte = pt_alloc()))
				return -1;

			pte |= pde_flags;
			pt[i] = pte;
		}

		if (lvl > 1 && __pt_map(va(pte & PTE_PHYS_MASK), virt, tomap,
					pte_flags, lvl - 1))
			return -1;

		virt += tomap;
		size -= tomap;
	}
	return 0;
}

int pt_map(pte_t *pml4, uintptr_t vaddr, size_t size, pte_t flags)
{
	return __pt_map(pml4, vaddr, size, flags, 4);
}

static void __pt_unmap(pte_t *pt, uintptr_t vaddr, size_t size, int lvl)
{
	const uint64_t esize = pt_size(lvl);
	const uint64_t emask = esize - 1;

	const int from = pt_index(vaddr, lvl);
	const int to = pt_index(vaddr + size - 1, lvl) + 1;

	for (int i = from; i != to; ++i) {
		#define MIN(a, b) ((a) < (b) ? (a) : (b))
		const uint64_t eend = (vaddr + esize) & ~emask;
		const uint64_t tounmap = MIN(size, eend - vaddr);
		#undef MIN

		const pte_t pte = pt[i];
		const uintptr_t phys = pte & PTE_PHYS_MASK;

		if (!(pte & PTE_PRESENT))
			continue;

		if (lvl == 1 || (pte & PTE_LARGE)) {
			buddy_free(phys, pt_order(lvl));
			pt[i] = 0;
			continue;
		}

		__pt_unmap(va(phys), vaddr, tounmap, lvl - 1);
		/**
		 * if the whole the subtree completely inside then
		 * release inner page table too
		 **/
		if (tounmap == esize) {
			pt_free(phys);
			pt[i] = 0;
		}
		vaddr += tounmap;
		size -= tounmap;
	}
}

void pt_unmap(pte_t *pml4, uintptr_t vaddr, size_t size)
{
	__pt_unmap(pml4, vaddr, size, 4);
}


static pte_t pt_early_alloc(void)
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


/**
 * This function tries to use large pages when possible. We need to
 * create mapping for the whole physical memory and kernel only once
 * and then we are never going to change that. Thus we can write a
 * simple function that don't need to consider unmapping, failuers
 * and so on. When we go to user space we will need to create more
 * elaborated API that will be able not only map but also unmap pages,
 * count references to pages and so on.
 **/
static void __pt_map_to(pte_t *pt, uint64_t virt, uint64_t size,
			uint64_t phys, pte_t pte_flags, int lvl)
{
	/* For the internal pages we always use most permissive flags. */
	const pte_t pde_flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
	const pte_t pte_large = (lvl == 3 || lvl == 2) ? PTE_LARGE : 0;

	const uint64_t esize = pt_size(lvl);
	const uint64_t emask = esize - 1;

	const int from = pt_index(virt, lvl);
	const int to = pt_index(virt + size - 1, lvl) + 1;

	for (int i = from; i != to; ++i) {
		#define MIN(a, b) ((a) < (b) ? (a) : (b))
		const uint64_t eend = (virt + esize) & ~emask;
		const uint64_t tomap = MIN(size, eend - virt);
		pte_t pte = pt[i];
		#undef MIN

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

			pte = pt_early_alloc() | pde_flags;
			pt[i] = pte;
		}

		__pt_map_to(va(pte & PTE_PHYS_MASK), virt, tomap, phys,
					pte_flags, lvl - 1);
		virt += tomap;
		phys += tomap;
		size -= tomap;
	}
}

static void pt_map_to(pte_t *pt, uintptr_t virt, uintptr_t size,
			uintptr_t phys, pte_t flags)
{
	__pt_map_to(pt, virt, size, phys, flags | PTE_PRESENT, 4);
}


void paging_setup(void)
{
	const uintptr_t mask = ~(uintptr_t)PAGE_MASK;
	const uintptr_t phys = pt_early_alloc();
	const uintptr_t gb = (uintptr_t)1024 * 1024 * 1024;

	pte_t *pt = va(phys);

	const uintptr_t e = balloc_phys_mem() & mask;
	const uintptr_t b = 0;

	printf("map [0x%llx-0x%llx]\n", (unsigned long long)b,
				(unsigned long long)e);
	pt_map_to(pt, HIGHER_BASE + b, e - b, b, PTE_WRITE);

	pt_map_to(pt, VIRTUAL_BASE, 2 * gb, 0, PTE_WRITE);
	initial_cr3 = phys;
	cr3_write(phys);
}
