#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "workload_generator.h"

//data seed table : update count
static int data_seed_table[USER_CAPACITY / MAPPING_UNIT_SIZE];
int damaged_block[NUM_OF_BUS][CHIPS_PER_BUS][PLANES_PER_CHIP];

void init_outstanding_request_list(struct outstanding_request_list **p_outstanding_request_list)
{
	*p_outstanding_request_list = (struct outstanding_request_list *)malloc(sizeof(struct outstanding_request_list));
	if (*p_outstanding_request_list == NULL)
	{
		printf("can't alloc outstanding request list\n");
		assert(*p_outstanding_request_list != NULL);
	}

	(*p_outstanding_request_list)->outstanding_request_list_head = NULL;
	(*p_outstanding_request_list)->outstanding_request_list_tail = NULL;
}	

int insert_outstanding_request_list(/*struct request, */struct outstanding_request_list *p_outstanding_request_list)
{
	struct outstanding_request_node *new_request_node;

	p_outstanding_request_list->num_of_entry++;

	new_request_node = (struct outstanding_request_node *)malloc(sizeof(struct outstanding_request_node));
/*	new_request_node->ftl_req = p_ftl_req;*/
	new_request_node->next = NULL;
	new_request_node->prev = NULL;
	
	//tail�� insert
	if (p_outstanding_request_list->outstanding_request_list_head == NULL)
	{
		p_outstanding_request_list->outstanding_request_list_head = new_request_node;
		p_outstanding_request_list->outstanding_request_list_tail = new_request_node;
	}
	else
	{
		new_request_node->prev = p_outstanding_request_list->outstanding_request_list_tail;
		p_outstanding_request_list->outstanding_request_list_tail->next = new_request_node;
		p_outstanding_request_list->outstanding_request_list_tail = new_request_node;
	}

	return 1;
}

//tail���� ���� ���, head���� ���� ��찡 ���� �˻��� �ʿ��� //ID�� �־�� ��
void del_outstanding_request_list(struct outstanding_request_list *p_outstanding_request_list, int p_id, int p_direction)
{
	struct outstanding_request_node *or_node_iter;

#ifdef DEBUG_MODE
	//empty check�� dequeue�� ȣ���ϴ� ������ ����
	assert(rq_q->num_of_entry > 0);
#endif
	p_outstanding_request_list->num_of_entry--;

	if (p_direction == 0)//head����
	{
		for (or_node_iter = p_outstanding_request_list->outstanding_request_list_head;
			or_node_iter != NULL; or_node_iter = or_node_iter->next)
		{
			//if (or_node_iter->id == p_id)
			{
				or_node_iter->prev->next = or_node_iter->next;
				or_node_iter->next->prev = or_node_iter->prev;
				if (or_node_iter->next == NULL) p_outstanding_request_list->outstanding_request_list_tail = or_node_iter->prev;
				if (or_node_iter->prev == NULL) p_outstanding_request_list->outstanding_request_list_head = or_node_iter->next;
				free(or_node_iter);
				break;
			}
		}
	}
	else//tail����
	{
		for (or_node_iter = p_outstanding_request_list->outstanding_request_list_tail;
			or_node_iter != NULL; or_node_iter = or_node_iter->prev)
		{
//			if (or_node_iter->id == p_id)
			{
				or_node_iter->prev->next = or_node_iter->next;
				or_node_iter->next->prev = or_node_iter->prev;
				if (or_node_iter->next == NULL) p_outstanding_request_list->outstanding_request_list_tail = or_node_iter->prev;
				if (or_node_iter->prev == NULL) p_outstanding_request_list->outstanding_request_list_head = or_node_iter->next;
				free(or_node_iter);
				break;
			}
		}
	}
}

//��� node ����
void reset_outstanding_request_list(struct outstanding_request_list *p_outstanding_request_list)
{
	struct outstanding_request_node *node_iter;
	struct outstanding_request_node *temp_node;

	node_iter = p_outstanding_request_list->outstanding_request_list_head;
	
	while (node_iter != NULL)
	{
		temp_node = node_iter;
		free(temp_node);
		node_iter = node_iter->next;
	}
	p_outstanding_request_list->outstanding_request_list_head = NULL;
	p_outstanding_request_list->outstanding_request_list_tail = NULL;
	p_outstanding_request_list->num_of_entry = 0;
}

struct outstanding_request_list *out_req_list;

/*
void ���_����()
{
	//��ũ�ε� �����⿡�� FTL�� ��ũ�ε� ����
	//���۷��� ��⿡�� ����
	insert_outstanding_request_list(out_req_list);
	//data seed table ������Ʈ

	//FTL�� ack������ �ش� command�� ���� (head����)
	del_outstanding_request_list(out_req_list, id, 0);
}*/


/*
//async fault �߻���
void async_fault_ó��()
{
	//FTL�� �������� ���� request�� ����
	for ()
	{
		del_outstanding_request_list(out_req_list, id, 1);
	}
	//���
}

//FTL ���� ��Ȯ�� �˻�
void recovery_result_verification()
{
	int i;
	//1. out_req_list �˻�
	//read every req in list

	//2. damaged block �˻�()
	damaged_block_check();

	//3. ��� block read
	for (i = 0; i < X; i++)
	{
		read(page i);
		if (data_seed_table[i] == read data)
		{
			ERROR;
		}
	}
}
*/
