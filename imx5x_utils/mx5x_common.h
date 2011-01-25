typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;
typedef unsigned int uint;
#define NULL 0
#define IO_READ(r,o)      (*((volatile uint32_t*)((r)+(o))))
#define IO_WRITE(r,o,d)   (*((volatile uint32_t*)((r)+(o))) = ((uint32_t)(d)))
#define IO_MOD(r,o,m,d)   IO_WRITE((r),(o),(IO_READ((r),(o)) & (~(m))) | (d))

#define IO_READ16(r,o)      (*((volatile uint16_t*)((r)+(o))))
#define IO_WRITE16(r,o,d)   (*((volatile uint16_t*)((r)+(o))) = ((uint16_t)(d)))
#define IO_MOD16(r,o,m,d)   IO_WRITE16((r),(o),(IO_READ16((r),(o)) & (~(m))) | (d))

struct common_info {
	void *buf;
	void *initial_buf;
	void *hdr;
	void *search;
	void *cur_end;
	void *end;

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
void print_str(char * str);
void print_buf(char* str, int cnt);
void print_hex(unsigned int val, unsigned int character_cnt);
void my_printf(char *str, ...);
void my_memset(unsigned char* p, unsigned val, unsigned count);
void my_memcpy(unsigned char* dest, unsigned char* src, unsigned count);
void delayMicro(unsigned cnt);
void reverse_word(unsigned *dst, int count);
void reverse_word2(unsigned *dst, unsigned *src, int count);

int common_load_block_of_file(struct common_info *pinfo, unsigned block_size);
int ram_test(unsigned *ram_base);

int get_ecspi_base(void);
uint get_uart_base(void);
uint get_mmc_base(void);
void ecspi_ss_active(void);
void ecspi_ss_inactive(void);
unsigned ecspi_read_miso(void);

void iomuxc_setup_mmc(void);
unsigned mmc_read_cd(void);

void iomuxc_setup_i2c1(void);
void iomuxc_setup_ecspi(void);
void iomuxc_miso_gp(void);
void iomuxc_miso_ecspi(void);
unsigned get_ram_base(void);

void header_search(struct common_info *pinfo);
void header_update_end(struct common_info *pinfo);
int header_present(struct common_info *pinfo);
void exec_program(struct common_info *pinfo, uint32_t start_block);
void exec_dl(unsigned char* ubl, unsigned length);
void hw_watchdog_reset(void);
