#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "flash_operation_unit.h"
#include "work_queue.h"
#include "flash_chip_bus_status.h"
#include "event_queue.h"
#include "flash_memory.h"
#include "page_fault_model.h"
#include "queue.h"
#include "timing_model.h"
#include "nand_flash_emulator.h"
#include "request_if.h"
#include "util.h"
#include "dynamic_scheduler.h"
#include "request_queue.h"
#include "nand_flash_emulator.h"
#include "fault_detector.h"
#include "workload_generator.h"
#include "fault_generator.h"
#include "data_transfer_engine.h"

LARGE_INTEGER freq;
struct queue_type *fou_queue;


//독립적인 쓰레드로 구현되어 있어야 함
//work queue를 해석하여 flash operation을 수행한다.
void do_flash_operation()
{
	int i, j;

	for (i = 0; i < NUM_OF_BUS; i++)
	{
		for (j = 0; j < CHIPS_PER_BUS; j++)
		{
			if (fm_status->wq[i][j].status == OP_READY)
			{
				run_nand_operation(i, j);
			}
		}
	}
}

//flash chip/bus status를 변경하고
//flash memory를 contorl 한다.
int run_nand_operation(int p_channel, int p_way)
{
	struct event_queue_node eq_node;
	struct ftl_request ftl_req;
	struct nand_chip *chip;
	struct dte_request dte_req;
	int plane, block, page;
	int op_result = 0;
	int i, j;
	int msb_page_flag = 0;
	int lsb_page_index;
	long long delay;
	int sector_offset;

	fm_status->wq[p_channel][p_way].status = OP_STARTED;
	ftl_req = fm_status->wq[p_channel][p_way].ftl_req;
	chip = &fm.buses[p_channel].chips[p_way];
	plane = addr_to_plane(ftl_req.addr);
	block = addr_to_block(ftl_req.addr);
	page = addr_to_page(ftl_req.addr);
	sector_offset = addr_to_sector_offset(ftl_req.addr);

	if (check_cmd_validity(ftl_req.cmd, chip->cmd) == FAIL)
	{
		chip->cmd = IDLE;
		fm_status->wq[p_channel][p_way].status = IDLE;
		if (ftl_req.cmd == READ_FINISH || ftl_req.cmd == PAGE_PROGRAM_FINISH || ftl_req.cmd == BLOCK_ERASE)
		{
			ftl_req.ack = INVALID_CMD_CHAIN;
			pthread_mutex_lock(&nand_to_ftl->mutex);
			if_enqueue(nand_to_ftl, ftl_req, 0);
			pthread_mutex_unlock(&nand_to_ftl->mutex);
		}
		set_bus_idle(p_channel);
		set_chip_idle(p_channel, p_way);

		dynamic_scheduling();
		return FAIL;
	}
	
	ftl_req.ack = SUCCESS;
	eq_node.ftl_req = ftl_req;
	QueryPerformanceCounter(&eq_node.time);
	//for debugging
	QueryPerformanceCounter(&eq_node.ftl_req.start_tick);
// 	cmd_to_char(ftl_req.cmd, char_cmd);
// 	printf("start tick\t: %16I64d, ID: %3d, cmd: %s\n", eq_node.ftl_req.start_tick.QuadPart, eq_node.ftl_req.id, char_cmd);
	delay = get_timing(ftl_req, ftl_req.addr);
	eq_node.time.QuadPart += delay;
	eq_node.time_offset = 0;
	eq_node.dst = FOU;
	
	enqueue_event_queue(eq, eq_node);

	msb_page_flag = is_msb_page(ftl_req.addr);
	
	switch (ftl_req.cmd)
	{
	case READ:
		op_result = sync_fault_gen(ftl_req.cmd, ftl_req.addr); //UECC_ERROR;
		chip->status = op_result;
		chip->planes[plane].reg_addr = ftl_req.addr;
#ifdef DATA_TRANSFER_ENGINE
		dte_req.deadline = 0;
		dte_req.dst = ftl_req.data;
		dte_req.id = ftl_req.id * PLANES_PER_CHIP + plane;
		dte_req.size = ftl_req.length * SIZE_OF_SECTOR;
		dte_req.src = (char *)chip->planes[plane].blocks[block].pages[page].data + sector_offset * SIZE_OF_SECTOR;

		pthread_mutex_lock(&dte_req_q->mutex);
		dte_request_enqueue(dte_req_q, dte_req);
		pthread_mutex_unlock(&dte_req_q->mutex);
#endif

		if (chip->cmd == READ_MP)
		{
			chip->cmd = READ;
			for (i = 0; i < PLANES_PER_CHIP; i++)
			{
				unreliable_read_violation(chip->planes[i].blocks[block].pages[page].state);
#ifdef MEMCPY
				memcpy(chip->planes[i].page_buffer + (sector_offset * SIZE_OF_SECTOR / 4),
					chip->planes[i].blocks[block].pages[page].data + (sector_offset * SIZE_OF_SECTOR / 4),
					ftl_req.length * SIZE_OF_SECTOR);
#endif
			}
		}
		else
		{
			unreliable_read_violation(chip->planes[plane].blocks[block].pages[page].state);
			chip->cmd = READ;
#ifdef MEMCPY
			memcpy(chip->planes[plane].page_buffer + (sector_offset * SIZE_OF_SECTOR / 4),
				chip->planes[plane].blocks[block].pages[page].data + (sector_offset * SIZE_OF_SECTOR / 4),
				ftl_req.length * SIZE_OF_SECTOR);
#endif
		}
		//chip->status = sync_fault()
		break;

	case READ_MP:
		chip->planes[plane].reg_addr = ftl_req.addr;
		chip->cmd = READ_MP;

#ifdef DATA_TRANSFER_ENGINE
		dte_req.deadline = 0;
		dte_req.dst = ftl_req.data;
		dte_req.id = ftl_req.id * PLANES_PER_CHIP + plane;
		dte_req.size = ftl_req.length * SIZE_OF_SECTOR;
		dte_req.src = (char *)chip->planes[plane].blocks[block].pages[page].data + sector_offset * SIZE_OF_SECTOR;

		pthread_mutex_lock(&dte_req_q->mutex);
		dte_request_enqueue(dte_req_q, dte_req);
		pthread_mutex_unlock(&dte_req_q->mutex);
#endif
		break;

	case DATA_OUT:
#ifdef MEMCPY
		memcpy(fm_status->wq[p_channel][p_way].ftl_req.data,
			chip->planes[plane].page_buffer + (sector_offset * SIZE_OF_SECTOR / 4),
			ftl_req.length * SIZE_OF_SECTOR);
#endif
#ifdef DATA_TRANSFER_ENGINE
		pthread_mutex_lock(&dte_req_q->mutex);
		set_dte_request_deadline(dte_req_q, ftl_req.id * PLANES_PER_CHIP + plane, eq_node.time.QuadPart);
		pthread_mutex_unlock(&dte_req_q->mutex);
#endif
		break;

	case READ_FINISH:
#ifdef DATA_TRANSFER_ENGINE
		pthread_mutex_lock(&dte_req_q->mutex);
		if (is_data_transfer_done(dte_req_q, ftl_req.id * PLANES_PER_CHIP + plane) == 0)
		{
			printf("data transfer is not done!\n");
			assert(0);
		}
		pthread_mutex_unlock(&dte_req_q->mutex);
#endif

		chip->cmd = IDLE;
		pthread_mutex_lock(&nand_to_ftl->mutex);
		if_enqueue(nand_to_ftl, ftl_req, 0);
		pthread_mutex_unlock(&nand_to_ftl->mutex);
		break;

	case BLOCK_ERASE:
		op_result = sync_fault_gen(ftl_req.cmd, ftl_req.addr); //UECC_ERROR;
		chip->status = op_result;
		chip->planes[plane].reg_addr = ftl_req.addr;

		if (chip->cmd == BLOCK_ERASE_MP)
		{
			for (j = 0; j < PLANES_PER_CHIP; j++)
			{
				chip->planes[j].blocks[block].last_programmed_page = 0;
				chip->planes[j].blocks[block].pecycle++;
				chip->planes[j].blocks[block].block_access_mode = chip->current_access_mode;

				for (i = 0; i < PAGES_PER_BLOCK; i++)
				{
#ifdef MEMCPY
					memset(chip->planes[j].blocks[block].pages[i].data, 0xff, SIZE_OF_PAGE);
#endif
					chip->planes[j].blocks[block].pages[i].nop = 0;
					chip->planes[j].blocks[block].pages[i].state = page_state_transition(chip->planes[j].blocks[block].pages[i].state, op_result);
				}
			}
		}
		else
		{
			chip->cmd = BLOCK_ERASE;
			for (i = 0; i < PAGES_PER_BLOCK; i++)
			{
				chip->planes[plane].blocks[block].last_programmed_page = 0;
				chip->planes[plane].blocks[block].pecycle++;
				chip->planes[plane].blocks[block].block_access_mode = chip->current_access_mode;
#ifdef MEMCPY
				memset(chip->planes[plane].blocks[block].pages[i].data, 0xff, SIZE_OF_PAGE);
#endif
				chip->planes[plane].blocks[block].pages[i].nop = 0;
				chip->planes[plane].blocks[block].pages[i].state = page_state_transition(chip->planes[plane].blocks[block].pages[i].state, op_result);
			}
		}
		//chip->status = sync_fault()
		break;

	case BLOCK_ERASE_MP:
		chip->cmd = BLOCK_ERASE_MP;
		chip->planes[plane].reg_addr = ftl_req.addr;	
		break;

	case READ_STATUS:
#ifdef MEMCPY
		memcpy(ftl_req.data, &chip->status, 1);
#endif
		break;

	case PAGE_PROGRAM:
		if (chip->cmd != PAGE_PROGRAM_MP)
		{
			chip->cmd = PAGE_PROGRAM;
		}
		chip->planes[plane].reg_addr = ftl_req.addr;
#ifdef MEMCPY
		memcpy(chip->planes[plane].page_buffer + (sector_offset * SIZE_OF_SECTOR / 4),
			ftl_req.data,
			ftl_req.length * SIZE_OF_SECTOR);
#endif
#ifdef DATA_TRANSFER_ENGINE
		chip->planes[plane].shadow_buffer = (char *)malloc(SIZE_OF_PAGE);
		
		dte_req.deadline = 0;
		dte_req.dst = chip->planes[plane].shadow_buffer;
		dte_req.id = ftl_req.id * PLANES_PER_CHIP + plane;
		dte_req.size = ftl_req.length * SIZE_OF_SECTOR;
		dte_req.src = ftl_req.data;

		pthread_mutex_lock(&dte_req_q->mutex);
		dte_request_enqueue(dte_req_q, dte_req);
		pthread_mutex_unlock(&dte_req_q->mutex);
#endif
		break;
	
	case PAGE_PROGRAM_MP:
		chip->cmd = PAGE_PROGRAM_MP;
		chip->planes[plane].reg_addr = ftl_req.addr;
#ifdef MEMCPY
		memcpy(chip->planes[plane].page_buffer + (sector_offset * SIZE_OF_SECTOR / 4),
			ftl_req.data,
			ftl_req.length * SIZE_OF_SECTOR);
#endif
#ifdef DATA_TRANSFER_ENGINE
		chip->planes[plane].shadow_buffer = (char *)malloc(SIZE_OF_PAGE);

		dte_req.deadline = 0;
		dte_req.dst = chip->planes[plane].shadow_buffer;
		dte_req.id = ftl_req.id * PLANES_PER_CHIP + plane;
		dte_req.size = ftl_req.length * SIZE_OF_SECTOR;
		dte_req.src = ftl_req.data;

		pthread_mutex_lock(&dte_req_q->mutex);
		dte_request_enqueue(dte_req_q, dte_req);
		pthread_mutex_unlock(&dte_req_q->mutex);
#endif
		break;

	case PAGE_PROGRAM_FINISH:
		op_result = sync_fault_gen(ftl_req.cmd, ftl_req.addr); //UECC_ERROR;
		chip->status = op_result;
		if (chip->cmd != PAGE_PROGRAM_MP)
		{
			ascending_order_program_violation(chip->planes[plane].blocks[block].last_programmed_page, page);
			program_after_erase_violation(chip->planes[plane].blocks[block].pages[page].state);
			nop_violation(chip->planes[plane].blocks[block].pages[page].nop, chip->planes[plane].blocks[block].block_access_mode);

			chip->planes[plane].blocks[block].last_programmed_page++;
			chip->planes[plane].blocks[block].pages[page].nop++;
#ifdef MEMCPY
			memcpy(chip->planes[plane].blocks[block].pages[page].data + (sector_offset * SIZE_OF_SECTOR / 4),
				chip->planes[plane].page_buffer + (sector_offset * SIZE_OF_SECTOR / 4),
				ftl_req.length * SIZE_OF_SECTOR);
#endif
#ifdef DATA_TRANSFER_ENGINE
			pthread_mutex_lock(&dte_req_q->mutex);
			set_dte_request_deadline(dte_req_q, ftl_req.id * PLANES_PER_CHIP + plane, eq_node.time.QuadPart);
			pthread_mutex_unlock(&dte_req_q->mutex);
#endif

			chip->planes[plane].blocks[block].pages[page].state = page_state_transition(chip->planes[plane].blocks[block].pages[page].state, op_result);
			if (op_result == PROGRAM_PF || op_result == PROGRAM_IF && msb_page_flag)
			{
				lsb_page_index = get_lsb_page(ftl_req.addr);
				chip->planes[plane].blocks[block].pages[lsb_page_index].state = lsb_page_state_transition(chip->planes[plane].blocks[block].pages[lsb_page_index].state, op_result);
			}
		}
		else
		{
			for (i = 0; i < PLANES_PER_CHIP; i++)
			{
				ascending_order_program_violation(chip->planes[i].blocks[block].last_programmed_page, page);
				program_after_erase_violation(chip->planes[i].blocks[block].pages[page].state);
				nop_violation(chip->planes[i].blocks[block].pages[page].nop, chip->planes[i].blocks[block].block_access_mode);

				chip->planes[i].blocks[block].last_programmed_page++;
				chip->planes[i].blocks[block].pages[page].nop++;
#ifdef MEMCPY
				memcpy(chip->planes[i].blocks[block].pages[page].data + (sector_offset * SIZE_OF_SECTOR / 4),
					chip->planes[i].page_buffer + (sector_offset * SIZE_OF_SECTOR / 4),
					ftl_req.length * SIZE_OF_SECTOR);
#endif
#ifdef DATA_TRANSFER_ENGINE
				pthread_mutex_lock(&dte_req_q->mutex);
				set_dte_request_deadline(dte_req_q, ftl_req.id * PLANES_PER_CHIP + i, eq_node.time.QuadPart);
				pthread_mutex_unlock(&dte_req_q->mutex);
#endif

				chip->planes[i].blocks[block].pages[page].state = page_state_transition(chip->planes[i].blocks[block].pages[page].state, op_result);
				if (op_result == PROGRAM_PF || op_result == PROGRAM_IF && msb_page_flag)
				{
					lsb_page_index = get_lsb_page(ftl_req.addr);
					chip->planes[i].blocks[block].pages[lsb_page_index].state = lsb_page_state_transition(chip->planes[i].blocks[block].pages[lsb_page_index].state, op_result);
				}
			}
		}

		break;

	case RESET:
		chip->cmd = IDLE;
		chip->status = IDLE;
		chip->current_access_mode = MLC_MODE;
		for (i = 0; i < PLANES_PER_CHIP; i++)
		{
			chip->planes[i].reg_addr = 0;
		}
		break;

	case CHANGE_ACCESS_MODE:
		chip->current_access_mode = *ftl_req.data;
		break;

	default :
		break;
	}

	return SUCCESS;
}

