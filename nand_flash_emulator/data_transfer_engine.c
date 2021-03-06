#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "data_transfer_engine.h"

struct dte_request_queue *dte_req_q;
struct dte_alloc_info_t dte_alloc_info;

void init_dte_request_queue(struct dte_request_queue **p_dte_req_q)
{
	int i;
	struct dte_request *reserved_dte_request;

	*p_dte_req_q = (struct dte_request_queue *)malloc(sizeof(struct dte_request_queue));
	if (*p_dte_req_q == NULL)
	{
		printf("can't alloc dte_request_queue\n");
		assert(*p_dte_req_q != NULL);
	}

	reserved_dte_request = (struct dte_request *)malloc(sizeof(struct dte_request) * RESERVED_QUEUE_SIZE);

	dte_alloc_info.remainder = RESERVED_QUEUE_SIZE;
	dte_alloc_info.head = dte_alloc_info.dte_alloc_table;
	dte_alloc_info.tail = &dte_alloc_info.dte_alloc_table[RESERVED_QUEUE_SIZE - 1];

	for (i = 0; i < RESERVED_QUEUE_SIZE - 1; i++)
	{
		dte_alloc_info.dte_alloc_table[i].container = &reserved_dte_request[i];
		dte_alloc_info.dte_alloc_table[i].next = &dte_alloc_info.dte_alloc_table[i + 1];
	}
	dte_alloc_info.dte_alloc_table[i].container = &reserved_dte_request[i];
	dte_alloc_info.dte_alloc_table[i].next = NULL;

	(*p_dte_req_q)->head = NULL;
	(*p_dte_req_q)->tail = NULL;
	(*p_dte_req_q)->num_of_entries = 0;

	pthread_mutex_init(&(*p_dte_req_q)->mutex, NULL);
}

struct dte_request* alloc_dte_req()
{
	struct dte_request *dte_req;
	struct dte_alloc_queue_t *temp_dte_alloc_queue;
	
	assert(dte_alloc_info.remainder > 0);
	temp_dte_alloc_queue = dte_alloc_info.head;
	
	dte_req = temp_dte_alloc_queue->container;

	dte_alloc_info.head = dte_alloc_info.head->next;
	temp_dte_alloc_queue->container = NULL;

	if (dte_alloc_info.tail->next != NULL)
	{
		temp_dte_alloc_queue->next = dte_alloc_info.tail->next;
		dte_alloc_info.tail->next = temp_dte_alloc_queue;
	}
	else
	{
		temp_dte_alloc_queue->next = NULL;
		dte_alloc_info.tail->next = temp_dte_alloc_queue;
	}

	dte_alloc_info.remainder--;
		
	return dte_req;
}

void free_dte_req(struct dte_request* p_free_node)
{
	assert(dte_alloc_info.remainder < RESERVED_QUEUE_SIZE);
	assert(dte_alloc_info.tail->next != NULL);
	dte_alloc_info.remainder++;
	dte_alloc_info.tail = dte_alloc_info.tail->next;
	dte_alloc_info.tail->container = p_free_node;
}

void dte_request_enqueue(struct dte_request_queue *p_dte_req_q, struct dte_request p_dte_req)
{
	struct dte_request *dte_req;
	struct dte_request *dte_req_iter;

	dte_req = alloc_dte_req();
	*dte_req = p_dte_req;
	dte_req->next = NULL;
	dte_req->prev = NULL;

	//request 순서대로 enqueue
	if (dte_req->deadline == 0)
	{
		if (p_dte_req_q->head != NULL)
		{
			p_dte_req_q->tail->next = dte_req;
			dte_req->prev = p_dte_req_q->tail;
			p_dte_req_q->tail = dte_req;
		}
		else
		{
			p_dte_req_q->head = dte_req;
			p_dte_req_q->tail = dte_req;
		}
		dte_req_q->num_of_entries++;
	}
	//deadline이 빠른 순서대로 enqueue
	else
	{
		if (p_dte_req_q->head != NULL)
		{
			//deadline 순서대로 enqueue
			for (dte_req_iter = p_dte_req_q->head; dte_req_iter != NULL; dte_req_iter = dte_req_iter->next)
			{
				if (dte_req_iter->deadline == 0
					|| dte_req_iter->deadline > dte_req->deadline)
				{
					dte_req->next = dte_req_iter;
					dte_req->prev = dte_req_iter->prev;
					
					if (dte_req_iter->prev != NULL) dte_req_iter->prev->next = dte_req;
					else p_dte_req_q->head = dte_req;
					
					dte_req_iter->prev = dte_req;

					break;
				}
			}
			//tail에 enqueue
			if (dte_req_iter == NULL)
			{
				p_dte_req_q->tail->next = dte_req;
				dte_req->prev = p_dte_req_q->tail;
				p_dte_req_q->tail = dte_req;
			}
		}
		else
		{
			p_dte_req_q->head = dte_req;
			p_dte_req_q->tail = dte_req;
		}
		dte_req_q->num_of_entries++;
	}
}

