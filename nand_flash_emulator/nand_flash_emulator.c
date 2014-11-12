#include "nand_flash_emulator.h"
#include "flash_memory.h"
#include "request_queue.h"
#include "dynamic_scheduler.h"
#include "flash_operation_unit.h"
#include "event_queue.h"
#include "queue.h"
#include "work_queue.h"
#include "tsc_timer.h"
#include "request_if.h"
#include "fault_generator.h"
#include "data_transfer_engine.h"
#include "reorder_buffer.h"

struct flashmodule fm;

void init_nfe()
{
	init_flashmodule(&fm);
	init_sibling_page_table();

	init_event_queue(&eq);
	init_request_queue(&request_queue_arr);
	init_queue(&ds_queue);
	init_queue(&fou_queue);
	init_queue(&reorder_buffer);
	init_dte_request_queue(&dte_req_q);
	init_flash_module_status(&fm_status);

	init_async_fault_generator();
	init_sync_fault_generator();

	//init timing model
}
