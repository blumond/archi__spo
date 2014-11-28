#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "flash_memory.h"
#include "flash_operation_unit.h"
#include "configuration.h"

static struct nand_memory_pointer nand_mem_pointer;

void init_memory_alloc(struct nand_memory_pointer *p_nand_mem_pointer)
{
	p_nand_mem_pointer->pages = 0;
	p_nand_mem_pointer->blocks = 0;
	p_nand_mem_pointer->planes = 0;
	p_nand_mem_pointer->chips = 0;
	p_nand_mem_pointer->buses = 0;

	p_nand_mem_pointer->bus = (struct bus*)malloc(sizeof(struct bus) * NUM_OF_BUS);
	if (p_nand_mem_pointer->bus == NULL)
	{
		printf("can't alloc buses\n");
		assert(p_nand_mem_pointer->bus != NULL);
	}
	p_nand_mem_pointer->chip = (struct nand_chip *)malloc(sizeof(struct nand_chip) * CHIPS_PER_BUS * NUM_OF_BUS);
	if (p_nand_mem_pointer->chip == NULL)
	{
		printf("can't alloc chips\n");
		assert(p_nand_mem_pointer->chip != NULL);
	}
	p_nand_mem_pointer->plane = (struct nand_plane*)malloc(sizeof(struct nand_plane) * PLANES_PER_CHIP * CHIPS_PER_BUS * NUM_OF_BUS);
	if (p_nand_mem_pointer->plane == NULL)
	{
		printf("can't alloc planes\n");
		assert(p_nand_mem_pointer->plane != NULL);
	}
	p_nand_mem_pointer->shadow_buffer = (char *)malloc(SIZE_OF_PAGE * PLANES_PER_CHIP * CHIPS_PER_BUS * NUM_OF_BUS);
	if (p_nand_mem_pointer->shadow_buffer == NULL)
	{
		printf("can't alloc shadow_buffer\n");
		assert(p_nand_mem_pointer->shadow_buffer != NULL);
	}
	p_nand_mem_pointer->block = (struct nand_block*)malloc(sizeof(struct nand_block) * BLOCKS_PER_PLANE * PLANES_PER_CHIP * CHIPS_PER_BUS * NUM_OF_BUS);
	if (p_nand_mem_pointer->block == NULL)
	{
		printf("can't alloc blocks\n");
		assert(p_nand_mem_pointer->block != NULL);
	}
	p_nand_mem_pointer->page = (struct nand_page *)malloc(sizeof(struct nand_page) * PAGES_PER_BLOCK * BLOCKS_PER_PLANE * PLANES_PER_CHIP * CHIPS_PER_BUS * NUM_OF_BUS);
	if (p_nand_mem_pointer->page == NULL)
	{
		printf("can't alloc pages\n");
		assert(p_nand_mem_pointer->page != NULL);
	}
}

void init_flashmodule(struct flashmodule *p_fm)
{
	int i;

	init_memory_alloc(&nand_mem_pointer);

	p_fm->power_fail_flag = 0;
	p_fm->buses = nand_mem_pointer.bus;

	for (i = 0; i < NUM_OF_BUS; i++)
	{
		init_bus(&p_fm->buses[i]);
	}
}

void init_bus(struct bus *p_bus)
{
	int i;

	p_bus->chips = &nand_mem_pointer.chip[nand_mem_pointer.buses * CHIPS_PER_BUS];
	nand_mem_pointer.buses++;

	for (i = 0; i < CHIPS_PER_BUS; i++)
	{
		init_chip(&p_bus->chips[i]);
	}
}

void init_chip(struct nand_chip *p_chip)
{
	int i;

	p_chip->status = 0;
	p_chip->current_access_mode = MLC_MODE;
	p_chip->cmd = IDLE;

	p_chip->planes = &nand_mem_pointer.plane[nand_mem_pointer.chips];
	nand_mem_pointer.chips++;

	for (i = 0; i < PLANES_PER_CHIP; i++)
	{
		init_plane(&p_chip->planes[i]);
	}
}

void init_plane(struct nand_plane *p_plane)
{
	int i;

#ifdef MEMCPY
	p_plane->page_buffer = &nand_mem_pointer.shadow_buffer[nand_mem_pointer.planes * SIZE_OF_PAGE];
	p_plane->shadow_buffer = NULL;
#endif
#ifdef DATA_TRANSFER_ENGINE
	p_plane->page_buffer = NULL;
	p_plane->shadow_buffer = &nand_mem_pointer.shadow_buffer[nand_mem_pointer.planes * SIZE_OF_PAGE];
#endif
	p_plane->reg_addr = 0;

	p_plane->blocks = &nand_mem_pointer.block[nand_mem_pointer.planes * BLOCKS_PER_PLANE];
	nand_mem_pointer.planes++;

	for (i = 0; i < BLOCKS_PER_PLANE; i++)
	{
		init_block(&p_plane->blocks[i]);
	}
}

