#include <stdint.h>

#include <buddy.h>
#include <balloc.h>
#include <ints.h>
#include <list.h>
#include <memory.h>
#include <paging.h>
#include <print.h>
#include <string.h>
#include <vga.h>


static void qemu_gdb_hang(void)
{
#ifdef DEBUG
	static volatile int wait = 1;

	while (wait);
#endif
}


static size_t allocate_all(int order, struct list_head *list)
{
	size_t count = 0;

	while (1) {
		struct page *page = __buddy_alloc(order);

		if (!page)
			break;

		list_add_tail(&page->ll, list);
		++count;
	}
	return count;
}

static void write_all(int order, struct list_head *list)
{
	struct list_head *head = list;

	for (struct list_head *ptr = head->next; ptr != head; ptr = ptr->next) {
		struct page *page = (struct page *)ptr;
		const uintptr_t phys = page_addr(page);

		memset(va(phys), 3148, PAGE_SIZE << order);
	}
}

static void release_all(int order, struct list_head *list)
{
	struct list_head *head = list;

	for (struct list_head *ptr = head->next; ptr != head;) {
		struct page *page = (struct page *)ptr;

		ptr = ptr->next;
		__buddy_free(page, order);
	}
	list_init(list);
}

static void buddy_test(void)
{
	struct list_head list;

	list_init(&list);

	for (int i = 0; i <= MAX_ORDER; ++i) {
		const size_t count = allocate_all(i, &list);

		write_all(i, &list);
		release_all(i, &list);
		if (!count)
			break;
		printf("Allocated %llu blocks of %llu bytes\n",
					(unsigned long long)count,
					(unsigned long long)(PAGE_SIZE << i));
	}
}


void main(uintptr_t mb_info_phys)
{
	const struct multiboot_info *info = va(mb_info_phys);

	qemu_gdb_hang();
	vga_clr();
	ints_setup();
	balloc_setup(info);
	paging_setup();
	buddy_setup();

	buddy_test();

	while (1);
}
