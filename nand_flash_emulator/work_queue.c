#include <assert.h>
#include "configuration.h"
#include "work_queue.h"
#include "flash_operation_unit.h"
#include "flash_chip_bus_status.h"

struct flash_module_status *fm_status;

void init_flash_module_status(struct flash_module_status **p_fm_status)
{
	int i, j;

	*p_fm_status = (struct flash_module_status *)malloc(sizeof(struct flash_module_status));
	
	for (i = 0; i < NUM_OF_BUS; i++)
	{
		for (j = 0; j < CHIPS_PER_BUS; j++)
		{
			(*p_fm_status)->wq[i][j].status = IDLE;
			(*p_fm_status)->chip_status[i][j] = IDLE;
		}
		(*p_fm_status)->bus_status[i] = IDLE;
	}
}

void send_command_to_nand(int p_channel, int p_way, struct ftl_request p_ftl_req)
{
#ifdef DEBUG_MODE
	assert(fm_status->wq[p_channel][p_way].status == IDLE);
#endif

	//queue에 command 저장
	fm_status->wq[p_channel][p_way].ftl_req = p_ftl_req;

	//cmd ready
	fm_status->wq[p_channel][p_way].status = OP_READY;

	//thread간 동기화 문제를 단순화 시키기 위해
	//chip, bus의 상태를 변경하는 코드가 이곳에 위치
	if (p_ftl_req.cmd & CHIP_OP)
	{
		set_chip_busy(p_channel, p_way);
	}
	else if (p_ftl_req.cmd & DATA_IN_OUT)
	{
		set_bus_busy(p_channel);
	}

	do_flash_operation();
}

void reset_flash_module_status(struct flash_module_status *p_fm_status)
{
	int i, j;

	for (i = 0; i < NUM_OF_BUS; i++)
	{
		for (j = 0; j < CHIPS_PER_BUS; j++)
		{
			p_fm_status->wq[i][j].status = IDLE;
			p_fm_status->chip_status[i][j] = IDLE;
		}
		p_fm_status->bus_status[i] = IDLE;
	}
}
