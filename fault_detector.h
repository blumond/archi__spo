#ifndef __FAULT_DETECTOR_H__
#define __FAULT_DETECTOR_H__

void ascending_order_program_violation(int p_last_programmed_page, int p_programming_page);
void program_after_erase_violation(int p_state);
void nop_violation(int p_nop, int p_block_mode);
void damaged_block_check();
void unreliable_read_violation(int p_state);

#endif
