#include <stdarg.h>
//#define DEBUG
#include "imx.h"
#include "mx5x_common.h"
#include "mx5x_ecspi.h"
#define NULL 0
extern const int payload_offset;

//#define USE_CRC32	//crc32 mx51_u-boot.no-padding
//#define USE_ADLER32	//adler32 mx51_u-boot.no-padding
#ifdef USE_CRC32
unsigned crc32(unsigned crc, const unsigned char *buf, unsigned len);
#endif
#ifdef USE_ADLER32
unsigned long adler32(unsigned long adler,const unsigned char *buf, unsigned int len);
#endif

/*
 * Possible return values
 * ERROR_MEMORY_TEST (-10)
 * ERROR_NO_HEADER (-9)
 */
int plug_main(void **pstart, unsigned *pbytes, unsigned *pivt_offset)
{
	struct common_info ci;
	int base = get_ecspi_base();
	unsigned char *ram_base = (unsigned char *)get_ram_base();
#ifdef DEBUG
	unsigned offset = ((unsigned)0x1400);	/* 1k aligned */
#else
	unsigned offset = ((unsigned)&payload_offset);	/* 1k aligned */
#endif
	unsigned diff;
	unsigned page;
	unsigned offset_bits;
	unsigned block_size;
	unsigned bytes, ivt_offset;
	unsigned i = 0;
	read_block_rtn read_rtn;
//	my_printf("pstart=%x pbytes=%x pivt_offset=%x\n", pstart, pbytes, pivt_offset);
	if (!ram_test((unsigned *)ram_base)) {
		flush_uart();
		return ERROR_MEMORY_TEST;
	}
	ci.search = ci.buf = ci.initial_buf = (unsigned *)(ram_base +0x03f00400);
	ci.hdr = NULL;
	ci.cur_end = (void*)(((unsigned)ci.buf) + 0x10000);
	ci.end = NULL;
	ecspi_init(base);

	for (;;) {
		offset_bits = identify_chip_read_rtn(base, &block_size, &read_rtn);
		if (offset_bits)
			break;
		i++;
		if (i > 7)
			return -9;
	}
	page = offset / block_size;
	diff = offset - (page * block_size);
	debug_pr("page=%x diff=%x block_size=%x\n", page, diff, block_size);
	do {
		read_rtn(base, page, ci.buf - diff, offset_bits, block_size);
		reverse_word(ci.buf - diff, block_size >> 2);
		debug_pr("*ci.buf(%x)=%x\n", ci.buf, *((int*)ci.buf));
		if (ci.buf == ci.initial_buf) debug_dump(ci.buf, (int)ci.buf, 1);
		common_load_block_of_file(&ci, block_size - diff);
		page++;
		diff = 0;
		if (!ci.end)
			header_update_end(&ci);
	} while (ci.buf < ci.cur_end);

#if defined(USE_CRC32) || defined(USE_ADLER32)
	{
		unsigned char *p = ((unsigned char *)ci.hdr);
		unsigned length = ((unsigned char *)ci.cur_end) - p;
#ifdef USE_CRC32
		my_printf("start=%x(%x), length=%x, crc=%x\n", p, *((unsigned *)p), length, crc32(0, p, length));
#else
		my_printf("start=%x(%x), length=%x, adler32=%x\n", p, *((unsigned *)p), length, adler32(0, p, length));
#endif
		p = (void*)((((unsigned)p) & 0xfff00000) - 0x00300000 + 0x000f0000);
		my_memset(p, 0xff, 0x4000);	//775f4000 is mx53 ttbr, 0x93cf4000 is mx51 ttbr
	}
#endif
	if (ci.hdr)
		debug_dump((void *)ci.hdr, offset, 1);
	flush_uart();
	if (pstart)
		*pstart = ci.dest;
	bytes = ci.end - ci.dest;
	ivt_offset = ci.hdr - ci.dest;
	if (pbytes)
		*pbytes = bytes;
	if (pivt_offset)
		*pivt_offset = ivt_offset;
//	my_printf("start=%x bytes=%x ivt_offset=%x\n", ci.dest, bytes, ivt_offset);
	return common_exec_program(&ci) ? 1 : 0;
}
