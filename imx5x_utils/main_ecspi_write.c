#include <stdarg.h>
#define DEBUG
#include "imx.h"
#include "mx5x_common.h"
#include "mx5x_ecspi.h"

void check_page_size(int base)
{
	unsigned offset_bits;
	unsigned block_size;
	read_block_rtn read_rtn;
	write_blocks_rtn write_rtn;
	int i = 0;

	ecspi_init(base);
	for (;;) {
		offset_bits = identify_chip_rtns(base, &block_size, &read_rtn, &write_rtn);
		if (offset_bits)
			break;
		i++;
		if (i > 7)
			break;
	}
	if (i)
		my_printf("offset_bits=%x\n", offset_bits);
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
	if (!offset_bits) {
		my_printf("unrecognized chip\n");
		return;
	}
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
void drive_power_on(void);

int plug_main(void **pstart, unsigned *pbytes, unsigned *pivt_offset)
{
	struct common_info ci;
	int base = get_ecspi_base();
	unsigned char *ram_base = (unsigned char *)get_ram_base();
	unsigned char *dest  = ram_base + 0x00600000;
	unsigned char *destX = ram_base + 0x03f00000;
	unsigned len;

	drive_power_on();
	my_printf("ram_base=%x ecspi_base=%x\n", ram_base, base);
	my_printf("pstart=%x pbytes=%x pivt_offset=%x\n", pstart, pbytes, pivt_offset);
	if (!ram_test((unsigned *)ram_base)) {
		return ERROR_MEMORY_TEST;
	}
#if 0
	for (;;) {
		*((unsigned *)(ram_base + 0x15555554)) = 0x55555555;
		*((unsigned *)(ram_base + 0x0aaaaaa8)) = 0xaaaaaaaa;
	}
#endif
	check_page_size(base);
	len = xmodem_load(destX);
	if (len)
		my_printf("OK len=0x%x\n", len);
	else {
		dump_mem(destX, 0x400, 2);
		my_printf("Xmodem error\n");
		return 0;
	}
#if 1
	reverse_word2((unsigned *)dest, (unsigned *)destX, len >> 2);
	write_ubl(base, dest, len, 0x400);
#endif
	destX += 0x400;
	len -= 0x400;

	ci.search = ci.initial_buf = destX;
	ci.buf = (destX + len);
	ci.hdr = 0;
	ci.cur_end = ci.end = ci.dest = 0;
	header_search(&ci);
	header_update_end(&ci);
	flush_uart();
	if (pstart)
		*pstart = ci.dest;
	if (pbytes)
		*pbytes = ci.end - ci.dest;
	if (pivt_offset)
		*pivt_offset = ci.hdr - ci.dest;
	return common_exec_program(&ci) ? 1 : 0;
}
