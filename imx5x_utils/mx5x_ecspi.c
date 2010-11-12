/*
 * Currently supports the ATMEL AT45DB642D and ATDB161D chips
 */
#include <stdarg.h>
//#define DEBUG
#include "mx5x_common.h"
#include "mx5x_ecspi.h"

#define ENABLE_ATMEL
#define ENABLE_ST_MICRO
#define ENABLE_SST

#define MAKE_CMD(cmd, page, shift) (((page << shift) & 0x00ffffff) | (cmd << 24))


//#define GPIO1_BASE 0x73f84000
#define GPIO4_BASE 0x73f90000
#define GPIO_DR 0
#define GPIO_DIR 4
#define GPIO_PSR 8


#define ECSPI_RXDATA	0x00
#define ECSPI_TXDATA	0x04
#define ECSPI_CONTROL	0x08
#define ECSPI_CONFIG	0x0c
#define ECSPI_INT	0x10
#define ECSPI_DMA	0x14
#define ECSPI_STATUS	0x18
#define ECSPI_PERIOD	0x1c
#define ECSPI_TEST	0x20
#define ECSPI_MSGDATA	0x40

#define CFG_SCLK_PHASE	0
#define CFG_SCLK_POL	4
#define CFG_SSB_CTL	8
#define CFG_SSB_POL	12
#define CFG_DATA_CTL	16
#define CFG_SCLK_CTL	20
#define CFG_HT_LENGTH	24

#define CTL_SMC		0x08
#define CTL_XCH		0x04
#define PRE_DIV(a) (a << 12)			/* divide clock by (a + 1) */
#define POST_DIV_P2(power2) (power2 << 8)	/* divide clock by (2 ** power2) */
#define CTL_DEFAULT(bits, clock)	((((bits)-1)<<20) | 0x400f1 | clock)		//512 bytes burst max
#define DEFAULT_CLOCK (PRE_DIV(2) | POST_DIV_P2(0))

#define NULL 0

/*
 *
 */
void ecspi_config(int base, unsigned ctl)
{
	IO_WRITE(base, ECSPI_CONTROL, ctl);
	if (!(ctl & 1))
		IO_WRITE(base, ECSPI_CONTROL, ctl | 1);
#define CFG_SCLK_PHA_BIT	0	//0 - Phase 0 operation, 1 - Phase 1 operation
#define CFG_SCLK_POL_BIT	4	//0 - active high(0 = idle), 1 - active low(1 = idle)
#define CFG_SSB_CTL_BIT	8	//0 - single burst, 1 - multiple bursts
#define CFG_SSB_POL_BIT	12	//0 - active low, 1 - active high
#define CFG_DATA_CTL_BIT	16	//0 - high when inactive, 1 - low when inactive
#define CFG_SCLK_CTL_BIT	20	//0 - high when inactive, 1 - low when inactive
#define CFG_HT_LENGTH_BIT	24	//+1 = # of bits in HT message

	IO_WRITE(base, ECSPI_CONFIG, (0x0 << CFG_SCLK_CTL_BIT) | (0x0 << CFG_DATA_CTL_BIT) | (0xf << CFG_SSB_CTL_BIT) | (0x0 << CFG_SCLK_POL_BIT) | (0x0 << CFG_SCLK_PHA_BIT));
	IO_WRITE(base, ECSPI_INT, 0);
	IO_WRITE(base, ECSPI_DMA, ((32 << 0) | ((32-1) << 16)));
	IO_WRITE(base, ECSPI_PERIOD, 0);
#define TEST_CL_BIT 28
	IO_WRITE(base, ECSPI_TEST, 0 << TEST_CL_BIT);
}

#define IOMUXC	0x73fa8000

#define SW_MUX_CTL_PAD_CSPI1_MOSI	0x210	//gpio4[22], alt 3
#define SW_MUX_CTL_PAD_CSPI1_MISO	0x214	//gpio4[23], alt 3
#define SW_MUX_CTL_PAD_CSPI1_SS0	0x218	//gpio4[24], alt 3
#define SW_MUX_CTL_PAD_CSPI1_SS1	0x21c	//gpio4[25], alt 3
#define SW_MUX_CTL_PAD_CSPI1_RDY	0x220	//gpio4[26], alt 3
#define SW_MUX_CTL_PAD_CSPI1_SCLK	0x224	//gpio4[27], alt 3


#define ALT0	0x0
#define ALT1	0x1
#define ALT3	0x3
#define SION	0x10	//0x10 means force input path of BGA contact

#define MUX_SS0			ALT3
#define MUX_SS1			ALT3

#define SW_PAD_CTL_CSPI1_MOSI		0x600
#define SW_PAD_CTL_CSPI1_MISO		0x604
#define SW_PAD_CTL_CSPI1_SS0		0x608
#define SW_PAD_CTL_CSPI1_SS1		0x60c
#define SW_PAD_CTL_CSPI1_RDY		0x610
#define SW_PAD_CTL_CSPI1_SCLK		0x614

#define SRE_FAST	(0x1 << 0)
#define DSE_LOW		(0x0 << 1)
#define DSE_MEDIUM	(0x1 << 1)
#define DSE_HIGH	(0x2 << 1)
#define DSE_MAX		(0x3 << 1)
#define OPENDRAIN_ENABLE (0x1 << 3)
#define R100K_PD	(0x0 << 4)
#define R47K_PU		(0x1 << 4)
#define R100K_PU	(0x2 << 4)
#define R22K_PU		(0x3 << 4)
#define KEEPER		(0x0 << 6)
#define PULL		(0x1 << 6)
#define PKE_ENABLE	(0x1 << 7)
#define HYS_ENABLE	(0x1 << 8)
#define VOT_LOW		(0x0 << 13)
#define VOT_HIGH	(0x1 << 13)

