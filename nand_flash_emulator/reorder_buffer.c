#include <pthread.h>
#include <assert.h>
#include "queue.h"
#include "event_queue.h"
#include "request_if.h"
#include "type.h"

struct queue_type *reorder_buffer;

//request의 start시 reorder buffer 할당
void alloc_reorder_buffer(struct ftl_request p_ftl_req)
{
	struct queue_node *q_node_iter;
	struct ftl_request ftl_req;

	for (q_node_iter = reorder_buffer->head; q_node_iter != NULL; q_node_iter = q_node_iter->next)
	{
		if (q_node_iter->ftl_req.id == p_ftl_req.id)
		{
			break;
		}
	}

	if (q_node_iter == NULL)
	{
		p_ftl_req.ack = UNDEFINE_INT;
		enqueue(reorder_buffer, p_ftl_req);
	}
}

//request가 완료되면 reorder buffer에 ack 값을 넣어놓고 순서대로 response를 날린다
void put_reorder_buffer(struct ftl_request p_ftl_req)
{
	struct queue_node *q_node_iter;
	struct ftl_request ftl_req;

	for (q_node_iter = reorder_buffer->head; q_node_iter != NULL; q_node_iter = q_node_iter->next)
	{
		if (q_node_iter->ftl_req.id == p_ftl_req.id)
		{
			q_node_iter->ftl_req = p_ftl_req;
			break;
		}
	}
	if (q_node_iter == NULL) assert(0);

	for (q_node_iter = reorder_buffer->head; q_node_iter != NULL; q_node_iter = reorder_buffer->head)
	{
		if (q_node_iter->ftl_req.ack == UNDEFINE_INT)
		{
			break;
		}
		else
		{
			ftl_req = dequeue(reorder_buffer);
			
			pthread_mutex_lock(&nand_to_ftl->mutex);
			if_enqueue(nand_to_ftl, ftl_req, 0);
			pthread_mutex_unlock(&nand_to_ftl->mutex);
		}
	}
}