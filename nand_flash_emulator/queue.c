#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "queue.h"

void init_queue(struct queue_type **p_queue)
{
	*p_queue = (struct queue_type *)malloc(sizeof(struct queue_type));
	if (*p_queue == NULL)
	{
		printf("can't alloc queue\n");
		assert(*p_queue != NULL);
	}

	(*p_queue)->head = NULL;
	(*p_queue)->tail = NULL;
	(*p_queue)->num_of_entries = 0;
}

void enqueue(struct queue_type *p_queue, struct ftl_request p_ftl_req)
{
	struct queue_node *queue_node;

	queue_node = (struct queue_node *)malloc(sizeof(struct queue_node));
	queue_node->ftl_req = p_ftl_req;
	queue_node->next = NULL;

	p_queue->num_of_entries++;

	if (p_queue->head == NULL)
	{
		p_queue->head = queue_node;
		p_queue->tail = queue_node;
	}
	else
	{
		p_queue->tail->next = queue_node;
		p_queue->tail = queue_node;
	}
}

struct ftl_request dequeue(struct queue_type *p_queue)
{
	struct queue_node *queue_node;
	struct ftl_request ftl_req;

	p_queue->num_of_entries--;
	queue_node = p_queue->head;
	p_queue->head = p_queue->head->next;
	if (p_queue->head == NULL) p_queue->tail = NULL;
	
	ftl_req = queue_node->ftl_req;
	free(queue_node);

	return ftl_req;
}
