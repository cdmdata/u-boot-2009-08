#include <stdarg.h>
//#define DEBUG
#include "mx51_common.h"
#include "mx51_ecspi.h"
#define NULL 0
extern const int payload_offset;

int main(void)
{
	struct common_info ci;
	struct app_header *hdr = NULL;
	int base = ECSPI1_BASE;
#ifdef DEBUG
	unsigned offset = ((unsigned)0x1400);	/* 1k aligned */
#else
	unsigned offset = ((unsigned)&payload_offset);	/* 1k aligned */
#endif
	unsigned diff;
	unsigned page;
	unsigned end;
	unsigned offset_bits; 
	unsigned block_size;
	read_block_rtn read_rtn;

	ci.search = ci.buf = ci.initial_buf = (unsigned *)0x93f00000;
	ci.hdr = NULL;
	end = ((unsigned)ci.buf) + 0x10000;
	ecspi_init(base);

	offset_bits = identify_chip_read_rtn(base, &block_size, &read_rtn);
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
		if (!hdr) {
			if (ci.hdr) {
				unsigned *p = (unsigned *)&ci.hdr->dcd_ptr;
				unsigned len = 0x10000;
				end = ((unsigned)ci.hdr) + 0x10000;
				if (p < (unsigned *)ci.buf) {
					p = (unsigned *)*p;
					if (p < (unsigned *)ci.buf) {
						if (*p == DCD_BARKER) {
							p = (unsigned *)(((unsigned char *)p) + p[1] + 8);
							if (p < (unsigned *)ci.buf) {
								len = *p;
								p = (unsigned *)&ci.hdr->app_dest_ptr;
								if (p < (unsigned *)ci.buf) {
									p = (unsigned *)*p;
									hdr = ci.hdr;
									debug_pr("len = %x", len);
									end = ((unsigned)p) + len;
									if ((unsigned)(end - ((unsigned)ci.buf)) > (4 << 20))
										end = ((unsigned)ci.buf) + (4 << 20);
								}
							}
						}
					}
				}
			}
		}
	} while (((unsigned)ci.buf) < end);

	exec_program(&ci, 0);
	dump_mem(ci.initial_buf, offset, 14);
	my_printf("exec_program failed, hdr=%x\n", ci.hdr);
	flush_uart();
	return 0;
}
