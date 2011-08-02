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
			pinfo->file_size = size;
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

exec_rtn mx51_header_get_rtn(void *_hdr)
{
	exec_rtn rtn = NULL;
	struct app_header *hdr = _hdr;
	if (hdr) {
		unsigned r = hdr->app_start_addr;
		if (r & 3)
			return rtn;
		if (r >= 0xb0000000)
			return rtn;
		if (r < 0x1ffe0000)
			return rtn;
		if ((r >= 0x20000000) && (r < 0x90000000))
			return rtn;
		rtn = (exec_rtn)r;
	}
	return rtn;
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
							pinfo->file_size += ((uint)end) - ((uint)hdr);
						}
					}
				}
			}
		}
	}
}