#define PAD_CSPI1_MOSI	(HYS_ENABLE | PKE_ENABLE | KEEPER | R100K_PU | DSE_HIGH | SRE_FAST)
#define PAD_CSPI1_MISO	(HYS_ENABLE | PKE_ENABLE | KEEPER | DSE_HIGH | SRE_FAST)
#define PAD_CSPI1_SS0	(HYS_ENABLE | PKE_ENABLE | KEEPER | DSE_HIGH | SRE_FAST)
#define PAD_CSPI1_SS1	(HYS_ENABLE | PKE_ENABLE | KEEPER | DSE_HIGH | SRE_FAST)
#define PAD_CSPI1_RDY	(HYS_ENABLE | PKE_ENABLE | KEEPER | DSE_LOW | SRE_FAST)
#define PAD_CSPI1_SCLK	(HYS_ENABLE | PKE_ENABLE | KEEPER | R100K_PU | DSE_HIGH | SRE_FAST)

#define GP4_MISO_BIT	23
#define GP4_SS0_BIT	24
#define GP4_SS1_BIT	25

void ecspi_init(int base)
{
	uint mux_base = IOMUXC;
	uint gpio_base = GPIO4_BASE;

	debug_pr("%s\n", __func__);
	IO_WRITE(mux_base, SW_PAD_CTL_CSPI1_MOSI, PAD_CSPI1_MOSI);
	IO_WRITE(mux_base, SW_PAD_CTL_CSPI1_MISO, PAD_CSPI1_MISO);
	IO_WRITE(mux_base, SW_PAD_CTL_CSPI1_SS0, PAD_CSPI1_SS0);
	IO_WRITE(mux_base, SW_PAD_CTL_CSPI1_SS1, PAD_CSPI1_SS1);
	IO_WRITE(mux_base, SW_PAD_CTL_CSPI1_SCLK, PAD_CSPI1_SCLK);

	IO_WRITE(mux_base, SW_MUX_CTL_PAD_CSPI1_MOSI, 0);
	IO_WRITE(mux_base, SW_MUX_CTL_PAD_CSPI1_MISO, 0);
	IO_WRITE(mux_base, SW_MUX_CTL_PAD_CSPI1_SS0, MUX_SS0);
	IO_WRITE(mux_base, SW_MUX_CTL_PAD_CSPI1_SS1, MUX_SS1);
	IO_WRITE(mux_base, SW_MUX_CTL_PAD_CSPI1_SCLK, 0);

// make sure SS0 is low(pmic), SS1 is high
	IO_MOD(gpio_base, GPIO_DR, (1 << GP4_SS0_BIT), (1 << GP4_SS1_BIT));
// make sure MISO is input, SS0/SS1 are output
	IO_MOD(gpio_base, GPIO_DIR, (1 << GP4_MISO_BIT), (1 << GP4_SS0_BIT) | (1 << GP4_SS1_BIT));
//	IO_WRITE(base, ECSPI_CONTROL, CTL_DEFAULT(32, DEFAULT_CLOCK) & ~1);
	ecspi_config(base, CTL_DEFAULT(32, DEFAULT_CLOCK) & ~1);
}

unsigned ecspi_cmd(int base, unsigned cmd, unsigned ctl)
{
	unsigned status;
	unsigned data = 0xffffffff;
	uint gpio_base = GPIO4_BASE;

//	debug_pr("%s\n", __func__);
//	debug_pr("cmd=%x ctl=%x\n", cmd, ctl);

	ecspi_config(base, ctl);
	IO_WRITE(base, ECSPI_TXDATA, cmd);

	IO_MOD(gpio_base, GPIO_DR, (1 << GP4_SS1_BIT), 0);	//make SS1 low
	IO_WRITE(base, ECSPI_CONTROL, ctl + CTL_SMC);
	do {
		status = IO_READ(base, ECSPI_STATUS);
		debug_pr("status=%x cmd=%x ctl=%x\n", status, cmd, ctl);
		if (status & (1<<3)) {	//rx fifo has at least 1 word?
			data = IO_READ(base, ECSPI_RXDATA);
			debug_pr("data = %x\n", data);
		}
	} while (!(status & (1<<7)));	//wait for transfer complete

	IO_MOD(gpio_base, GPIO_DR, 0, (1 << GP4_SS1_BIT));	//make SS1 high
	IO_WRITE(base, ECSPI_STATUS, (1<<7));
	return data;
}

