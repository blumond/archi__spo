#ifndef __DATA_TRANSFER_ENGINE__
#define __DATA_TRANSFER_ENGINE__

#include <pthread.h>

#define DATA_TRANSFER_UNIT 512

extern struct dte_request_queue *dte_req_q;

struct dte_request
{
	int id;//ftl_cmdÀÇ id * plane ¼ö + plane number;
	long long deadline;
	char *src;
	char *dst;
	int size;

	struct dte_request *prev;
	struct dte_request *next;
};

struct dte_request_queue
{
	struct dte_request *head;
	struct dte_request *tail;
	int num_of_entries;

	pthread_mutex_t mutex;
};

void init_dte_request_queue(struct dte_request_queue **p_dte_req_q);

void dte_request_enqueue(struct dte_request_queue *p_dte_req_q, struct dte_request p_dte_req);
void set_dte_request_deadline(struct dte_request_queue *p_dte_req_q, int p_id, long long p_deadline);

struct dte_request get_dte_request(struct dte_request_queue *p_dma_req_q);
void unit_data_transfer(struct dte_request_queue *p_dma_req_q);

int is_data_transfer_done(struct dte_request_queue *p_dma_req_q, int p_id);

void *data_transfer_engine();
#endif