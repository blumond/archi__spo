#ifndef __FLASH_MEMORY_H__
#define __FLASH_MEMORY_H__

#include "type.h"

#define NUM_OF_DIES 1
#define DIES_PER_LUN 1
#define NUM_OF_BUS 2
#define CHIPS_PER_BUS 2
#define PLANES_PER_CHIP 2
#define BLOCKS_PER_PLANE 512
#define PAGES_PER_BLOCK 64
#define SECTOR_PER_PAGE 16
#define SPARE_SIZE (2 * 1024)
#define SIZE_OF_PAGE (SECTOR_PER_PAGE * 512 + SPARE_SIZE)	//including spare area
#define SIZE_OF_SECTOR 512
#define BITS_PER_CELL 2

#define NOP_MLC 1
#define NOP_SLC 1

//addressing
//lun  : channel * way == 2bit
//block : 1024	== 10bit
//plane : 2	== 1bit
//page : 128 == 7bit
//sector offset : 16 == 4bit
#define ADDR_SECTOR_OFFSET 4
#define ADDR_PAGE 7
#define ADDR_PLANE 1
#define ADDR_BLOCK 10
#define ADDR_LUN 2

#define MASK_SECTOR (BIT4-1)
#define MASK_PAGE	(BIT7-1)
#define MASK_PLANE	(BIT1-1)
#define MASK_BLOCK	(BIT10-1)
#define MASK_LUN	(BIT0)
#define MASK_ADDR	((1 << SHIFT_ADDR) - 1)

#define SHIFT_PAGE	(ADDR_SECTOR_OFFSET)
#define SHIFT_PLANE (ADDR_SECTOR_OFFSET+ADDR_PAGE)
#define SHIFT_BLOCK (ADDR_SECTOR_OFFSET+ADDR_PAGE+ADDR_PLANE)
#define SHIFT_LUN	(ADDR_SECTOR_OFFSET+ADDR_PAGE+ADDR_PLANE+ADDR_BLOCK)
#define SHIFT_ADDR	(ADDR_SECTOR_OFFSET+ADDR_PAGE+ADDR_PLANE+ADDR_BLOCK+ADDR_LUN)

struct nand_memory_pointer
{
	char *shadow_buffer;
	char *page_cell;
	struct nand_page *page;
	struct nand_block *block;
	struct nand_plane *plane;
	struct nand_chip *chip;
	struct bus *bus;

	int pages;
	int blocks;
	int planes;
	int chips;
	int buses;
};

struct nand_page
{
	int state;	//abstract page fault model state
	int nop;	//number of programming
	char *data;
};

struct nand_block
{
	struct nand_page *pages;
	int pecycle;				//program/erase cycle
	int last_programmed_page;	//for checking ascending ordered programming
	int block_access_mode;			//SLC mode or MLC mode
	int bad_block;				//1: bad, 0: good
	
};

struct nand_plane
{
	struct nand_block *blocks;
	char *page_buffer;
	char *shadow_buffer;
	int reg_addr;
};

struct nand_chip
{
	struct nand_plane *planes;
	char status;
	int cmd;
	int current_access_mode;
};

struct bus
{
	struct nand_chip *chips;
};

struct flashmodule
{
	struct bus *buses;

	int power_fail_flag;
};

extern int sibling_page_table[PAGES_PER_BLOCK][BITS_PER_CELL];

void init_flashmodule(struct flashmodule *p_fm);
void init_bus(struct bus *p_bus);
void init_chip(struct nand_chip *p_chip);
void init_plane(struct nand_plane *p_plane);
void init_block(struct nand_block *p_block);
void init_page(struct nand_page *p_page);

void init_sibling_page_table();

int is_msb_page(int p_addr);
int get_lsb_page(int p_addr);
int is_same_addr(int p_addr1, int p_addr2, int p_mp_cmd);
int addr_to_page(int p_addr);
int addr_to_plane(int p_addr);
int addr_to_block(int p_addr);
int addr_to_lun(int p_addr);
int addr_to_channel(int p_addr);
int addr_to_way(int p_addr);
int addr_to_sector_offset(int p_addr);

void reset_flashmodule(struct flashmodule *p_fm);
void reset_bus(struct bus *p_bus);
void reset_chip(struct nand_chip *p_chip);
void reset_plane(struct nand_plane *p_plane);
#endif
