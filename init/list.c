#include <danux/list.h>

void list_add_prev(struct list_head *base, struct list_head *node) {
	list_head_link(base->prev, node);
	list_head_link(node, base);
}

void list_add_next(struct list_head *base, struct list_head *node) {
	list_head_link(node, base->next);
	list_head_link(base, node);
}

void list_del(struct list_head *node) {
	list_head_link(node->prev, node->next);
	list_head_init(node);
}
