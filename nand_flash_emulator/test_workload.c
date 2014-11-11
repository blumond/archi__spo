#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "test_workload.h"
#include "event_queue.h"
#include "flash_memory.h"
#include "flash_operation_unit.h"
#include "request_if.h"

void make_data(char *p_data, int p_addr);

void *workload()
{
	int j, id, id_temp;
	char *data1, *data2;
	struct ftl_request ftl_req;
	struct event_queue_node ack_node;
	long long time_offset;
	id = 0;

	//addr = page | (plane << SHIFT_PLANE) | (block << SHIFT_BLOCK) | (lun << SHIFT_LUN);

	for (j = 0; j < 3; j++)
	{
		id_temp = id++;

		ftl_req.addr = j | (0 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = PAGE_PROGRAM_MP;
		ftl_req.data = (char *)malloc(SIZE_OF_PAGE);
		ftl_req.length = SECTOR_PER_PAGE;
		data1 = ftl_req.data;
		make_data(ftl_req.data, ftl_req.addr);
		time_offset = 0;

		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (1 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = PAGE_PROGRAM;
		ftl_req.data = (char *)malloc(SIZE_OF_PAGE);
		ftl_req.length = SECTOR_PER_PAGE;
		data2 = ftl_req.data;
		make_data(ftl_req.data, ftl_req.addr);
		time_offset = 0;
		
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		while (1)
		{
			pthread_mutex_lock(&nand_to_ftl->mutex);
			if (nand_to_ftl->num_of_entries > 0)
			{
				pthread_mutex_unlock(&nand_to_ftl->mutex);
				break;
			}
			pthread_mutex_unlock(&nand_to_ftl->mutex);
		}

		pthread_mutex_lock(&nand_to_ftl->mutex);
		ack_node = if_dequeue(nand_to_ftl);
		pthread_mutex_unlock(&nand_to_ftl->mutex);
		assert(ack_node.ftl_req.id == id_temp);
		free(data1);
		free(data2);
	}
	
	for (j = 0; j < 3; j++)
	{
		id_temp = id++;
		data1 = (char *)malloc(SIZE_OF_PAGE);
		data2 = (char *)malloc(SIZE_OF_PAGE);

		ftl_req.addr = j | (0 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = READ_MP;
		ftl_req.data = data1;
		ftl_req.length = SECTOR_PER_PAGE;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (1 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = READ;
		ftl_req.data = data2;
		ftl_req.length = SECTOR_PER_PAGE;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (0 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = DATA_OUT;
		ftl_req.data = data1;
		ftl_req.length = SECTOR_PER_PAGE;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (1 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = DATA_OUT;
		ftl_req.data = data2;
		ftl_req.length = SECTOR_PER_PAGE;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (1 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = READ_FINISH;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		while (1)
		{
			pthread_mutex_lock(&nand_to_ftl->mutex);
			if (nand_to_ftl->num_of_entries > 0)
			{
				pthread_mutex_unlock(&nand_to_ftl->mutex);
				break;
			}
			pthread_mutex_unlock(&nand_to_ftl->mutex);
		}
		pthread_mutex_lock(&nand_to_ftl->mutex);
		ack_node = if_dequeue(nand_to_ftl);
		pthread_mutex_unlock(&nand_to_ftl->mutex);
		assert(ack_node.ftl_req.id == id_temp);
		free(data2);
		free(data1);
	}

	//erase
	{
		id_temp = id++;

		ftl_req.addr = j | (0 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = BLOCK_ERASE_MP;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (1 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = BLOCK_ERASE;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		while (1)
		{
			pthread_mutex_lock(&nand_to_ftl->mutex);
			if (nand_to_ftl->num_of_entries > 0)
			{
				pthread_mutex_unlock(&nand_to_ftl->mutex);
				break;
			}
			pthread_mutex_unlock(&nand_to_ftl->mutex);
		}
		pthread_mutex_lock(&nand_to_ftl->mutex);
		ack_node = if_dequeue(nand_to_ftl);
		pthread_mutex_unlock(&nand_to_ftl->mutex);
		assert(ack_node.ftl_req.id == id_temp);
	}

	for (j = 0; j < 3; j++)
	{
		id_temp = id++;

		data1 = (char *)malloc(SIZE_OF_PAGE);
		data2 = (char *)malloc(SIZE_OF_PAGE);

		ftl_req.addr = j | (0 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = READ_MP;
		ftl_req.data = data1;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (1 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = READ;
		ftl_req.data = data2;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (0 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = DATA_OUT;
		ftl_req.length = SECTOR_PER_PAGE;
		ftl_req.data = data1;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (1 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = DATA_OUT;
		ftl_req.length = SECTOR_PER_PAGE;
		ftl_req.data = data2;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		ftl_req.addr = j | (1 << SHIFT_PLANE);
		ftl_req.id = id_temp;
		ftl_req.cmd = READ_FINISH;
		time_offset = 0;
		pthread_mutex_lock(&ftl_to_nand->mutex);
		if_enqueue(ftl_to_nand, ftl_req, time_offset);
		pthread_mutex_unlock(&ftl_to_nand->mutex);

		while (1)
		{
			pthread_mutex_lock(&nand_to_ftl->mutex);
			if (nand_to_ftl->num_of_entries > 0)
			{
				pthread_mutex_unlock(&nand_to_ftl->mutex);
				break;
			}
			pthread_mutex_unlock(&nand_to_ftl->mutex);
		}
		pthread_mutex_lock(&nand_to_ftl->mutex);
		ack_node = if_dequeue(nand_to_ftl);
		pthread_mutex_unlock(&nand_to_ftl->mutex);
		assert(ack_node.ftl_req.id == id_temp);
		free(data1);
		free(data2);
	}

	return 0;
}

void make_data(char *p_data, int p_addr)
{
	int i;
	int *data;
	
	data = (int *)p_data;

	for (i = 0; i < SIZE_OF_PAGE / 4; i++)
	{
		data[i] = p_addr;
	}

	data = NULL;
}
