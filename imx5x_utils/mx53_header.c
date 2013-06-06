#include <stdarg.h>
//#define DEBUG
#include "mx5x_common.h"
#include "mx53_header.h"

extern unsigned header_base;

void mx53_header_search(struct common_info *pinfo)
{
	unsigned barker = header_base;
	for (;;) {
		struct ivt_header *hdr = (struct ivt_header *)pinfo->search;
		uint32_t *pself = &hdr->self_ptr;
		debug_pr("hdr=%x pself=%x buf=%x\n", hdr, pself, pinfo->buf);
		if ((void*)pself >= pinfo->buf)
			break;
		debug_pr("barker=%x %x\n", hdr->barker, barker);
		if (hdr->barker == barker) {
			uint32_t end, dest, buf, initial_buf;
			uint32_t cvt_src_to_dest = *pself - ((uint32_t)hdr);
			struct boot_data *pbd = (struct boot_data *)(hdr->boot_data_ptr - cvt_src_to_dest);
			if (((void*)&pbd->image_len) >= pinfo->buf)
				break;
			initial_buf  = (uint32_t)pinfo->initial_buf;
			dest = initial_buf + cvt_src_to_dest;
			buf = (uint32_t)pinfo->buf;
			end = (pbd->dest + pbd->image_len);
			if (cvt_src_to_dest) {
				uint32_t src_end = end - cvt_src_to_dest;
				if (src_end > buf)
					src_end = buf;
				debug_pr("dest=%x initial_buf=%x src_end=%x\n", dest, initial_buf, src_end);
				if (initial_buf < src_end)
					my_memcpy((void*)dest, (void*)initial_buf, src_end - initial_buf);
			}
			pinfo->initial_buf = (void *)dest;
			pinfo->hdr = (struct app_header *)(((uint32_t)hdr) + cvt_src_to_dest);
			pinfo->buf = (void *)(buf + cvt_src_to_dest);
			pinfo->dest = (void *)pbd->dest;
			pinfo->end = pinfo->cur_end = (void *)end;
			pinfo->file_size = end - dest;
			debug_pr("initial_buf=%x hdr=%x buf=%x end=%x file_size=%x\n", pinfo->initial_buf, pinfo->hdr, pinfo->buf, end, pinfo->file_size);
			break;
		}
		pinfo->search += 0x40;	//header should be 64 byte aligned
	}
}

exec_rtn mx53_header_get_rtn(void *_hdr)
{
	exec_rtn rtn = NULL;
	struct ivt_header *hdr = _hdr;
	if (hdr) {
		unsigned r = hdr->start_addr;
		if (r & 3)
			return rtn;
		if (r < 0x70000000)
			return rtn;
		if (r >= 0xf8020000)
			return rtn;
		if ((r >= 0xf0000000) && (r < 0xf8000000))
			return rtn;
		rtn = (exec_rtn)r;
	}
	return rtn;
}

void mx53_header_update_end(struct common_info *pinfo)
{
	//already correct
}