void ecspi_read(int base, unsigned* dst, unsigned length, unsigned cmd, unsigned cmd_bits, unsigned rx_drop, unsigned clock)
{
	unsigned i = 0;
	unsigned ctrl, ctrl_start;
	unsigned tx_cnt, outstanding, max;
	unsigned status;
	unsigned loop_count = 0;
	unsigned bits;
	uint gpio_base = GPIO4_BASE;

//	debug_pr("%s length=%x\n", __func__, length);
	length >>= 2;
	tx_cnt = length + rx_drop;	//words left to write
	bits = (tx_cnt << 5) + cmd_bits;
	if (cmd_bits)
		bits -= 32;	//already included in total

	max = 64;		//64 - 32 bit entries per fifo
	if (max > tx_cnt)
		max = tx_cnt;
	if (bits < 512*8) {
		ctrl = CTL_DEFAULT(bits, clock);
		ctrl_start = CTL_SMC;
	} else {
		ctrl = CTL_DEFAULT(512*8, clock);	//!!!! cmd_bits must = 32/0 for this to work
		ctrl_start = CTL_XCH;
	}
	ecspi_config(base, ctrl);
	if (cmd_bits) {
		IO_WRITE(base, ECSPI_TXDATA, cmd);
		i++;
	}
	for (; i < max; i++)
		IO_WRITE(base, ECSPI_TXDATA, 0);
	tx_cnt -= max;
	outstanding = max;
//fifo is full, start device

	IO_MOD(gpio_base, GPIO_DR, (1 << GP4_SS1_BIT), 0);	//make SS1 low
	IO_WRITE(base, ECSPI_CONTROL, ctrl | ctrl_start);
	do {
		status = IO_READ(base, ECSPI_STATUS);
		if (rx_drop) {
			if (status & (1<<3)) {	//rx fifo has at least 1 word?
				IO_READ(base, ECSPI_RXDATA);	//throw away data
				rx_drop--;
				outstanding--;
			}
		} else if (outstanding >= 32) {
			if (status & (1<<4)) {	//rx half full ???
				int i;
				for (i = 0; i < (32/8); i++) {
					dst[0] = IO_READ(base, ECSPI_RXDATA);
					dst[1] = IO_READ(base, ECSPI_RXDATA);
					dst[2] = IO_READ(base, ECSPI_RXDATA);
					dst[3] = IO_READ(base, ECSPI_RXDATA);
					dst[4] = IO_READ(base, ECSPI_RXDATA);
					dst[5] = IO_READ(base, ECSPI_RXDATA);
					dst[6] = IO_READ(base, ECSPI_RXDATA);
					dst[7] = IO_READ(base, ECSPI_RXDATA);
					dst += 8;
				}
				outstanding -= 32;
			} else if (status & (1 << 7))
				goto single;
		} else {
single:
			/* Not expecting another 1/2 full fifo */
			if (status & (1<<3)) {	//rx fifo has at least 1 word?
				unsigned a = IO_READ(base, ECSPI_RXDATA);
				if (outstanding) {
					*dst++ = a;
					outstanding--;
				} else
					debug_pr("too much data received\n");
			}
		}

		if ((tx_cnt >= 32) && (outstanding <= 32)) {
			if (status & (1<<1)) {	//tx half empty ???
				int i;
				for (i = 0; i < (32/8); i++) {
					IO_WRITE(base, ECSPI_TXDATA, 0);
					IO_WRITE(base, ECSPI_TXDATA, 0);
					IO_WRITE(base, ECSPI_TXDATA, 0);
					IO_WRITE(base, ECSPI_TXDATA, 0);
					IO_WRITE(base, ECSPI_TXDATA, 0);
					IO_WRITE(base, ECSPI_TXDATA, 0);
					IO_WRITE(base, ECSPI_TXDATA, 0);
					IO_WRITE(base, ECSPI_TXDATA, 0);
				}
				tx_cnt -= 32;
				outstanding += 32;
				continue;
			}
		} else if (tx_cnt && (outstanding < 64)) {
			if (!(status & (1<<2))) {	//tx fifo NOT full
				IO_WRITE(base, ECSPI_TXDATA, 0);
				tx_cnt--;
				outstanding++;
				continue;
			}
		}
		if (status & (1<<7)) {	//transfer complete?
			if (outstanding || tx_cnt) {
				if (!(status & (1<<3))) {
					debug_pr("tc - but more data required, status=%x tx_cnt=%x outstanding=%x ctrl=%x cs=%x test=%x\n",
						status, tx_cnt, outstanding, IO_READ(base, ECSPI_CONTROL), ctrl_start, IO_READ(base, ECSPI_TEST));
					if (tx_cnt < ((512*8) >> 5)) {
						ctrl = CTL_DEFAULT(tx_cnt << 5, clock);
						ctrl_start = CTL_SMC;
					} else {
						ctrl = CTL_DEFAULT(512*8, clock);
						ctrl_start = CTL_XCH;
					}
					outstanding = 0;
					IO_WRITE(base, ECSPI_STATUS, (1<<7));
					IO_WRITE(base, ECSPI_CONTROL, ctrl | ctrl_start);
				}
			} else {
				debug_pr("tc\n");
				break;
			}
		}
		if (!(outstanding + tx_cnt)) {
			break;
		}
		loop_count++;
		if (!(loop_count & 0xfff)) {
			debug_pr("tx_cnt=%x outstanding=%x %2x\n", tx_cnt, outstanding, status);
		}
	} while (1);
	IO_MOD(gpio_base, GPIO_DR, 0, (1 << GP4_SS1_BIT));	//make SS1 high
	IO_WRITE(base, ECSPI_STATUS, (1<<7));
//	IO_WRITE(base, ECSPI_CONTROL, ctrl & ~1);
	ecspi_config(base, ctrl & ~1);
}

