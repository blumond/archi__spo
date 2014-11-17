#ifndef __FLASH_OPERATION_UNIT_H__
#define __FLASH_OPERATION_UNIT_H__

#include <pthread.h>
#include "type.h"

#define CHIP_OP					BIT7
#define DATA_IN_OUT				BIT6
#define MULTI_PLANE_CMD			BIT5

#define IDLE					0					//0x00

#define READ					(CHIP_OP|1)			//0x81
#define READ_MP					(MULTI_PLANE_CMD|1)	//0x21
#define DATA_OUT				(DATA_IN_OUT|1)		//0x41
#define READ_FINISH				1					//0x01

#define BLOCK_ERASE				(CHIP_OP|2)			//0x82
#define BLOCK_ERASE_MP			(MULTI_PLANE_CMD|2)	//0x22
#define BLOCK_ERASE_REPORT		7					//0x07

#define READ_STATUS				3					//0x03

#define PAGE_PROGRAM			(DATA_IN_OUT|4)		//0x44
#define PAGE_PROGRAM_MP			(DATA_IN_OUT|MULTI_PLANE_CMD|4)//0x64
#define PAGE_PROGRAM_FINISH		(CHIP_OP|4)			//0x84
#define PAGE_PROGRAM_REPORT		8					//0x08

#define RESET					5					//0x05

#define CHANGE_ACCESS_MODE		6					//0x06


//error message

extern struct queue_type *fou_queue;

int run_nand_operation(int p_channel, int p_way);
void sync_nand_operation();
void do_flash_operation();
int check_cmd_validity(int p_curr_cmd, int p_prev_cmd);
void async_fault_processing();

#endif
