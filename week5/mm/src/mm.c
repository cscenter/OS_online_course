#include <mm.h>

#include <buddy.h>
#include <memory.h>
#include <paging.h>
#include <slab.h>
#include <string.h>


static const uint64_t USER_MASK = 0x0000ffffffffffffull;
static struct slab_cache mm_slab;
static struct slab_cache vma_slab;


struct mm *mm_create(void)
{
	struct mm *mm = slab_cache_alloc(&mm_slab);

	if (!mm)
		return NULL;

	list_init(&mm->vmas);
	mm->pt = __buddy_alloc(0);

	if (!mm->pt) {
		slab_cache_free(&mm_slab, mm);
		return NULL;
	}

	mm->cr3 = page_addr(mm->pt);

	/**
	 * kernel part of the mapping is the same for every process
	 * and never changes so we need to copy it from the initial
	 **/
	const size_t offs = pt_index(HIGHER_BASE, 4) * sizeof(pte_t);
	char *ptr = va(mm->cr3);

	memset(ptr, 0, PAGE_SIZE);
	memcpy(ptr + offs, va(initial_cr3 + offs), PAGE_SIZE - offs);

	return mm;
}


void mm_release(struct mm *mm)
{
	munmap(mm, 0, HIGHER_BASE & USER_MASK);
	__buddy_free(mm->pt, 0);
	slab_cache_free(&mm_slab, mm);
}

int mset(struct mm *dst, uintptr_t to, int c, size_t size)
{
	const uintptr_t mask = ~((uintptr_t)PAGE_SIZE - 1);

	const pte_t *pt = va(dst->cr3);
	uintptr_t ptr = to;

	while (size) {
		#define MIN(a, b) ((a) < (b) ? (a) : (b))
		const size_t pg = ((ptr + PAGE_SIZE) & mask) - ptr;
		const size_t toset = MIN(pg, size);
		#undef MIN

		const uintptr_t d = pt_addr(pt, ptr);

		if (!d)
			return -1;

		memset(va(d), c, toset);
		ptr += toset;
		size -= toset;
	}
	return 0;
}

int mcopy(struct mm *dst, uintptr_t to, struct mm *src, uintptr_t from,
			size_t size)
{
	const uintptr_t mask = ~((uintptr_t)PAGE_SIZE - 1);

	const pte_t *dst_pt = va(dst->cr3);
	const pte_t *src_pt = va(src->cr3);

	uintptr_t dst_ptr = to;
	uintptr_t src_ptr = from;

	while (size) {
		#define MIN(a, b) ((a) < (b) ? (a) : (b))
		const size_t dst_pg = ((dst_ptr + PAGE_SIZE) & mask) - dst_ptr;
		const size_t src_pg = ((src_ptr + PAGE_SIZE) & mask) - src_ptr;
		const size_t tocopy = MIN(MIN(dst_pg, src_pg), size);
		#undef MIN

		const uintptr_t d = pt_addr(dst_pt, dst_ptr);
		const uintptr_t s = pt_addr(src_pt, src_ptr);

		if (!s || !d)
			return -1;

		memcpy(va(d), va(s), tocopy);
		dst_ptr += tocopy;
		src_ptr += tocopy;
		size -= tocopy;
	}
	return 0;
}

int mm_copy(struct mm *dst, struct mm *src)
{
	struct list_head *head = &src->vmas;

	for (struct list_head *ptr = head->next; ptr != head; ptr = ptr->next) {
		struct vma *vma = (struct vma *)ptr;

		if (mmap(dst, vma->begin, vma->end, vma->perm)) {
			munmap(dst, 0, HIGHER_BASE & USER_MASK);
			return -1;
		}

		/* mcopy can only fail if there is no mapping */
		mcopy(dst, vma->begin, src, vma->begin, vma->end - vma->begin);
	}
	return 0;
}


int munmap(struct mm *mm, uintptr_t from, uintptr_t to)
{
	struct list_head *head = &mm->vmas;
	struct list_head *prev = head;
	struct list_head *next = head;

	for (struct list_head *ptr = head->next; ptr != head; ptr = ptr->next) {
		struct vma *vma = (struct vma *)ptr;

		/* find the last before [from; to) */
		if (vma->end <= from) {
			prev = &vma->ll;
			continue;
		}

		/* find the first after [from; to) */
		if (vma->begin >= to) {
			next = &vma->ll;
			break;
		}

		/* found vma paritally overlapping with [from; to) - error */
		if (vma->begin < from || vma->end > to)
			return -1;
	}

	struct list_head lst;

	list_init(&lst);

	/**
	 * TODO: i would create a separate function for this operation
	 *       if i knew how to call it
	 **/
	if (prev->next != next) {
		__list_splice(prev->next, next->prev, &lst);
		prev->next = next;
		next->prev = prev;
	}

	for (struct list_head *ptr = lst.next; ptr != &lst;) {
		struct vma *vma = (struct vma *)ptr;

		ptr = ptr->next;
		slab_cache_free(&vma_slab, vma);
	}

	from = 0;
	to = HIGHER_BASE & USER_MASK;
	if (prev != head) from = ((struct vma *)prev)->end;
	if (next != head) to = ((struct vma *)next)->begin;
	pt_unmap(va(mm->cr3), from, to - from);
	return 0;
}

static pte_t user_flags(unsigned perm)
{
	pte_t flags = PTE_USER | PTE_PRESENT;

	if (perm & VMA_ACCESS_WRITE)
		flags |= PTE_WRITE;
	return flags;
}

int mmap(struct mm *mm, uintptr_t from, uintptr_t to, unsigned perm)
{
	if (to > HIGHER_BASE)
		return -1;

	from &= USER_MASK;
	to &= USER_MASK;

	if (from == to)
		return 0;

	if (from > to)
		return -1;

	const pte_t flags = user_flags(perm);
	struct list_head *head = &mm->vmas;
	struct list_head *next = head->next;

	for (; next != head; next = next->next) {
		struct vma *vma = (struct vma *)next;

		if (vma->begin >= to)
			break;

		/* we don't allow overlapped mappings */
		if (vma->end > from && vma->begin < to)
			return -1;
	}

	struct vma *vma = slab_cache_alloc(&vma_slab);

	if (!vma)
		return -1;

	if (pt_map(va(mm->cr3), from, to - from, flags)) {
		/**
		 * This part is really tricky, the problem is that when we
		 * unmap a region of logical address space a region an
		 * internal page table is responsible might overlap with the
		 * region we want to unmap but not be fully inside/otside it.
		 *
		 * For such internal page tables we need means to detect
		 * whether there is no mapped ranges using these internal
		 * page tables and we can release them or not. This problem
		 * might be solved with some kind of reference counter, but
		 * i decided to do it another way.
		 *
		 * My solution is to look at the neighbours and extend the
		 * unmapped region boundaries as wide as possible. So that all
		 * internal pages that can be released would be inside the
		 * unmapped region, that is why i keep track of vmas.
		 **/
		uintptr_t begin = 0, end = HIGHER_BASE & USER_MASK;

		if (next->prev != head) {
			const struct vma *p = (struct vma *)next->prev;

			begin = p->end;
		}

		if (next != head) {
			const struct vma *p = (struct vma *)next;

			begin = p->begin;
		}

		pt_unmap(va(mm->cr3), begin, end - begin);
		return -1;
	}

	vma->begin = from;
	vma->end = to;
	vma->perm = perm;
	list_add_before(&vma->ll, next);
	return 0;
}


void mm_setup(void)
{
	slab_cache_setup(&mm_slab, sizeof(struct mm));
	slab_cache_setup(&vma_slab, sizeof(struct vma));
}
