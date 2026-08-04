#ifndef DANUX_LIST_H
#define DANUX_LIST_H

#include <stdbool.h>

// Circular doubly linked-list implementation.
struct list_head {
	struct list_head *prev, *next;
};

static inline void list_head_init(struct list_head *node) {
	node->prev = node->next = node;
}

static inline void list_head_link(struct list_head *node1, struct list_head *node2) {
	node1->next = node2;
	node2->prev = node1;
}

// A list is empty if it is a singleton.
static inline bool list_empty(struct list_head *node) {
	return node->next == node;
}

extern void list_add_prev(struct list_head *, struct list_head *);
extern void list_add_next(struct list_head *, struct list_head *);
extern void list_del(struct list_head *);

#endif
