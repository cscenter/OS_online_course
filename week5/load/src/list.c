#include <list.h>


void list_init(struct list_head *list)
{
	list->next = list->prev = list;
}

static void list_insert(struct list_head *node, struct list_head *prev,
			struct list_head *next)
{
	node->prev = prev;
	node->next = next;
	prev->next = node;
	next->prev = node;
}

void list_add_tail(struct list_head *node, struct list_head *list)
{
	list_insert(node, list->prev, list);
}

void list_add(struct list_head *node, struct list_head *list)
{
	list_insert(node, list, list->next);
}

void list_del(struct list_head *node)
{
	struct list_head *prev = node->prev;
	struct list_head *next = node->next;

	prev->next = next;
	next->prev = prev;
}

void __list_splice(struct list_head *first, struct list_head *last,
			struct list_head *prev)
{
	struct list_head *next = prev->next;

	first->prev = prev;
	last->next = next;
	prev->next = first;
	next->prev = last;
}

void list_splice(struct list_head *from, struct list_head *to)
{
	if (list_empty(from))
		return;

	struct list_head *first = from->next;
	struct list_head *last = from->prev;

	list_init(from);
	__list_splice(first, last, to);	
}

void list_splice_tail(struct list_head *from, struct list_head *to)
{
	if (list_empty(from))
		return;

	struct list_head *first = from->next;
	struct list_head *last = from->prev;

	list_init(from);
	__list_splice(first, last, to->prev);	
}

int list_empty(const struct list_head *list)
{
	return list->next == list;
}
