#include <stdio.h>
#include <assert.h>
#include "page_fault_model.h"
#include "type.h"
#include "nand_flash_emulator.h"
#include "workload_generator.h"

void ascending_order_program_violation(int p_last_programmed_page, int p_programming_page)
{
	if (p_last_programmed_page > p_programming_page)
	{
		printf("ascending order program violation\n");
		assert(0);
	}
}

void program_after_erase_violation(int p_state)
{
	if (p_state != PFM_ERASED)
	{
		printf("program after erase violation\n");
		assert(0);
	}
}

void nop_violation(int p_nop, int p_block_mode)
{
	switch (p_block_mode)
	{
	case MLC_MODE:
		if (p_nop >= NOP_MLC)
		{
			printf("number of program violation\n");
			assert(0);
		}
		break;

	case SLC_MODE:
		if (p_nop > NOP_SLC)
		{
			printf("number of program violation\n");
			assert(0);
		}
		break;
	}
}

void unreliable_read_violation(int p_state)
{
	if ((fm.power_fail_flag == 0)
		&& p_state != PFM_PROGRAMMED
		&& p_state != PFM_ERASED)
	{
		printf("unreliable  read violation\n");
		assert(0);
	}
}

void damaged_block_check()
{
	struct nand_chip *chip;
	int i, j, k, l;

	for (i = 0; i < NUM_OF_BUS; i++)
	{
		for (j = 0; j < CHIPS_PER_BUS; j++)
		{
			chip = &fm.buses[i].chips[j];
			for (k = 0; k < PLANES_PER_CHIP; k++)
			{
				if (damaged_block[i][j][k] != 0xFFFFFFFF)
				{
					for (l = 0; l < PAGES_PER_BLOCK; l++)
					{
						if (chip->planes[k].blocks[damaged_block[i][j][k]].pages[l].state != PFM_ERASED)
						{
							printf("latent error detected.\n");
							assert(0);
						}
					}
				}
			}
		}
	}
}