void ecspi_write(int base, unsigned* src, unsigned length, unsigned cmd, unsigned cmd_bits, unsigned clock)
{
	unsigned i = 0;
	unsigned ctrl, ctrl_start;
	unsigned tx_cnt, outstanding, max;
	unsigned status;
	unsigned loop_count = 0;
	unsigned bits;
	uint gpio_base = GPIO4_BASE;

//	debug_pr("%s length=%x\n", __func__, length);
	length >>= 2;
	tx_cnt = length;	//words left to write
	if (cmd_bits)
		tx_cnt++;
	max = 64;		//64 - 32 bit entries per fifo
	if (max > tx_cnt)
		max = tx_cnt;
	bits = (length << 5) + cmd_bits;
	if (bits < (512*8)) {
		ctrl = CTL_DEFAULT(bits, clock);
		ctrl_start = CTL_SMC;
	} else {
		ctrl = CTL_DEFAULT(512*8, clock);	//!!!! cmd_bits must = 32 for this to work
		ctrl_start = CTL_XCH;
	}
	ecspi_config(base, ctrl);
	if (cmd_bits) {
		IO_WRITE(base, ECSPI_TXDATA, cmd);
		i++;
	}
	for (; i < max; i++)
		IO_WRITE(base, ECSPI_TXDATA, *src++);
	tx_cnt -= max;
	outstanding = max;
//fifo is full, start device
	IO_MOD(gpio_base, GPIO_DR, (1 << GP4_SS1_BIT), 0);	//make SS1 low
	IO_WRITE(base, ECSPI_CONTROL, ctrl | ctrl_start);
	do {
		status = IO_READ(base, ECSPI_STATUS);
		if (outstanding >= 32) {
			if (status & (1<<4)) {	//rx half full ???
				int i;
				for (i = 0; i < (32/8); i++) {
					IO_READ(base, ECSPI_RXDATA);
					IO_READ(base, ECSPI_RXDATA);
					IO_READ(base, ECSPI_RXDATA);
					IO_READ(base, ECSPI_RXDATA);
					IO_READ(base, ECSPI_RXDATA);
					IO_READ(base, ECSPI_RXDATA);
					IO_READ(base, ECSPI_RXDATA);
					IO_READ(base, ECSPI_RXDATA);
				}
				outstanding -= 32;
			} else if (status & (1 << 7))
				goto single;
		} else {
single:
			if (status & (1<<3)) {	//rx fifo has at least 1 word?
				IO_READ(base, ECSPI_RXDATA);	//throw away data
				if (outstanding)
					outstanding--;
				else
					debug_pr("too much data received\n");
			}
		}

		if ((tx_cnt >= 32) && (outstanding <= 32)) {
			if (status & (1<<1)) {	//tx half empty ???
				int i;
				for (i = 0; i < (32/8); i++) {
					IO_WRITE(base, ECSPI_TXDATA, *src++);
					IO_WRITE(base, ECSPI_TXDATA, *src++);
					IO_WRITE(base, ECSPI_TXDATA, *src++);
					IO_WRITE(base, ECSPI_TXDATA, *src++);
					IO_WRITE(base, ECSPI_TXDATA, *src++);
					IO_WRITE(base, ECSPI_TXDATA, *src++);
					IO_WRITE(base, ECSPI_TXDATA, *src++);
					IO_WRITE(base, ECSPI_TXDATA, *src++);
				}
				tx_cnt -= 32;
				outstanding += 32;
				continue;
			}
		} else if (tx_cnt && (outstanding < 64)) {
			if (!(status & (1<<2))) {	//tx fifo NOT full
				IO_WRITE(base, ECSPI_TXDATA, *src++);
				tx_cnt--;
				outstanding++;
				continue;
			}
		}
		if (status & (1<<7)) {	//transfer complete?
			if (tx_cnt || outstanding) {
				if (!(status & (1<<3))) {
					debug_pr("tc - but more data required, status=%x tx_cnt=%x outstanding=%x ctrl=%x cs=%x\n",
						status, tx_cnt, outstanding, IO_READ(base, ECSPI_CONTROL), ctrl_start);
					break;
				}
			} else {
				debug_pr("tc\n");
				break;
			}
		}
		if (!(outstanding + tx_cnt)) {
			if (ctrl_start == CTL_XCH) {
				/* a large block write will not end */
				ecspi_config(base, ctrl & ~1);
				break;
			}
		}
		loop_count++;
		if (!(loop_count & 0xfff)) {
			debug_pr("tx_cnt=%x outstanding=%x %2x\n", tx_cnt, outstanding, status);
		}
	} while (1);
	IO_WRITE(base, ECSPI_STATUS, (1<<7));
//	IO_WRITE(base, ECSPI_CONTROL, ctrl & ~1);
}

#define JEDEC_ID		0x9f	//1 opcode byte in, 0x20,0x20,0x15 out ST_MICRO
#define JEDEC_CLOCK	(PRE_DIV(7) | POST_DIV_P2(0))

unsigned jedec_read_id(int base)
{
#if 1
	unsigned id = ecspi_cmd(base, JEDEC_ID << 24, CTL_DEFAULT(32, JEDEC_CLOCK));
#else
	unsigned id;
	//	   base, dest, len, cmd,       cmd_bits,  rx_drop, unsigned clock
	ecspi_read(base, &id, 4, JEDEC_ID << 24, 32,      0, JEDEC_CLOCK);
#endif
	id &= 0x00ffffff;
	debug_pr("%s: id %08x\n", __func__, id);
	return id;
}
static unsigned atmel_status(int base);

static unsigned ident_chip_rtns(int base, unsigned *pblock_size, unsigned *ptype)
{
	unsigned offset_bits = 0;
	unsigned block_size;
	unsigned chip_status = jedec_read_id(base);
	unsigned type;
	switch (chip_status) {
#ifdef ENABLE_ST_MICRO
	case JEDEC_ID_ST_M25P16:
		type = 0;
		offset_bits = 8;
		block_size = (1 << offset_bits);
		break;
#endif
#ifdef ENABLE_SST
	case JEDEC_ID_SST_25VF016B:
		type = 1;
		offset_bits = 8;
		block_size = (1 << offset_bits);
		break;
#endif
#ifdef ENABLE_ATMEL
	case JEDEC_ID_ATMEL_45DB041D:
		offset_bits = AT45DB041D_P2_OFFSET_BITS;
		goto join_atmel;
	case JEDEC_ID_ATMEL_45DB081D:
		offset_bits = AT45DB081D_P2_OFFSET_BITS;
		goto join_atmel;
	case JEDEC_ID_ATMEL_45DB161D:
		offset_bits = AT45DB161D_P2_OFFSET_BITS;
		goto join_atmel;
	case JEDEC_ID_ATMEL_45DB642D:
		offset_bits = AT45DB642D_P2_OFFSET_BITS;
join_atmel:
		chip_status = atmel_status(base);
		if (chip_status & 1) {
			block_size = (1 << offset_bits);
		} else {
			block_size = ((1 << offset_bits) + (1 << (offset_bits - 5)));
			offset_bits++;
		}
		type = 2;
		break;
#endif
	default:
		my_printf("!!!error id=%x\n", chip_status);
		return 0;
	}
	if (pblock_size)
		*pblock_size = block_size;
	if (ptype)
		*ptype = type;
	return offset_bits;
}

static void st_read_block(int base, unsigned page, unsigned* dst, unsigned offset_bits, unsigned block_size);
static int st_write_blocks(int base, unsigned page, unsigned* src, unsigned length, unsigned offset_bits, unsigned block_size);
static void st_erase_sector(int base, unsigned page, unsigned offset_bits);

static void sst_read_block(int base, unsigned page, unsigned* dst, unsigned offset_bits, unsigned block_size);
static int sst_write_blocks(int base, unsigned page, unsigned* src, unsigned length, unsigned offset_bits, unsigned block_size);
static void sst_erase_sector(int base, unsigned page, unsigned offset_bits);

static void atmel_read_block(int base, unsigned page, unsigned* dst, unsigned offset_bits, unsigned block_size);
static int atmel_write_blocks(int base, unsigned page, unsigned* src, unsigned length, unsigned offset_bits, unsigned block_size);
static void atmel_erase_sector(int base, unsigned page, unsigned offset_bits);

