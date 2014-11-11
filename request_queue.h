#ifndef __REQUEST_QUEUE_H__
#define __REQUEST_QUEUE_H__

#include "event_queue.h"

struct request_queue_node
{
	struct ftl_request ftl_req;
	struct request_queue_node *next;
	struct request_queue_node *prev;
};

struct request_queue_type
{
	struct request_queue_node *req_q_head;
	struct request_queue_node *req_q_tail;
	int num_of_entry;
};

extern struct request_queue_type *request_queue_arr;

void init_request_queue(struct request_queue_type **p_request_queue_arr);
int enqueue_request_queue(int p_channel, int p_way, struct ftl_request p_ftl_req, struct request_queue_type *p_rq_q_array);
struct ftl_request dequeue_request_queue(int p_channel, int p_way, struct request_queue_type *p_rq_q_array);

#endif