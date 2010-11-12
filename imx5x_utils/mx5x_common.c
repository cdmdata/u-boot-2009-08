#include <stdarg.h>
//#define DEBUG
#include "mx51_common.h"

#define UART1_BASE 0x73fbc000
#define UTXD 0x0040
#define USR2 0x0098
#define UTS  0x00b4

void flush_uart(void)
{
	unsigned usr2;
	uint uart = UART1_BASE;
	do {
		usr2 = IO_READ(uart, USR2);
	} while (!(usr2 & (1<<3)));
}

void print_buf(char* str, int cnt)
{
	unsigned uts;
	unsigned char ch;
	uint uart = UART1_BASE;
	while (cnt) {
		ch = *str++;
		do {
			uts = IO_READ(uart, UTS);
		} while (uts & (1<<4));
		IO_WRITE(uart, UTXD, ch);
		cnt--;
	}
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
		cnt2 = 1;
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
		for (;;) {
			struct app_header *hdr = (struct app_header *)pinfo->search;
			void *pdest = (void *)&hdr->app_dest_ptr;
			debug_pr("hdr=%x pdest=%x buf=%x block_size=%x\n", hdr, pdest, pinfo->buf, block_size);
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
	return 0;
}

typedef void (*exec_rtn)(void);

void exec_program(struct common_info *pinfo, uint32_t start_block)
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

void exec_dl(unsigned char* base, unsigned length)
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
			exec_program(&ci, 0);
			my_printf("exec_program failed\r\n");
			return;
		}
		base += 0x400;
	} while (base < end);
}
