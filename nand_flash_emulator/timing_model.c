/* ���� Timing Model�� ���, data dependency�� ������� ���� ��Ȳ	//
// init������ �̸� : init_timing_model.txt							//
// init������ ���� : Strict�ϰ� �Ʒ��� ���� ������ ��				//								
//	LSB_READ [��]													//
//	MSB_READ [��]													//
//	...																//
//	DATA_TRANSFER_TIME_PER_SECTOR [��]								*/

#include <Windows.h>
#include <stdio.h>
#include "flash_operation_unit.h"
#include "timing_model.h"
#include "flash_memory.h"
#include "event_queue.h"


//set timing model
void init_timing_model()
{
	//ini file�� �о���̰� timing model table �ۼ�
	LARGE_INTEGER frequency;
	FILE *init;
	int no_error;

	struct nand_timing timing_init_v;
	
	fopen_s(&init, "init_timing_model.config","r");

	// init ������ �������
	if (init != NULL){
		no_error = fscanf_s(init, "LSB_READ %lld\n", &timing_init_v.lsb_read);
		if (no_error < 0) printf("init_timing_model.txt error\n");

		no_error = fscanf_s(init, "MSB_READ %lld\n", &timing_init_v.msb_read);
		if (no_error < 0) printf("init_timing_model.txt error\n");

		no_error = fscanf_s(init, "LSB_PROGRAM %lld\n", &timing_init_v.lsb_program);
		if (no_error < 0) printf("init_timing_model.txt error\n");

		no_error = fscanf_s(init, "MSB_PROGRAM %lld\n", &timing_init_v.msb_program);
		if (no_error < 0) printf("init_timing_model.txt error\n");

		no_error = fscanf_s(init, "ERASE %lld\n", &timing_init_v.erase);
		if (no_error < 0) printf("init_timing_model.txt error\n");

		no_error = fscanf_s(init, "DATA_TRANSFER_TIME_PER_SECTOR %lld\n", &timing_init_v.data_transfer_time_per_sector);
		if (no_error < 0) printf("init_timing_model.txt error\n");
	}// init ������ ������ ���� ��� default value ���
	else{
		timing_init_v.lsb_read		= DEF_LSB_READ;
		timing_init_v.msb_read		= DEF_MSB_READ;
		timing_init_v.lsb_program	= DEF_LSB_PROGRAM;
		timing_init_v.msb_program	= DEF_MSB_PROGRAM;
		timing_init_v.erase			= DEF_ERASE;
		timing_init_v.data_transfer_time_per_sector = DEF_DATA_TRAN_PER_SECTOR;
	}
	
	if(init != NULL)
		fclose(init);
	
	QueryPerformanceFrequency(&frequency);

	// us���� Ȥ�� ms���� ���� ���� �ʿ�
	nand_timing_table.erase			= timing_init_v.erase * frequency.QuadPart / 1000000;
	nand_timing_table.lsb_program	= timing_init_v.lsb_program * frequency.QuadPart / 1000000;
	nand_timing_table.lsb_read		= timing_init_v.lsb_read * frequency.QuadPart / 1000000;
	nand_timing_table.msb_program	= timing_init_v.msb_program * frequency.QuadPart / 1000000;
	nand_timing_table.msb_read		= timing_init_v.msb_read * frequency.QuadPart / 1000000;
	nand_timing_table.data_transfer_time_per_sector = timing_init_v.data_transfer_time_per_sector * frequency.QuadPart / 1000000;
}


long long get_timing(struct ftl_request p_ftl_req, int p_addr)
{
	//timing table�� ���� timing �� return
	long long delay = 0;

	switch (p_ftl_req.cmd)
	{
	case READ:
		if (is_msb_page(p_addr)) delay = nand_timing_table.msb_read;
		else delay = nand_timing_table.lsb_read;
		break;
	case DATA_OUT:
		delay = nand_timing_table.data_transfer_time_per_sector * p_ftl_req.length;
		break;
	case BLOCK_ERASE:
		delay = nand_timing_table.erase;
		break;
	case PAGE_PROGRAM:
	case PAGE_PROGRAM_MP:
		delay = nand_timing_table.data_transfer_time_per_sector * p_ftl_req.length;
		break;
	case PAGE_PROGRAM_FINISH:
		if (is_msb_page(p_addr)) delay = nand_timing_table.msb_program;
		else delay = nand_timing_table.lsb_program;
		break;
	default :
		break;
	}
	return delay;
}
