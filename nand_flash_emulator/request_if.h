#ifndef __REQUEST_IF_H__
#define __REQUEST_IF_H__

#include <pthread.h>

extern struct if_queue_type *ftl_to_nand;
extern struct if_queue_type *nand_to_ftl;

struct if_queue_node
{
	struct event_queue_node eq_node;
	struct if_queue_node *next;
};

struct if_queue_type
{
	struct if_queue_node *head;
	struct if_queue_node *tail;
	int num_of_entries;

	pthread_mutex_t mutex;
};

void init_request_if_queue();
void init_if_queue(struct if_queue_type **p_queue);
void if_enqueue(struct if_queue_type *p_queue, struct ftl_request p_ftl_req, long long p_time_offset);
struct event_queue_node if_dequeue(struct if_queue_type *p_queue);

#endif
