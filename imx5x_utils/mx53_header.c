#include <stdarg.h>
//#define DEBUG
#include "mx5x_common.h"
#include "mx53_header.h"

void mx53_header_search(struct common_info *pinfo)
{
	for (;;) {
		struct ivt_header *hdr = (struct ivt_header *)pinfo->search;
		uint32_t *pself = &hdr->self_ptr;
		debug_pr("hdr=%x pself=%x buf=%x\n", hdr, pself, pinfo->buf);
		if ((void*)pself >= pinfo->buf)
			break;
		if (hdr->barker == IVT_BARKER) {
			uint32_t dest;
			uint32_t offset;
			uint32_t size;
			uint32_t self = *pself;
			struct boot_data *pbd = (struct boot_data *)(hdr->boot_data_ptr - self + ((uint32_t)hdr));
			uint32_t *pimage_len = &pbd->image_len;
			if ((void*)pimage_len >= pinfo->buf)
				break;
			dest = pbd->dest;
			offset = self - dest;
			size = ((uint32_t)hdr) - ((uint32_t)pinfo->initial_buf);
			dest -= size;
			dest += offset;
			pinfo->hdr = (void *)(self);
			pinfo->end = pinfo->cur_end = (void *)(pbd->dest + pbd->image_len);
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

void mx53_exec_program(struct common_info *pinfo, uint32_t start_block)
{
	exec_rtn rtn;
	struct ivt_header *hdr = pinfo->hdr;
	if (hdr) {
		rtn = (exec_rtn)hdr->start_addr;
		if (((unsigned)rtn) & 3)
			return;
		if (((unsigned)rtn) < 0x70000000)
			return;
		if (((unsigned)rtn) >= 0xf8020000)
			return;
		if ((((unsigned)rtn) >= 0xf0000000) && (((unsigned)rtn) < 0xf8000000))
			return;
	} else {
//		hdr = (struct ivt_header *)pinfo->initial_buf;
//		rtn = (exec_rtn)pinfo->initial_buf;
		return;
	}
	debug_dump((void *)hdr, start_block, 1);
	my_printf("file loaded, app_start %x\r\n", rtn);
	flush_uart();
	rtn();
}

void mx53_exec_dl(unsigned char* base, unsigned length)
{
	struct common_info ci;
	ci.search = ci.initial_buf = base;
	ci.buf = (base + length);
	ci.hdr = 0;
	mx53_header_search(&ci);
	mx53_exec_program(&ci, 0);
	my_printf("exec_program failed\r\n");
}

void mx53_header_update_end(struct common_info *pinfo)
{
	//already correct
}
