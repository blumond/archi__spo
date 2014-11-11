#ifndef __FLASH_CHIP_BUS_STATUS_H__
#define __FLASH_CHIP_BUS_STATUS_H__

#define IDLE 0
#define BUSY 1

void set_chip_busy(int p_channel, int p_way);
void set_chip_idle(int p_channel, int p_way);
int get_chip_status(int p_channel, int p_way);

void set_bus_busy(int p_channel);
void set_bus_idle(int p_channel);
int get_bus_status(int p_channel);

#endif
