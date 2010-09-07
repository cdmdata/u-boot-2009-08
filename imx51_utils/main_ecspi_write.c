#include <stdarg.h>
//#define DEBUG
#include "mx51_common.h"
#include "mx51_ecspi.h"

void check_page_size(int base)
{
	unsigned chip_status;
	unsigned offset_bits; 
	unsigned block_size;
	ecspi_init(base);
	chip_status = atmel_status(base);
	if (!(chip_status & 1)) {
		offset_bits = atmel_get_offset_bits(chip_status, &block_size);
		if (offset_bits) {
			//atmel and not a power of 2
			my_printf("Unsupported page size of 0x%x bytes.\n", block_size);
			atmel_config_p2(base);
			my_printf("Reprogrammed the page size to 0x%x bytes.\n", (1 << (offset_bits - 1)));
			my_printf("Please power cycle the board for the change to take effect.\n");
			for (;;) {}
		}
	}
}

void write_ubl(int base, unsigned char* ubl, unsigned length, unsigned offset)
{
	unsigned ret;
	unsigned partial = length & 0x3ff; 
	unsigned chip_status;
	unsigned offset_bits; 
	unsigned block_size;
	unsigned page;
	unsigned diff;
	unsigned st_micro = 0;

	if (partial) {
		partial = 0x400 - partial;
		my_memset(&ubl[length], 0xff, partial);
		length += partial;
	}
	
	ecspi_init(base);
	chip_status = atmel_status(base);
	if ((chip_status & 0xff) == 0xff) {
		//st micro part ?
		chip_status = st_read_id(base);
		if (chip_status != 0x202015) {
			my_printf("!!!error id=%x\n", chip_status);
			return;
		}
		offset_bits = 8;
		block_size = (1 << offset_bits);
		page = offset >> offset_bits;
		st_micro = 1;
	} else {
		//atmel
		unsigned chip_id = atmel_id(base);
		my_printf("chip_id=%x\n", chip_id);
		offset_bits = atmel_get_offset_bits(chip_status, &block_size);
		if (!offset_bits) {
			my_printf("!!!error chip_status=%x\n", chip_status);
			return;
		}
		page = offset / block_size;
#if 0
		{
			chip_id = atmel_chip_erase(base);
			my_printf("chip_erase=%x\n", chip_id);
			return;
		}
#endif
	}
	my_printf("block_size = 0x%x\n", block_size);
	diff = offset - (page * block_size);
	if (diff) {
		unsigned char buf[1056];
		if (st_micro)
			st_read_block(base, page, (unsigned *)buf, offset_bits, block_size);
		else
			atmel_read_block(base, page, (unsigned *)buf, offset_bits, block_size);
		ubl -= diff;
		length += diff;
		my_memcpy(ubl, buf, diff);
	}
//	debug_dump(ubl, (int)offset, 12); /* byte order reversed already */
	if (st_micro)
		ret = st_write_blocks(base, page, (unsigned *)ubl, length, offset_bits, block_size);
	else
		ret = atmel_write_blocks(base, page, (unsigned *)ubl, length, offset_bits, block_size);
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
