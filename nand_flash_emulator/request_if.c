#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "queue.h"
#include "request_if.h"

struct if_queue_type *ftl_to_nand;
struct if_queue_type *nand_to_ftl;
struct if_queue_alloc_info_t if_q_alloc_info;

void init_request_if_queue()
{
	int i;
	struct if_queue_node *reserved_if_q_node;

	reserved_if_q_node = (struct if_queue_node *)malloc(sizeof(struct if_queue_node) * RESERVED_QUEUE_SIZE);

	if_q_alloc_info.remainder = RESERVED_QUEUE_SIZE;
	if_q_alloc_info.head = if_q_alloc_info.if_queue_alloc_table;
	if_q_alloc_info.tail = &if_q_alloc_info.if_queue_alloc_table[RESERVED_QUEUE_SIZE - 1];

	for (i = 0; i < RESERVED_QUEUE_SIZE - 1; i++)
	{
		if_q_alloc_info.if_queue_alloc_table[i].container = &reserved_if_q_node[i];
		if_q_alloc_info.if_queue_alloc_table[i].next = &if_q_alloc_info.if_queue_alloc_table[i + 1];
	}
	if_q_alloc_info.if_queue_alloc_table[i].container = &reserved_if_q_node[i];
	if_q_alloc_info.if_queue_alloc_table[i].next = NULL;

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

struct if_queue_node* alloc_if_queue()
{
	struct if_queue_node *if_q_node;
	struct if_queue_alloc_t *temp_if_q_alloc;

	assert(if_q_alloc_info.remainder > 0);
	temp_if_q_alloc = if_q_alloc_info.head;

	if_q_node = temp_if_q_alloc->container;

	if_q_alloc_info.head = if_q_alloc_info.head->next;
	temp_if_q_alloc->container = NULL;

	if (if_q_alloc_info.tail->next != NULL)
	{
		temp_if_q_alloc->next = if_q_alloc_info.tail->next;
		if_q_alloc_info.tail->next = temp_if_q_alloc;
	}
	else
	{
		temp_if_q_alloc->next = NULL;
		if_q_alloc_info.tail->next = temp_if_q_alloc;
	}

	if_q_alloc_info.remainder--;

	return if_q_node;
}

void free_if_queue(struct if_queue_node *p_free_node)
{
	assert(if_q_alloc_info.remainder < RESERVED_QUEUE_SIZE);
	assert(if_q_alloc_info.tail->next != NULL);
	if_q_alloc_info.remainder++;
	if_q_alloc_info.tail = if_q_alloc_info.tail->next;
	if_q_alloc_info.tail->container = p_free_node;
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