#ifdef ENABLE_ST_MICRO
#define ST_READ_BLOCK st_read_block
#define ST_WRITE_BLOCKS st_write_blocks
#define ST_ERASE_SECTOR st_erase_sector
#else
#define ST_READ_BLOCK NULL
#define ST_WRITE_BLOCKS NULL
#define ST_ERASE_SECTOR NULL
#endif

#ifdef ENABLE_SST
#define SST_READ_BLOCK sst_read_block
#define SST_WRITE_BLOCKS sst_write_blocks
#define SST_ERASE_SECTOR sst_erase_sector
#else
#define SST_READ_BLOCK NULL
#define SST_WRITE_BLOCKS NULL
#define SST_ERASE_SECTOR NULL
#endif

#ifdef ENABLE_ATMEL
#define ATMEL_READ_BLOCK atmel_read_block
#define ATMEL_WRITE_BLOCKS atmel_write_blocks
#define ATMEL_ERASE_SECTOR atmel_erase_sector
#else
#define ATMEL_READ_BLOCK NULL
#define ATMEL_WRITE_BLOCKS NULL
#define ATMEL_ERASE_SECTOR NULL
#endif

static const read_block_rtn read_rtns[] = {ST_READ_BLOCK, SST_READ_BLOCK, ATMEL_READ_BLOCK};
static const write_blocks_rtn write_rtns[] = {ST_WRITE_BLOCKS, SST_WRITE_BLOCKS, ATMEL_WRITE_BLOCKS};
static const erase_sector_rtn erase_rtns[] = {ST_ERASE_SECTOR, SST_ERASE_SECTOR, ATMEL_ERASE_SECTOR};

unsigned identify_chip_read_rtn(int base, unsigned *pblock_size, read_block_rtn *pread_rtn)
{
	unsigned type;
	unsigned offset_bits = ident_chip_rtns(base, pblock_size, &type);
	if (offset_bits) {
		if (pread_rtn)
			*pread_rtn = read_rtns[type];
	}
	return offset_bits;
}

unsigned identify_chip_erase_rtn(int base, unsigned *pblock_size, erase_sector_rtn *perase_rtn)
{
	unsigned type;
	unsigned offset_bits = ident_chip_rtns(base, pblock_size, &type);
	if (offset_bits) {
		if (perase_rtn)
			*perase_rtn = erase_rtns[type];
	}
	return offset_bits;
}

unsigned identify_chip_rtns(int base, unsigned *pblock_size, read_block_rtn *pread_rtn, write_blocks_rtn *pwrite_rtn)
{
	unsigned type;
	unsigned offset_bits = ident_chip_rtns(base, pblock_size, &type);
	if (offset_bits) {
		if (pread_rtn)
			*pread_rtn = read_rtns[type];
		if (pwrite_rtn)
			*pwrite_rtn = write_rtns[type];
	}
	return offset_bits;
}

#ifdef ENABLE_ATMEL
/*
 * These are ATMEL specific defines
 */
#define SECTOR_ERASE 1
//#define BLOCK_ERASE 1

#define SF_BLOCK_ERASE		0x50
#define SF_SECTOR_ERASE		0x7c
#define SF_READ			0xd2
#define SF_STATUS		0xd7
//status d7
//bit 7 - 0 busy, 1 idle
//bit 6 - 0 last compare matched, 1 - last compare didn't match
//bit 1 - 0 protection disabled, 1 - protection enabled
//bit 0 - 0 extra bytes, 1 - power of 2 page size
#define SF_BUFFER1_WRITE	0x84
#define SF_BUFFER2_WRITE	0x87
#define SF_BUFFER1_PROGRAM	0x88
#define SF_BUFFER2_PROGRAM	0x89
#define SF_CHIP_ERASE		0xc794809a
#define SF_CONFIG_P2		0x3d2a80a6

#define MASK_STATUS_ID		0x3c

#define SECTOR0a_page		0
#define SECTOR0b_page		8
#define SECTOR1_page		256
#define NORMAL_SECTOR_page_cnt	256

#define ATMEL_STATUS_CLOCK	(PRE_DIV(7) | POST_DIV_P2(0))
#define ATMEL_CLOCK		(PRE_DIV(1) | POST_DIV_P2(0))

static unsigned atmel_status(int base)
{
	unsigned chip_status; 
//	debug_pr("%s\n", __func__);
	chip_status = ecspi_cmd(base, SF_STATUS << 8, CTL_DEFAULT(16, ATMEL_STATUS_CLOCK));
//	debug_pr("chip_status = %x\n", chip_status);
	return chip_status;
}

static void atmel_wait_for_ready(int base)
{
	unsigned status;
	debug_pr("%s\n", __func__);
	do {
		delayMicro(1000);	//1 ms
		status = atmel_status(base);
	} while (!(status & 0x80));
}

static void atmel_read_block(int base, unsigned page, unsigned* dst, unsigned offset_bits, unsigned block_size)
{
	unsigned cmd = MAKE_CMD(SF_READ, page, offset_bits);	//continuous array read, throw away 1st 8 bytes from read fifo
	ecspi_read(base, dst, block_size, cmd, 32, 2, ATMEL_CLOCK);
	debug_pr("%s: block_size=0x%x page=%x, *dst(%x)=%x\n", __func__, block_size, page, dst, *dst);
}

unsigned atmel_config_p2(int base)
{
	ecspi_cmd(base, SF_CONFIG_P2, CTL_DEFAULT(32, ATMEL_STATUS_CLOCK));
	atmel_wait_for_ready(base);
	return 0;
}

/*
 * Sector 0a = 8 pages,   start 0x000000
 * Sector 0b = 248 pages, start 0x002000
 * Sector 1 = 256 pages,  start 0x040000
 * Sector 2 = 256 pages,  start 0x080000
 * Sector 3 = 256 pages,  start 0x0c0000
 * Sector 4 = 256 pages,  start 0x100000
 * Sector 5 = 256 pages,  start 0x140000
 * Sector 6 = 256 pages,  start 0x180000
 * Sector 7 = 256 pages,  start 0x1c0000
 * Sector 15 = 256 pages, start 0x3c0000
 * Sector 31 = 256 pages, start 0x7c0000
 */
