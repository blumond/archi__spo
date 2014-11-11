#ifndef __WORKLOAD_GENERATOR_H__
#define __WORKLOAD_GENERATOR_H__

#include "flash_memory.h"

#define USER_CAPACITY		2048000	//KB
#define MAPPING_UNIT_SIZE	8	//KB

//reference module

//write request만 관리? or read reclaim과 같은 scheme도 고려하려면 read도 필요?
//outstanding request list // dual linked list로 관리하면서 뒤로 추가하고 ack온 것은 앞에서 뺀다.


//write request를 read request로 바꾸어 replay한다. 
//data compare를 통해 semantic에 맞게 sector 단위로 old or new가 저장되어있는지 확인
//old인 경우 data seed table 다시 변경
//이때 unreliable read가 발생하는지 확인
void replay_outstanding_request_list();

//모든 block을 읽고 data compare
//unreliable read 발생 여부 확인 -- sibling page data loss detecting // mapping data corruption detection
void read_all_logical_page();

//nand simulator에 저장되어있는 damaged block list를 읽어보고 page state가 정상인지 (erased) 확인
void damaged_block_aware_detection();
//각 die당 multi-plane 단위의 block이 fail될 수 있음

extern int damaged_block[NUM_OF_BUS][CHIPS_PER_BUS][PLANES_PER_CHIP];


struct outstanding_request_node
{
	//struct workload_request workload_req;
	struct outstanding_request_node *next;
	struct outstanding_request_node *prev;
};

struct outstanding_request_list
{
	struct outstanding_request_node *outstanding_request_list_head;
	struct outstanding_request_node *outstanding_request_list_tail;
	int num_of_entry;
};

void init_outstanding_request_list(struct outstanding_request_list **p_outstanding_request_list);
int insert_outstanding_request_list(/*struct request, */struct outstanding_request_list *p_outstanding_request_list);
void del_outstanding_request_list(struct outstanding_request_list *p_outstanding_request_list, int p_id, int p_direction);
void reset_outstanding_request_list(struct outstanding_request_list *p_outstanding_request_list);

#endif
