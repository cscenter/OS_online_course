#include <ramfs.h>

#include <buddy.h>
#include <memory.h>
#include <mutex.h>
#include <slab.h>
#include <string.h>


struct ramfs_page {
	struct list_head ll;
	long offs;
	struct page *page;
};


static struct slab_cache ramfs_slab;
static struct slab_cache page_slab;
static struct list_head ramfs_cache;
static struct mutex ramfs_mtx;


static int __ramfs_open(const char *name, int create, struct file **res)
{
	struct list_head *head = &ramfs_cache;

	for (struct list_head *ptr = head->next; ptr != head; ptr = ptr->next) {
		struct file *file = (struct file *)ptr;

		if (strcmp(name, file->name))
			continue;

		*res = file;
		return 0;
	}

	if (!create)
		return -1;

	struct file *new = slab_cache_alloc(&ramfs_slab);

	new->size = 0;
	strcpy(new->name, name);
	list_add(&new->ll, head);
	list_init(&new->data);
	*res = new;
	return 0;
}

int ramfs_create(const char *name, struct file **res)
{
	int err;

	mutex_lock(&ramfs_mtx);
	err = __ramfs_open(name, 1, res);
	mutex_unlock(&ramfs_mtx);
	return err;
}

int ramfs_open(const char *name, struct file **res)
{
	int err;

	mutex_lock(&ramfs_mtx);
	err = __ramfs_open(name, 0, res);
	mutex_unlock(&ramfs_mtx);
	return err;
}

/**
 * We don't actually need to do anything on close because the filesystem
 * is very simplistic doesn't support deletes and read/write functions
 * are stateless.
 **/
void ramfs_close(struct file *file)
{
	(void) file;
}


static long __ramfs_readat(struct file *file, char *data, long size, long offs)
{
	if (offs >= file->size)
		return 0;

	struct list_head *head = &file->data;

	const long from = offs;
	const long to = offs + size;

	for (struct list_head *ptr = head->next; ptr != head; ptr = ptr->next) {
		struct ramfs_page *rp = (struct ramfs_page *)ptr;

		if (rp->offs + PAGE_SIZE <= from)
			continue;

		if (rp->offs >= to)
			break;

		#define MIN(a, b) ((a) < (b) ? (a) : (b))
		#define MAX(a, b) ((a) < (b) ? (b) : (a))
		const long data_offs = MAX(rp->offs - from, 0);
		const long page_offs = MAX(offs - rp->offs, 0);
		const long to_copy = MIN(PAGE_SIZE - page_offs, to - data_offs);
		const char *page = va(page_addr(rp->page));
		#undef MAX
		#undef MIN

		memcpy(data + data_offs, page + page_offs, to_copy);
	}
	return to >= file->size ? file->size - offs : size;
}

long ramfs_readat(struct file *file, void *data, long size, long offs)
{
	long ret;

	memset(data, 0, size);
	mutex_lock(&ramfs_mtx);
	ret = __ramfs_readat(file, data, size, offs);
	mutex_unlock(&ramfs_mtx);
	return ret;
}

static long __ramfs_writeat(struct file *file, const char *data, long size,
			long offs)
{
	struct list_head *head = &file->data;
	const long from = offs;
	const long to = offs + size;

	if (to > file->size)
		file->size = to;

	for (struct list_head *ptr = head->next; offs != to; ptr = ptr->next) {
		struct ramfs_page *rp = (struct ramfs_page *)ptr;

		if (ptr != head && rp->offs + PAGE_SIZE <= offs)
			continue;

		if (ptr == head || rp->offs > offs) {
			struct ramfs_page *new = slab_cache_alloc(&page_slab);

			if (!new)
				return offs == from ? -1 : offs - from;

			new->page = __buddy_alloc(0);
			if (!new->page) {
				slab_cache_free(&page_slab, new);
				return offs == from ? -1 : offs - from;	
			}

			/* all offsets must be PAGE_SIZE aligned */
			new->offs = offs & ~(PAGE_SIZE - 1l);
			list_add_before(&new->ll, ptr);
			ptr = &new->ll;
			rp = new;
		}

		#define MIN(a, b) ((a) < (b) ? (a) : (b))
		#define MAX(a, b) ((a) < (b) ? (b) : (a))
		const long data_offs = MAX(rp->offs - from, 0);
		const long page_offs = MAX(offs - rp->offs, 0);
		const long to_copy = MIN(PAGE_SIZE - page_offs, to - data_offs);
		char *page = va(page_addr(rp->page));
		#undef MAX
		#undef MIN

		memcpy(page + page_offs, data + data_offs, to_copy);
		offs += to_copy;
	}
	return offs - from;
}

long ramfs_writeat(struct file *file, const void *data, long size, long offs)
{
	long ret;

	mutex_lock(&ramfs_mtx);
	ret = __ramfs_writeat(file, data, size, offs);
	mutex_unlock(&ramfs_mtx);
	return ret;
}

void ramfs_setup(void)
{
	slab_cache_setup(&ramfs_slab, sizeof(struct file));
	slab_cache_setup(&page_slab, sizeof(struct ramfs_page));
	mutex_setup(&ramfs_mtx);
	list_init(&ramfs_cache);
}
