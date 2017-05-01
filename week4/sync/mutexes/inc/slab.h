#ifndef __SLAB_H__
#define __SLAB_H__

#include <stddef.h>

#include <lock.h>
#include <list.h>

/**
 * This is a simplified version of classical SLAB allocator algorithm.
 * Simplified because we dropped some of the optimisation described in
 * the original paper, specifically:
 *  - cache coloring
 *  - constructors/destructors
 **/

struct slab_cache {
	struct spinlock lock;
	struct list_head full;
	struct list_head partial;
	struct list_head empty;

	int slab_order;
	size_t slab_size;
	size_t obj_size;
};


void __slab_cache_setup(struct slab_cache *cache, size_t size, size_t align);
void slab_cache_setup(struct slab_cache *cache, size_t size);
void slab_cache_release(struct slab_cache *cache);

void slab_cache_shrink(struct slab_cache *cache);
void *slab_cache_alloc(struct slab_cache *cache);
void slab_cache_free(struct slab_cache *cache, void *ptr);

#endif /*__SLAB_H__*/
