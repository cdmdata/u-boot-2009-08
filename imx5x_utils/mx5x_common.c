#include <stdarg.h>
//#define DEBUG
#include "mx5x_common.h"

#define UTXD 0x0040
#define USR2 0x0098
#define UTS  0x00b4

void flush_uart(void)
{
	unsigned usr2;
	uint uart = get_uart_base();
	do {
		usr2 = IO_READ(uart, USR2);
	} while (!(usr2 & (1<<3)));
}

void print_buf(char* str, int cnt)
{
	unsigned uts;
	unsigned char ch;
	uint uart = get_uart_base();
	while (cnt) {
		ch = *str++;
		do {
			uts = IO_READ(uart, UTS);
		} while (uts & (1<<4));
		IO_WRITE(uart, UTXD, ch);
		cnt--;
	}
	hw_watchdog_reset();
}

void print_str(char * str)
{
	char *p = str;
	int cnt = 0;
	while (*p++) {
		cnt++;
	}
	print_buf(str, cnt);
}

void __div0(void)
{
	print_str("div by 0\n");
}

void print_hex(unsigned int val, unsigned int character_cnt)
{
	char buf[12];
	char* p = buf;
	if (character_cnt < 8)
		val <<= ((8 - character_cnt) << 2);
	else if (character_cnt > 8)
		character_cnt = 8;

	while (character_cnt) {
		char ch = (val >> 28);
		val <<= 4;
		if (ch <= 9)
			ch += '0';
		else
			ch += 'a' - 10;
		*p++ = ch;
		character_cnt--;
	}
	print_buf(buf, p - buf);
}

void my_printf(char *str, ...)
{
	char *buf = str;
	unsigned char ch;
	unsigned cnt;
	unsigned int arg;
	va_list arglist;

	va_start(arglist, str);
	do {
		ch = *str;
		if (!ch) break;
		if (ch == '%') {
			print_buf(buf, str - buf);
			buf = str;
			str++;
			ch = *str++;
			if (ch == '0')
				ch = *str++;
			cnt = 8;
			if ((ch > '0') && (ch <= '8')) {
				cnt = ch - '0';
				ch = *str++;
			}
			arg = va_arg(arglist, int);
			if (ch == 's')
				print_str((char *)arg);
			else
				print_hex(arg, cnt);
			buf = str;
		} else
			str++;
	} while (1);
	va_end(arglist);
	print_buf(buf, str - buf);
}

void dump_mem(void *buf, uint block, uint block_cnt)
{
	uint8_t ascii_buf[17];
	ascii_buf[16] = 0;
	while (block_cnt) {
		int i;
		uint *p = (uint *)buf;
		my_printf("\nBlock %x, mem=%x\n", block, (uint)buf);
		for (i = 0; i < 512; i += 16, p += 4) {
			int j;
			for (j = 0; j < 16; j++) {
				uint8_t ch = ((uint8_t *)p)[j];
				ascii_buf[j] = ((ch >= 0x20) && (ch <= 0x7f)) ? ch : 0x20;
			}
			my_printf("%3x  %x %x %x %x  ", i, p[0], p[1], p[2], p[3]);
			my_printf((char*)ascii_buf);
			my_printf("\n");
		}
		buf = ((uint8_t *)buf) + 512;
		block_cnt--;
	}
}

void my_memset(unsigned char* p, unsigned val, unsigned count)
{
	while (count) {
		*p++ = val;
		count--;
	}
}

void my_memcpy(unsigned char* dest, unsigned char* src, unsigned count)
{
	while (count) {
		*dest++ = *src++;
		count--;
	}
}

void delayMicro(unsigned cnt)
{
	volatile unsigned cnt2;
	while (cnt--) {
		cnt2 = 100;
		while (cnt2--);
	}
}

void reverse_word(unsigned *dst, int count)
{
	while (count--) {
		unsigned val = *dst;
		*dst++ = (val >> 24) | (val << 24) | (((val >> 8) & 0x0000ff00) | ((val << 8) & 0x00ff0000));
	}
}

void reverse_word2(unsigned *dst, unsigned *src, int count)
{
	while (count--) {
		unsigned val = *src++;
		*dst++ = (val >> 24) | (val << 24) | (((val >> 8) & 0x0000ff00) | ((val << 8) & 0x00ff0000));
	}
}

int common_load_block_of_file(struct common_info *pinfo, unsigned block_size)
{
	pinfo->buf += block_size;
	if (!pinfo->hdr) {
		header_search(pinfo);
	}
	return 0;
}

int ram_test(unsigned *ram_base)
{
	unsigned *p = ram_base;
	unsigned *p_end = &ram_base[1<<10];	//test 4K of memory 
	unsigned expected;
	unsigned val[4];
	while (p < p_end) {
		p[0] = ((unsigned)p);
		p[1] = ~((unsigned)p);
		p[2] = ((unsigned)p) ^ 0xaaaaaaaa;
		p[3] = ((unsigned)p) ^ 0x55555555;
		p += 4;
	}
	p = ram_base;
	for (;;) {
		if (p >= p_end) {
//			my_printf("ram test passed\r\n");
			return 1;
		}
		val[0] = p[0];
		val[1] = p[1];
		val[2] = p[2];
		val[3] = p[3];
		expected = (unsigned)p;
		if (val[0] != expected) break;

		expected = ~((unsigned)p);
		if (val[1] != expected) break;

		expected = ((unsigned)p) ^ 0xaaaaaaaa;
		if (val[2] != expected) break;

		expected = ((unsigned)p) ^ 0x55555555; 
		if (val[3] != expected) break;
		p += 4;
	}
	my_printf(" %08x: %08x %08x %08x %08x, %08x\r\n", p, val[0], val[1], val[2], val[3], expected);
	return 0;
}
