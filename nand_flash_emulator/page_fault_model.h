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
//���� �Է¿��� �ұ��ϰ� �ٸ� ����� ���� �� ����
//������ transition�� random�ϰ� �����ؾ���
//sibling page�� ���� transition ���Ѿ� ��

int lsb_page_state_transition(int p_cur_state, int p_operation_result);

#endif
