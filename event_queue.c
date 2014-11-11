//event queue�� outstanding request�� �����ϴ� �ڷ� ����
//queue�� head�� ���� ���� �Ϸ�� request (priority queue)

//������ �ڷ� ������ heap
//1. event queue�� Ư���� �ʰԵ��� request�� �ʰ� �Ϸ�� ���ɼ��� ���� (NAND operation�� ���)
//2. node ���԰� ���Ű� �����
//3. �ִ� node ���� ������ ���� (NAND chip�� bus�� ��)

#include <stdlib.h>
#include <pthread.h>
#include "event_queue.h"
#include "type.h"

struct event_queue *eq;

//event queue - timing�� �����Ͽ� request���� ���ĵǾ� �ִ� queue ����

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
