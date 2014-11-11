#include "page_fault_model.h"
#include "simple_rand.h"

//persistent model (read operation이 state에 영향을 주지 않는 model)
int page_state_transition(int p_cur_state, int p_operation_result)
{
	int next_state = p_cur_state;
	g_random_no = simple_rand(g_random_no);

	switch (p_cur_state)
	{
	case PFM_ERASED:
		switch (p_operation_result)
		{
		case ERASE_OK:
			next_state = PFM_ERASED;
			break;
		case ERASE_PF:
			next_state = (g_random_no % 2) ? PFM_ERASED_BAD_A : PFM_PROGRAMMED_BAD_A;
			break;
		case ERASE_IF:
			next_state = PFM_PROGRAMMED_BAD_A;
			break;
		case PROGRAM_OK:
			next_state = PFM_PROGRAMMED;
			break;
		case PROGRAM_PF:
			if ((g_random_no % 3) == 1) next_state = PFM_PROGRAMMED;
			else if ((g_random_no % 3) == 2) next_state = PFM_PROGRAMMED_BAD_B;
			else next_state = PFM_ERASED_BAD_B;
			break;
		case PROGRAM_IF:
			if ((g_random_no % 3) == 1) next_state = PFM_PROGRAMMED;
			else if ((g_random_no % 3) == 2) next_state = PFM_PROGRAMMED_BAD_B;
			else next_state = PFM_ERASED_BAD_B;
			break;
		default:
			break;
		}
		break;

	case PFM_ERASED_BAD_A:
		switch (p_operation_result)
		{
		case ERASE_OK:
			next_state = PFM_ERASED;
			break;
		case ERASE_PF:
			next_state = (g_random_no % 2) ? PFM_ERASED_BAD_A : PFM_PROGRAMMED_BAD_A;
			break;
		case ERASE_IF:
			next_state = PFM_PROGRAMMED_BAD_A;
			break;
		default:
			break;
		}
		break;

	case PFM_ERASED_BAD_B:
		switch (p_operation_result)
		{
		case ERASE_OK:
			next_state = PFM_ERASED;
			break;
		case ERASE_PF:
			if ((g_random_no % 3) == 1) next_state = PFM_PROGRAMMED;
			else if ((g_random_no % 3) == 2) next_state = PFM_PROGRAMMED_BAD_B;
			else next_state = PFM_ERASED_BAD_B;
			break;
		case ERASE_IF:
			next_state = (g_random_no % 2) ? PFM_PROGRAMMED : PFM_PROGRAMMED_BAD_B;
			break;
		default:
			break;
		}
		break;

	case PFM_PROGRAMMED:
		switch (p_operation_result)
		{
		case ERASE_OK:
			next_state = PFM_ERASED;
			break;
		case ERASE_PF:
			next_state = (g_random_no % 2) ? PFM_ERASED_BAD_B : PFM_PROGRAMMED_BAD_B;
			break;
		case ERASE_IF:
			next_state = PFM_PROGRAMMED_BAD_B;
			break;
		default:
			break;
		}
		break;

	case PFM_PROGRAMMED_BAD_A:
		switch (p_operation_result)
		{
		case ERASE_OK:
			next_state = PFM_ERASED;
			break;
		case ERASE_PF:
			next_state = (g_random_no % 2) ? PFM_PROGRAMMED_BAD_A : PFM_ERASED_BAD_A;
			break;
		case ERASE_IF:
			next_state = PFM_PROGRAMMED_BAD_A;
			break;
		default:
			break;
		}
		break;

	case PFM_PROGRAMMED_BAD_B:
		switch (p_operation_result)
		{
		case ERASE_OK:
			next_state = PFM_ERASED;
			break;
		case ERASE_PF:
			if ((g_random_no % 3) == 1) next_state = PFM_PROGRAMMED;
			else if ((g_random_no % 3) == 2) next_state = PFM_PROGRAMMED_BAD_B;
			else next_state = PFM_ERASED_BAD_B;
			break;
		case ERASE_IF:
			next_state = (g_random_no % 2) ? PFM_PROGRAMMED : PFM_PROGRAMMED_BAD_B;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return next_state;
}

//LSB page transition
int lsb_page_state_transition(int p_cur_state, int p_operation_result)
{
	int next_state = p_cur_state;

	switch (p_operation_result)
	{
	case PROGRAM_PF:
	case PROGRAM_IF:
		if ((g_random_no % 3) == 1) next_state = PFM_PROGRAMMED;
		else if ((g_random_no % 3) == 2) next_state = PFM_PROGRAMMED_BAD_B;
		else next_state = PFM_ERASED_BAD_B;
	default:
		break;
	}

	return next_state;
}