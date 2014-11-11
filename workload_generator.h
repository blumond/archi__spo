#ifndef __WORKLOAD_GENERATOR_H__
#define __WORKLOAD_GENERATOR_H__

#include "flash_memory.h"

#define USER_CAPACITY		2048000	//KB
#define MAPPING_UNIT_SIZE	8	//KB

//reference module

//write request�� ����? or read reclaim�� ���� scheme�� ����Ϸ��� read�� �ʿ�?
//outstanding request list // dual linked list�� �����ϸ鼭 �ڷ� �߰��ϰ� ack�� ���� �տ��� ����.


//write request�� read request�� �ٲپ� replay�Ѵ�. 
//data compare�� ���� semantic�� �°� sector ������ old or new�� ����Ǿ��ִ��� Ȯ��
//old�� ��� data seed table �ٽ� ����
//�̶� unreliable read�� �߻��ϴ��� Ȯ��
void replay_outstanding_request_list();

//��� block�� �а� data compare
//unreliable read �߻� ���� Ȯ�� -- sibling page data loss detecting // mapping data corruption detection
void read_all_logical_page();

//nand simulator�� ����Ǿ��ִ� damaged block list�� �о�� page state�� �������� (erased) Ȯ��
void damaged_block_aware_detection();
//�� die�� multi-plane ������ block�� fail�� �� ����

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
