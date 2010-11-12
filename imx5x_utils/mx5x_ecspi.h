
#define ECSPI1_BASE	0x70010000

#define ATMEL_STATUS_MASK	0x3c
#define AT45DB642D_STATUS_ID	0x3c
#define AT45DB161D_STATUS_ID	0x2c
#define AT45DB081D_STATUS_ID	0x24
#define AT45DB041D_STATUS_ID	0x1c

#define AT45DB642D_P2_OFFSET_BITS 10	//powerof2 bits for offset
//page size
//1056 - 13 page bits, 11 offset bits
//1024 - 1 dc bit, 13 page bits, 10 page offset bits
//add a bit between bits 10 and 11


#define AT45DB161D_P2_OFFSET_BITS 9	//powerof2 bits for offset
//page size
//528 - 2 dc bits, 12 page bits, 10 page offset bits
//512 - 3 dc bits, 12 page bits, 9 page offset
//add a bit between bits 9 and 10
//chip status value - AC means 528 byte/page, protection disabled,
// 1010 1100
//bit 0 - 0 528 byte page size, 1 - 512 byte page size

#define AT45DB081D_P2_OFFSET_BITS 8	//powerof2 bits for offset
#define AT45DB041D_P2_OFFSET_BITS 8	//powerof2 bits for offset
//page size
//264 - 2 dc bits, 12 page bits, 9 page offset bits
//256 - 3 dc bits, 12 page bits, 8 page offset
//add a bit between bits 8 and 9
//chip status value - A4 means 264 byte/page, protection disabled,
// 1010 1100
//bit 0 - 0 264 byte page size, 1 - 256 byte page size
typedef void (*read_block_rtn)(int base, unsigned page, unsigned* dst, unsigned offset_bits, unsigned block_size);
typedef int (*write_blocks_rtn)(int base, unsigned page, unsigned* src, unsigned length, unsigned offset_bits, unsigned block_size);
typedef void (*erase_sector_rtn)(int base, unsigned page, unsigned offset_bits);
void ecspi_init(int base);

unsigned jedec_read_id(int base);
#define JEDEC_ID_ST_M25P16	0x202015	//16 Mbit
#define JEDEC_ID_SST_25VF016B	0xbf2541	//16 Mbit
#define JEDEC_ID_ATMEL_45DB041D	0x1f2400	// 4 Mbit
#define JEDEC_ID_ATMEL_45DB081D	0x1f2500	// 8 Mbit
#define JEDEC_ID_ATMEL_45DB161D	0x1f2600	//16 Mbit
#define JEDEC_ID_ATMEL_45DB642D	0x1f2800	//64 Mbit

unsigned identify_chip_read_rtn(int base, unsigned *pblock_size, read_block_rtn *pread_rtn);
unsigned identify_chip_erase_rtn(int base, unsigned *pblock_size, erase_sector_rtn *perase_rtn);
unsigned identify_chip_rtns(int base, unsigned *pblock_size, read_block_rtn *pread_rtn, write_blocks_rtn *pwrite_rtn);

unsigned atmel_chip_erase(int base);
unsigned atmel_config_p2(int base);
