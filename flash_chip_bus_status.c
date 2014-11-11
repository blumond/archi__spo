#include <assert.h>
#include "flash_chip_bus_status.h"
#include "configuration.h"
#include "work_queue.h"

void set_chip_busy(int p_channel, int p_way)
{
#ifdef DEBUG_MODE
	assert(fm_status->chip_status[p_channel][p_way] == IDLE);
#endif // DEBUG_MODE

	fm_status->chip_status[p_channel][p_way] = BUSY;
}

void set_chip_idle(int p_channel, int p_way)
{
#ifdef DEBUG_MODE
	assert(fm_status->chip_status[p_channel][p_way] == BUSY);
#endif // DEBUG_MODE

	fm_status->chip_status[p_channel][p_way] = IDLE;
}

int get_chip_status(int p_channel, int p_way)
{
	return fm_status->chip_status[p_channel][p_way];
}

void set_bus_busy(int p_channel)
{
#ifdef DEBUG_MODE
	assert(fm_status->bus_status[p_channel] == IDLE);
#endif // DEBUG_MODE

	fm_status->bus_status[p_channel] = BUSY;
}

void set_bus_idle(int p_channel)
{
#ifdef DEBUG_MODE
	assert(fm_status->bus_status[p_channel] == BUSY);
#endif // DEBUG_MODE

	fm_status->bus_status[p_channel] = IDLE;
}

int get_bus_status(int p_channel)
{
	return fm_status->bus_status[p_channel];
}