static void atmel_erase_sector(int base, unsigned page, unsigned offset_bits)
{
	unsigned cmd = MAKE_CMD(SF_SECTOR_ERASE, page, offset_bits);
	debug_pr("%s page=0x%x, cmd=%x\n", __func__, page, cmd);
	ecspi_cmd(base, cmd, CTL_DEFAULT(32, ATMEL_CLOCK));
//	delayMicro(5000000);	//(5 seconds max sector erase time)
	atmel_wait_for_ready(base);
}

static void atmel_erase_block(int base, unsigned page, unsigned offset_bits)
{
	unsigned cmd = MAKE_CMD(SF_BLOCK_ERASE, page, offset_bits);
	debug_pr("%s page=0x%x, cmd=%x\n", __func__, page, cmd);
	ecspi_cmd(base, cmd, CTL_DEFAULT(32, ATMEL_CLOCK));
//	delayMicro(100000);	//0.10 seconds max
	atmel_wait_for_ready(base);
}


unsigned atmel_chip_erase(int base)
{
	unsigned chip_erase; 
//	debug_pr("%s\n", __func__);
	chip_erase = ecspi_cmd(base, SF_CHIP_ERASE, CTL_DEFAULT(32, ATMEL_CLOCK));
	atmel_wait_for_ready(base);
	debug_pr("chip_erase = %x\n", chip_erase);
	return chip_erase;
}

static void atmel_write_block(int base, unsigned page, unsigned* src, unsigned offset_bits, unsigned block_size)
{
	unsigned cmd = MAKE_CMD(SF_BUFFER1_PROGRAM, page, offset_bits); //program buffer
	uint gpio_base = GPIO4_BASE;
	debug_pr("%s: block_size=0x%x page=%x, *src(%x)=%x\n", __func__, block_size, page, src, *src);
	ecspi_write(base, src, block_size, (SF_BUFFER1_WRITE << 24), 32, ATMEL_CLOCK);
	IO_MOD(gpio_base, GPIO_DR, 0, (1 << GP4_SS1_BIT));	//make SS1 high
	ecspi_cmd(base, cmd, CTL_DEFAULT(32, ATMEL_CLOCK));
//	delayMicro(6000);	//6 ms max
	atmel_wait_for_ready(base);
}

static void atmel_erase_sector_range(int base, unsigned epage, unsigned endpage, unsigned offset_bits)
{
	debug_pr("%s: epage=0x%x endpage=0x%x\n", __func__, epage, endpage);

	while (epage < endpage) {
		my_printf("erasing sector containing page %x\n", epage);
		atmel_erase_sector(base, epage, offset_bits);

		if (epage < SECTOR0b_page)
			epage = SECTOR0b_page;
		else if (epage < SECTOR1_page)
			epage = SECTOR1_page;
		else
			epage = (epage | (NORMAL_SECTOR_page_cnt - 1)) + 1;
	}
}

static void atmel_erase_block_range(int base, unsigned epage, unsigned endpage, unsigned offset_bits)
{
	debug_pr("%s: epage=0x%x endpage=0x%x\n", __func__, epage, endpage);

	while (epage < endpage) {
		atmel_erase_block(base, epage, offset_bits);
		epage = (epage | 7) + 1;
	}
}

static int atmel_write_blocks(int base, unsigned page, unsigned* src, unsigned length, unsigned offset_bits, unsigned block_size)
{
	unsigned page_cnt;
	if (!length)
		return 0;
	page_cnt = ((length + block_size -1) / block_size);
#ifdef SECTOR_ERASE
	atmel_erase_sector_range(base, page, page + page_cnt, offset_bits);
#endif
#ifdef BLOCK_ERASE
	atmel_erase_block_range(base, page, page + page_cnt, offset_bits);
#endif
#if 1
	for (;;) {
		atmel_write_block(base, page, src, offset_bits, block_size);
		src += (block_size / 4);
		page++;
		if (length <= block_size)
			break;
		length -= block_size;
		my_printf(".");
	}
	my_printf("\r\n");
#endif
	return 0;
}
#endif //ENABLE_ATMEL

/*
 ***************************************************************************
 */

#ifdef ENABLE_ST_MICRO
/*
 * These are ST micro specific defines
 * Latch data on rising edge of clock, drive data starts on falling edge of clock
 * slow read has max of 33 MHz
 * ST_FAST_READ has max of 75 MHz
 */
#define ST_WRITE_ENABLE		0x06	//1 opcode byte only
#define ST_WRITE_DISABLE	0x04	//1 opcode byte only
#define ST_READ_ID		0x9f	//1 opcode byte in, 0x20,0x20,0x15 out
#define ST_READ_STATUS		0x05	//1 opcode byte in, repeated status out
#define ST_WRITE_STATUS		0x01	//2 - opcode,data bytes in
#define ST_SLOW_READ		0x03	//4 - 1 opcode, 3 address bytes in, infinite out (33 MHZ max)
#define ST_FAST_READ		0x0b	//5 - 1 opcode, 3 address, 1 dummy bytes in, infinite out (75 MHz max)
#define ST_PAGE_PROGRAM		0x02	//260 - 1 opcode, 3 address, 256 data bytes in
#define ST_SECTOR_ERASE		0xd8	//4 - 1 opcode, 3 address
#define ST_BULK_ERASE		0xc7	//1 opcode only
#define ST_DEEP_POWER_DOWN	0xb9	//1 opcode only
#define ST_RESUME		0xab	//4 - 1 opcode, 3 dummy bytes

#define ST_SLOW_CLOCK		(PRE_DIV(2) | POST_DIV_P2(0))
#define ST_CLOCK		(PRE_DIV(1) | POST_DIV_P2(0))
/*
 *
 */

