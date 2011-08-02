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
		if (hdr->barker == barker) {
			uint32_t dest;
			uint32_t offset;
			uint32_t size, max_size;
			uint32_t self = *pself;
			struct boot_data *pbd = (struct boot_data *)(hdr->boot_data_ptr - self + ((uint32_t)hdr));
			uint32_t *pimage_len = &pbd->image_len;
			if ((void*)pimage_len >= pinfo->buf)
				break;
			dest = pbd->dest;
			offset = self - dest;	//offset of ivt_header from destination
			size = ((uint32_t)hdr) - ((uint32_t)pinfo->initial_buf);	//offset of ivt_header into file loaded
			dest -= size;
			dest += offset;		//new destination to include all of file
			pinfo->hdr = (void *)(self);
			pinfo->end = pinfo->cur_end = (void *)(pbd->dest + pbd->image_len);
			pinfo->dest = (void *)pbd->dest;
			max_size = ((uint32_t)pinfo->end) - dest;
			pinfo->file_size = max_size;
			size = pinfo->buf - pinfo->initial_buf;
			if (size > max_size)
				size = max_size;
			if (dest != (uint32_t)pinfo->initial_buf)
				if (dest < 0xf0000000)
					my_memcpy((void*)dest, pinfo->initial_buf, size);
			pinfo->buf = (void *)(dest + size);
			break;
		}
		pinfo->search += 0x400;	//search 1k at a time
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
