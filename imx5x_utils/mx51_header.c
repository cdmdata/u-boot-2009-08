#include <stdarg.h>
//#define DEBUG
#include "mx5x_common.h"
#include "mx51_header.h"

void mx51_header_search(struct common_info *pinfo)
{
	for (;;) {
		struct app_header *hdr = (struct app_header *)pinfo->search;
		void *pdest = (void *)&hdr->app_dest_ptr;
		debug_pr("hdr=%x pdest=%x buf=%x start=%x barker=%x\n", hdr, pdest, pinfo->buf, hdr->app_start_addr, hdr->app_barker);
		if (pdest >= pinfo->buf)
			break;
		if (hdr->app_barker == APP_BARKER) {
			uint32_t cvt_src_to_dest = ((uint32_t)hdr->dcd_ptr_ptr) - ((uint32_t)&hdr->dcd_ptr);
			uint32_t initial_buf = (uint32_t)pinfo->initial_buf;
			uint32_t dest = initial_buf + cvt_src_to_dest;
			uint32_t buf = (uint32_t)pinfo->buf;
			if (cvt_src_to_dest) {
				debug_pr("dest=%x initial_buf=%x\n", dest, initial_buf);
				if (initial_buf < buf)
					my_memcpy((void*)dest, (void*)initial_buf, buf - initial_buf);
			}
			pinfo->initial_buf = (void *)dest;
			pinfo->hdr = (struct app_header *)(((uint32_t)hdr) + cvt_src_to_dest);
			pinfo->buf = (void *)(buf + cvt_src_to_dest);
			debug_pr("initial_buf=%x hdr=%x buf=%x\n", pinfo->initial_buf, pinfo->hdr, pinfo->buf);
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
						debug_pr("len = %x, p = %p\n", len, p);
						p = (unsigned *)&hdr->app_dest_ptr;
						if (p < (unsigned *)pinfo->buf) {
							unsigned end;
							unsigned dest = *p;
							debug_pr("dest = %x, p = %p\n", dest, p);
							end = dest + len;
							pinfo->dest = (void *)dest;
							pinfo->end = pinfo->cur_end = (void*)end;
							pinfo->file_size = ((uint)end) - ((uint)pinfo->initial_buf);
							debug_pr("end = %p, file_size = %p\n", pinfo->end, pinfo->file_size);
						}
					}
				}
			}
		}
	}
}
