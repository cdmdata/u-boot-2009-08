#include <stdarg.h>
//#define DEBUG
#include "mx5x_common.h"
#include "mx5x_ecspi.h"

void check_page_size(int base)
{
	unsigned offset_bits; 
	unsigned block_size;
	erase_sector_rtn erase_rtn;

	ecspi_init(base);
	offset_bits = identify_chip_erase_rtn(base, &block_size, &erase_rtn);
	if (offset_bits && (block_size != (1 << offset_bits))) {
		//atmel and not a power of 2
		my_printf("Unsupported page size of 0x%x bytes.\n", block_size);
		atmel_config_p2(base);
		my_printf("Reprogrammed the page size to 0x%x bytes.\n", (1 << (offset_bits - 1)));
		my_printf("Please power cycle the board for the change to take effect.\n");
		for (;;) {}
	}
}

void clearenv(int base, unsigned offset)
{
	unsigned offset_bits; 
	unsigned block_size;
	unsigned page;
	erase_sector_rtn erase_rtn;

	ecspi_init(base);
	offset_bits = identify_chip_erase_rtn(base, &block_size, &erase_rtn);
	page = offset / block_size;
	my_printf("block_size = 0x%x\n", block_size);
	erase_rtn(base, page, offset_bits);
}

int xmodem_load(unsigned char * dest);

#define CONFIG_ENV_SF_SECT_SIZE		(4 * 1024)
#define CONFIG_ENV_SF_OFFSET		((384 * 1024)-CONFIG_ENV_SF_SECT_SIZE)

int main(void)
{
	int base = get_ecspi_base();
	check_page_size(base);
	clearenv(base, CONFIG_ENV_SF_OFFSET);
	flush_uart();
	return 0;
}