void set_dte_request_deadline(struct dte_request_queue *p_dte_req_q, int p_id, long long p_deadline)
{
	struct dte_request *dte_req_iter;
	
	for (dte_req_iter = p_dte_req_q->head; dte_req_iter != NULL; dte_req_iter = dte_req_iter->next)
	{
		if (dte_req_iter->id == p_id) break;
	}

	if (dte_req_iter != NULL)
	{
		if (dte_req_iter == p_dte_req_q->head
			|| dte_req_iter->prev->deadline <= dte_req_iter->deadline)
		{
			dte_req_iter->deadline = p_deadline;
		}
		else
		{
			//node 분리
			dte_req_iter->prev->next = dte_req_iter->next;
			if (dte_req_iter->next != NULL)
			{
				dte_req_iter->next->prev = dte_req_iter->prev;
			}
			else p_dte_req_q->tail = dte_req_iter->prev;

			dte_req_iter;

			dte_request_enqueue(p_dte_req_q, *dte_req_iter);
			free_dte_req(dte_req_iter);
		}
	}
}

struct dte_request get_dte_request(struct dte_request_queue *p_dma_req_q)
{
	struct dte_request dte_req;
	struct dte_request *temp_dte_req;

	dte_req = *p_dma_req_q->head;
	temp_dte_req = p_dma_req_q->head;

	p_dma_req_q->head = p_dma_req_q->head->next;
	if (temp_dte_req == p_dma_req_q->tail) p_dma_req_q->tail = NULL;
	p_dma_req_q->num_of_entries--;

	free_dte_req(temp_dte_req);
	return dte_req;
}

void unit_data_transfer(struct dte_request_queue *p_dma_req_q)
{
	struct dte_request *dte_req;

	dte_req = p_dma_req_q->head;
	memcpy(dte_req->dst, dte_req->src, DATA_TRANSFER_UNIT);

	pthread_mutex_lock(&dte_req_q->mutex);
	dte_req->src = dte_req->src + DATA_TRANSFER_UNIT;
	dte_req->dst = dte_req->dst + DATA_TRANSFER_UNIT;
	dte_req->size = dte_req->size - DATA_TRANSFER_UNIT;

	if (dte_req->size == 0)
	{
		get_dte_request(p_dma_req_q);
	}
	pthread_mutex_unlock(&dte_req_q->mutex);
}

int is_data_transfer_done(struct dte_request_queue *p_dma_req_q, int p_id)
{
	struct dte_request *dte_req_iter;
	int ret = 0;

	for (dte_req_iter = p_dma_req_q->head; dte_req_iter != NULL; dte_req_iter = dte_req_iter->next)
	{
		if (dte_req_iter->id == p_id) break;
	}

	if (dte_req_iter == NULL) ret = 1;
	
	return ret;
}

void *data_transfer_engine()
{
	int in = 0;
	while (1)
	{
		pthread_mutex_lock(&dte_req_q->mutex);
		if (dte_req_q->num_of_entries)
		{
			in = 1;
		}
		pthread_mutex_unlock(&dte_req_q->mutex);
		if (in)
		{
			unit_data_transfer(dte_req_q);
			in = 0;
		}
	}
}
