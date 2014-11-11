#ifndef __WORK_QUEUE_H__
#define __WORK_QUEUE_H__

#include <pthread.h>
#include "event_queue.h"
#include "configuration.h"
#include "flash_memory.h"

#define IDLE 0
#define OP_READY 1
#define OP_STARTED 2
#define OP_COMPLETED 3

struct work_queue
{
	struct ftl_request ftl_req;
	int status;
};

struct flash_module_status
{
	struct work_queue wq[NUM_OF_BUS][CHIPS_PER_BUS];
	int chip_status[NUM_OF_BUS][CHIPS_PER_BUS];
	int bus_status[NUM_OF_BUS];
};

extern struct flash_module_status *fm_status;

void send_command_to_nand(int p_channel, int p_way, struct ftl_request p_ftl_req);
void init_flash_module_status(struct flash_module_status **p_fm_status);
void reset_flash_module_status(struct flash_module_status *p_fm_status);

#endif
