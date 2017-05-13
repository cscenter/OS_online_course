#include <slab.h>
#include <buddy.h>
#include <print.h>


struct list {
	struct list *next;
};

struct slab {
	struct list_head ll;
	struct page *page;
	struct list *free;
	size_t size;
};


static struct slab *slab_get_meta(struct slab_cache *cache, void *virt)
{
	const size_t bytes = (size_t)PAGE_SIZE << cache->slab_order;

	return (struct slab *)((char *)virt + bytes - sizeof(struct slab));
}

static struct slab *slab_create(struct slab_cache *cache)
{
	struct page *page = __buddy_alloc(cache->slab_order);

	if (!page)
		return 0;

	const uintptr_t phys = page_addr(page);
	char *ptr = va(phys);
	struct slab *slab = slab_get_meta(cache, ptr);
	struct list *head = 0;

	slab->page = page;
	slab->size = cache->slab_size;

	for (size_t i = 0; i != slab->size; ++i, ptr += cache->obj_size) {
		struct list *node = (struct list *)ptr;

		node->next = head;
		head = node;
	}
	slab->free = head;
	return slab;
}

static void slab_destroy(struct slab_cache *cache, struct slab *slab)
{
	__buddy_free(slab->page, cache->slab_order);
}

static void *slab_alloc(struct slab *slab)
{
	struct list *head = slab->free;
	void *ptr = head;

	slab->free = head->next;
	--slab->size;
	return ptr;
}

static void slab_free(struct slab *slab, void *ptr)
{
	struct list *head = ptr;

	head->next = slab->free;
	slab->free = head;
	++slab->size;
}


static size_t slab_align_down(size_t x, size_t align)
{
	return x - x % align;
}

static size_t slab_align_up(size_t x, size_t align)
{
	return slab_align_down(x + align - 1, align);
}

void __slab_cache_setup(struct slab_cache *cache, size_t size, size_t align)
{
	if (size < sizeof(struct list))
		size = sizeof(struct list);

	if (size > (size_t)PAGE_SIZE << MAX_ORDER) {
		printf("Object size is to large to allocate (%llu)\n",
					(unsigned long long)size);
		while (1);
	}

	const size_t obj_size = slab_align_up(size, align);

	/**
	 * We select slab_order so that amortized allocation time will be O(1).
	 * Since we use buddy allocator to allocate slabs which works in
	 * O(MAX_ORDER) time (plus time required to check a zone), one slab
	 * should consist of at least MAX_ORDER objects;
	 **/
	size_t bytes;
	int slab_order;

	for (slab_order = 0; slab_order <= MAX_ORDER; ++slab_order) {
		bytes = ((size_t)PAGE_SIZE << slab_order) - sizeof(struct slab);
		if (bytes / obj_size >= MAX_ORDER)
			break;
	}

	const size_t slab_size = bytes / obj_size;

	spin_setup(&cache->lock);

	list_init(&cache->full);
	list_init(&cache->partial);
	list_init(&cache->empty);

	cache->slab_order = slab_order;
	cache->slab_size = slab_size;
	cache->obj_size = obj_size;
}

void slab_cache_setup(struct slab_cache *cache, size_t size)
{
	const size_t DEFAULT_ALIGN = 8;

	__slab_cache_setup(cache, size, DEFAULT_ALIGN);
}

void slab_cache_release(struct slab_cache *cache)
{
	if (!list_empty(&cache->full) || !list_empty(&cache->partial)) {
		printf("Slab cache still contains busy objects\n");
		while (1);
	}
	slab_cache_shrink(cache);
}


void slab_cache_shrink(struct slab_cache *cache)
{
	struct list_head list;
	struct list_head *head = &list;

	const int enabled = spin_lock_int_save(&cache->lock);

	list_init(&list);
	list_splice(&cache->empty, &list);
	spin_unlock_int_restore(&cache->lock, enabled);

	for (struct list_head *ptr = head->next; ptr != head;) {
		struct slab *slab = (struct slab *)ptr;

		ptr = ptr->next;
		slab_destroy(cache, slab);
	}
}

static void *__slab_cache_alloc(struct slab_cache *cache)
{
	struct slab *slab = 0;

	if (!list_empty(&cache->partial)) {
		slab = (struct slab *)cache->partial.next;
		list_del(&slab->ll);
	} else if (!list_empty(&cache->empty)) {
		slab = (struct slab *)cache->empty.next;
		list_del(&slab->ll);
	} else {
		slab = slab_create(cache);
		if (!slab)
			return 0;
	}

	void *ptr = slab_alloc(slab);

	if (slab->size)
		list_add(&slab->ll, &cache->partial);
	else
		list_add(&slab->ll, &cache->full);
	return ptr;
}

void *slab_cache_alloc(struct slab_cache *cache)
{
	const int enabled = spin_lock_int_save(&cache->lock);
	void *ptr = __slab_cache_alloc(cache);

	spin_unlock_int_restore(&cache->lock, enabled);
	return ptr;
}

static void __slab_cache_free(struct slab_cache *cache, void *ptr)
{
	const size_t bytes = (size_t)PAGE_SIZE << cache->slab_order;
	const uintptr_t addr = ~(bytes - 1) & (uintptr_t)ptr;

	struct slab *slab = slab_get_meta(cache, (void *)addr);

	list_del(&slab->ll);
	slab_free(slab, ptr);

	if (slab->size == cache->slab_size)
		list_add(&slab->ll, &cache->empty);
	else
		list_add(&slab->ll, &cache->partial);
}

void slab_cache_free(struct slab_cache *cache, void *ptr)
{
	const int enabled = spin_lock_int_save(&cache->lock);

	__slab_cache_free(cache, ptr);
	spin_unlock_int_restore(&cache->lock, enabled);
}
