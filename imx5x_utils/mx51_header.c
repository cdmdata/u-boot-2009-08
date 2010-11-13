#include <stdarg.h>
//#define DEBUG
#include "mx5x_common.h"
#include "mx51_header.h"

void mx51_header_search(struct common_info *pinfo)
{
	for (;;) {
		struct app_header *hdr = (struct app_header *)pinfo->search;
		void *pdest = (void *)&hdr->app_dest_ptr;
		debug_pr("hdr=%x pdest=%x buf=%x\n", hdr, pdest, pinfo->buf);
		if (pdest >= pinfo->buf)
			break;
		if (hdr->app_barker == APP_BARKER) {
			uint32_t dest = hdr->app_dest_ptr;
			uint32_t offset = ((hdr->dcd_ptr_ptr - dest) > 0x400) ? 0x400 : 0;
			uint32_t size = ((uint32_t)hdr) - ((uint32_t)pinfo->initial_buf);
			dest -= size;
			dest += offset;
			if ( (hdr->app_start_addr - dest) > (1<<20) )
				dest = (hdr->app_start_addr & ~0xfff);
			pinfo->hdr = (struct app_header *)(dest + size);
			size = pinfo->buf - pinfo->initial_buf;
			if (dest != (uint32_t)pinfo->initial_buf)
				my_memcpy((void*)dest, pinfo->initial_buf, size);
			pinfo->buf = (void *)(dest + size);
			break;
		}
		pinfo->search += 0x400;	//search 1k at a time
	}
}

typedef void (*exec_rtn)(void);

void mx51_exec_program(struct common_info *pinfo, uint32_t start_block)
{
	exec_rtn rtn;
	struct app_header *hdr = pinfo->hdr;
	if (hdr) {
		rtn = (exec_rtn)hdr->app_start_addr;
		if (((unsigned)rtn) & 3)
			return;
		if (((unsigned)rtn) >= 0xb0000000)
			return;
		if (((unsigned)rtn) < 0x1ffe0000)
			return;
		if ((((unsigned)rtn) >= 0x20000000) && (((unsigned)rtn) < 0x90000000))
			return;
	} else {
//		hdr = (struct app_header *)pinfo->initial_buf;
//		rtn = (exec_rtn)pinfo->initial_buf;
		return;
	}
	debug_dump((void *)hdr, start_block, 1);
	my_printf("file loaded, app_start %x\r\n", rtn);
	flush_uart();
	rtn();
}

void mx51_exec_dl(unsigned char* base, unsigned length)
{
	struct app_header *hdr;
	unsigned char* end = base + length;
	do {
		hdr = (struct app_header *)base;
		if (hdr->app_barker == APP_BARKER) {
			struct common_info ci;
			uint32_t dest = hdr->app_dest_ptr;
			uint32_t offset = ((hdr->dcd_ptr_ptr - dest) > 0x400) ? 0x400 : 0;
			base -= offset;
			my_memcpy((void*)dest, base, end - base);
			ci.hdr = (void *)(dest + offset);
			ci.initial_buf = (void *)(dest);
			mx51_exec_program(&ci, 0);
			my_printf("exec_program failed dest=%x\r\n", dest);
			return;
		}
		base += 0x400;
	} while (base < end);
}

void mx51_header_update_end(struct common_info *pinfo)
{
	struct app_header *hdr = (struct app_header *)pinfo->hdr;
	if (hdr && !pinfo->end) {
		unsigned *p = (unsigned *)&hdr->dcd_ptr;
		unsigned len = 0x10000;
		pinfo->cur_end = (void *)(((unsigned)hdr) + 0x10000);
		if (p < (unsigned *)pinfo->buf) {
			p = (unsigned *)*p;
			if (p < (unsigned *)pinfo->buf) {
				if (*p == DCD_BARKER) {
					p = (unsigned *)(((unsigned char *)p) + p[1] + 8);
					if (p < (unsigned *)pinfo->buf) {
						len = *p;
						p = (unsigned *)&hdr->app_dest_ptr;
						if (p < (unsigned *)pinfo->buf) {
							unsigned end;
							p = (unsigned *)*p;
							debug_pr("len = %x", len);
							end = ((unsigned)p) + len;
							if ((unsigned)(end - ((unsigned)pinfo->buf)) > (4 << 20))
								end = ((unsigned)pinfo->buf) + (4 << 20);
							pinfo->end = pinfo->cur_end = (void*)end;
						}
					}
				}
			}
		}
	}
}
