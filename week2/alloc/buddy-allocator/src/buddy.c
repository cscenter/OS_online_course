#include <buddy.h>
#include <balloc.h>
#include <print.h>

#include <stdint.h>
#include <stddef.h>


#define PAGE_FREE_MASK	0x1ul


/**
 * Every zone is a buddy allocator responsible for contigous range
 * of the physical memory. We link all zones together in a linked
 * list.
 **/
struct zone {
	struct list_head ll;

	/* page indexes (not physical addresses) */
	uintptr_t begin;
	uintptr_t end;

	struct list_head order[MAX_ORDER + 1];
	struct page page[1];
};


static int page_order(const struct page *page)
{
	return page->order;
}

static void page_set_order(struct page *page, int order)
{
	page->order = order;
}

static int page_free(const struct page *page)
{
	return (page->flags & PAGE_FREE_MASK) != 0;
}

static void page_set_free(struct page *page)
{
	page->flags |= PAGE_FREE_MASK;
}

static void page_set_busy(struct page *page)
{
	page->flags &= ~PAGE_FREE_MASK;
}


static struct list_head buddy_zones;


static void buddy_zone_create(uintptr_t begin, uintptr_t end)
{
	if (begin >= end)
		return;

	const uintptr_t pages = (end - begin) / PAGE_SIZE;
	const size_t size = sizeof(struct zone) +
				sizeof(struct page) * (pages - 1);
	const uintptr_t phys = balloc_alloc(size, PAGE_SIZE);

	if (!phys) {
		printf("Failed to allocate zone\n");
		while (1);
	}

	struct zone *zone = va(phys);

	zone->begin = begin / PAGE_SIZE;
	zone->end = end / PAGE_SIZE;
	for (int i = 0; i <= MAX_ORDER; ++i)
		list_init(&zone->order[i]);
	list_add_tail(&zone->ll, &buddy_zones);
}

static struct zone *buddy_find_zone(uintptr_t phys)
{
	struct list_head *head = &buddy_zones;

	phys /= PAGE_SIZE;
	for (struct list_head *ptr = head->next; ptr != head; ptr = ptr->next) {
		struct zone *zone = (struct zone *)ptr;

		if (zone->begin <= phys && zone->end > phys)
			return zone;
	}
	return 0;
}

static struct zone *page_zone(const struct page *page)
{
	struct list_head *head = &buddy_zones;

	for (struct list_head *ptr = head->next; ptr != head; ptr = ptr->next) {
		struct zone *zone = (struct zone *)ptr;
		const size_t pages = zone->end - zone->begin;

		if (page >= zone->page && page < &zone->page[pages])
			return zone;
	}
	return 0;
}

static void buddy_zone_free(uintptr_t begin, uintptr_t end)
{
	if (begin >= end)
		return;

	struct zone *zone = buddy_find_zone(begin);

	if (!zone || zone->end < end / PAGE_SIZE) {
		printf("There is no zone including free range 0x%llx-0x%llx\n",
					(unsigned long long)begin,
					(unsigned long long)end);
		while (1);
	}

	begin /= PAGE_SIZE;
	end /= PAGE_SIZE;

	/**
	 * Since range not neccessary consists of 2^i pages this might
	 * look a bit complicated.
	 **/
	for (uintptr_t page = begin; page != end;) {
		int order;

		for (order = 0; order < MAX_ORDER; ++order) {
			/* page is not aligned */
			if (page & (1ull << order))
				break;
			/* range is too large */
			if (page + (1ull << (order + 1)) > end)
				break;
		}

		const size_t pages = (size_t)1 << order;
		struct page *ptr = &zone->page[page - zone->begin];

		page_set_order(ptr, order);
		page_set_free(ptr);
		list_add_tail(&ptr->ll, &zone->order[order]);
		page += pages;
	}
}

