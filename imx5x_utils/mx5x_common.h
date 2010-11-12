typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;
typedef unsigned int uint;

#define IO_READ(r,o)      (*((volatile uint32_t*)((r)+(o))))
#define IO_WRITE(r,o,d)   (*((volatile uint32_t*)((r)+(o))) = ((uint32_t)(d)))
#define IO_MOD(r,o,m,d)   IO_WRITE((r),(o),(IO_READ((r),(o)) & (~(m))) | (d))

struct app_header {
	uint32_t app_start_addr;
#define APP_BARKER	0xb1
#define DCD_BARKER	0xb17219e9
	uint32_t app_barker;
	uint32_t csf_ptr;
	uint32_t dcd_ptr_ptr;
	uint32_t srk_ptr;
	uint32_t dcd_ptr;
	uint32_t app_dest_ptr;
};

struct common_info {
	void *buf;
	void *initial_buf;
	struct app_header *hdr;
	void *search;
};

void dump_mem(void *buf, uint block, uint block_cnt);

#ifdef DEBUG
#define debug_pr(format, arg...)	my_printf(format,  ##arg)
#define debug_dump(buf, block, block_cnt)	dump_mem(buf, block, block_cnt)
#else
#define debug_pr(format, arg...)		do { } while(0)
#define debug_dump(buf, block, block_cnt)	do { } while(0)
#endif

#ifdef __compiler_offsetof
#define offsetof(TYPE, MEMBER) __compiler_offsetof(TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

void flush_uart(void);
void print_buf(char* str, int cnt);
void print_hex(unsigned int val, unsigned int character_cnt);
void my_printf(char *str, ...);
void my_memset(unsigned char* p, unsigned val, unsigned count);
void my_memcpy(unsigned char* dest, unsigned char* src, unsigned count);
void delayMicro(unsigned cnt);
void reverse_word(unsigned *dst, int count);
void reverse_word2(unsigned *dst, unsigned *src, int count);

int common_load_block_of_file(struct common_info *pinfo, unsigned block_size);
void exec_program(struct common_info *pinfo, uint32_t start_block);
void exec_dl(unsigned char* ubl, unsigned length);
