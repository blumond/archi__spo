#include <stdio.h>
#include <time.h>
#include "configuration.h"
#include "event_queue.h"
#include "test_workload.h"
#include "queue.h"
#include "request_if.h"
#include "util.h"

//�ܺη� ���� �� async request ó��
//FTL���� ���� cmd�� ���� �� time stamp�� �޾ƾ� ��
//notification�� ���� �ڵ����� �Լ��� invoke �Ǿ�� ��
//interface�� ���� queue�� �ִ� request�� �����ͼ� event queue�� timing�� ����ؼ� �ְ�, timer setting

//IPC�� ���� ���� ������

void receive_external_request()
{
	struct event_queue_node eq_node;

#ifdef DEBUG_PRINT
	char char_cmd[20];
#endif

	//externel request�� �ִ��� Ȯ�� (from FTL)
	pthread_mutex_lock(&ftl_to_nand->mutex);

	while (ftl_to_nand->num_of_entries != 0)
	{
		eq_node = if_dequeue(ftl_to_nand);

		//request ����� timing offset check
		QueryPerformanceCounter(&eq_node.time);

		//time offset �� ��ŭ delay �߰�
		if (eq_node.time_offset >= 0)
		{
			eq_node.time.QuadPart += eq_node.time_offset;
		}
		eq_node.dst = DS;

		//event queue�� enqueue, timer setting
		enqueue_event_queue(eq, eq_node);

#ifdef DEBUG_PRINT
		//cmd_to_char(eq_node.ftl_req.cmd, char_cmd);
		//printf("receive_external_request : cmd - %s\n", char_cmd);
#endif
	}
	pthread_mutex_unlock(&ftl_to_nand->mutex);
}
