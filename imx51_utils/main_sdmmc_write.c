#include <stdarg.h>
#include "mx51_common.h"
#include "mx51_sdmmc.h"

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

int main(void)
{
	unsigned char *dest = (unsigned char *)0x90600000;
	unsigned len = xmodem_load(dest);
	if (len)
		my_printf("OK len=0x%x\n", len);
	else {
		my_printf("Xmodem error\n");
		return -1;
	}
	write_ubl(dest, len, (((struct app_header *)dest)->app_barker == APP_BARKER) ?
			(0x400/0x200) :		/* UBL starts at block #2 */
			(0x20000/0x200));	/* eboot starts at block #256 */
	exec_dl(dest, len);
	dump_mem(dest, 0x800, 14);
	my_printf("hdr not found\n");
	flush_uart();
	return 0;
}
