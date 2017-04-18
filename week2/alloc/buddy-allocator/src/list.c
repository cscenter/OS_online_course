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

int list_empty(const struct list_head *list)
{
	return list->next == list;
}
