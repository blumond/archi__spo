#include <stdio.h>
#include <pthread.h>
#include "dynamic_scheduler.h"
#include "flash_chip_bus_status.h"
#include "request_queue.h"
#include "configuration.h"
#include "work_queue.h"
#include "queue.h"
#include "flash_memory.h"
#include "flash_operation_unit.h"
#include "reorder_buffer.h"

struct queue_type *ds_queue;
static int prev_channel = 0;		//round robin���� channel scheduling
static int last_way[MAX_CHANNEL];	//round robin���� way scheduling

//async notification�� ������ �����
void dynamic_scheduling()
{
	int i, j;
	int channel, way;
	struct ftl_request ftl_req;

	//���ο� request�� request_queue�� �Է�
	while (ds_queue->head != NULL)
	{
		//request decode
		ftl_req = dequeue(ds_queue);
		channel = addr_to_channel(ftl_req.addr);
		way = addr_to_way(ftl_req.addr);

		enqueue_request_queue(channel, way, ftl_req, request_queue_arr);
		alloc_reorder_buffer(ftl_req);

		if (ftl_req.cmd == PAGE_PROGRAM)
		{
			ftl_req.cmd = PAGE_PROGRAM_FINISH;
			enqueue_request_queue(channel, way, ftl_req, request_queue_arr);
			ftl_req.cmd = PAGE_PROGRAM_REPORT;
			enqueue_request_queue(channel, way, ftl_req, request_queue_arr);
		}
		else if (ftl_req.cmd == BLOCK_ERASE)
		{
			ftl_req.cmd = BLOCK_ERASE_REPORT;
			enqueue_request_queue(channel, way, ftl_req, request_queue_arr);
		}
	}

	for (i = 0; i < NUM_OF_BUS; i++)
	{
		if (get_bus_status(prev_channel) == BUSY)
		{
			prev_channel++;
			if (prev_channel >= NUM_OF_BUS) prev_channel = 0;
			continue;
		}
		for (j = 0; j < CHIPS_PER_BUS; j++)
		{
			if (request_queue_arr[prev_channel + NUM_OF_BUS * last_way[prev_channel]].num_of_entry
				&& fm_status->wq[prev_channel][last_way[prev_channel]].status == IDLE)
			{
				if (get_chip_status(prev_channel, last_way[prev_channel]) != BUSY)
				{
					ftl_req = dequeue_request_queue(prev_channel, last_way[prev_channel], request_queue_arr);
					send_command_to_nand(prev_channel, last_way[prev_channel], ftl_req);

					//�Ʒ� cmd ����ÿ��� ���� chip�� �ٷ� ���� cmd���� ����
					//data out ���� program�� ���޾� ���� ��� ���� chip�� ��� �Ǵ� ����?
					if (ftl_req.cmd == PAGE_PROGRAM_REPORT || ftl_req.cmd == BLOCK_ERASE_REPORT
						|| ftl_req.cmd == READ_FINISH || ftl_req.cmd == PAGE_PROGRAM || ftl_req.cmd == DATA_OUT)
					{
						return;
					}
				}
			}
			last_way[prev_channel]++;
			if (last_way[prev_channel] >= CHIPS_PER_BUS) last_way[prev_channel] = 0;
		}
		prev_channel++;
		if (prev_channel == NUM_OF_BUS) prev_channel = 0;
	}
}
