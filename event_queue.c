//event queue는 outstanding request를 관리하는 자료 구조
//queue의 head는 가장 빨리 완료될 request (priority queue)

//적절한 자료 구조는 heap
//1. event queue의 특성상 늦게들어온 request가 늦게 완료될 가능성이 높음 (NAND operation의 경우)
//2. node 삽입과 제거가 빈번함
//3. 최대 node 수가 정해져 있음 (NAND chip과 bus의 수)

#include <stdlib.h>
#include <pthread.h>
#include "event_queue.h"
#include "type.h"

struct event_queue *eq;

//event queue - timing과 관련하여 request들이 정렬되어 있는 queue 구조

void init_event_queue(struct event_queue **p_eq)
{
	int i;

	*p_eq = (struct event_queue*)malloc(sizeof(struct event_queue));
	(*p_eq)->eq_size = 0;

	for (i = 0; i < MAX_ELEMENT; i++)
	{
		memset(&(*p_eq)->eq_node[i], 0, sizeof(struct event_queue_node));
		(*p_eq)->eq_node[i].time.QuadPart = 0xFFFFFFFFFFFFFFFF;
	}
	//insert async fault event
}

void enqueue_event_queue(struct event_queue *p_eq, struct event_queue_node p_eq_node)
{
	int i;
	p_eq->eq_size++;
	i = p_eq->eq_size;
	
	while ((i != 1) && (p_eq_node.time.QuadPart < p_eq->eq_node[i / 2].time.QuadPart))
	{
		p_eq->eq_node[i] = p_eq->eq_node[i / 2];
		i /= 2;
	}
	p_eq->eq_node[i] = p_eq_node;
}

struct event_queue_node dequeue_event_queue(struct event_queue *p_eq)
{
	int parent, child;
	struct event_queue_node item, temp;

	item = p_eq->eq_node[1];
	temp = p_eq->eq_node[p_eq->eq_size];
	p_eq->eq_size--;

	parent = 1;
	child = 2;

	while (child <= p_eq->eq_size)
	{
		if ((child < p_eq->eq_size) && (p_eq->eq_node[child].time.QuadPart > p_eq->eq_node[child + 1].time.QuadPart))
		{
			child++;
		}

		if (temp.time.QuadPart <= p_eq->eq_node[child].time.QuadPart) break;
		else
		{
			p_eq->eq_node[parent] = p_eq->eq_node[child];
			parent = child;
			child = child * 2;
		}
	}
	p_eq->eq_node[parent] = temp;

	return item;
}
