#include <stdarg.h>
//#define DEBUG
#include "mx5x_common.h"
#include "mx5x_ecspi.h"

void check_page_size(int base)
{
	unsigned offset_bits; 
	unsigned block_size;
	read_block_rtn read_rtn;
	write_blocks_rtn write_rtn;

	ecspi_init(base);
	offset_bits = identify_chip_rtns(base, &block_size, &read_rtn, &write_rtn);
	if (offset_bits && (block_size != (1 << offset_bits))) {
		//atmel and not a power of 2
		my_printf("Unsupported page size of 0x%x bytes.\n", block_size);
		atmel_config_p2(base);
		my_printf("Reprogrammed the page size to 0x%x bytes.\n", (1 << (offset_bits - 1)));
		my_printf("Please power cycle the board for the change to take effect.\n");
		for (;;) {}
	}
}

void write_ubl(int base, unsigned char* ubl, unsigned length, unsigned offset)
{
	unsigned ret;
	unsigned partial = length & 0x3ff; 
	unsigned offset_bits; 
	unsigned block_size;
	unsigned page;
	unsigned diff;
	read_block_rtn read_rtn;
	write_blocks_rtn write_rtn;

	if (partial) {
		partial = 0x400 - partial;
		my_memset(&ubl[length], 0xff, partial);
		length += partial;
	}
	
	ecspi_init(base);
	offset_bits = identify_chip_rtns(base, &block_size, &read_rtn, &write_rtn);
	page = offset / block_size;
	my_printf("block_size = 0x%x\n", block_size);
	diff = offset - (page * block_size);
	if (diff) {
		unsigned char buf[1056];
		read_rtn(base, page, (unsigned *)buf, offset_bits, block_size);
		ubl -= diff;
		length += diff;
		my_memcpy(ubl, buf, diff);
	}
//	debug_dump(ubl, (int)offset, 12); /* byte order reversed already */
	ret = write_rtn(base, page, (unsigned *)ubl, length, offset_bits, block_size);
	if (!ret)
		my_printf("write complete, offset 0x%x, length 0x%x\n", offset, length);
	else
		my_printf("!!!ret=%x\n", ret);
}

int xmodem_load(unsigned char * dest);

int main(void)
{
	int base = ECSPI1_BASE;
	unsigned char *dest = (unsigned char *)0x90600000;
	unsigned char *destX = (unsigned char *)0x93f00000;
	unsigned len;
	check_page_size(base);
	len = xmodem_load(destX);
	if (len)
		my_printf("OK len=0x%x\n", len);
	else {
		my_printf("Xmodem error\n");
		return -1;
	}
#if 1
	reverse_word2((unsigned *)dest, (unsigned *)destX, len >> 2);
	write_ubl(base, dest, len, 0x400);
#endif
	exec_dl(destX + 0x400, len - 0x400);
	dump_mem(destX, 0x400, 14);
	my_printf("hdr not found\n");
	flush_uart();
	return 0;
}
