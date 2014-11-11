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

	//expired event가 있는지 확인
	while (1)
	{
		QueryPerformanceCounter(&current_tsc);
		if (eq->eq_size && eq->eq_node[1].time.QuadPart <= current_tsc.QuadPart)
		{
			//진행중인 flash operation이 완료된 경우
			if (eq->eq_node[1].dst == FOU)
			{
				eq_node = dequeue_event_queue(eq);
				eq_node.ftl_req.elapsed_tick = current_tsc;
				
				enqueue(fou_queue, eq_node.ftl_req);
				//sync_nand_operation thread 호출
				sync_nand_operation();

				//sync 완료시 ds_호출 필요 (새로운 cmd 스케쥴링해야 하므로)
				dynamic_scheduling();
			}
			//새로운 request가 도착한 경우
			else if (eq->eq_node[1].dst == DS)
			{
				eq_node = dequeue_event_queue(eq);
				enqueue(ds_queue, eq_node.ftl_req);
				dynamic_scheduling();
			}
			else if (eq->eq_node[1].dst == ASYNC_FAULT)
 			{
				//async fault 처리
				//현재 on-going cmd에 대한 처리
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
