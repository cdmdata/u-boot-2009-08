#include <config.h>
#include <stdarg.h>
//#define DEBUG
#include "mx5x_common.h"
#include "mx5x_i2c.h"
#include "imx.h"

#define UTXD 0x0040
#define USR2 0x0098
#define UTS  0x00b4

void flush_uart(void)
{
	unsigned usr2;
	uint uart = uart_bases[uart_index];
	do {
		usr2 = IO_READ(uart, USR2);
	} while (!(usr2 & (1<<3)));
}

void print_buf(char* str, int cnt)
{
	unsigned uts;
	unsigned char ch;
	uint uart = uart_bases[uart_index];
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

int common_exec_program(struct common_info *pinfo)
{
	exec_rtn rtn;
	rtn = header_get_rtn(pinfo->hdr);
	if (!rtn) {
		unsigned char *ram_base = (unsigned char *)get_ram_base();
		my_printf("common_exec_program failed, hdr=%x\n", pinfo->hdr);
		dump_mem(pinfo->initial_buf, 0x400, 1);
		if (!ram_test((unsigned *)ram_base)) {
			my_printf("ram test failed\r\n");
		}
	} else {
		my_printf("file loaded, app_start %x\r\n", rtn);
	}
	flush_uart();
	return (int)rtn;
}

int is_vbuckcore_boosted(void);

int ram_test(unsigned *ram_base)
{
	unsigned *p = ram_base;
	unsigned *p_end = &ram_base[1<<10];	//test 4K of memory
	unsigned expected;
	unsigned val[4];
	delayMicro(30);		/* 10 gives errors on nitrogen53k, 15 ok */
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
#if defined(CONFIG_ALLOW_VBUCKCORE_BOOST_IF_OLD) || defined(CONFIG_ALLOW_VBUCKCORE_BOOST)
			if (!is_vbuckcore_boosted())
				my_printf("Warning: using old rev code for new board, please update firmware!!!\r\n");
#endif
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

#ifdef CONFIG_MX53
static unsigned char da9052_init_data[] = {
//If LDO10 is turned off then DA9053 accesses will fail
//And it can never be turned back on. Fixed on next board
#ifdef CONFIG_LDO10_OFF
		0x3b, 0x2a,		/* off, LDO10, tfp410(6a:3.3V) */
#else
		0x3b, 0x6a,		/* on,  LDO10, tfp410(6a:3.3V) */
#endif
		0x33, 0x4c,		/* on,  LDO2, 0.893V (4c:0.9V) */
//		0x34, 0x73,		/* on,  LDO3, 3.0V (73:3.0V) default (7f:3.3v) */
		0x2e, 0x61,		/* on,  VBUCKCORE, 1.325V  (0x73:1.775V) if old rev of board*/
		0x2f, 0x62,		/* on,  VBUCK_PRO, 1.350V (62:1.350V) */
		0x30, 0x62,		/* on,  VBUCKMEM, 1.800V (62:1.775V) */
		0x3c, 0x7f,		/* go:core, pro, mem, LDO2, LDO3 */
		0x31, 0x7d,		/* on,  VBUCK_PERI 2.489V (7d:2.450V) */
//		0x32, 0x4e		/* on,  LDO1, 1.3V (4e:1.3V) */
		0x35, 0x73,		/* on, LDO4, audio amp(73:3.0V) */
		0x36, 0x40,		/* on, LDO5, sata(40:1.2V) */
		0x37, 0x40,		/* on,  LDO6 (40:1.2V) */
		0x38, 0x5e,		/* on,  LDO7, 2.75V (5e:2.70V) */
/* LDO8 - leave on 1.8V, serial port messed up otherwise on newest board */
//		0x39, 0x0c,		/* off, LDO8, camera db(4c:1.8V) */
		0x3a, 0x1f,		/* off, LDO9, 2.8V camera(5f:2.8V) */
#ifdef CONFIG_DA9052_CHARGER_ENABLE
		0x3e, 0xd7,		/* turn charger on, max 450mA from USB */
		0x40, 0xf0,		/* max current for DCIN */
		0x41, 0xff,		/* max current for ICHG_BAT, ICHG_PRE */
		0x43, 0x0f,		/* max 450 minutes */
#endif
		0x15, 0xee,		/* GPIO 0/1 open drain high */
		0x16, 0xee,		/* GPIO 2/3 open drain high */
		0x17, 0xee,		/* GPIO 4/5 open drain high */
		0x18, 0xee,		/* GPIO 6/7 open drain high */
		0x19, 0xee,		/* GPIO 8(SYS_EN)/9(PWR_EN) open drain high */
		0x1a, 0xee,		/* GPIO 10/11(OTG_ID) open drain high */
		0x45, 0xae,		/* charge coin cell @1mA to 3.0V */
		0x12, 0xdf,		/* Disable all but backup battery charging during power-down */
		0x46, 0x30,		/* R70 powers up as 0x36, disable LED1/2 boost controler */
		0x47, 0xc0,		/* R71 powers up as 0xcf, turn off led1/2/3 */
		0x0d, 0xd8,		/* Mask GP11/12 wake up event*/
		0x1b, 0x0a,		/* GPIO 12 output, open drain, internal pullup, high */
		0x1b, 0x09,		/* GPIO 12 input, no LDO9_en , active low/ GPIO 13(nVDD_FAULT) */
};

unsigned char vbuckcore_val = 0;

int i2c_write_byte(unsigned i2c_base, unsigned chip, unsigned reg, unsigned char val)
{
	unsigned char buf[4];
	buf[0] = val;
	if (reg == 0x2e)
		vbuckcore_val = val;
	return i2c_write(i2c_base, chip, reg, 1, buf, 1);
}

int i2c_write_array(unsigned i2c_base, unsigned chip, const unsigned char* p, int size)
{
	int ret;
	int i;
	for (i = 0; i < size; i += 2) {
		ret = i2c_write_byte(i2c_base, chip, p[0], p[1]);
		p += 2;
		if (ret)
			return ret;
	}
	return 0;
}

int i2c_read_byte(unsigned i2c_base, unsigned chip, unsigned reg)
{
	int ret;
	unsigned char buf[4];
	ret = i2c_read(i2c_base, chip, reg, 1, buf, 1);
	if (ret)
		return -1;
	if (reg == 0x2e)
		vbuckcore_val = buf[0];
	return buf[0];
}

#if defined(CONFIG_ALLOW_VBUCKCORE_BOOST_IF_OLD) || defined(CONFIG_ALLOW_VBUCKCORE_BOOST)
static const unsigned char da9052_boost_vbuckcore_data[] = {
		0x2e, 0x73,		/* on,  VBUCKCORE (0x73:1.775V) , old rev of board*/
		0x3c, 0x61,		/* go:core */
};
#endif

unsigned gp12_val = 0;

int is_vbuckcore_boosted()
{
	int ret = vbuckcore_val;
	if ((ret & 0x7f) >= 0x60)
		return 1;
	return 0;
}

//Return 0 if voltage was boosted
int vbuckcore_boost(unsigned i2c_base, unsigned chip)
{
	int ret;
#ifdef CONFIG_ALLOW_VBUCKCORE_BOOST_IF_OLD
	unsigned srev = *((int *)0x63f98024); /* silicon revision */
	if (srev >= 3)
		return -5;
#endif
	ret = i2c_read_byte(i2c_base, chip, 0x2e);
	if (ret < 0)
		return ret;
	if (ret == 0x73)
		return 1;
#ifdef CONFIG_ALLOW_VBUCKCORE_BOOST_IF_OLD
	if (gp12_val == 0)
		return ERROR_GP12_LOW;	/* gp12 is grounded (new rev), don't change voltage */
#endif
#if defined(CONFIG_ALLOW_VBUCKCORE_BOOST_IF_OLD) || defined(CONFIG_ALLOW_VBUCKCORE_BOOST)
	ret = i2c_write_array(i2c_base, chip, da9052_boost_vbuckcore_data, sizeof(da9052_boost_vbuckcore_data));
	delayMicro(1000);
#else
	ret = -6;
#endif
	return ret;
}

int power_up_ddr(unsigned i2c_base, unsigned chip)
{
	int ret;
	unsigned srev = *((int *)0x63f98024); /* silicon revision */

	i2c_init(i2c_base, 400000);
	if (srev < 3) {
		ret = i2c_read_byte(i2c_base, chip, 0x2e);
		if (ret > 0) {
			int code = ret & 0x3f;
			if ((code >= 0x30) && (code <= 0x36)) {
				/* don't change vbuckcore, reg 0x2e */
				int i;
				for (i = 0; i < sizeof(da9052_init_data); i+=2) {
					if (da9052_init_data[i] == 0x2e) {
						da9052_init_data[i+1] = (unsigned char)ret;
						break;
					}
				}
			}
		}
	}
	ret = i2c_write_array(i2c_base, chip, da9052_init_data, sizeof(da9052_init_data));
	if (!ret) {
		ret = i2c_read_byte(i2c_base, chip, 0x04);
		if (ret >= 0) {
			if (ret & 0x10)
				gp12_val = 1;
#ifdef CONFIG_GP12_UART_SELECT
			if (uart_index != 1)
				uart_index = gp12_val ? 0 : 2;	/* gp12 high means old rev, uart1,  */
								/* gp12 low means new rev, uart3 */
#endif
			debug_pr("statusd=%x\n", ret);
		}
		i2c_write_byte(i2c_base, chip, 0x1b, 0x0e);	/* gp12 output, open drain, external pullup, high */
	}
//
	delayMicro(1000);
//	vbuckcore_boost(i2c_base, chip);
	return ret;
}
#endif
