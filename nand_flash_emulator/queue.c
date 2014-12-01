#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "queue.h"

struct queue_alloc_info_t queue_alloc_info;

void init_queue(struct queue_type **p_queue)
{
	int i;
	struct queue_node *reserved_queue_node;

	*p_queue = (struct queue_type *)malloc(sizeof(struct queue_type));
	if (*p_queue == NULL)
	{
		printf("can't alloc queue\n");
		assert(*p_queue != NULL);
	}

	reserved_queue_node = (struct queue_node *)malloc(sizeof(struct queue_node) * RESERVED_QUEUE_SIZE);

	queue_alloc_info.remainder = RESERVED_QUEUE_SIZE;
	queue_alloc_info.head = queue_alloc_info.queue_alloc_table;
	queue_alloc_info.tail = &queue_alloc_info.queue_alloc_table[RESERVED_QUEUE_SIZE - 1];

	for (i = 0; i < RESERVED_QUEUE_SIZE - 1; i++)
	{
		queue_alloc_info.queue_alloc_table[i].container = &reserved_queue_node[i];
		queue_alloc_info.queue_alloc_table[i].next = &queue_alloc_info.queue_alloc_table[i + 1];
	}
	queue_alloc_info.queue_alloc_table[i].container = &reserved_queue_node[i];
	queue_alloc_info.queue_alloc_table[i].next = NULL;

	(*p_queue)->head = NULL;
	(*p_queue)->tail = NULL;
	(*p_queue)->num_of_entries = 0;
}

struct queue_node* alloc_queue()
{
	struct queue_node *q_node;
	struct queue_node_alloc_t *temp_queue_node_alloc;

	assert(queue_alloc_info.remainder > 0);
	temp_queue_node_alloc = queue_alloc_info.head;

	q_node = temp_queue_node_alloc->container;

	queue_alloc_info.head = queue_alloc_info.head->next;
	temp_queue_node_alloc->container = NULL;

	if (queue_alloc_info.tail->next != NULL)
	{
		temp_queue_node_alloc->next = queue_alloc_info.tail->next;
		queue_alloc_info.tail->next = temp_queue_node_alloc;
	}
	else
	{
		temp_queue_node_alloc->next = NULL;
		queue_alloc_info.tail->next = temp_queue_node_alloc;
	}

	queue_alloc_info.remainder--;

	return q_node;
}

void free_queue(struct queue_node *p_free_node)
{
	assert(queue_alloc_info.remainder < RESERVED_QUEUE_SIZE);
	assert(queue_alloc_info.tail->next != NULL);
	queue_alloc_info.remainder++;
	queue_alloc_info.tail = queue_alloc_info.tail->next;
	queue_alloc_info.tail->container = p_free_node;
}

void enqueue(struct queue_type *p_queue, struct ftl_request p_ftl_req)
{
	struct queue_node *queue_node;

	queue_node = alloc_queue();
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
	free_queue(queue_node);

	return ftl_req;
}
