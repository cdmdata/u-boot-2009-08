/*
 * Currently supports the ATMEL AT45DB642D and ATDB161D chips
 */
#include <stdarg.h>
//#define DEBUG
#include "mx51_common.h"
#include "mx51_ecspi.h"

#define ST_MICRO

#define MAKE_CMD(cmd, page, shift) (((page << shift) & 0x00ffffff) | (cmd << 24))


//#define GPIO1_BASE 0x73f84000
#define GPIO4_BASE 0x73f90000
#define GPIO_DR 0
#define GPIO_DIR 4


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

	IO_MOD(gpio_base, GPIO_DR, (1 << 24), (1 << 25));	//make sure SS0 is low(pmic), SS1 is high
	IO_MOD(gpio_base, GPIO_DIR, 0, (1 << 24) | (1 << 25));	//make sure SS0/SS1 are output
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

	IO_MOD(gpio_base, GPIO_DR, (1 << 25), 0);	//make SS1 low
	IO_WRITE(base, ECSPI_CONTROL, ctl + CTL_SMC);
	do {
		status = IO_READ(base, ECSPI_STATUS);
		debug_pr("status=%x cmd=%x ctl=%x\n", status, cmd, ctl);
		if (status & (1<<3)) {	//rx fifo has at least 1 word?
			data = IO_READ(base, ECSPI_RXDATA);
			debug_pr("data = %x\n", data);
		}
	} while (!(status & (1<<7)));	//wait for transfer complete

	IO_MOD(gpio_base, GPIO_DR, 0, (1 << 25));	//make SS1 high
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

	IO_MOD(gpio_base, GPIO_DR, (1 << 25), 0);	//make SS1 low
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
	IO_MOD(gpio_base, GPIO_DR, 0, (1 << 25));	//make SS1 high
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
	IO_MOD(gpio_base, GPIO_DR, (1 << 25), 0);	//make SS1 low
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
#define SF_MANUFACTURER_DEVICE_ID_READ	0x9f
#define SF_CHIP_ERASE		0xc794809a
#define SF_CONFIG_P2		0x3d2a80a6

#define MASK_STATUS_ID		0x3c

#define SECTOR0a_page		0
#define SECTOR0b_page		8
#define SECTOR1_page		256
#define NORMAL_SECTOR_page_cnt	256

#define ATMEL_STATUS_CLOCK	(PRE_DIV(7) | POST_DIV_P2(0))
#define ATMEL_CLOCK		(PRE_DIV(1) | POST_DIV_P2(0))

unsigned atmel_status(int base)
{
	unsigned chip_status; 
//	debug_pr("%s\n", __func__);
	chip_status = ecspi_cmd(base, SF_STATUS << 8, CTL_DEFAULT(16, ATMEL_STATUS_CLOCK));
//	debug_pr("chip_status = %x\n", chip_status);
	return chip_status;
}

unsigned atmel_get_offset_bits(unsigned chip_status, unsigned * pblock_size)
{
	unsigned offset_bits = 0;
	unsigned block_size;
	if ((chip_status & ATMEL_STATUS_MASK) == AT45DB161D_STATUS_ID)
		offset_bits = AT45DB161D_P2_OFFSET_BITS;
	else if ((chip_status & ATMEL_STATUS_MASK) == AT45DB081D_STATUS_ID)
		offset_bits = AT45DB081D_P2_OFFSET_BITS;
	else if ((chip_status & ATMEL_STATUS_MASK) == AT45DB041D_STATUS_ID)
		offset_bits = AT45DB041D_P2_OFFSET_BITS;
	else
		return 0;
	if (chip_status & 1) {
		block_size = (1 << offset_bits);
	} else {
		block_size = ((1 << offset_bits) + (1 << (offset_bits - 5)));
		offset_bits++;
	}
	*pblock_size = block_size;
	return offset_bits;
}

void atmel_wait_for_ready(int base)
{
	unsigned status;
	debug_pr("%s\n", __func__);
	do {
		delayMicro(1000);	//1 ms
		status = atmel_status(base);
	} while (!(status & 0x80));
}

void atmel_read_block(int base, unsigned page, unsigned* dst, unsigned offset_bits, unsigned block_size)
{
	unsigned cmd = MAKE_CMD(SF_READ, page, offset_bits);	//continuous array read, throw away 1st 8 bytes from read fifo
	ecspi_read(base, dst, block_size, cmd, 32, 2, ATMEL_CLOCK);
	debug_pr("%s: block_size=0x%x page=%x, *dst(%x)=%x\n", __func__, block_size, page, dst, *dst);
}

