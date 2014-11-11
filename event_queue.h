#ifndef __EVENT_QUEUE_H__
#define __EVENT_QUEUE_H__

#include <Windows.h>
#include <pthread.h>

#define MAX_ELEMENT 100
#define DS 0			//dynamic scheduler
#define FOU 1			//flash operation unit
#define ASYNC_FAULT	2	//async fault 

typedef struct ftl_request
{
	int id;
	int cmd;
	int addr;
	int length;
	char *data;
	int ack;
	LARGE_INTEGER start_tick;
	LARGE_INTEGER elapsed_tick;
};

struct event_queue_node
{
	LARGE_INTEGER time;				//key
	struct ftl_request ftl_req;		//flash request
	long long time_offset;			//FTL simulation 타이밍 보정
	int dst;						//destination (dynamic scheduler or flash operation unit)
};

struct event_queue
{
	struct event_queue_node eq_node[MAX_ELEMENT];
	int eq_size;
};

extern struct event_queue *eq;

void init_event_queue(struct event_queue **p_eq);
void enqueue_event_queue(struct event_queue *p_eq, struct event_queue_node p_eq_node);
struct event_queue_node dequeue_event_queue(struct event_queue *p_eq);

#endif __EVENT_QUEUE_H__
