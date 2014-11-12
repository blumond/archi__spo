#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "event_queue.h"

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

void init_queue(struct queue_type **p_queue);

void enqueue(struct queue_type *p_queue, struct ftl_request p_ftl_req);
struct ftl_request dequeue(struct queue_type *p_queue);

#endif