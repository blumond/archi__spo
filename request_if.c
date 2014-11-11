#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "queue.h"
#include "request_if.h"

struct if_queue_type *ftl_to_nand;
struct if_queue_type *nand_to_ftl;

void init_request_if_queue()
{
	init_if_queue(&ftl_to_nand);
	init_if_queue(&nand_to_ftl);
}

void init_if_queue(struct if_queue_type **p_queue)
{
	*p_queue = (struct if_queue_type *)malloc(sizeof(struct if_queue_type));
	if (*p_queue == NULL)
	{
		printf("can't alloc queue\n");
		assert(*p_queue != NULL);
	}

	(*p_queue)->head = NULL;
	(*p_queue)->tail = NULL;
	(*p_queue)->num_of_entries = 0;

	pthread_mutex_init(&(*p_queue)->mutex, NULL);
}

void if_enqueue(struct if_queue_type *p_queue, struct ftl_request p_ftl_req, long long p_time_offset)
{
	struct if_queue_node *new_queue_node;

	new_queue_node = (struct if_queue_node *)malloc(sizeof(struct if_queue_node));
	new_queue_node->eq_node.ftl_req = p_ftl_req;
	new_queue_node->eq_node.time_offset = p_time_offset;
	new_queue_node->next = NULL;

	p_queue->num_of_entries++;

	if (p_queue->head == NULL)
	{
		p_queue->head = new_queue_node;
		p_queue->tail = new_queue_node;
	}
	else
	{
		p_queue->tail->next = new_queue_node;
		p_queue->tail = new_queue_node;
	}
}

struct event_queue_node if_dequeue(struct if_queue_type *p_queue)
{
	struct if_queue_node *del_queue_node;
	struct event_queue_node return_eq_node;

	p_queue->num_of_entries--;
	del_queue_node = p_queue->head;
	p_queue->head = p_queue->head->next;
	if (p_queue->head == NULL) p_queue->tail = NULL;

	return_eq_node = del_queue_node->eq_node;
	free(del_queue_node);

	return return_eq_node;
}
