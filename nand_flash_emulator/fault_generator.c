#include "fault_generator.h"
#include "event_queue.h"
#include "nand_flash_emulator.h"
#include "page_fault_model.h"
#include "simple_rand.h"
#include "flash_operation_unit.h"
#include "configuration.h"
#include <stdio.h>


extern unsigned int g_random_no;

// 리커버리 확인
int is_recovery = 0;
// 폴트 모델
int fault_model;

//동기 폴트 환경 변수
SYN_FAULT_ENV *syn_env;

//비동기 폴트 환경 변수
ASYN_FAULT_ENV *asyn_env;


//동기 폴트 내부 인터페이스
int erase_error(int pe_cycle);
int program_error(int pe_cycle);
int read_error(int state);
int get_pe_cycle(int addr);
int get_state(int addr);
LONGLONG get_variation(void);
LONGLONG get_interval(void);

//확률분포 함수
LONGLONG dis_uniform(LONGLONG start, LONGLONG end);
LONGLONG dis_gausian(LONGLONG mean, LONGLONG stand_dev);
LONGLONG dis_user_def(LONGLONG start, LONGLONG end, int iter, int w_sum, int *w);

// 확률분포 관련 기본 함수
int mylog(double src);
double mysqrt(unsigned int src);
int reg_asynch_fault(struct event_queue_node eq_node);
int reg_asynch_fault_without_lock(struct event_queue_node eq_node);

//비동기 폴트 내부 인터페이스
int reg_asynch_fault_rel(LARGE_INTEGER time);


int fini_fault_module(void){
	if (asyn_env->variation_mode == DIS_USERDEF){
		free(asyn_env->variation_w);
	}
	if (asyn_env->interval_mode == DIS_USERDEF){
		free(asyn_env->interval_w);
	}
	free(asyn_env);
	free(syn_env);

	return 1;
}

/* 동기 폴트 생성기 */
void init_sync_fault_generator(void)
{

	char mode;

	FILE *init;
	int no_error;

	syn_env = (SYN_FAULT_ENV *)malloc(sizeof(SYN_FAULT_ENV));

	fopen_s(&init, "init_syn_fault_model.config", "r");

	if (init != NULL){
		/* 폴트 모델 */
		no_error = fscanf_s(init, "FAULT_MODEL %c\n", &mode, 1);
		if (no_error < 0) printf("init_syn_fault_model.config error\n");

		if (mode == 'P' || mode == 'p') fault_model = PERSISTENT_MODEL;
		else if (mode == 'U' || mode == 'u') fault_model = UNPERSISTENT_MODEL;
		else printf("init_syn_fault_model.config error\n");

		/* 동기적 폴트 생성 모드 */
		no_error = fscanf_s(init, "SYNC_MODE %c\n", &mode, 1);
		if (no_error < 0) printf("init_syn_fault_model.config error\n");

		if (mode == 'T' || mode == 't') syn_env->syn_pattern = SYN_M_THRESHOLD;
		else if (mode == 'R' || mode == 'r') syn_env->syn_pattern = SYN_M_REAL;
		else printf("init_syn_fault_model.config error\n");

		/* 리커버리 관련 모드 */
		no_error = fscanf_s(init, "RECOVERY_ASSOCIATED_MODE %c\n", &mode, 1);
		if (no_error < 0) printf("init_syn_fault_model.config error\n");

		if (mode == 'S' || mode == 's') syn_env->recovery_asso_mode = RECO_ASSO_SAFE;
		else if (mode == 'N' || mode == 'n') syn_env->recovery_asso_mode = RECO_ASSO_NORMAL;
		else if (mode == 'P' || mode == 'p') syn_env->recovery_asso_mode = RECO_ASSO_PILED_UP;
		else printf("init_syn_fault_model.config error\n");


		no_error = fscanf_s(init, "ERASE_THRESHOLD %d\n", &(syn_env->erase_threshold));
		if (no_error < 0) printf("init_syn_fault_model.config error\n");

		no_error = fscanf_s(init, "ERASE_FAILURE_RATE %d\n", &(syn_env->erase_error_rate));
		if (no_error < 0) printf("init_syn_fault_model.config error\n");

		no_error = fscanf_s(init, "PROGRAM_THRESHOLD %d\n", &(syn_env->program_threshold));
		if (no_error < 0) printf("init_syn_fault_model.config error\n");

		no_error = fscanf_s(init, "PROGRAM_FAILURE_RATE %d\n", &(syn_env->program_error_rate));
		if (no_error < 0) printf("init_syn_fault_model.config error\n");

	}
	else{
		fault_model = UNPERSISTENT_MODEL;
		syn_env->syn_pattern = SYN_M_THRESHOLD;
		syn_env->recovery_asso_mode = RECO_ASSO_NORMAL;
		syn_env->erase_threshold = ERASE_PE_CYCLE_THRESHOLD;
		syn_env->erase_error_rate = ERASE_ERROR_RATE;
		syn_env->program_threshold = PROGRAM_PE_CYCLE_THRESHOLD;
		syn_env->program_error_rate = PROGRAM_ERROR_RATE;
	}

	if (init != NULL)
		fclose(init);

}


