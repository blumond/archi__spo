#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#define MAX_CHANNEL 2
#define MAX_WAY 2
#define MAX_CHIP (MAX_CHANNEL * MAX_WAY) //chip number order�� channel -> way ����

#define DEBUG_MODE
//#define DEBUG_PRINT
//#define MEMCPY
#define DATA_TRANSFER_ENGINE

#define NCQ 4

#define RESERVED_QUEUE_SIZE 64

#define SYNC_FAULT_FREE
//#define ASYNC_FAULT_FREE

#endif
