#include <Windows.h>

/* Synchronous Fault					*/

// 폴트 모델
enum fault_modeles{
	PERSISTENT_MODEL, UNPERSISTENT_MODEL
};

enum sync_fault_mode {
	SYN_M_THRESHOLD, SYN_M_REAL
};
	
enum recovery_asso_mode {
	RECO_ASSO_SAFE, RECO_ASSO_NORMAL, RECO_ASSO_PILED_UP
};

#define ERASE_PE_CYCLE_THRESHOLD	100			
#define PROGRAM_PE_CYCLE_THRESHOLD	100			

#define ERASE_ERROR_RATE			1000
#define PROGRAM_ERROR_RATE			1000

enum fault_or_not{
	FAULT, NO_FAULT
};

struct _syn_fault_env
{
	int syn_pattern;
	int recovery_asso_mode;
	int erase_threshold;
	int erase_error_rate;
	int program_threshold;
	int program_error_rate;
} ;

typedef struct _syn_fault_env SYN_FAULT_ENV;

typedef struct _asyn_fault_env
{
	int asyn_pattern;
	int interval_mode;
	LONGLONG interval;
	LONGLONG interval_start;
	LONGLONG interval_end;
	LONGLONG interval_mean;
	LONGLONG interval_stand_dev;
	int interval_iter;
	int interval_w_sum;
	int *interval_w;

	int variation_mode;
	LONGLONG variation;
	LONGLONG variation_start;
	LONGLONG variation_end;
	LONGLONG variation_mean;
	LONGLONG variation_stand_dev;
	int variation_iter;
	int variation_w_sum;
	int *variation_w;
};

typedef struct _asyn_fault_env ASYN_FAULT_ENV;

/* Asynchronous Fault */

#define MAX_ASYNC_FAULT_NUM	50
	

// 리커버리 테스트 모드
enum recovery_test_mode {
	ASYN_M_RANDOM, ASYN_M_POST_RECOVERY, ASYN_M_TEST_RECOVERY
};

enum distributions {
	DIS_CONSTANT, DIS_UNIFORM, DIS_GAUSIAN, DIS_USERDEF
};

// Timer interval values				
#define ASYN_T_UNIT			100					
#define ASYN_T_INTERVAL		(10*ASYN_T_UNIT)
#define ASYN_T_VARIATION	(1*ASYN_T_UNIT)			
#define ASYN_T_INITIAL		(177777*ASYN_T_UNIT)			


// Dynamic fault injection				
#define FAULT_SPARSE_NUMBER	5
#define FAULT_MODE_NUMBER	(100+FAULT_SPARSE_NUMBER)


//확률분포 관련
#define NAT_EX 2.71828

// 전역 변수
int fault_model;
int recovery_point;


// 외부 인터페이스
void init_sync_fault_generator(void);
int init_async_fault_generator(void);

int sync_fault_gen(int cmd, int addr);
int async_fault_gen(void); // Asynchronous fault의 Mode 가 random이 아닌 경우, power fault시 / recovery point 알림 시 호출

int fini_fault_module(void);