int sync_fault_gen(int cmd, int addr){
	
	int pe_cycle;
	int state;

	switch (cmd){
	case BLOCK_ERASE:
		pe_cycle = get_pe_cycle(addr);

#ifdef SYNC_FAULT_FREE
		return ERASE_OK;
#endif
		if (erase_error(pe_cycle) == FAULT){
			return ERASE_IF;
		}
		else return ERASE_OK;
	case PAGE_PROGRAM_FINISH:
		pe_cycle = get_pe_cycle(addr);

#ifdef SYNC_FAULT_FREE
		return PROGRAM_OK;
#endif

		if (program_error(pe_cycle) == FAULT){
			return PROGRAM_IF;
		}
		else return PROGRAM_OK;
	case READ:
		state = get_state(addr);
		return read_error(state);
	default:
		printf("undefiend operation\n");
	}

	return NO_FAULT;
}

/* 비동기 폴트 생성기 */
int init_async_fault_generator(void){

	struct event_queue_node eq_node;

	char mode;
	LARGE_INTEGER fault_time;

	FILE *init;
	int no_error;

	int i, temp;

	asyn_env = (ASYN_FAULT_ENV *)malloc(sizeof(ASYN_FAULT_ENV));

	fopen_s(&init, "init_asyn_fault_model.config", "r");


	if (init != NULL){
		
		no_error = fscanf_s(init, "ASYN_MODE %c\n", &mode, 1);
		if (no_error < 0) printf("init_asyn_fault_model.config error\n");

		if (mode == 'R' || mode == 'r') asyn_env->asyn_pattern = ASYN_M_RANDOM;
		else if (mode == 'P' || mode == 'p') asyn_env->asyn_pattern = ASYN_M_POST_RECOVERY;
		else if (mode == 'T' || mode == 't') asyn_env->asyn_pattern = ASYN_M_TEST_RECOVERY;
		else printf("init_asyn_fault_model.config error\n");
		
		no_error = fscanf_s(init, "INTERVAL_MODE %c\n", &mode, 1);
		if (no_error < 0) printf("init_asyn_fault_model.config error\n");

		if (mode=='C' || mode=='c'){
			asyn_env->interval_mode = DIS_CONSTANT;
			no_error = fscanf_s(init, "INTERVAL %lld\n", &(asyn_env->interval));
			if (no_error < 0) printf("init_asyn_fault_model.config error\n");
		}
		else if (mode=='U' || mode=='u'){
			asyn_env->interval_mode = DIS_UNIFORM;
			no_error = fscanf_s(init, "INTERVAL -S %lld -E %lld\n", &(asyn_env->interval_start), &(asyn_env->interval_end));
			if (no_error < 0) printf("init_asyn_fault_model.config error\n");
		}
		else if (mode == 'G' || mode == 'g'){
			asyn_env->interval_mode = DIS_GAUSIAN;
			no_error = fscanf_s(init, "INTERVAL -M %lld -S %lld\n", &(asyn_env->interval_mean), &(asyn_env->interval_stand_dev));
			if (no_error < 0) printf("init_asyn_fault_model.config error\n");
		}
		else if (mode == 'D' || mode == 'd'){
			asyn_env->interval_mode = DIS_USERDEF;
			no_error = fscanf_s(init, "INTERVAL -S %lld -E %lld -N %d\n", &(asyn_env->interval_start), &(asyn_env->interval_end), &(asyn_env->interval_iter));
			if (no_error < 0) printf("init_asyn_fault_model.config error\n");
			
			/*malloc! free해주세요 나중에...*/
			asyn_env->interval_w_sum = 0;
			asyn_env->interval_w = malloc(asyn_env->interval_iter * sizeof(int));
			for (i = 0; i < asyn_env->interval_iter - 1 ; i++){
				no_error = fscanf_s(init, "%d ", &temp);
				if (no_error < 0) printf("init_asyn_fault_model.config error\n");

				asyn_env->interval_w[i] = temp;
				asyn_env->interval_w_sum += asyn_env->interval_w[i];
			}
			no_error = fscanf_s(init, "%d ", &temp);
			if (no_error < 0) printf("init_asyn_fault_model.config error\n");

			asyn_env->interval_w[i] = temp;
			asyn_env->interval_w_sum += asyn_env->interval_w[i];
		}
		else printf("init_asyn_fault_model.config error\n");

		no_error = fscanf_s(init, "VARIATION_MODE %c\n", &mode, 1);
		if (no_error < 0) printf("init_asyn_fault_model.config error\n");

		if (mode == 'U' || mode == 'u'){
			asyn_env->variation_mode = DIS_UNIFORM;
			no_error = fscanf_s(init, "VARIATION -S %lld -E %lld\n", &(asyn_env->variation_start), &(asyn_env->variation_end));
			if (no_error < 0) printf("init_asyn_fault_model.config error\n");
		}
		else if (mode == 'G' || mode == 'g'){
			asyn_env->variation_mode = DIS_GAUSIAN;
			no_error = fscanf_s(init, "VARIATION -M %lld -S %lld\n", &(asyn_env->variation_mean), &(asyn_env->variation_stand_dev));
			if (no_error < 0) printf("init_asyn_fault_model.config error");
		}
		else if (mode == 'D' || mode == 'd'){
			asyn_env->variation_mode = DIS_USERDEF;
			no_error = fscanf_s(init, "VARIATION -S %lld -E %lld -N %d\n", &(asyn_env->variation_start), &(asyn_env->variation_end), &(asyn_env->variation_iter));
			if (no_error < 0) printf("init_asyn_fault_model.config error\n");
			
			asyn_env->variation_w_sum = 0;
			asyn_env->variation_w = malloc(asyn_env->variation_iter * sizeof(int));
			for (i = 0; i < asyn_env->variation_iter - 1; i++){
				no_error = fscanf_s(init, "%d ", &temp);
				if (no_error < 0) printf("init_asyn_fault_model.config error\n");
				
				asyn_env->variation_w[i] = temp;
				asyn_env->variation_w_sum += asyn_env->variation_w[i];
			}
			no_error = fscanf_s(init, "%d", &temp);
			if (no_error < 0) printf("init_asyn_fault_model.config error\n");

			asyn_env->variation_w[i] = temp;
			asyn_env->variation_w_sum += asyn_env->variation_w[i];
		}

	}
	else{
		asyn_env->asyn_pattern = ASYN_M_RANDOM;
		asyn_env->interval_mode = DIS_CONSTANT;
		asyn_env->interval = ASYN_T_INTERVAL;
		asyn_env->variation_mode = DIS_UNIFORM;
		asyn_env->variation_start = ASYN_T_VARIATION * (-1);
		asyn_env->variation_end = ASYN_T_VARIATION;
	}


	QueryPerformanceCounter(&fault_time);

	fault_time.QuadPart = fault_time.QuadPart + ASYN_T_INITIAL;
	
#ifndef ASYNC_FAULT_FREE
	eq_node.time = fault_time;
	eq_node.dst = ASYNC_FAULT;
	reg_asynch_fault(eq_node);
#endif

	if (init != NULL)
		fclose(init);

	return 0;
}

