#include <stdio.h>
#include <Windows.h>
#include "timing_alignment_unit.h"
#include "test_workload.h"
#include "tsc_timer.h"
#include "flash_operation_unit.h"
#include "dynamic_scheduler.h"
#include "queue.h"
#include "request_queue.h"
#include "nand_flash_emulator.h"
#include "request_if.h"
#include "timing_model.h"
#include "work_queue.h"
#include "data_transfer_engine.h"

pthread_t thread0, thread1, thread2;

int main(int argc, char **argv)
{
	int join_status = 0;

	//init
	init_nfe();
	init_timing_model();
	init_request_if_queue();

	pthread_create(&thread0, NULL, (void *)tsc_timer, NULL);
	pthread_create(&thread2, NULL, (void *)data_transfer_engine, NULL);
	pthread_create(&thread1, NULL, (void *)workload, NULL);
	

	pthread_join(thread0, (void **)join_status);
	pthread_join(thread1, (void **)join_status);
	pthread_join(thread2, (void **)join_status);
}