void sync_nand_operation()
{
	//operation 종료후에 chip status 변경 추가 필요
	struct ftl_request ftl_req;
	int channel, way, plane;
	char char_cmd[20];
	char *temp_page_buf;

	while (fou_queue->num_of_entries > 0)
	{
		//request decode
		ftl_req = dequeue(fou_queue);
		channel = addr_to_channel(ftl_req.addr);
		way = addr_to_way(ftl_req.addr);
		plane = addr_to_plane(ftl_req.addr);
		ftl_req.ack = fm.buses[channel].chips[way].status;
		fm_status->wq[channel][way].status = IDLE;

		//QueryPerformanceCounter(&ftl_req.elapsed_tick);
		ftl_req.elapsed_tick.QuadPart = ftl_req.elapsed_tick.QuadPart - ftl_req.start_tick.QuadPart;
		QueryPerformanceFrequency(&freq);
		cmd_to_char(ftl_req.cmd, char_cmd);
		printf("elapsed time(us)\t: %16I64d, ID: %3d, cmd: %s\n", ftl_req.elapsed_tick.QuadPart * 1000000 / 3318393, ftl_req.id, char_cmd);

		switch (ftl_req.cmd)
		{
		case READ:
			set_chip_idle(channel, way);
			break;

		case READ_MP:
			break;

		case DATA_OUT:
			set_bus_idle(channel);
			break;

		case BLOCK_ERASE:
			set_chip_idle(channel, way);
			fm.buses[channel].chips[way].cmd = IDLE;

			pthread_mutex_lock(&nand_to_ftl->mutex);
			if_enqueue(nand_to_ftl, ftl_req, 0);
			pthread_mutex_unlock(&nand_to_ftl->mutex);
			break;

		case BLOCK_ERASE_MP:
			break;

		case READ_STATUS:
			break;

		case PAGE_PROGRAM:
			set_bus_idle(channel);
			break;

		case PAGE_PROGRAM_MP:
			set_bus_idle(channel);
			break;

		case PAGE_PROGRAM_FINISH:
#ifdef DATA_TRANSFER_ENGINE
			pthread_mutex_lock(&dte_req_q->mutex);
			if (is_data_transfer_done(dte_req_q, ftl_req.id * PLANES_PER_CHIP + plane) == 0)
			{
				printf("data transfer is not done!\n");
				assert(0);
			}
			pthread_mutex_unlock(&dte_req_q->mutex);
			if (fm.buses[channel].chips[way].cmd == PAGE_PROGRAM_MP)
			{
				int i, block, page;

				for (i = 0; i < PLANES_PER_CHIP; i++)
				{
					block = addr_to_block(fm.buses[channel].chips[way].planes[i].reg_addr);
					page = addr_to_page(fm.buses[channel].chips[way].planes[i].reg_addr);

					temp_page_buf = (char *)fm.buses[channel].chips[way].planes[i].blocks[block].pages[page].data;
					fm.buses[channel].chips[way].planes[i].blocks[block].pages[page].data = (int *)fm.buses[channel].chips[way].planes[i].shadow_buffer;
					fm.buses[channel].chips[way].planes[i].shadow_buffer = NULL;
					free(temp_page_buf);
				}
			}
			else
			{
				int block, page;
				block = addr_to_block(fm.buses[channel].chips[way].planes[plane].reg_addr);
				page = addr_to_page(fm.buses[channel].chips[way].planes[plane].reg_addr);

				temp_page_buf = (char *)fm.buses[channel].chips[way].planes[plane].blocks[block].pages[page].data;
				fm.buses[channel].chips[way].planes[plane].blocks[block].pages[page].data = (int *)fm.buses[channel].chips[way].planes[plane].shadow_buffer;
				fm.buses[channel].chips[way].planes[plane].shadow_buffer = NULL;
				free(temp_page_buf);
			}
#endif

			fm.buses[channel].chips[way].cmd = IDLE;
			set_chip_idle(channel, way);

			pthread_mutex_lock(&nand_to_ftl->mutex);
			if_enqueue(nand_to_ftl, ftl_req, 0);
			pthread_mutex_unlock(&nand_to_ftl->mutex);
			break;

		case RESET:
			break;

		default:
			break;
		}
	}
}

