#ifndef __REQUEST_QUEUE_H__
#define __REQUEST_QUEUE_H__

#include "event_queue.h"
#include "configuration.h"

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

struct request_alloc_queue_t
{
	struct request_queue_node *container;
	struct request_alloc_queue_t *next;
};

struct request_alloc_info_t
{
	int remainder;
	struct request_alloc_queue_t *head;
	struct request_alloc_queue_t *tail;
	struct request_alloc_queue_t request_alloc_table[RESERVED_QUEUE_SIZE];
};

extern struct request_queue_type *request_queue_arr;

void init_request_queue(struct request_queue_type **p_request_queue_arr);
struct request_queue_node* alloc_req_queue();
void free_req_queue(struct request_queue_node *p_free_node);
int enqueue_request_queue(int p_channel, int p_way, struct ftl_request p_ftl_req, struct request_queue_type *p_rq_q_array);
struct ftl_request dequeue_request_queue(int p_channel, int p_way, struct request_queue_type *p_rq_q_array);

#endif