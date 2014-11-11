#ifndef __SIMPLE_RAND_H__
#define __SIMPLE_RAND_H__

#define SEED_A 1103515245
#define SEED_B 12345

extern unsigned int g_random_no;

int simple_rand(int p_value);

#endif