int check_cmd_validity(int p_curr_cmd, int p_prev_cmd)
{
	int result;

	switch (p_curr_cmd)
	{
	case READ_MP:
		if (p_prev_cmd == IDLE) result = SUCCESS;
		else result = FAIL;
		break;
	case READ:
		if (p_prev_cmd == IDLE || p_prev_cmd == READ_MP) result = SUCCESS;
		else result = FAIL;
		break;
	case DATA_OUT:
		if (p_prev_cmd == READ) result = SUCCESS;
		else result = FAIL;
		break;
	case READ_FINISH:
		if (p_prev_cmd == READ) result = SUCCESS;
		else result = FAIL;
		break;
	case BLOCK_ERASE_MP:
		if (p_prev_cmd == IDLE) result = SUCCESS;
		else result = FAIL;
		break;
	case BLOCK_ERASE:
		if (p_prev_cmd == IDLE || p_prev_cmd == BLOCK_ERASE_MP) result = SUCCESS;
		else result = FAIL;
		break;
	case READ_STATUS:
		result = SUCCESS;
		break;
	case PAGE_PROGRAM_MP:
		if (p_prev_cmd == IDLE) result = SUCCESS;
		else result = FAIL;
		break;
	case PAGE_PROGRAM:
		if (p_prev_cmd == IDLE || p_prev_cmd == PAGE_PROGRAM_MP) result = SUCCESS;
		else result = FAIL;
		break;
	case PAGE_PROGRAM_FINISH:
		if (p_prev_cmd == PAGE_PROGRAM || p_prev_cmd == PAGE_PROGRAM_MP) result = SUCCESS;
		else result = FAIL;
		break;
	case RESET:
		result = SUCCESS;
		break;
	case CHANGE_ACCESS_MODE:
		if (p_prev_cmd == IDLE) result = SUCCESS;
		else result = FAIL;
	default:
		result = SUCCESS;
		break;
	}
	return result;
}