/* Asynchronous fault의 pattern이 ASYN_M_RECOVERY의 경우, fault 발행 후 이 함수를 호출*/
/*	ASYN_M_RECOVRED의 경우, RECOVERY POINT가 지난 이후 이 함수를 호출*/
/* 특정 횟수 만큼 반복한 이후, 충분히 긴 시간 동안 FAULT 발생 휴식기*/
int async_fault_gen(void){

	static LONGLONG old_recovery_time = ASYN_T_UNIT;
	static LONGLONG min_recovery_time = ASYN_T_UNIT;
	static LONGLONG max_recovery_time = ASYN_T_INTERVAL * FAULT_MODE_NUMBER;

	struct event_queue_node eq_node;
	
	LARGE_INTEGER fault_time;
	int sign;

	static fault_count = 0;

	g_random_no = simple_rand(g_random_no);


	QueryPerformanceCounter(&fault_time);

	if (asyn_env->asyn_pattern == ASYN_M_RANDOM){

		asyn_env->variation = get_variation();
		if (asyn_env->variation == 0) asyn_env->variation = 1;
		asyn_env->interval = get_interval();

		g_random_no = simple_rand(g_random_no);
		fault_time.QuadPart += asyn_env->variation + asyn_env->interval;

		//FIXME
		eq_node.time = fault_time;
		eq_node.dst = ASYNC_FAULT;

		reg_asynch_fault_without_lock(eq_node);
	}
	else if (asyn_env->asyn_pattern == ASYN_M_POST_RECOVERY){
		
		asyn_env->variation = get_variation();
		asyn_env->interval = get_interval();

		if (g_random_no % 2 == 0) sign = 1;
		else sign = -1;

		g_random_no = simple_rand(g_random_no);
		
		fault_time.QuadPart += sign * (g_random_no% asyn_env->variation) + asyn_env->interval;

		if (fault_time.QuadPart < 0)
			fault_time.QuadPart = ASYN_T_UNIT;

		eq_node.time = fault_time;
		eq_node.dst = ASYNC_FAULT;

		reg_asynch_fault(eq_node);
	}
	else if (asyn_env->asyn_pattern == ASYN_M_TEST_RECOVERY){

		if (fault_count < FAULT_SPARSE_NUMBER){

			asyn_env->variation = get_variation();
			asyn_env->interval = get_interval();

			if (g_random_no % 2 == 0) sign = 1;
			else sign = -1;

			g_random_no = simple_rand(g_random_no);
			fault_time.QuadPart += sign * (g_random_no % asyn_env->variation) + asyn_env->interval;
			fault_count++;
		}
		else {
			if (g_random_no % 2 == 0) sign = 1;
			else sign = -1;

			g_random_no = simple_rand(g_random_no);
			fault_time.QuadPart += sign * (g_random_no % asyn_env->variation) + old_recovery_time;

			if (fault_time.QuadPart < 0)
				fault_time.QuadPart = ASYN_T_UNIT;

			if (fault_time.QuadPart > max_recovery_time){
				g_random_no = simple_rand(g_random_no);
				fault_time.QuadPart = g_random_no % max_recovery_time;
			}
			fault_count++;
			if (fault_count < FAULT_MODE_NUMBER) fault_count = 0;
		}

		eq_node.time = fault_time;
		eq_node.dst = ASYNC_FAULT;

		reg_asynch_fault_without_lock(eq_node);

	}

	return 0;
}

