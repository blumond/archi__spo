#include <stdio.h>
#include <time.h>
#include "configuration.h"
#include "event_queue.h"
#include "test_workload.h"
#include "queue.h"
#include "request_if.h"
#include "util.h"

//외부로 부터 온 async request 처리
//FTL에서 실제 cmd가 전달 될 time stamp를 받아야 함
//notification이 오면 자동으로 함수가 invoke 되어야 함
//interface에 따라 queue에 있는 request를 가져와서 event queue에 timing을 고려해서 넣고, timer setting

//IPC를 통해 값을 받을때

void receive_external_request()
{
	struct event_queue_node eq_node;

#ifdef DEBUG_PRINT
	char char_cmd[20];
#endif

	//externel request가 있는지 확인 (from FTL)
	pthread_mutex_lock(&ftl_to_nand->mutex);

	while (ftl_to_nand->num_of_entries != 0)
	{
		eq_node = if_dequeue(ftl_to_nand);

		//request 내용과 timing offset check
		QueryPerformanceCounter(&eq_node.time);

		//time offset 값 만큼 delay 추가
		if (eq_node.time_offset >= 0)
		{
			eq_node.time.QuadPart += eq_node.time_offset;
		}
		eq_node.dst = DS;

		//event queue에 enqueue, timer setting
		enqueue_event_queue(eq, eq_node);

#ifdef DEBUG_PRINT
		//cmd_to_char(eq_node.ftl_req.cmd, char_cmd);
		//printf("receive_external_request : cmd - %s\n", char_cmd);
#endif
	}
	pthread_mutex_unlock(&ftl_to_nand->mutex);
}
