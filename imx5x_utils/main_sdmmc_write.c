#include <stdarg.h>
#include "mx5x_common.h"
#include "mx5x_sdmmc.h"

void write_ubl(unsigned char* ubl, unsigned length, unsigned start_block)
{
	struct sdmmc_dev dev;
	unsigned ret;
	unsigned partial = length & 0x1ff;

	if (partial) {
		partial = 0x200 - partial;
		my_memset(&ubl[length], 0xff, partial);
		length += partial;
	}
	do {
		my_memset((unsigned char *)&dev, 0, sizeof(struct sdmmc_dev));
		if (mmc_init(&dev)) {
			delayMicro(1000000);
			continue;
		}
		ret = sd_write_blocks(&dev, start_block, length >> 9, ubl);
		if (!GET_CODE(ret)) {
			my_printf("write complete, block 0x%x, length 0x%x, blockcnt=0x%x\n", start_block, length, ret);
			break;
		}
		my_printf("!!!ret=%x\n", ret);
	} while (1);
}

int xmodem_load(unsigned char * dest);

int plug_main(void **pstart, unsigned *pbytes, unsigned *pivt_offset)
{
	struct common_info ci;
	unsigned char *dest = (unsigned char *)0x90600000;
	unsigned len = xmodem_load(dest);
	if (len)
		my_printf("OK len=0x%x\n", len);
	else {
		my_printf("Xmodem error\n");
		return 0;
	}
	write_ubl(dest, len, header_present((void *)dest) ?
			(0x400/0x200) :		/* UBL starts at block #2 */
			(0x20000/0x200));	/* eboot starts at block #256 */
	ci.search = ci.initial_buf = dest;
	ci.buf = (dest + len);
	ci.hdr = 0;
	header_search(&ci);
	header_update_end(&ci);
	if (pstart)
		*pstart = ci.dest;
	if (pbytes)
		*pbytes = ci.end - ci.dest;
	if (pivt_offset)
		*pivt_offset = ci.hdr - ci.dest;
	return common_exec_program(&ci) ? 1 : 0;
}