int reg_asynch_fault_without_lock(struct event_queue_node eq_node){

	enqueue_event_queue(eq, eq_node);

	return 1;
}

int reg_asynch_fault(struct event_queue_node eq_node){

	enqueue_event_queue(eq, eq_node);

	return 1;
}

/* 커맨드 인퍼페이스 대비 */
int set_asyn_pattern(int in){
	asyn_env->asyn_pattern = in;
	return 1;
}

/* 커맨드 인퍼페이스 대비 */
int manual_fault_injector(LARGE_INTEGER time){

	int ret;
	ret = reg_asynch_fault_rel(time);

	return ret;
}

int reg_asynch_fault_rel(LARGE_INTEGER time){
	// Register a asynchronous fault into Event queue

	struct event_queue_node eq_node;

	LARGE_INTEGER c_time;

	QueryPerformanceCounter(&c_time);
	eq_node.time.QuadPart = c_time.QuadPart + time.QuadPart;
	eq_node.dst = ASYNC_FAULT;

	reg_asynch_fault(eq_node);
	return 1;
}




int erase_error(int pe_cycle){

	int i;

	g_random_no = simple_rand(g_random_no);
	switch (syn_env->syn_pattern){
	case SYN_M_THRESHOLD:
		if (pe_cycle > syn_env->erase_threshold){
			i = g_random_no % syn_env->erase_error_rate;
			if (i == 0) return FAULT;
			else return NO_FAULT;
		}
		else return NO_FAULT;
	case SYN_M_REAL:
		if (pe_cycle > syn_env->erase_threshold){
			i = g_random_no % syn_env->erase_error_rate;
			if (i == 0) return FAULT;
			else return NO_FAULT;
		}
		else {
			i = g_random_no % (syn_env->erase_error_rate * 1000);
			if (i == 0) return FAULT;
			else return NO_FAULT;
		}
	}

	return NO_FAULT;

}