void buddy_setup(void)
{
	const uintptr_t mask = ~(uintptr_t)PAGE_MASK;

	list_init(&buddy_zones);

	/* For every known physical memory range create it's own zone. */
	const size_t ranges = balloc_ranges();
	for (size_t i = 0; i != ranges; ++i) {
		struct balloc_range range;

		balloc_get_range(i, &range);

		/**
		 * Buddy allocator works only with the whole pages, so align
		 * the range begining and ending on PAGE_SIZE border.
		 **/
		const uintptr_t begin = (range.begin + PAGE_SIZE - 1) & mask;
		const uintptr_t end = range.end & mask;

		buddy_zone_create(begin, end);
	}

	/**
	 * All zones are initially empty (contains no free pages), so
	 * the next step is to go over all free ranges and actually free them.
	 **/
	const size_t free_ranges = balloc_free_ranges();
	for (size_t i = 0; i != free_ranges; ++i) {
		struct balloc_range range;

		balloc_get_free_range(i, &range);

		const uintptr_t begin = (range.begin + PAGE_SIZE - 1) & mask;
		const uintptr_t end = range.end & mask;

		buddy_zone_free(begin, end);
	}
}


/* Main buddy allocator alloc routine. */
static struct page *buddy_alloc_zone(struct zone *zone, int order)
{
	int current = order;

	while (current <= MAX_ORDER && list_empty(&zone->order[current]))
		++current;

	if (current > MAX_ORDER)
		return 0;

	struct page *page = (struct page *)(zone->order[current].next);
	const uintptr_t idx = zone->begin + (page - zone->page);

	page_set_busy(page);
	list_del(&page->ll);

	while (current != order) {
		/* Find index of the buddy descriptor. */
		const uintptr_t bidx = idx ^ (1ull << --current);
		struct page *buddy = zone->page + (bidx - zone->begin);

		/* Split block in halfs and return buddy to the allocator. */
		page_set_order(buddy, current);
		page_set_free(buddy);
		list_add(&buddy->ll, &zone->order[current]);
	}

	return page;
}


/* Main buddy allocator free routine. */
static void buddy_free_zone(struct zone *zone, struct page *page, int order)
{
	uintptr_t idx = zone->begin + (page - zone->page);

	while (order < MAX_ORDER) {
		/* Find buddy index and check it's exists and free. */
		const uintptr_t bidx = idx ^ (1ull << order);

		if (bidx < zone->begin || bidx >= zone->end)
			break;

		struct page *buddy = &zone->page[bidx - zone->begin];

		if (!page_free(buddy) || page_order(buddy) != order)
			break;

		/* Buddy is free, remove it from the list and unite halfs. */
		list_del(&buddy->ll);
		++order;

		/**
		 * If the buddy was to the left of the page it becomes a
		 * representative of a new united block
		 **/
		if (bidx < idx) {
			page = buddy;
			idx = bidx;
		}
	}

	/* Finally return united block of pages to the allocator. */
	page_set_order(page, order);
	page_set_free(page);
	list_add(&page->ll, &zone->order[order]);
}


struct page *__buddy_alloc(int order)
{
	struct list_head *head = &buddy_zones;

	for (struct list_head *ptr = head->next; ptr != head; ptr = ptr->next) {
		struct zone *zone = (struct zone *)ptr;
		struct page *page = buddy_alloc_zone(zone, order);

		if (page)
			return page;
	}
	return 0;
}

uintptr_t buddy_alloc(int order)
{
	struct list_head *head = &buddy_zones;

	for (struct list_head *ptr = head->next; ptr != head; ptr = ptr->next) {
		struct zone *zone = (struct zone *)ptr;
		struct page *page = buddy_alloc_zone(zone, order);

		if (page)
			return (zone->begin + (page - zone->page)) * PAGE_SIZE;
	}
	return 0;
}

void __buddy_free(struct page *page, int order)
{
	struct zone *zone = page_zone(page);

	buddy_free_zone(zone, page, order);
}

void buddy_free(uintptr_t phys, int order)
{
	const uintptr_t idx = phys / PAGE_SIZE;
	struct zone *zone = buddy_find_zone(phys);
	struct page *page = zone->page + (idx - zone->begin);

	buddy_free_zone(zone, page, order);
}


uintptr_t page_addr(const struct page *page)
{
	struct zone *zone = page_zone(page);
	const uintptr_t idx = zone->begin + (page - zone->page);

	return idx * PAGE_SIZE;
}

struct page *addr_page(uintptr_t phys)
{
	struct zone *zone = buddy_find_zone(phys);
	const uintptr_t idx = phys / PAGE_SIZE;

	return &zone->page[idx - zone->begin];
}
