#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "configuration.h"
#include "event_queue.h"
#include "request_queue.h"
#include "type.h"
#include "flash_operation_unit.h"
#include "flash_memory.h"

#define MAX_REQ_Q_CAPACITY 16

struct request_queue_type *request_queue_arr;

//알고리즘에따라 다르게 동작할 수 있도록 interface는 통일하고 implementation은 별도로 한다.
//queue 관련 function call 이전에 수행 가능한지 먼저 검사 한다. (full, empty...)

void init_request_queue(struct request_queue_type **p_request_queue_arr)
{
	int i;
	
	*p_request_queue_arr = (struct request_queue_type *)malloc(sizeof(struct request_queue_type) * NUM_OF_BUS * CHIPS_PER_BUS);
	if (*p_request_queue_arr == NULL)
	{
		printf("can't alloc request_queue\n");
		assert(*p_request_queue_arr != NULL);
	}

	for (i = 0; i < NUM_OF_BUS * CHIPS_PER_BUS; i++)
	{
		(*p_request_queue_arr)[i].num_of_entry = 0;
		(*p_request_queue_arr)[i].req_q_head = NULL;
		(*p_request_queue_arr)[i].req_q_tail = NULL;
	}
}

int enqueue_request_queue(int p_channel, int p_way, struct ftl_request p_ftl_req, struct request_queue_type *p_rq_q_array)
{
	struct request_queue_node *new_request_node;
	struct request_queue_node *rq_q_iter;
	struct request_queue_type *rq_q;
	rq_q = &(p_rq_q_array[p_channel + p_channel * p_way]);

#ifdef DEBUG_MODE
	//queue full check는 enqueue하는 곳에서 수행
	assert(rq_q->num_of_entry <= MAX_REQ_Q_CAPACITY);
#endif
	
	rq_q->num_of_entry++;

	new_request_node = (struct request_queue_node *)malloc(sizeof(struct request_queue_node));
	new_request_node->ftl_req = p_ftl_req;
	new_request_node->next = NULL;
	new_request_node->prev = NULL;

	//tail부터 search 하다가 같은 id 나오면 바로 뒤에 enqueue
	if (rq_q->req_q_tail == NULL)
	{
		//assert(is_same_addr(wq[channel][way].ftl_req.addr, ftl_req.addr, (ftl_req.cmd == PAGE_PROGRAM_MP)));

		rq_q->req_q_tail = new_request_node;
		rq_q->req_q_head = new_request_node;
	}
	else
	{
		for (rq_q_iter = rq_q->req_q_tail; rq_q_iter != NULL; rq_q_iter = rq_q_iter->prev)
		{
			if (rq_q_iter->ftl_req.id == p_ftl_req.id)
			{
				//tail에 연결
				if (rq_q_iter == rq_q->req_q_tail)
				{
					rq_q_iter->next = new_request_node;
					new_request_node->prev = rq_q_iter;
					rq_q->req_q_tail = new_request_node;
					break;
				}
				else//중간에 연결
				{
					rq_q_iter->next->prev = new_request_node;
					new_request_node->next = rq_q_iter->next;
					new_request_node->prev = rq_q_iter;
					rq_q_iter->next = new_request_node;
					break;
				}
			}
		}
		//같은 id가 없는 경우는 head에 연결
		if (rq_q_iter == NULL)
		{
			//assert(is_same_addr(wq[channel][way].ftl_req.addr, ftl_req.addr, (ftl_req.cmd == PAGE_PROGRAM_MP)));
			rq_q->req_q_head->prev = new_request_node;
			new_request_node->next = rq_q->req_q_head;
			rq_q->req_q_head = new_request_node;
		}
	}

	return SUCCESS;
}

struct ftl_request dequeue_request_queue(int p_channel, int p_way, struct request_queue_type *p_rq_q_array)
{
	struct request_queue_node *rq_head;
	struct ftl_request ftl_req;
	struct request_queue_type *rq_q;

	rq_q = &(p_rq_q_array[p_channel + p_channel * p_way]);

#ifdef DEBUG_MODE
	//empty check는 dequeue를 호출하는 곳에서 수행
	assert(rq_q->num_of_entry > 0);
#endif
	rq_q->num_of_entry--;
	
	rq_head = rq_q->req_q_head;
	
	rq_q->req_q_head = rq_q->req_q_head->next;
	if (rq_q->req_q_head != NULL)
	{
		rq_q->req_q_head->prev = NULL;
	}
	else
	{
		rq_q->req_q_tail = NULL;
	}
	
	ftl_req = rq_head->ftl_req;
	free(rq_head);

	return ftl_req;
}
