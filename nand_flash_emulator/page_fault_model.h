#ifndef __PAGE_FAULT_MODEL_H__
#define __PAGE_FAULT_MODEL_H__

//persistent model
enum pfm_states {
	PFM_ERASED, PFM_ERASED_BAD_A, PFM_ERASED_BAD_B,
	PFM_PROGRAMMED, PFM_PROGRAMMED_BAD_A, PFM_PROGRAMMED_BAD_B
};
enum pfm_transition {
	ERASE_OK, ERASE_PF, ERASE_IF,
	PROGRAM_OK, PROGRAM_PF, PROGRAM_IF,
	READ_OK, READ_ALL_FF, READ_ECC_FAIL
};

//error message
#define INVALID_CMD_CHAIN 9

int page_state_transition(int p_cur_state, int p_operation_result);
//같은 입력에도 불구하고 다른 결과가 나올 수 있음
//가능한 transition중 random하게 선택해야함
//sibling page도 같이 transition 시켜야 함

int lsb_page_state_transition(int p_cur_state, int p_operation_result);

#endif
