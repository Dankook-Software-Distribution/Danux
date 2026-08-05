#ifndef DANUX_LIST_H
#define DANUX_LIST_H

#include <stdbool.h>
#include <stddef.h>

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

static inline void list_add_prev(struct list_head *base, struct list_head *node) {
	list_head_link(base->prev, node);
	list_head_link(node, base);
}

static inline void list_add_next(struct list_head *base, struct list_head *node) {
	list_head_link(node, base->next);
	list_head_link(base, node);
}

static inline void list_del(struct list_head *node) {
	list_head_link(node->prev, node->next);
	list_head_init(node);
}

// A list is empty if it is a singleton.
static inline bool list_empty(struct list_head *node) {
	return node->next == node;
}

/*
 * list_entry: returns the pointer to the enclosing struct.
 * node: pointer to list_head.
 * type: type of the enclosing struct.
 * member: name of list_head in the enclosing struct.
 *
 * For example, to get the pointer to the enclosing page struct, one would use list_entry(node, struct page, linkage).
 */
#define list_entry(node, type, member) \
	((type *) ((char *) (node) - offsetof(type, member)))

/*
 * list_for_each_entry: defines a for loop header to iterate over the list entries.
 * it: name of the iterator variable, must be pre-defined before the call.
 * head: pointer to the head of the list.
 * member: name of list_head in the enclosing struct.
 *
 * Example usage:
 *
 * struct page *p;
 * list_for_each_entry(p, head, linkage) {...}
 */
#define list_for_each_entry(it, head, member) \
	for ( \
		it = list_entry((head)->next, typeof(*it), member); \
		&it->member != (head); \
		it = list_entry(it->member.next, typeof(*it), member) \
	)

#endif