unsigned atmel_id(int base)
{
	unsigned buf[3];
	unsigned cmd = SF_MANUFACTURER_DEVICE_ID_READ << 24;
	ecspi_read(base, buf, 4, cmd, 32, 0, ATMEL_STATUS_CLOCK);
	debug_pr("%s: id %08x\n", __func__, buf[0]);
	return buf[0] & 0x00ffffff;
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
void atmel_erase_sector(int base, unsigned page, unsigned offset_bits)
{
	unsigned cmd = MAKE_CMD(SF_SECTOR_ERASE, page, offset_bits);
	debug_pr("%s page=0x%x, cmd=%x\n", __func__, page, cmd);
	ecspi_cmd(base, cmd, CTL_DEFAULT(32, ATMEL_CLOCK));
//	delayMicro(5000000);	//(5 seconds max sector erase time)
	atmel_wait_for_ready(base);
}

void atmel_erase_block(int base, unsigned page, unsigned offset_bits)
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

void atmel_write_block(int base, unsigned page, unsigned* src, unsigned offset_bits, unsigned block_size)
{
	unsigned cmd = MAKE_CMD(SF_BUFFER1_PROGRAM, page, offset_bits); //program buffer
	uint gpio_base = GPIO4_BASE;
	debug_pr("%s: block_size=0x%x page=%x, *src(%x)=%x\n", __func__, block_size, page, src, *src);
	ecspi_write(base, src, block_size, (SF_BUFFER1_WRITE << 24), 32, ATMEL_CLOCK);
	IO_MOD(gpio_base, GPIO_DR, 0, (1 << 25));	//make SS1 high
	ecspi_cmd(base, cmd, CTL_DEFAULT(32, ATMEL_CLOCK));
//	delayMicro(6000);	//6 ms max
	atmel_wait_for_ready(base);
}

void atmel_erase_sector_range(int base, unsigned epage, unsigned endpage, unsigned offset_bits)
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

void atmel_erase_block_range(int base, unsigned epage, unsigned endpage, unsigned offset_bits)
{
	debug_pr("%s: epage=0x%x endpage=0x%x\n", __func__, epage, endpage);

	while (epage < endpage) {
		atmel_erase_block(base, epage, offset_bits);
		epage = (epage | 7) + 1;
	}
}

int atmel_write_blocks(int base, unsigned page, unsigned* src, unsigned length, unsigned offset_bits, unsigned block_size)
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

#ifdef ST_MICRO
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
unsigned st_read_id(int base)
{
	unsigned id = ecspi_cmd(base, ST_READ_ID << 24, CTL_DEFAULT(32, ST_SLOW_CLOCK));
	return id & 0x00ffffff;
}

void st_read_block(int base, unsigned page, unsigned* dst, unsigned offset_bits, unsigned block_size)
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

unsigned st_status(int base)
{
	unsigned chip_status; 
	debug_pr("%s\n", __func__);
	chip_status = ecspi_cmd(base, ST_READ_STATUS << 8, CTL_DEFAULT(16, ST_SLOW_CLOCK));
	debug_pr("chip_status = %x\n", chip_status);
	return chip_status;
}

void st_wait_for_ready(int base)
{
	unsigned status;
	debug_pr("%s\n", __func__);
	do {
		delayMicro(10);
		status = st_status(base);
	} while (status & 1);
}

void st_erase_sector(int base, unsigned page, unsigned offset_bits)
{
	unsigned cmd = MAKE_CMD(ST_SECTOR_ERASE, page, offset_bits);
	debug_pr("%s page=0x%x, cmd=%x\n", __func__, page, cmd);
	ecspi_cmd(base, ST_WRITE_ENABLE, CTL_DEFAULT(8, ST_CLOCK));
	ecspi_cmd(base, cmd, CTL_DEFAULT(32, ST_CLOCK));
	st_wait_for_ready(base);
}

void st_erase_sector_range(int base, unsigned epage, unsigned endpage, unsigned offset_bits)
{
	debug_pr("%s: epage=0x%x endpage=0x%x\n", __func__, epage, endpage);

	while (epage < endpage) {
		//a sector is 512Kbit = 64K bytes = 256 pages * 256 bytes /page
		my_printf("erasing sector containing page %x\n", epage);
		st_erase_sector(base, epage, offset_bits);
		epage = (epage | (256 - 1)) + 1;
	}
}

void st_write_block(int base, unsigned page, unsigned* src, unsigned offset_bits, unsigned block_size)
{
	unsigned cmd = MAKE_CMD(ST_PAGE_PROGRAM, page, offset_bits); //program buffer
	uint gpio_base = GPIO4_BASE;
	debug_pr("%s: block_size=0x%x page=%x, *src(%x)=%x\n", __func__, block_size, page, src, *src);
	ecspi_cmd(base, ST_WRITE_ENABLE, CTL_DEFAULT(8, ST_CLOCK));
	ecspi_write(base, src, block_size, cmd, 32, ST_CLOCK);
	IO_MOD(gpio_base, GPIO_DR, 0, (1 << 25));	//make SS1 high
	st_wait_for_ready(base);
}

int st_write_blocks(int base, unsigned page, unsigned* src, unsigned length, unsigned offset_bits, unsigned block_size)
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

#endif