void async_fault_processing()
{
	struct nand_chip *chip;
	int i, j, k, l, plane, block, page;

	fm.power_fail_flag = 1;
	memset(damaged_block, 0xFF, sizeof(damaged_block));

	//on-going operation들 fault 처리
	for (i = 0; i < NUM_OF_BUS; i++)
	{
		for (j = 0; j < CHIPS_PER_BUS; j++)
		{
			if (fm_status->wq[i][j].status == OP_STARTED)
			{
				fm_status->wq[i][j].ftl_req.addr;

				chip = &fm.buses[i].chips[j];
				plane = addr_to_plane(fm_status->wq[i][j].ftl_req.addr);
				block = addr_to_block(fm_status->wq[i][j].ftl_req.addr);
				page = addr_to_page(fm_status->wq[i][j].ftl_req.addr);

				switch (fm_status->wq[i][j].ftl_req.cmd)
				{
				case PAGE_PROGRAM_FINISH:
					if (chip->cmd == PAGE_PROGRAM_MP)
					{
						for (k = 0; k < PLANES_PER_CHIP; k++)
						{
							damaged_block[i][j][k] = block;
							chip->planes[k].blocks[block].pages[page].state = page_state_transition(chip->planes[k].blocks[block].pages[page].state, PROGRAM_PF);
						}
					}
					else
					{
						damaged_block[i][j][plane] = block;
						chip->planes[plane].blocks[block].pages[page].state = page_state_transition(chip->planes[plane].blocks[block].pages[page].state, PROGRAM_PF);
					}
					break;

				case BLOCK_ERASE:
					if (chip->cmd == BLOCK_ERASE_MP)
					{
						for (k = 0; k < PLANES_PER_CHIP; k++)
						{
							damaged_block[i][j][k] = block;
							for (l = 0; l < PAGES_PER_BLOCK; l++)
							{
								chip->planes[k].blocks[block].pages[l].state = page_state_transition(chip->planes[k].blocks[block].pages[l].state, ERASE_PF);
							}
						}
					}
					else
					{
						damaged_block[i][j][plane] = block;
						for (l = 0; l < PAGES_PER_BLOCK; l++)
						{
							chip->planes[plane].blocks[block].pages[l].state = page_state_transition(chip->planes[plane].blocks[block].pages[l].state, ERASE_PF);
						}
					}
				default:
					break;
				}
			}
		}
	}

	//---reset nand flash emulator---
	//clear event queue
	while (eq->eq_size)
	{
		dequeue_event_queue(eq);
	}

	//clear externel request queue
	while (ftl_to_nand->num_of_entries)
	{
		if_dequeue(ftl_to_nand);
	}

	//clear request queue
	for (i = 0; i < NUM_OF_BUS; i++)
	{
		for (j = 0; j < CHIPS_PER_BUS; j++)
		{
			while (request_queue_arr[i + NUM_OF_BUS * j].num_of_entry)
			{
				dequeue_request_queue(i, j, request_queue_arr);
			}

		}
	}
	//clear dynamic scheduler queue
	while (ds_queue->num_of_entries)
	{
		dequeue(ds_queue);
	}

	//clear flash operation queue
	while (fou_queue->num_of_entries)
	{
		dequeue(fou_queue);
	}

	//init chip and bus status
	reset_flash_module_status(fm_status);
	reset_flashmodule(&fm);

	async_fault_gen();
}
