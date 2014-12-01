#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "event_queue.h"
#include "configuration.h"

struct queue_node
{
	struct ftl_request ftl_req;
	struct queue_node *next;
};

struct queue_type
{
	struct queue_node *head;
	struct queue_node *tail;
	int num_of_entries;
};

struct queue_node_alloc_t
{
	struct queue_node *container;
	struct queue_node_alloc_t *next;
};

struct queue_alloc_info_t
{
	int remainder;
	struct queue_node_alloc_t *head;
	struct queue_node_alloc_t *tail;
	struct queue_node_alloc_t queue_alloc_table[RESERVED_QUEUE_SIZE];
};

void init_queue(struct queue_type **p_queue);
struct queue_node* alloc_queue();
void free_queue(struct queue_node *p_free_node);
void enqueue(struct queue_type *p_queue, struct ftl_request p_ftl_req);
struct ftl_request dequeue(struct queue_type *p_queue);

#endif