static void st_read_block(int base, unsigned page, unsigned* dst, unsigned offset_bits, unsigned block_size)
{
#if 1
	//5 - 1 opcode, 3 address, 1 dummy bytes in, infinite out
	unsigned cmd2 = (page << (offset_bits + 8)); 		// +8 for dummy byte
	debug_pr("%s: cmd2=%x page=%x\n", __func__, cmd2, page);
	ecspi_write(base, &cmd2, 4, ST_FAST_READ, 8, ST_CLOCK);
	ecspi_read(base, dst, block_size, 0, 0, 0, ST_CLOCK);
#else
	unsigned cmd = MAKE_CMD(ST_SLOW_READ, page, offset_bits);
	ecspi_read(base, dst, block_size, cmd, 32, 1, ST_SLOW_CLOCK);
#endif
	debug_pr("%s: block_size=0x%x page=%x, *dst(%x)=%x\n", __func__, block_size, page, dst, *dst);
}

static unsigned st_status(int base)
{
	unsigned chip_status; 
	debug_pr("%s\n", __func__);
	chip_status = ecspi_cmd(base, ST_READ_STATUS << 8, CTL_DEFAULT(16, ST_SLOW_CLOCK));
	debug_pr("chip_status = %x\n", chip_status);
	return chip_status;
}

static void st_wait_for_ready(int base)
{
	unsigned status;
	debug_pr("%s\n", __func__);
	do {
		delayMicro(10);
		status = st_status(base);
	} while (status & 1);
}

static void st_erase_sector(int base, unsigned page, unsigned offset_bits)
{
	unsigned cmd = MAKE_CMD(ST_SECTOR_ERASE, page, offset_bits);
	debug_pr("%s page=0x%x, cmd=%x\n", __func__, page, cmd);
	ecspi_cmd(base, ST_WRITE_ENABLE, CTL_DEFAULT(8, ST_CLOCK));
	ecspi_cmd(base, cmd, CTL_DEFAULT(32, ST_CLOCK));
	st_wait_for_ready(base);
}

static void st_erase_sector_range(int base, unsigned epage, unsigned endpage, unsigned offset_bits)
{
	debug_pr("%s: epage=0x%x endpage=0x%x\n", __func__, epage, endpage);

	while (epage < endpage) {
		//a sector is 512Kbit = 64K bytes = 256 pages * 256 bytes /page
		my_printf("erasing sector containing page %x\n", epage);
		st_erase_sector(base, epage, offset_bits);
		epage = (epage | (256 - 1)) + 1;
	}
}

static void st_write_block(int base, unsigned page, unsigned* src, unsigned offset_bits, unsigned block_size)
{
	unsigned cmd = MAKE_CMD(ST_PAGE_PROGRAM, page, offset_bits); //program buffer
	uint gpio_base = GPIO4_BASE;
	debug_pr("%s: block_size=0x%x page=%x, *src(%x)=%x\n", __func__, block_size, page, src, *src);
	ecspi_cmd(base, ST_WRITE_ENABLE, CTL_DEFAULT(8, ST_CLOCK));
	ecspi_write(base, src, block_size, cmd, 32, ST_CLOCK);
	IO_MOD(gpio_base, GPIO_DR, 0, (1 << GP4_SS1_BIT));	//make SS1 high
	st_wait_for_ready(base);
}

static int st_write_blocks(int base, unsigned page, unsigned* src, unsigned length, unsigned offset_bits, unsigned block_size)
{
	unsigned page_cnt;
	if (!length)
		return 0;
	page_cnt = ((length + block_size -1) / block_size);
	st_erase_sector_range(base, page, page + page_cnt, offset_bits);
#if 1
	for (;;) {
		st_write_block(base, page, src, offset_bits, block_size);
		src += (block_size / 4);
		page++;
		if (length <= block_size)
			break;
		length -= block_size;
		my_printf(".");
	}
	my_printf("\r\n");
#endif
	return 0;
}

#endif	//ENABLE_ST_MICRO

/*
 ***************************************************************************
 */

#ifdef ENABLE_SST
#define SST_SLOW_READ		0x03	//3 address, 0 dummy, [data]
#define SST_FAST_READ		0x0b	//3 address, 1 dummy, [data]
#define SST_ERASE_4K		0x20	//3 address
#define SST_ERASE_32K		0x52	//3 address
#define SST_ERASE_64K		0xd8	//3 address
#define SST_ERASE_ALL		0x60	//just command
#define SST_BYTE_PROGRAM	0x02	//3 address, 1 byte
#define SST_AAI_W_PROGRAM	0xad	//3 address, data, data, [[status], 0xad,data,data]
#define SST_READ_SR		0x05	//[status]
#define SST_ENABLE_WRITE_SR	0x50	//just command
#define SST_WRITE_SR		0x01	//data
#define SST_WRITE_ENABLE	0x06	//just command
#define SST_WRITE_DISABLE	0x04	//just command
#define SST_READ_ID		0x90	//3 address(a0=0 ManId, a0=1 DevId) data
#define SST_ENABLE_BUSY_SO	0x70	//just command
#define SST_DISABLE_BUSY_SO	0x80	//just command

#define SST_SLOW_CLOCK		(PRE_DIV(2) | POST_DIV_P2(0))
#define SST_CLOCK		(PRE_DIV(1) | POST_DIV_P2(0))
/*
 *
 */

static void sst_read_block(int base, unsigned page, unsigned* dst, unsigned offset_bits, unsigned block_size)
{
#if 1
	//5 - 1 opcode, 3 address, 1 dummy bytes in, infinite out
	unsigned cmd2 = (page << (offset_bits + 8)); 		// +8 for dummy byte
	debug_pr("%s: cmd2=%x page=%x\n", __func__, cmd2, page);
	ecspi_write(base, &cmd2, 4, SST_FAST_READ, 8, SST_CLOCK);
	ecspi_read(base, dst, block_size, 0, 0, 0, SST_CLOCK);
#else
	unsigned cmd = MAKE_CMD(SST_SLOW_READ, page, offset_bits);
	ecspi_read(base, dst, block_size, cmd, 32, 1, SST_SLOW_CLOCK);
#endif
	debug_pr("%s: block_size=0x%x page=%x, *dst(%x)=%x\n", __func__, block_size, page, dst, *dst);
}