int program_error(int pe_cycle){

	int i;

	g_random_no = simple_rand(g_random_no);

	switch (syn_env->syn_pattern){
	case SYN_M_THRESHOLD:
		if (pe_cycle > syn_env->program_threshold){
			i = g_random_no % syn_env->program_error_rate;
			if (i == 0) return FAULT;
			else return NO_FAULT;
		}
		else return NO_FAULT;
	case SYN_M_REAL:
		if (pe_cycle > syn_env->program_threshold){
			i = g_random_no % syn_env->program_error_rate;
			if (i == 0) return FAULT;
			else return NO_FAULT;
		}
		else {
			i = g_random_no % (syn_env->program_error_rate * 1000);
			if (i == 0) return FAULT;
			else return NO_FAULT;
		}
	}

	return NO_FAULT;

}

int read_error(int state){

	int select;

	if (fault_model == PERSISTENT_MODEL){
		if (state == PFM_PROGRAMMED)
			return READ_OK;
		else if (state == PFM_ERASED || state == PFM_ERASED_BAD_A)
			return READ_ALL_FF;
		else if (state == PFM_ERASED_BAD_B){
			g_random_no = simple_rand(g_random_no);
			if (g_random_no % 2 == 0) return READ_OK;
			return READ_ECC_FAIL;
		}
		else return READ_ECC_FAIL;
	}
	else if (fault_model == UNPERSISTENT_MODEL){
		if (state == PFM_ERASED_BAD_B || state == PFM_PROGRAMMED_BAD_B ){
			g_random_no = simple_rand(g_random_no);
			select = g_random_no % 3;
			if (select == 0) return READ_OK;
			else if (select == 1) return READ_ALL_FF;
			else return READ_ECC_FAIL;
		}
		else if (state == PFM_PROGRAMMED)
			return READ_OK;
		else if (state == PFM_ERASED || state == PFM_ERASED_BAD_A )
			return READ_ALL_FF;
		else return READ_ECC_FAIL;
	}
	else printf("Fault model error!\n");
	return FAULT;
}

int get_pe_cycle(int addr){

	int n_channel, n_way, n_plane, n_block, n_page;

	n_channel = addr_to_channel(addr);
	n_way = addr_to_way(addr);
	n_plane = addr_to_plane(addr);
	n_block = addr_to_block(addr);
	n_page = addr_to_page(addr);


	return fm.buses[n_channel].chips[n_way].planes[n_plane].blocks[n_block].pecycle;

}


