#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "test_workload.h"
#include "event_queue.h"
#include "flash_memory.h"
#include "flash_operation_unit.h"
#include "request_if.h"
#include "simple_rand.h"
#include "configuration.h"
#include "nand_flash_emulator.h"

unsigned int g_rand_workload_seed = 1234;

void make_data(char *p_data, int p_addr);
void rand_workload_gen();
void send_mp_read(int p_id, char *p_data, int p_addr);
void send_read(int p_id, char *p_data, int p_addr);
void send_mp_program(int p_id, char *p_data, int p_addr);
void send_program(int p_id, char *p_data, int p_addr);
void send_mp_erase(int p_id, char *p_data, int p_addr);
void send_erase(int p_id, char *p_data, int p_addr);

void *workload()
{
	rand_workload_gen();

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
}

void rand_workload_gen()
{
	int addr, lun, block, plane, page;
	int id = 1;
	int res_id = 0;
	char *data;
	struct event_queue_node ack_node;

	data = (char *)malloc(SIZE_OF_PAGE * PLANES_PER_CHIP);
	
	while (1)
	{
		if (fm.power_fail_flag)
		{
			free(data);
			pthread_exit(0);
		}
		if (id - res_id >= NCQ)
		{
			while (1)
			{
				if (fm.power_fail_flag)
				{
					free(data);
					pthread_exit(0);
				}
				if (nand_to_ftl->num_of_entries > 0)
				{
					pthread_mutex_lock(&nand_to_ftl->mutex);
					if (nand_to_ftl->num_of_entries > 0)
					{
						ack_node = if_dequeue(nand_to_ftl);
					}
					pthread_mutex_unlock(&nand_to_ftl->mutex);

					res_id = ack_node.ftl_req.id;
					break;
				}
			}
		}

		if (id - res_id >= NCQ)
		{
			printf("??\n");
		}

		g_rand_workload_seed = simple_rand(g_rand_workload_seed);

		//address trim
		addr = g_rand_workload_seed & MASK_ADDR;
		addr &= ~MASK_SECTOR;
		lun = (addr >> SHIFT_LUN) % (CHIPS_PER_BUS * NUM_OF_BUS);
		block = (addr >> SHIFT_BLOCK) % BLOCKS_PER_PLANE;
		plane = (addr >> SHIFT_PLANE) % PLANES_PER_CHIP;
		page = (addr >> SHIFT_PAGE) % PAGES_PER_BLOCK;
		addr = (lun << SHIFT_LUN) | (block << SHIFT_BLOCK) | (plane << SHIFT_PLANE) | (page << SHIFT_PAGE);

		switch (g_rand_workload_seed % 6)
		{
		case 0:
			//read
			send_read(id, data, addr);
			break;
		case 1:
			//program
			send_program(id, data, addr);
			break;
		case 2:
			//erase
			send_erase(id, data, addr);
			break;
		case 3:
			//mp read 
			send_mp_read(id, data, addr);
			break;
		case 4:
			//mp program
			send_mp_program(id, data, addr);
			break;
		case 5:
			//mp erase
			send_mp_erase(id, data, addr);
			break;
		default:
			break;
		}

		id++;
	}
	
	free(data);
}

void send_mp_read(int p_id, char *p_data, int p_addr)
{
	struct ftl_request ftl_req;

	ftl_req.addr = p_addr & ~(1 << SHIFT_PLANE);
	ftl_req.id = p_id;
	ftl_req.cmd = READ_MP;
	ftl_req.data = p_data;
	ftl_req.length = SECTOR_PER_PAGE;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);

	ftl_req.addr = p_addr | (1 << SHIFT_PLANE);
	ftl_req.id = p_id;
	ftl_req.cmd = READ;
	ftl_req.data = p_data + SIZE_OF_PAGE;
	ftl_req.length = SECTOR_PER_PAGE;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);

	ftl_req.addr = p_addr & ~(1 << SHIFT_PLANE);
	ftl_req.id = p_id;
	ftl_req.cmd = DATA_OUT;
	ftl_req.data = p_data;
	ftl_req.length = SECTOR_PER_PAGE;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);

	ftl_req.addr = p_addr | (1 << SHIFT_PLANE);
	ftl_req.id = p_id;
	ftl_req.cmd = DATA_OUT;
	ftl_req.data = p_data + SIZE_OF_PAGE;
	ftl_req.length = SECTOR_PER_PAGE;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);

	ftl_req.addr = p_addr & ~(1 << SHIFT_PLANE);
	ftl_req.id = p_id;
	ftl_req.cmd = READ_FINISH;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);
}

void send_mp_program(int p_id, char *p_data, int p_addr)
{
	struct ftl_request ftl_req;

	ftl_req.addr = p_addr & ~(1 << SHIFT_PLANE);
	ftl_req.id = p_id;
	ftl_req.cmd = PAGE_PROGRAM_MP;
	ftl_req.data = p_data;
	ftl_req.length = SECTOR_PER_PAGE;
	make_data(ftl_req.data, ftl_req.addr);
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);

	ftl_req.addr = p_addr | (1 << SHIFT_PLANE);
	ftl_req.id = p_id;
	ftl_req.cmd = PAGE_PROGRAM;
	ftl_req.data = p_data + SIZE_OF_PAGE;
	ftl_req.length = SECTOR_PER_PAGE;
	make_data(ftl_req.data, ftl_req.addr);
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);
}

void send_mp_erase(int p_id, char *p_data, int p_addr)
{
	struct ftl_request ftl_req;

	ftl_req.addr = p_addr & ~(1 << SHIFT_PLANE);
	ftl_req.id = p_id;
	ftl_req.cmd = BLOCK_ERASE_MP;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);

	ftl_req.addr = p_addr | (1 << SHIFT_PLANE);
	ftl_req.id = p_id;
	ftl_req.cmd = BLOCK_ERASE;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);
}

void send_read(int p_id, char *p_data, int p_addr)
{
	struct ftl_request ftl_req;

	ftl_req.addr = p_addr;
	ftl_req.id = p_id;
	ftl_req.cmd = READ;
	ftl_req.data = p_data;
	ftl_req.length = SECTOR_PER_PAGE;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);

	ftl_req.addr = p_addr;
	ftl_req.id = p_id;
	ftl_req.cmd = DATA_OUT;
	ftl_req.data = p_data;
	ftl_req.length = SECTOR_PER_PAGE;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);

	ftl_req.addr = p_addr;
	ftl_req.id = p_id;
	ftl_req.cmd = READ_FINISH;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);
}

void send_program(int p_id, char *p_data, int p_addr)
{
	struct ftl_request ftl_req;

	ftl_req.addr = p_addr;
	ftl_req.id = p_id;
	ftl_req.cmd = PAGE_PROGRAM;
	ftl_req.data = p_data + SIZE_OF_PAGE;
	ftl_req.length = SECTOR_PER_PAGE;
	make_data(ftl_req.data, ftl_req.addr);
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);
}

void send_erase(int p_id, char *p_data, int p_addr)
{
	struct ftl_request ftl_req;

	ftl_req.addr = p_addr;
	ftl_req.id = p_id;
	ftl_req.cmd = BLOCK_ERASE;
	pthread_mutex_lock(&ftl_to_nand->mutex);
	if_enqueue(ftl_to_nand, ftl_req, 0);
	pthread_mutex_unlock(&ftl_to_nand->mutex);
}