void init_block(struct nand_block *p_block)
{
	int i;

	p_block->bad_block = 0;
	p_block->last_programmed_page = 0;
	p_block->pecycle = 0;
	p_block->block_access_mode = MLC_MODE;
	p_block->pages = &nand_mem_pointer.page[nand_mem_pointer.blocks * PAGES_PER_BLOCK];
	nand_mem_pointer.page_cell = (char *)malloc(SIZE_OF_PAGE * PAGES_PER_BLOCK);
	nand_mem_pointer.pages = 0;
	nand_mem_pointer.blocks++;

	for (i = 0; i < PAGES_PER_BLOCK; i++)
	{
		init_page(&p_block->pages[i]);
	}
}

void init_page(struct nand_page *p_page)
{
	p_page->nop = 0;
	p_page->state = 0;
	p_page->data = &nand_mem_pointer.page_cell[nand_mem_pointer.pages * SIZE_OF_PAGE];
	nand_mem_pointer.pages++;
}

void init_sibling_page_table()
{
	int i;

	if (BITS_PER_CELL == 1)
	{
		for (i = 0; i < PAGES_PER_BLOCK; i++) 
		{
			sibling_page_table[i][0] = i;
		}
	}
	else if (BITS_PER_CELL == 2)
	{
		for (i = 0; i < PAGES_PER_BLOCK; i++)
		{
			if (i == 0 || i == 1 || i == PAGES_PER_BLOCK - 6 || i == PAGES_PER_BLOCK - 5)
			{
				// first page
				sibling_page_table[i][0] = i;
				sibling_page_table[i][1] = i + 4;
			}
			else if (i == 4 || i == 5 || i == PAGES_PER_BLOCK - 2 || i == PAGES_PER_BLOCK - 1)
			{
				// second page
				sibling_page_table[i][0] = i - 4;
				sibling_page_table[i][1] = i;
			}
			else if (i % 4 == 0 || i % 4 == 1)
			{
				// second page
				sibling_page_table[i][0] = i - 6;
				sibling_page_table[i][1] = i;
			}
			else if (i % 4 == 2 || i % 4 == 3)
			{
				// first page
				sibling_page_table[i][0] = i;
				sibling_page_table[i][1] = i + 6;
			}
		}
	}
	else
	{
		printf("can't allocate sibling scheme!\n");
	}
}

int is_msb_page(int p_addr)
{
	int page_index;

	page_index = p_addr & MASK_PAGE;

	if (page_index == sibling_page_table[page_index][1])
	{
		return 1;
	}

	return 0;
}

int get_lsb_page(int p_addr)
{
	int page_index;

	page_index = p_addr & MASK_PAGE;
	return (sibling_page_table[page_index][0]);
}

int addr_to_page(int p_addr)
{
	return ((p_addr >> SHIFT_PAGE) & MASK_PAGE);
}

int addr_to_plane(int p_addr)
{
	return ((p_addr >> SHIFT_PLANE) & MASK_PLANE);
}

int addr_to_block(int p_addr)
{
	return ((p_addr >> SHIFT_BLOCK) & MASK_BLOCK);
}

int addr_to_lun(int p_addr)
{
	return ((p_addr >> SHIFT_LUN) & MASK_LUN);
}

int addr_to_channel(int p_addr)
{
	int lun = (p_addr >> SHIFT_LUN) & MASK_LUN;

	return (lun % NUM_OF_BUS);
}

int addr_to_way(int p_addr)
{
	int lun = (p_addr >> SHIFT_LUN) & MASK_LUN;

	return (lun / NUM_OF_BUS);
}

int addr_to_sector_offset(int p_addr)
{
	int sector_offset = (p_addr % SECTOR_PER_PAGE);

	return sector_offset;
}

void reset_flashmodule(struct flashmodule *p_fm)
{
	int i;

	for (i = 0; i < NUM_OF_BUS; i++)
	{
		reset_bus(&p_fm->buses[i]);
	}
}

void reset_bus(struct bus *p_bus)
{
	int i;

	for (i = 0; i < CHIPS_PER_BUS; i++)
	{
		reset_chip(&p_bus->chips[i]);
	}
}

void reset_chip(struct nand_chip *p_chip)
{
	int i;

	p_chip->status = 0;
	p_chip->current_access_mode = MLC_MODE;
	p_chip->cmd = IDLE;

	for (i = 0; i < PLANES_PER_CHIP; i++)
	{
		reset_plane(&p_chip->planes[i]);
	}
}

void reset_plane(struct nand_plane *p_plane)
{
#ifdef MEMCPY
	memset(p_plane->page_buffer, 0xff, SIZE_OF_PAGE);
#endif
	p_plane->reg_addr = 0;
}