int get_state(int addr){

	int n_channel, n_way, n_plane, n_block, n_page;

	n_channel = addr_to_channel(addr);
	n_way = addr_to_way(addr);
	n_plane = addr_to_plane(addr);
	n_block = addr_to_block(addr);
	n_page = addr_to_page(addr);

	return fm.buses[n_channel].chips[n_way].planes[n_plane].blocks[n_block].pages[n_page].state;

}

LONGLONG get_variation(){
	if (asyn_env->variation_mode == DIS_UNIFORM){
		return dis_uniform(asyn_env->variation_start, asyn_env->variation_end);
	}
	else if (asyn_env->variation_mode == DIS_GAUSIAN){
		return dis_gausian(asyn_env->variation_mean, asyn_env->variation_stand_dev);
	}
	else if (asyn_env->variation_mode == DIS_USERDEF){
		return dis_user_def(asyn_env->variation_start, asyn_env->variation_end, asyn_env->variation_iter, asyn_env->variation_w_sum, asyn_env->variation_w);
	}
	else{
		printf("Undefined variation mode\n");
		return asyn_env->variation;
	}
}

LONGLONG get_interval(){
	if (asyn_env->interval_mode == DIS_CONSTANT)
		return asyn_env->interval;
	else if (asyn_env->interval_mode == DIS_UNIFORM){
		return dis_uniform(asyn_env->interval_start, asyn_env->interval_end);
	}
	else if (asyn_env->interval_mode == DIS_GAUSIAN){
		return dis_gausian(asyn_env->interval_mean, asyn_env->interval_stand_dev);
	}
	else if (asyn_env->interval_mode == DIS_USERDEF){
		return dis_user_def(asyn_env->interval_start, asyn_env->interval_end, asyn_env->interval_iter, asyn_env->interval_w_sum, asyn_env->interval_w);
	}
	else{
		printf("Undefined interval mode\n");
		return asyn_env->interval;
	}
}

// 확률분포 함수
LONGLONG dis_uniform(LONGLONG start, LONGLONG end){

	LONGLONG ret;

	g_random_no = simple_rand(g_random_no);
	ret = start + g_random_no % (end - start);

	return ret;
}


LONGLONG dis_gausian(LONGLONG mean, LONGLONG stand_dev){
	double v1, v2, s;
	LONGLONG ret;

	do {
		g_random_no = simple_rand(g_random_no);
		v1 = 2 * ((double)g_random_no / 0x7fff) - 1;
		g_random_no = simple_rand(g_random_no);
		v2 = 2 * ((double)g_random_no / 0x7fff) - 1;

		s = v1 * v1 + v2 * v2;
	} while (s >= 1 || s == 0);

	s = mysqrt((unsigned int)((-2 * mylog(s)) / s));

	ret = (LONGLONG) (v1 * s);
	ret = (stand_dev * ret) + mean;

	return ret;
}

LONGLONG dis_user_def(LONGLONG start, LONGLONG end, int iter, int w_sum, int *w){

	int i, select, n;
	LONGLONG chunk_start, chunk_end;

	g_random_no = simple_rand(g_random_no);

	select = g_random_no % w_sum;

	for (i = 0; i < iter; i++){
		if (select < w[i]){
			n = i;
			break;
		}
		else{
			select = select - w[i];
		}
	}
	//for safety
	if (i == iter) n = iter - 1;

	chunk_start = start + n * (start - end) / iter;
	chunk_end = chunk_start + (start - end) / iter;

	return dis_gausian(chunk_start, chunk_end);

}

//확률분포 관련 기본 함수
double mysqrt(unsigned int src)
{
	unsigned int NUM_REPEAT = 16;
	unsigned int k;
	double t;
	double buf = (double)src;
	for (k = 0, t = buf; k<NUM_REPEAT; k++)
	{
		if (t<1.0)
			break;
		t = (t*t + buf) / (2.0*t);
	}
	return t;
}


int mylog(double src){

	int integer = 0;

	if (src > 1){
		printf("error on gausian distribution \n");
		return 0;
	}
	else if (src == 1)
		return 0;

	while (1){
		if (src >= 1)
			break;
		else{
			integer--;
			src = src * NAT_EX;
		}
	}
	return integer;
}
