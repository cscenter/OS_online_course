#ifndef __MM_H__
#define __MM_H__

#include <buddy.h>
#include <list.h>
#include <stddef.h>
#include <stdint.h>


enum vma_access {
	VMA_ACCESS_READ,
	VMA_ACCESS_WRITE,
	VMA_ACCESS_EXECUTE
};

enum vma_perm {
	VMA_PERM_READ = (1u << VMA_ACCESS_READ),
	VMA_PERM_WRITE = (1u << VMA_ACCESS_WRITE),
	VMA_PERM_EXECUTE = (1u << VMA_ACCESS_EXECUTE)
};

struct vma {
	struct list_head ll;
	uintptr_t begin;
	uintptr_t end;
	unsigned perm;
};

struct mm {
	/* list of mapped region descriptors */
	struct list_head vmas;

	/* root page table */
	struct page *pt;
	uintptr_t cr3;
};

/**
 * We support only one thread per process, so every thread needs it's own
 * struct mm, that represents the process logical address space.
 **/
struct mm *mm_create(void);
void mm_release(struct mm *mm);
int mm_copy(struct mm *dst, struct mm *src);

/* Create/delete mapping with given permissons */
int mmap(struct mm *mm, uintptr_t from, uintptr_t to, unsigned perm);
int munmap(struct mm *mm, uintptr_t from, uintptr_t to);

/* Copy data to the other process address space */
int mcopy(struct mm *dst, uintptr_t to, struct mm *src, uintptr_t from,
			size_t size);
/* Fill data in the other process address space */
int mset(struct mm *dst, uintptr_t to, int c, size_t size);

void mm_setup(void);

#endif /*__MM_H__*/
