
#define DEF_LSB_READ 60
#define DEF_MSB_READ 90
#define DEF_LSB_PROGRAM 500
#define DEF_MSB_PROGRAM 2500
#define DEF_ERASE	3000
#define DEF_DATA_TRAN_PER_SECTOR 10


struct nand_timing
{
	long long lsb_read;
	long long msb_read;
	long long lsb_program;
	long long msb_program;
	long long erase;
	long long data_transfer_time_per_sector;
};

struct nand_timing nand_timing_table;

long long get_timing(struct ftl_request p_ftl_req, int p_addr);
void init_timing_model();
