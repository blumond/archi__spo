#include <string.h>
#include "flash_operation_unit.h"

void cmd_to_char(int p_cmd, char *p_char_cmd)
{
	switch (p_cmd)
	{
	case READ:
		strcpy_s(p_char_cmd, sizeof("READ"), "READ");
		break;
	case READ_MP:
		strcpy_s(p_char_cmd, sizeof("READ_MP"), "READ_MP");
		break;
	case DATA_OUT:
		strcpy_s(p_char_cmd, sizeof("DATA_OUT"), "DATA_OUT");
		break;
	case READ_FINISH:
		strcpy_s(p_char_cmd, sizeof("READ_FINISH"), "READ_FINISH");
		break;
	case BLOCK_ERASE:
		strcpy_s(p_char_cmd, sizeof("BLOCK_ERASE"), "BLOCK_ERASE");
		break;
	case BLOCK_ERASE_MP:
		strcpy_s(p_char_cmd, sizeof("BLOCK_ERASE_MP"), "BLOCK_ERASE_MP");
		break;
	case BLOCK_ERASE_REPORT:
		strcpy_s(p_char_cmd, sizeof("BLOCK_ERASE_REPORT"), "BLOCK_ERASE_REPORT");
		break;
	case READ_STATUS:
		strcpy_s(p_char_cmd, sizeof("READ_STATUS"), "READ_STATUS");
		break;
	case PAGE_PROGRAM:
		strcpy_s(p_char_cmd, sizeof("PAGE_PROGRAM"), "PAGE_PROGRAM");
		break;
	case PAGE_PROGRAM_MP:
		strcpy_s(p_char_cmd, sizeof("PAGE_PROGRAM_MP"), "PAGE_PROGRAM_MP");
		break;
	case PAGE_PROGRAM_FINISH:
		strcpy_s(p_char_cmd, sizeof("PAGE_PROGRAM_FINISH"), "PAGE_PROGRAM_FINISH");
		break;
	case PAGE_PROGRAM_REPORT:
		strcpy_s(p_char_cmd, sizeof("PAGE_PROGRAM_FINISH"), "PAGE_PROGRAM_FINISH");
		break;
	case RESET:
		strcpy_s(p_char_cmd, sizeof("RESET"), "RESET");
		break;
	}
}