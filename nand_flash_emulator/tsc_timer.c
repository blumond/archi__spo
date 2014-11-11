#include <Windows.h>
#include "tsc_timer.h"
#include "flash_operation_unit.h"
#include "dynamic_scheduler.h"
#include "event_queue.h"
#include "queue.h"
#include "request_if.h"
#include "timing_alignment_unit.h"

//run thread at dedicated core
void *tsc_timer()
{
	struct event_queue_node eq_node;
	LARGE_INTEGER current_tsc;

	//expired event�� �ִ��� Ȯ��
	while (1)
	{
		QueryPerformanceCounter(&current_tsc);
		if (eq->eq_size && eq->eq_node[1].time.QuadPart <= current_tsc.QuadPart)
		{
			//�������� flash operation�� �Ϸ�� ���
			if (eq->eq_node[1].dst == FOU)
			{
				eq_node = dequeue_event_queue(eq);
				eq_node.ftl_req.elapsed_tick = current_tsc;
				
				enqueue(fou_queue, eq_node.ftl_req);
				//sync_nand_operation thread ȣ��
				sync_nand_operation();

				//sync �Ϸ�� ds_ȣ�� �ʿ� (���ο� cmd �����층�ؾ� �ϹǷ�)
				dynamic_scheduling();
			}
			//���ο� request�� ������ ���
			else if (eq->eq_node[1].dst == DS)
			{
				eq_node = dequeue_event_queue(eq);
				enqueue(ds_queue, eq_node.ftl_req);
				dynamic_scheduling();
			}
			else if (eq->eq_node[1].dst == ASYNC_FAULT)
 			{
				//async fault ó��
				//���� on-going cmd�� ���� ó��
				eq_node = dequeue_event_queue(eq);
				//async_fault_processing();
 			}
		}
		if (ftl_to_nand->num_of_entries > 0)
		{
			receive_external_request();
		}
	}
}
