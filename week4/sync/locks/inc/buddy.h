#ifndef __BUDDY_H__
#define __BUDDY_H__

#include <memory.h>
#include <list.h>

/* Maximum possible allocation size 2^20 pages */
#define MAX_ORDER	20

struct page {
	struct list_head ll;
	unsigned long flags;
	int order;
};


void buddy_setup(void);

/**
 * Buddy alloc/free routines are given in two versions:
 *  - one returns descriptor (struct page)
 *  - other resutrns physical address
 **/
struct page *__buddy_alloc(int order);
uintptr_t buddy_alloc(int order);
void __buddy_free(struct page *page, int order);
void buddy_free(uintptr_t phys, int order);


/* Convertion routines: descriptor to physical address and vice versa. */
uintptr_t page_addr(const struct page *page);
struct page *addr_page(uintptr_t phys);

#endif /*__BUDDY_H__*/