static unsigned sst_status(int base)
{
	unsigned chip_status; 
	debug_pr("%s\n", __func__);
	chip_status = ecspi_cmd(base, SST_READ_SR << 8, CTL_DEFAULT(16, SST_SLOW_CLOCK));
	debug_pr("chip_status = %x\n", chip_status);
	return chip_status;
}

static void sst_wait_for_ready(int base)
{
	unsigned status;
	do {
		delayMicro(10);
		status = sst_status(base);
	} while (status & 1);
	debug_pr("%s status=%x\n", __func__, status);
}

static void sst_erase_sector_cmd(int base, unsigned page, unsigned offset_bits, unsigned cmd)
{
	cmd = MAKE_CMD(cmd, page, offset_bits);
	debug_pr("%s page=0x%x, cmd=%x\n", __func__, page, cmd);
	ecspi_cmd(base, SST_WRITE_ENABLE, CTL_DEFAULT(8, SST_CLOCK));
	ecspi_cmd(base, cmd, CTL_DEFAULT(32, SST_CLOCK));
	sst_wait_for_ready(base);
}

static void sst_erase_sector(int base, unsigned page, unsigned offset_bits)
{
	sst_erase_sector_cmd(base, page, offset_bits, SST_ERASE_4K);
}

static void sst_erase_sector_range(int base, unsigned epage, unsigned endpage, unsigned offset_bits)
{
	debug_pr("%s: epage=0x%x endpage=0x%x\n", __func__, epage, endpage);
	epage &= ~0x0f;
	endpage = ((endpage - 1) | 0x0f) + 1;
	while (epage < endpage) {
		//a sector
		// is 4K bytes = 16 pages * 256 bytes /page
		// or 32K bytes = 128 pages * 256 bytes /page
		// or 64K bytes = 256 pages * 256 bytes /page
		my_printf("erasing sector containing page %x\n", epage);
		if ( (!(epage & 0xff)) && ((endpage - epage) >= 256)) {
			sst_erase_sector_cmd(base, epage, offset_bits, SST_ERASE_64K);
			epage = (epage | (256 - 1)) + 1;
		} else if ( (!(epage & 0x7f)) && ((endpage - epage) >= 128)) {
			sst_erase_sector_cmd(base, epage, offset_bits, SST_ERASE_32K);
			epage = (epage | (128 - 1)) + 1;
		} else {
			sst_erase_sector_cmd(base, epage, offset_bits, SST_ERASE_4K);
			epage = (epage | (16 - 1)) + 1;
		}
	}
}

static void wait_for_miso_high(int base)
{
	uint mux_base = IOMUXC;
	uint gpio_base = GPIO4_BASE;
	unsigned dr;
	IO_WRITE(mux_base, SW_MUX_CTL_PAD_CSPI1_MISO, ALT3);
	delayMicro(1);

	IO_MOD(gpio_base, GPIO_DR, (1 << GP4_SS1_BIT), 0);	//make SS1 low
	delayMicro(1);
	for (;;) {
		dr = IO_READ(gpio_base, GPIO_PSR);
		if (dr & (1 << GP4_MISO_BIT))
			break;
	}
	IO_MOD(gpio_base, GPIO_DR, 0, (1 << GP4_SS1_BIT));	//make SS1 high
	IO_WRITE(mux_base, SW_MUX_CTL_PAD_CSPI1_MISO, 0);
	delayMicro(1);
}

static void sst_write_block(int base, unsigned page, unsigned* src, unsigned offset_bits, unsigned block_size)
{
	unsigned char *p = (unsigned char *)src; 
	unsigned cmd_rem = ((page << offset_bits) << 16) | (p[3] << 8) | p[2];
	unsigned i = 0;
	uint gpio_base = GPIO4_BASE;
	debug_pr("%s: block_size=0x%x page=%x, *src(%x)=%x\n", __func__, block_size, page, src, *src);
#ifdef DEBUG
	sst_status(base);
#endif
	ecspi_cmd(base, SST_WRITE_ENABLE, CTL_DEFAULT(8, SST_CLOCK));
#ifdef DEBUG
	sst_status(base);
#endif
	ecspi_write(base, &cmd_rem, 4, (SST_AAI_W_PROGRAM << 8) | (page << offset_bits) >> 16, 16, SST_CLOCK);
	IO_MOD(gpio_base, GPIO_DR, 0, (1 << GP4_SS1_BIT));	//make SS1 high
	for (;;) {
		wait_for_miso_high(base);
		if (block_size <= 2)
			break;
		block_size -= 2;
		ecspi_cmd(base, (SST_AAI_W_PROGRAM << 16) | (p[i + 1] << 8) | p[i], CTL_DEFAULT(24, SST_CLOCK));
		i ^= 2;
		if (i)
			p += 4;
	}
	ecspi_cmd(base, SST_WRITE_DISABLE, CTL_DEFAULT(8, SST_CLOCK));
	sst_wait_for_ready(base);
}

static int sst_write_blocks(int base, unsigned page, unsigned* src, unsigned length, unsigned offset_bits, unsigned block_size)
{
	unsigned page_cnt;
	if (!length)
		return 0;
#ifdef DEBUG
	sst_status(base);
#endif
	ecspi_cmd(base, SST_ENABLE_WRITE_SR, CTL_DEFAULT(8, SST_CLOCK));
	ecspi_cmd(base, (SST_WRITE_SR << 8) | 0, CTL_DEFAULT(16, SST_CLOCK));
#ifdef DEBUG
	sst_status(base);
#endif
	ecspi_cmd(base, SST_ENABLE_BUSY_SO, CTL_DEFAULT(8, SST_CLOCK));
#ifdef DEBUG
	sst_status(base);
#endif

	page_cnt = ((length + block_size -1) / block_size);
	sst_erase_sector_range(base, page, page + page_cnt, offset_bits);
#if 1
	for (;;) {
		sst_write_block(base, page, src, offset_bits, block_size);
		src += (block_size / 4);
		page++;
		if (length <= block_size)
			break;
		length -= block_size;
		my_printf(".");
	}
	my_printf("\r\n");
#endif
	ecspi_cmd(base, SST_DISABLE_BUSY_SO, CTL_DEFAULT(8, SST_CLOCK));
	return 0;
}

#endif
