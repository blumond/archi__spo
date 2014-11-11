/* 현재 Timing Model의 경우, data dependency가 적용되지 않은 상황	//
// init파일의 이름 : init_timing_model.txt							//
// init파일의 형식 : Strict하게 아래와 같이 만들어야 함				//								
//	LSB_READ [값]													//
//	MSB_READ [값]													//
//	...																//
//	DATA_TRANSFER_TIME_PER_SECTOR [값]								*/

#include <Windows.h>
#include <stdio.h>
#include "flash_operation_unit.h"
#include "timing_model.h"
#include "flash_memory.h"
#include "event_queue.h"


//set timing model
void init_timing_model()
{
	//ini file을 읽어들이고 timing model table 작성
	LARGE_INTEGER frequency;
	FILE *init;
	int no_error;

	struct nand_timing timing_init_v;
	
	fopen_s(&init, "init_timing_model.config","r");

	// init 파일이 열린경우
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
	}// init 파일이 열리지 않은 경우 default value 사용
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

	// us단위 혹은 ms단위 계산법 적용 필요
	nand_timing_table.erase			= timing_init_v.erase * frequency.QuadPart / 1000000;
	nand_timing_table.lsb_program	= timing_init_v.lsb_program * frequency.QuadPart / 1000000;
	nand_timing_table.lsb_read		= timing_init_v.lsb_read * frequency.QuadPart / 1000000;
	nand_timing_table.msb_program	= timing_init_v.msb_program * frequency.QuadPart / 1000000;
	nand_timing_table.msb_read		= timing_init_v.msb_read * frequency.QuadPart / 1000000;
	nand_timing_table.data_transfer_time_per_sector = timing_init_v.data_transfer_time_per_sector * frequency.QuadPart / 1000000;
}


long long get_timing(struct ftl_request p_ftl_req, int p_addr)
{
	//timing table에 의해 timing 값 return
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
