#ifndef __LIST_H__
#define __LIST_H__


struct list_head {
	struct list_head *next;
	struct list_head *prev;
};


void list_init(struct list_head *list);
void list_add_tail(struct list_head *node, struct list_head *list);
void list_add(struct list_head *node, struct list_head *list);


static inline void list_add_after(struct list_head *node,
			struct list_head *ptr)
{
	list_add(node, ptr);
}

static inline void list_add_before(struct list_head *node,
			struct list_head *ptr)
{
	list_add_tail(node, ptr);
}

void list_del(struct list_head *node);
void __list_splice(struct list_head *first, struct list_head *last,
                        struct list_head *ptr);
void list_splice(struct list_head *from, struct list_head *to);
void list_splice_tail(struct list_head *from, struct list_head *to);
int list_empty(const struct list_head *list);

#endif /*__LIST_H__*/
