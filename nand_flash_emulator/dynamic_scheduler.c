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
int last_way[MAX_CHANNEL];

//async notification을 받으면 수행됨
void dynamic_scheduling()
{
	int i, j;
	struct ftl_request ftl_req;
	int channel, way;

	//새로운 request를 request_queue에 입력
	while (ds_queue->head != NULL)
	{
		//request decode
		ftl_req = dequeue(ds_queue);
		channel = addr_to_channel(ftl_req.addr);
		way = addr_to_way(ftl_req.addr);

		//cmd_to_char(ftl_req.cmd, char_cmd);
		//printf("get ds_queue, ID: %3d, cmd: %s\n", ftl_req.id, char_cmd);

		//check queue full
		//check_full(channel, way, rq_q_array);

		//enqueue
		enqueue_request_queue(channel, way, ftl_req, request_queue_arr);
		alloc_reorder_buffer(ftl_req);

		if (ftl_req.cmd == PAGE_PROGRAM)
		{
			ftl_req.cmd = PAGE_PROGRAM_FINISH;
			enqueue_request_queue(channel, way, ftl_req, request_queue_arr);
		}
	}

	//data 전송이 필요없는 cmd부터 먼저 실행
	for (i = 0; i < NUM_OF_BUS; i++)
	{
		if (get_bus_status(i) != BUSY)
		{
			for (j = 0; j < CHIPS_PER_BUS; j++)
			{
				if (request_queue_arr[i + NUM_OF_BUS * j].num_of_entry
					&& fm_status->wq[i][j].status == IDLE)
				{
					if (!(request_queue_arr[i + /*NUM_OF_BUS*/2 * j].req_q_head->ftl_req.cmd & DATA_IN_OUT)
						&& (get_chip_status(i, j) != BUSY))
					{
						ftl_req = dequeue_request_queue(i, j, request_queue_arr);
						//cmd_to_char(ftl_req.cmd, char_cmd);
						//printf("send_command_to_nand, ID: %3d, cmd: %s\n", ftl_req.id, char_cmd);
						send_command_to_nand(i, j, ftl_req);
					}
				}
			}
		}
	}
	//data 전송이 필요한 cmd 실행 round robin으로 실행
	for (i = 0; i < NUM_OF_BUS; i++)
	{
		for (j = 0; j < CHIPS_PER_BUS; j++)
		{
			if  (get_bus_status(i) == BUSY) break;

			last_way[i]++;
			if (last_way[i] >= CHIPS_PER_BUS) last_way[i] = 0;

			if (request_queue_arr[i + NUM_OF_BUS * last_way[i]].num_of_entry
				&& fm_status->wq[i][last_way[i]].status == IDLE)
			{
				if (get_chip_status(i, last_way[i]) != BUSY)
				{
					ftl_req = dequeue_request_queue(i, last_way[i], request_queue_arr);

					//cmd_to_char(ftl_req.cmd, char_cmd);
					//printf("send_command_to_nand, ID: %3d, cmd: %s\n", ftl_req.id, char_cmd);
					//send CMD
					send_command_to_nand(i, last_way[i], ftl_req);
				}
			}				
		}
	}
}
