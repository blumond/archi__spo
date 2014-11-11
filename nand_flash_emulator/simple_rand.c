#include "simple_rand.h"

unsigned int g_random_no;

int simple_rand(int p_value)
{
	return (p_value * SEED_A + SEED_B);
}
