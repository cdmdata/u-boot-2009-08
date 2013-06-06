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

extern unsigned chain_ivt;
extern unsigned program_end;
extern unsigned prog_start;

int plug_main2(void **pstart, unsigned *pbytes, unsigned *pivt_offset)
{
	struct common_info ci;
	int base = get_ecspi_base();
	int rtn;
	unsigned char *ram_base = (unsigned char *)get_ram_base();
	unsigned char *destX = ram_base + 0x03f00000;
	unsigned len;
	void *src;
	int plug;

//	my_printf("ram_base=%x ecspi_base=%x\n", ram_base, base);
//	my_printf("pstart=%x pbytes=%x pivt_offset=%x\n", pstart, pbytes, pivt_offset);
	check_page_size(base);
	my_printf("s2\n");

	if (!header_present(destX)) {
//		dump_mem(destX, 0x400, 2);
		my_printf("header not valid\n");
		flush_uart();
		return 0;	/* 0 is failure */
	}
	ci.initial_buf = destX;
	ci.search = destX + 0x40;
	ci.buf = src = (destX + 0x80000);
	ci.hdr = 0;
	ci.cur_end = ci.end = ci.dest = 0;
	ci.file_size = 0;
	header_search(&ci);
	header_update_end(&ci);
	if (!ci.hdr) {
		//use 1st header if no 2nd header
		my_printf("2nd header missing, using 1st\n");
		ci.search = destX;
		header_search(&ci);
		header_update_end(&ci);
	}
	if (ci.buf < ci.end)
		if (ci.buf != src)
			my_memcpy(ci.buf, src, ci.end - ci.buf);
	rtn = common_exec_program(&ci);
	plug = ((((int)pstart) < 0x100) || (((int)pbytes) < 0x100) || (((int)pivt_offset) < 0x100)) ? 0 : 1;
	if (((int)pstart| (int)pbytes | (int)pivt_offset) & 3)
		plug = 0;
	len = ci.end - ci.dest;
	if (plug) {
		*pstart = ci.dest;
		*pbytes = len;
		*pivt_offset = ci.hdr - ci.dest;
	}
	my_printf("OK len=0x%x file_size=0x%x rtn=0x%x\n", len, ci.file_size, rtn);
#if 1
	if (rtn) {
		unsigned char *dest  = ram_base + 0x00600000;
		reverse_word2((unsigned *)dest, (unsigned *)ci.initial_buf, ci.file_size >> 2);
		write_ubl(base, dest, ci.file_size, 0x400);
	}
#endif
	flush_uart();
	if (!plug && rtn)
		((exec_rtn)rtn)();
	return (rtn) ? 1 : 0;	/* 1 is success, 0 is failure */
}

int plug_main(void **pstart, unsigned *pbytes, unsigned *pivt_offset)
{
	unsigned char *ram_base = (unsigned char *)get_ram_base();
	my_printf("pstart=%x pbytes=%x pivt_offset=%x\n", pstart, pbytes, pivt_offset);
	if (!ram_test((unsigned *)ram_base)) {
		my_printf("ram error\n");
		return ERROR_MEMORY_TEST;
	}
	my_printf("ecspi_ram_write ram test passed!!!\n");

#if 1
	{
		struct common_info ci;
		int rtn;
		unsigned bytes;
		unsigned ivt_offset;
		ci.search = &chain_ivt;
		ci.initial_buf = &prog_start;
		ci.buf = &program_end;
		ci.hdr = 0;
		ci.cur_end = ci.end = ci.dest = 0;
		header_search(&ci);
		header_update_end(&ci);
		bytes = ci.end - ci.dest;
		rtn = common_exec_program(&ci);
		ivt_offset = ci.hdr - ci.dest;
		if (pstart)
			*pstart = ci.dest;
		if (pbytes)
			*pbytes = bytes;
		if (pivt_offset)
			*pivt_offset = ivt_offset;
		my_printf("s1: start=%x bytes=%x ivt_offset=%x\n", ci.dest, bytes, ivt_offset);
//		return 0;
		return (rtn) ? 1 : 0;	/* 1 is success, 0 is failure */
	}
#else
	//this works, but then mfgtool won't find chain_ivt for boot command
	return plug_main2(pstart, pbytes, pivt_offset);
#endif
}

