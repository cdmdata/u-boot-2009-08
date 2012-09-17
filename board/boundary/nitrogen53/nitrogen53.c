/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/mx53.h>
#include <asm/arch/mx53_pins.h>
#include <asm/arch/iomux.h>
#include <asm/arch/iomux-mx53.h>
#include <asm/gpio.h>
#include <asm/imx-common/mxc_i2c.h>
#include <asm/imx-common/boot_mode.h>
#include <asm/errno.h>
#include <imx_spi.h>
#include <netdev.h>
#include <malloc.h>
#include "da9052.h"
#include "bq2416x.h"

#define GPIO_NUMBER(port, offset) (((port - 1) << 5) | offset)

#define GP_LCD_BACKLIGHT		GPIO_NUMBER(2, 16)	/* NitrogenA, EIM_A22 */

#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_A
#define I2C2_HUB_PIC16F616_TOUCH	GPIO_NUMBER(3, 7)	/* EIM_DA7 */
#define I2C2_HUB_CAMERA			GPIO_NUMBER(3, 10)	/* EIM_DA10 */
#define I2C2_HUB_TFP410_ACCEL		GPIO_NUMBER(3, 11)	/* EIM_DA11 */
#define I2C2_HUB_BATT_EDID		GPIO_NUMBER(6, 11)	/* NANDF_CS0 */
#define I2C2_HUB_OLD_EMPTY		GPIO_NUMBER(3, 9)	/* EIM_DA9 */
#define I2C2_HUB_RTC_ISL1208		GPIO_NUMBER(6, 12)	/* NANDF_WE */
#define I2C3_HUB_SC16IS7XX		GPIO_NUMBER(6, 10)	/* NANDF_RB0 */
#define GP_LCD_3_3V_POWER_ENABLE	GPIO_NUMBER(2, 6)	/* PATA_DATA6 */
#define GP_BT_RESET			GPIO_NUMBER(3, 3)	/* EIM_DA3 */
#endif

#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
#define N53K_I2C2_HUB_EDID		GPIO_NUMBER(3, 8)	/* EIM_DA8 */
#define N53K_I2C2_HUB_BATTERY		GPIO_NUMBER(3, 9)	/* EIM_DA9 */
#define N53K_I2C2_HUB_AMBIENT		GPIO_NUMBER(3, 10)	/* EIM_DA10 */
#define N53K_I2C2_HUB_CAMERA		GPIO_NUMBER(6, 10)	/* NANDF_RB0 */
#define N53K_POWER_KEEP_ON		GPIO_NUMBER(6, 12)	/* NANDF_WE */
#endif


//#define DEBUG		//if enabled, also enable in start.S
#ifdef DEBUG
void TransmitX(char ch);
#define debug_putc(ch) TransmitX(ch)
#else
#define debug_putc(ch)
#endif

#define N53_BUTTON_100KPU_PAD_CTL	(PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_100K_PU)

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

#include <asm/imx_iim.h>

#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include "../../freescale/common/recovery.h"
#include <part.h>
#include <ext2fs.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <ubi_uboot.h>
#include <jffs2/load_kernel.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static u32 system_rev;
static enum boot_device boot_dev;

#define GPIO_DIR 4

unsigned get_machid(void)
{
	char *s = getenv ("machid");
	unsigned machid = (s) ? simple_strtoul(s, NULL, 16) : CONFIG_MACH_TYPE;
	return machid;
}

static inline void setup_boot_device(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	uint bt_mem_ctl = (soc_sbmr & 0x000000FF) >> 4 ;
	uint bt_mem_type = (soc_sbmr & 0x00000008) >> 3;

	switch (bt_mem_ctl) {
	case 0x0:
		if (bt_mem_type)
			boot_dev = ONE_NAND_BOOT;
		else
			boot_dev = WEIM_NOR_BOOT;
		break;
	case 0x2:
		if (bt_mem_type)
			boot_dev = SATA_BOOT;
		else
			boot_dev = PATA_BOOT;
		break;
	case 0x3:
		if (bt_mem_type)
			boot_dev = SPI_NOR_BOOT;
		else
			boot_dev = I2C_BOOT;
		break;
	case 0x4:
	case 0x5:
		boot_dev = SD_BOOT;
		break;
	case 0x6:
	case 0x7:
		boot_dev = MMC_BOOT;
		break;
	case 0x8 ... 0xf:
		boot_dev = NAND_BOOT;
		break;
	default:
		boot_dev = UNKNOWN_BOOT;
		break;
	}
}

enum boot_device get_boot_device(void)
{
	return boot_dev;
}

u32 get_board_rev(void)
{
	return system_rev;
}

static inline void setup_soc_rev(void)
{
	int reg;

	/* Si rev is obtained from ROM */
	reg = __REG(ROM_SI_REV);

	switch (reg) {
	case 0x10:
		system_rev = 0x53000 | CHIP_REV_1_0;
		break;
	case 0x20:
		system_rev = 0x53000 | CHIP_REV_2_0;
		break;
	default:
		system_rev = 0x53000 | CHIP_REV_2_0;
	}
}

inline int is_soc_rev(int rev)
{
	return (system_rev & 0xFF) - rev;
}

#ifdef CONFIG_ARCH_MMU
void board_mmu_init(unsigned long ttb_base)
{
	unsigned long i;

	/*
	* Set the TTB register
	*/
	asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb_base) /*:*/);

	/*
	* Set the Domain Access Control Register
	*/
	i = ARM_ACCESS_DACR_DEFAULT;
	asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

	/*
	* First clear all TT entries - ie Set them to Faulting
	*/
	memset((void *)ttb_base, 0, ARM_FIRST_LEVEL_PAGE_TABLE_SIZE);
	/* Actual   Virtual  Size   Attributes          Function */
	/* Base     Base     MB     cached? buffered?  access permissions */
	/* xxx00000 xxx00000 */
	X_ARM_MMU_SECTION(0x000, 0x000, 0x10,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* ROM, 16M */
	X_ARM_MMU_SECTION(0x070, 0x070, 0x010,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IRAM */
	X_ARM_MMU_SECTION(0x100, 0x100, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* SATA */
	X_ARM_MMU_SECTION(0x180, 0x180, 0x100,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* IPUv3M */
	X_ARM_MMU_SECTION(0x200, 0x200, 0x200,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* GPU */
	X_ARM_MMU_SECTION(0x400, 0x400, 0x300,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* periperals */
	X_ARM_MMU_SECTION(0x700, 0x700, 0x400,
			ARM_CACHEABLE, ARM_BUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 1G */
	X_ARM_MMU_SECTION(0x700, 0xB00, 0x400,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CSD0 1G */
	X_ARM_MMU_SECTION(0xF00, 0xF00, 0x100,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* CS1 EIM control*/
	X_ARM_MMU_SECTION(0xF7F, 0xF7F, 0x040,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* NAND Flash buffer */
	X_ARM_MMU_SECTION(0xF80, 0xF80, 0x001,
			ARM_UNCACHEABLE, ARM_UNBUFFERABLE,
			ARM_ACCESS_PERM_RW_RW); /* iRam */

	/* Workaround for arm errata #709718 */
	/* Setup PRRR so device is always mapped to non-shared */
	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r"(i) : /*:*/);
	i &= (~(3 << 0x10));
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(i) /*:*/);

	/* Enable MMU */
	MMU_ON();
}
#endif

#define ESD_BASE	0x63FD9000
#define ESD_ZQHWCTRL	0x040
#define ESD_CTL		0x00
#define ESD_MISC	0x18

int dram_init(void)
{
///	.word	0x63FD9000, 0x83110000	/* ESDCTL, (3)14 rows, (1)10 columns (0)BL 4,  (1)32 bit width */
	unsigned base = ESD_BASE;
	unsigned shift;
	unsigned esdctl = readl(base + ESD_CTL);
	shift = (esdctl >> 30) & 1;		//0 = cs1 bit
	shift += ((esdctl >> 24) & 7) + 11;	//3+11 = 14 rows
	shift += ((esdctl >> 20) & 7) + 9;	//1+9 = 10 columns
	shift += ((esdctl >> 16) & 1) + 1;	//1+1 = 2 memory width (32 bit wide)
	shift += (((readl(base + ESD_MISC) >> 5) & 1) ^ 1) + 2;	//1+2 = 3 bank bits
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = 1 << shift;
	return 0;
}

/*
 * continued fraction
 *      2  1  2  1  2
 * 0 1  2  3  8 11 30
 * 1 0  1  1  3  4 11
 */
static void get_best_ratio(unsigned *pnum, unsigned *pdenom, unsigned max)
{
	unsigned a = *pnum;
	unsigned b = *pdenom;
	unsigned c;
	unsigned n[] = {0, 1};
	unsigned d[] = {1, 0};
	unsigned whole;
	unsigned i = 1;
	while (b) {
		i ^= 1;
		whole = a / b;
		n[i] += (n[i ^ 1] * whole);
		d[i] += (d[i ^ 1] * whole);
//		printf("cf=%i n=%i d=%i\n", whole, n[i], d[i]);
		if ((n[i] | d[i]) > max) {
			i ^= 1;
			break;
		}
		c = a - (b * whole);
		a = b;
		b = c;
	}
	*pnum = n[i];
	*pdenom = d[i];
}

#if defined(CONFIG_I2C_MXC) && defined(CONFIG_UART_DA9052_GP12)
unsigned uart_base = 0;
#endif

unsigned get_uart_base(void)
{
#if defined(CONFIG_I2C_MXC) && defined(CONFIG_UART_DA9052_GP12)
	unsigned char buf[4];
	unsigned base;
	if (uart_base)
		return uart_base;
	/* Prevent recursion if i2c prints message */
	uart_base = UART3_BASE_ADDR;
	buf[0] = 0x09;		/* gp12 input, no LDO9_en , active low */

	if (bus_i2c_write(DA90_I2C_BUS, DA90_I2C_ADDR, 0x1b, 1, buf, 1)) {
		printf("reg 27(gp12-13) of DA9053 failed\n");
	} else {
		if (bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR, 0x04, 1, buf, 1)) {
			printf("reg 4(statusd) of DA9053 failed\n");
		} else {
			int i = (buf[0] >> 4) & 1;
			base =  i ? UART1_BASE_ADDR : UART3_BASE_ADDR;
			uart_base = base;
			system_rev |= ((i ^ 1) + 1) << 8;
			return base;
		}
	}
#endif
#ifdef CONFIG_UART_BASE_ADDR
	return CONFIG_UART_BASE_ADDR;
#else
	return UART1_BASE_ADDR;
#endif
}

#define UFCR           0x0090
#define UBIR           0x00a4
#define UBMR           0x00a8

static void setup_uart(void)
{
	u32 uart = get_uart_base();
	u32 clk_src = mxc_get_clock(MXC_UART_CLK);
	u32 mult, div;
	unsigned int pad = PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
			 PAD_CTL_PUE_PULL | PAD_CTL_100K_PU | PAD_CTL_DRV_HIGH;
#define BAUDRATE 115200

	debug_putc('A');
	writel(0x4 << 7, uart + UFCR);	/* divide input clock by 2 */
	mult = BAUDRATE * 2 * 16;	/* 16 samples/clock */
	div = clk_src;
//	writel(611 - 1, uart + UBIR);
//	writel(11022 - 1, uart + UBMR);
	get_best_ratio(&mult, &div, 0x10000);
	writel(mult - 1, uart + UBIR);
	writel(div - 1, uart + UBMR);

	/* UART1 RXD */
	mxc_request_iomux(MX53_PIN_ATA_DMACK, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_pad(MX53_PIN_ATA_DMACK, pad);
	mxc_iomux_set_input(MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT, 0x3);

	/* UART1 TXD */
	mxc_request_iomux(MX53_PIN_ATA_DIOW, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_pad(MX53_PIN_ATA_DIOW, pad);

	/* UART2 RXD */
	mxc_request_iomux(MX53_PIN_ATA_BUFFER_EN, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_pad(MX53_PIN_ATA_BUFFER_EN, pad);
	mxc_iomux_set_input(MUX_IN_UART2_IPP_UART_RXD_MUX_SELECT_INPUT, 0x3);

	/* UART2 TXD */
	mxc_request_iomux(MX53_PIN_ATA_DMARQ, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_pad(MX53_PIN_ATA_DMARQ, pad);

	/* UART2 CTS */
	mxc_request_iomux(MX53_PIN_ATA_INTRQ, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_pad(MX53_PIN_ATA_INTRQ, pad);

	/* UART2 RTS */
	mxc_request_iomux(MX53_PIN_ATA_DIOR, IOMUX_CONFIG_ALT3);
	mxc_iomux_set_pad(MX53_PIN_ATA_DIOR, pad);
	mxc_iomux_set_input(MUX_IN_UART2_IPP_UART_RTS_B_SELECT_INPUT, 0x3);

#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_A
	if ((system_rev & 0xf00) == 0x100) {
#else
#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
	if (0) {
#else
	if (1) {
#endif
#endif
		/* Nitrogen53/ Nitrogen A rev 1 */
		/* UART3 RXD */
		mxc_request_iomux(MX53_PIN_ATA_CS_1, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_ATA_CS_1, pad);
		mxc_iomux_set_input(MUX_IN_UART3_IPP_UART_RXD_MUX_SELECT_INPUT, 0x3);

		/* UART3 TXD */
		mxc_request_iomux(MX53_PIN_ATA_CS_0, IOMUX_CONFIG_ALT4);
		mxc_iomux_set_pad(MX53_PIN_ATA_CS_0, pad);
	} else {
		/* UART3 RXD */
		mxc_request_iomux(MX53_PIN_EIM_D25, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX53_PIN_EIM_D25, pad);
		mxc_iomux_set_input(MUX_IN_UART3_IPP_UART_RXD_MUX_SELECT_INPUT, 0x1);

		/* UART3 TXD */
		mxc_request_iomux(MX53_PIN_EIM_D24, IOMUX_CONFIG_ALT2);
		mxc_iomux_set_pad(MX53_PIN_EIM_D24, pad);
	}
//	mxc_request_iomux(MX51_PIN_UART1_RTS, IOMUX_CONFIG_ALT0);
//	mxc_iomux_set_pad(MX51_PIN_UART1_RTS, pad);
//	mxc_request_iomux(MX51_PIN_UART1_CTS, IOMUX_CONFIG_ALT0);
//	mxc_iomux_set_pad(MX51_PIN_UART1_CTS, pad);

	/* enable GPIO1_9 for CLK0 and GPIO1_8 for CLK02 */
//	writel(0x00000004, 0x73fa83e8);
//	writel(0x00000004, 0x73fa83ec);

#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
	mxc_request_iomux(MX53_PIN_EIM_DA2, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA2, PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_360K_PD);
#endif

	udelay(10);
	printf("setup_uart clk_src=%i mult=%i div=%i\n", clk_src, mult, div);
}

#ifdef CONFIG_W3
void w3_setup(void)
{
	gpio_direction_output(CONFIG_W3_CS, 1);
	gpio_direction_input(CONFIG_W3_SCL);
	gpio_direction_input(CONFIG_W3_SDA);
	mxc_iomux_set_pad(CONFIG_W3_CS_PIN, 0x1ec);
	mxc_iomux_set_pad(CONFIG_W3_SCL_PIN, 0x1ec);
	mxc_iomux_set_pad(CONFIG_W3_SDA_PIN, 0x1ec);
	mxc_request_iomux(CONFIG_W3_CS_PIN, 1 | IOMUX_CONFIG_SION);
	mxc_request_iomux(CONFIG_W3_SCL_PIN, 1 | IOMUX_CONFIG_SION);
	mxc_request_iomux(CONFIG_W3_SDA_PIN, 1 | IOMUX_CONFIG_SION);
}

void w3_write(unsigned reg, unsigned data)
{
	int i;
	unsigned val = (reg << 10) | (data & 0xff);
	gpio_direction_output(CONFIG_W3_SCL, 1);
	gpio_direction_output(CONFIG_W3_SDA, 1);
	gpio_direction_output(CONFIG_W3_CS, 0);
	for (i = 0; i < 16; i++) {
		gpio_direction_output(CONFIG_W3_SCL, 0);
		gpio_direction_output(CONFIG_W3_SDA, (val >> 15) & 1);
		udelay(10);
		gpio_direction_output(CONFIG_W3_SCL, 1);
		val <<= 1;
		udelay(10);
	}
	gpio_direction_output(CONFIG_W3_CS, 1);
	gpio_direction_input(CONFIG_W3_SCL);
	gpio_direction_input(CONFIG_W3_SDA);
	udelay(20);
}

unsigned w3_read(unsigned reg)
{
	int i;
	unsigned val = (reg << 10) | 0x3ff;
	gpio_direction_output(CONFIG_W3_SCL, 1);
	gpio_direction_output(CONFIG_W3_SDA, 1);
	gpio_direction_output(CONFIG_W3_CS, 0);
	for (i = 0; i < 16; i++) {
		gpio_direction_output(CONFIG_W3_SCL, 0);
		if (i >= 7)
			gpio_direction_input(CONFIG_W3_SDA);
		else
			gpio_direction_output(CONFIG_W3_SDA, (val >> 15) & 1);
		udelay(10);
		gpio_direction_output(CONFIG_W3_SCL, 1);
		val <<= 1;
		if (i >= 7)
			val |= gpio_get_value(CONFIG_W3_SDA);
		udelay(10);
	}
	gpio_direction_output(CONFIG_W3_CS, 1);
	gpio_direction_input(CONFIG_W3_SCL);
	gpio_direction_input(CONFIG_W3_SDA);
	udelay(20);
	return val & 0xff;
}

void w3_enable_panel(void)
{
	w3_setup();
	w3_write(0x00, 0x29);
	w3_write(0x00, 0x25);
	w3_write(0x02, 0x40);
	w3_write(0x01, 0x30);
	w3_write(0x0e, 0x5f);
	w3_write(0x0f, 0xa4);
	w3_write(0x0d, 0x09);
	w3_write(0x10, 0x41);
	udelay(100*1000);
	w3_write(0x00, 0xad);
}

int do_w3read(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (2 == argc) {
		unsigned reg = simple_strtoul(argv[1], NULL, 16);
		unsigned val;
		w3_setup();
		val = w3_read(reg);
		printf("reg(%02x) = 0x%02x\n", reg, val);
	} else
		cmd_usage(cmdtp);
	return 0 ;
}

U_BOOT_CMD(
	w3read, 3, 0, do_w3read,
	"3-wire spi read",
	"Usage: w3read 1f\n"
	"This example will read reg 0x1f\n"
);

int do_w3write(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (3 == argc) {
		unsigned reg = simple_strtoul(argv[1], NULL, 16);
		unsigned val = simple_strtoul(argv[2], NULL, 16);
		w3_setup();
		w3_write(reg, val);
	} else
		cmd_usage(cmdtp);
	return 0 ;
}

U_BOOT_CMD(
	w3write, 3, 0, do_w3write,
	"3-wire spi write",
	"Usage: w3write 1f 02\n"
	"This example will set reg 0x1f to 0x02\n"
);
#endif

static int const di0_prgb_pins[] = {
	MX53_PIN_DISP0_DAT0,
	MX53_PIN_DISP0_DAT1,
	MX53_PIN_DISP0_DAT2,
	MX53_PIN_DISP0_DAT3,
	MX53_PIN_DISP0_DAT4,
	MX53_PIN_DISP0_DAT5,
	MX53_PIN_DISP0_DAT6,
	MX53_PIN_DISP0_DAT7,
	MX53_PIN_DISP0_DAT8,
	MX53_PIN_DISP0_DAT9,
	MX53_PIN_DISP0_DAT10,
	MX53_PIN_DISP0_DAT11,
	MX53_PIN_DISP0_DAT12,
	MX53_PIN_DISP0_DAT13,
	MX53_PIN_DISP0_DAT14,
	MX53_PIN_DISP0_DAT15,
	MX53_PIN_DISP0_DAT16,
	MX53_PIN_DISP0_DAT17,
	MX53_PIN_DISP0_DAT18,
	MX53_PIN_DISP0_DAT19,
	MX53_PIN_DISP0_DAT20,
	MX53_PIN_DISP0_DAT21,
	MX53_PIN_DISP0_DAT22,
	MX53_PIN_DISP0_DAT23,
	MX53_PIN_DI0_PIN2,
	MX53_PIN_DI0_PIN3,
	MX53_PIN_DI0_DISP_CLK,
	MX53_PIN_DI0_PIN15,
	0
};

void backlight_state(int enable)
{
#ifdef GP_LCD_BACKLIGHT
	/* Some boards enable backlight power supply with this (NitrogenA) */
	gpio_set_value(GP_LCD_BACKLIGHT, enable);	/* high active */
#endif
#ifdef GP_LCD_3_3V_POWER_ENABLE
	gpio_set_value(GP_LCD_3_3V_POWER_ENABLE, enable); /* high active */
#endif
}

void init_display_pins(void)
{
	unsigned machid = get_machid();
#ifdef CONFIG_TFP410_BUS
	void *tfp410_bus = CONFIG_TFP410_BUS;
	unsigned tfp410_i2c_addr = 0x38;
#endif
	unsigned char buf[4];
	unsigned int pad = PAD_CTL_HYS_NONE | PAD_CTL_DRV_MEDIUM | PAD_CTL_SRE_FAST ;
	int const *pins = di0_prgb_pins ;
	unsigned pwm_base = PWM2_BASE_ADDR;
	while (*pins) {
		mxc_request_iomux(*pins,IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(*pins,pad);
		pins++ ;
	}

	gpio_direction_output(GPIO_NUMBER(2, 29), 1);		//tfp410, i2c_mode
	gpio_direction_input(GPIO_NUMBER(4,15));			//tfp410, interrupt
	mxc_request_iomux(MX53_PIN_EIM_EB1, IOMUX_CONFIG_ALT1);		//gpio2[29] - i2c_mode
	mxc_iomux_set_pad(MX53_PIN_EIM_EB1, PAD_CTL_HYS_ENABLE | PAD_CTL_DRV_HIGH);
	mxc_request_iomux(MX53_PIN_KEY_ROW4, IOMUX_CONFIG_ALT1);	//gpio4[15] - interrupt pin
	mxc_iomux_set_pad(MX53_PIN_KEY_ROW4, PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE);	//pullup disabled

	/* PWM backlight */
	mxc_request_iomux(MX53_PIN_GPIO_1, IOMUX_CONFIG_ALT4);
	mxc_iomux_set_pad(MX53_PIN_GPIO_1, PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE);	//pullup disabled

	/* backlight power enable for GE board, rts on UART3 for nitrogen53 */
	gpio_direction_input(GPIO_NUMBER(3, 31));
	mxc_request_iomux(MX53_PIN_EIM_D31, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_D31, PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL);

	/* gpio2[20] - Display enable for chimei 7" panel */
	gpio_direction_output(GPIO_NUMBER(2, 20), 1);
	mxc_request_iomux(MX53_PIN_EIM_A18, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_A18, PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE);	//pullup disabled

#define PWMCR	0x0
#define PWMSR	0x4
#define PWMIR	0x8
#define PWMSAR	0x0c
#define PWMPR	0x10
#define PWMCNR	0x14
	writel(0x03c20000, pwm_base + PWMCR);
	writel(0, pwm_base + PWMIR);
	writel((CONFIG_PWM2_PERIOD * CONFIG_PWM2_DUTY) >> 8, pwm_base + PWMSAR);
	writel(CONFIG_PWM2_PERIOD - 2, pwm_base + PWMPR);
	writel(0x03c20001, pwm_base + PWMCR);

#ifdef CONFIG_TFP410_BUS
	/* Power up LDO10 of DA9053 for tfp410 */
#ifdef CONFIG_TFP410_LDO10
	buf[0] = 0x6a;
	if (bus_i2c_write(DA90_I2C_BUS, DA90_I2C_ADDR, 0x3b, 1, buf, 1))
		printf("LDO10 reg of DA9053 failed\n");
	udelay(500);
#endif
#ifdef CONFIG_TFP410_HUB_EN
	gpio_direction_output(CONFIG_TFP410_HUB_EN, 1);		/* Enable */
#endif

#ifdef N53_I2C_CONNECTOR_BUFFER_ENABLE
	gpio_direction_output(N53_I2C_CONNECTOR_BUFFER_ENABLE, 0);	//disable external i2c connector
	mxc_request_iomux(PIN_N53_I2C_CONNECTOR_BUFFER, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(PIN_N53_I2C_CONNECTOR_BUFFER, PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE);	//pullup disabled
#endif
	gpio_direction_output(GPIO_NUMBER(2, 29), 0);		//tfp410, i2c_mode
	udelay(5);
	gpio_direction_output(GPIO_NUMBER(2, 29), 1);		//tfp410 low to high is reset, i2c sel mode
	if (machid == MACH_TYPE_MX53_NITROGEN_V1)
		tfp410_bus = (void *)I2C1_BASE_ADDR;

	/* Init tfp410 */
	buf[0] = 0xbd;
	for (;;) {
		if (!bus_i2c_write(tfp410_bus, tfp410_i2c_addr, 0x8, 1, buf, 1)) {
			printf("tfp410 found at 0x%x\n", tfp410_i2c_addr);
			break;
		}
		tfp410_i2c_addr++;
		if (tfp410_i2c_addr > 0x39) {
			printf("tfp410 init failed, machid = %x\n", machid);
			gpio_direction_output(GPIO_NUMBER(2, 29), 0);		//put back into non-i2c mode
			break;
		}
	}
#ifdef N53_I2C_CONNECTOR_BUFFER_ENABLE
	gpio_direction_output(N53_I2C_CONNECTOR_BUFFER_ENABLE, 1);	//reenable external i2c connector
#endif
#ifdef CONFIG_TFP410_HUB_EN
	gpio_direction_output(CONFIG_TFP410_HUB_EN, 0);		/* Disable */
#endif
#endif
}

static int const di0_lvds_pins[] = {
	MX53_PIN_LVDS0_TX3_P,
	MX53_PIN_LVDS0_CLK_P,
	MX53_PIN_LVDS0_TX2_P,
	MX53_PIN_LVDS0_TX1_P,
	MX53_PIN_LVDS0_TX0_P,
	0
};

static int const di1_lvds_pins[] = {
	MX53_PIN_LVDS1_TX3_P,
	MX53_PIN_LVDS1_TX2_P,
	MX53_PIN_LVDS1_CLK_P,
	MX53_PIN_LVDS1_TX1_P,
	MX53_PIN_LVDS1_TX0_P,
	0
};

static int const *const lvds_pins[] = {
	di0_lvds_pins,
	di1_lvds_pins
};

void init_lvds_pins(int ch,int lvds)
{
	int const *pins = lvds_pins[ch&1];
	int const alt = 0 != lvds ? IOMUX_CONFIG_ALT1 : IOMUX_CONFIG_ALT0 ;
	while (*pins) {
		mxc_request_iomux(*pins,alt);
		pins++ ;
	}
	//don't do below, allow both lvds and hdmi to work simultaneously
	if (0) if (!ch) {
		unsigned req_alt = (lvds) ? IOMUX_CONFIG_ALT1 : IOMUX_CONFIG_ALT0;
		pins = di0_prgb_pins ;
		while (*pins) {
			mxc_request_iomux(*pins, req_alt); /* GPIO, or display */
			pins++ ;
		}
	}
#ifdef CONFIG_W3
	if (!ch)
		w3_enable_panel();
#endif
}

void setup_core_voltages(void)
{
	/* Raise the core frequency to highest */
	writel(0x0, CCM_BASE_ADDR + CLKCTL_CACRR);
}

#ifdef CONFIG_IMX_ECSPI
s32 spi_get_cfg(struct imx_spi_dev_t *dev)
{
	dev->base = CSPI1_BASE_ADDR;
	dev->freq = 54000000;
	dev->fifo_sz = 64 * 4;
	dev->us_delay = 0;
	switch (dev->slave.cs) {
	case 0:
		/* pmic */
		dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
		dev->ss = 0;
		break;
	case 1:
		/* spi_nor */
		dev->ss_pol = IMX_SPI_ACTIVE_LOW;
		dev->ss = 1;
		break;
	default:
		printf("Invalid Bus ID! \n");
		break;
	}
	return 0;
}

void spi_io_init(struct imx_spi_dev_t *dev, int active)
{
	if (dev->ss == 1) {
		gpio_direction_output(GPIO_NUMBER(3, 19), active ? 0 : 1);
	}
}

void setup_spi(void)
{
	gpio_direction_output(GPIO_NUMBER(3, 19), 1);	/* SS1 low active */
	/* de-select SS1 of instance: ecspi1. */
	mxc_request_iomux(MX53_PIN_EIM_D19, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_D19, 0x104);
	mxc_iomux_set_input(MUX_IN_ECSPI1_IPP_IND_SS_B_2_SELECT_INPUT, 0x2);

	/* Select mux mode: ALT4 mux port: MOSI of instance: ecspi1. */
	mxc_request_iomux(MX53_PIN_EIM_D18, IOMUX_CONFIG_ALT4);
	mxc_iomux_set_pad(MX53_PIN_EIM_D18, 0x104);
	mxc_iomux_set_input(MUX_IN_ECSPI1_IPP_IND_MOSI_SELECT_INPUT, 0x3);

	/* Select mux mode: ALT4 mux port: MISO of instance: ecspi1. */
	mxc_request_iomux(MX53_PIN_EIM_D17, IOMUX_CONFIG_ALT4);
	mxc_iomux_set_pad(MX53_PIN_EIM_D17, 0x104);
	mxc_iomux_set_input(MUX_IN_ECSPI1_IPP_IND_MISO_SELECT_INPUT, 0x3);

	/* Select mux mode: ALT0 mux port: SCLK of instance: ecspi1. */
	mxc_request_iomux(MX53_PIN_EIM_D16, IOMUX_CONFIG_ALT4);
	mxc_iomux_set_pad(MX53_PIN_EIM_D16, 0x104);
	mxc_iomux_set_input(MUX_IN_CSPI_IPP_CSPI_CLK_IN_SELECT_INPUT, 0x3);
}
#endif

#ifdef CONFIG_MXC_FEC

static void setup_fec(void)
{
	/* gp7[13] - low active reset pin*/
	gpio_direction_output(GPIO_NUMBER(7, 13), 0);
	mxc_request_iomux(MX53_PIN_GPIO_18, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_GPIO_18, 0x0);

	/* FEC TX_CLK, input */
	mxc_request_iomux(MX53_PIN_FEC_REF_CLK, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_REF_CLK, 0x180);

	/* FEC TX_EN, output */
	mxc_request_iomux(MX53_PIN_FEC_TX_EN, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_TX_EN, 0x004);

	/* FEC TXD0, output */
	mxc_request_iomux(MX53_PIN_FEC_TXD0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_TXD0, 0x004);

	 /* FEC TXD1, output */
	mxc_request_iomux(MX53_PIN_FEC_TXD1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_TXD1, 0x004);

	 /* FEC TXD2, output */
	mxc_request_iomux(MX53_PIN_KEY_ROW2, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX53_PIN_KEY_ROW2, 0x004);

	 /* FEC TXD3, output */
	mxc_request_iomux(MX53_PIN_GPIO_19, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX53_PIN_GPIO_19, 0x004);

	/* FEC TX_ER - unused output from mx53 */

	/* FEC COL, input */
	mxc_request_iomux(MX53_PIN_KEY_ROW1, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX53_PIN_KEY_ROW1, 0x180);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_COL_SELECT_INPUT, 0);

	/* FEC CRS, input */
	mxc_request_iomux(MX53_PIN_KEY_COL3, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX53_PIN_KEY_COL3, 0x180);

	/* FEC RX_CLK, input */
	mxc_request_iomux(MX53_PIN_KEY_COL1, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX53_PIN_KEY_COL1, 0x180);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_RX_CLK_SELECT_INPUT, 0);

	/* FEC RXDV (CRS_DV), input */
	mxc_request_iomux(MX53_PIN_FEC_CRS_DV, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_CRS_DV, 0x180);

	/* FEC RXD0, input */
	mxc_request_iomux(MX53_PIN_FEC_RXD0, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_RXD0, 0x180);

	/* FEC RXD1, input */
	mxc_request_iomux(MX53_PIN_FEC_RXD1, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_RXD1, 0x180);

	/* FEC RXD2, input */
	mxc_request_iomux(MX53_PIN_KEY_COL2, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX53_PIN_KEY_COL2, 0x180);

	/* FEC RXD3, input */
	mxc_request_iomux(MX53_PIN_KEY_COL0, IOMUX_CONFIG_ALT6);
	mxc_iomux_set_pad(MX53_PIN_KEY_COL0, 0x180);

	/* FEC RX_ER, input */
	mxc_request_iomux(MX53_PIN_FEC_RX_ER, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_RX_ER, 0x180);

	/*FEC_MDC, output */
	mxc_request_iomux(MX53_PIN_FEC_MDC, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_MDC, 0x004);

	/*FEC_MDIO, i/o */
	mxc_request_iomux(MX53_PIN_FEC_MDIO, IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX53_PIN_FEC_MDIO, 0x1FC);
	mxc_iomux_set_input(MUX_IN_FEC_FEC_MDI_SELECT_INPUT, 0x1);


	udelay(50);
	gpio_direction_output(GPIO_NUMBER(7, 13), 1);
}
#endif

#if defined(CONFIG_MXC_KPD)
int setup_mxc_kpd(void)
{
	mxc_request_iomux(MX53_PIN_KEY_COL0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_COL1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_COL2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_COL3, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_COL4, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_GPIO_19,  IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_ROW0, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_ROW1, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_ROW2, IOMUX_CONFIG_ALT0);
	mxc_request_iomux(MX53_PIN_KEY_ROW3, IOMUX_CONFIG_ALT0);

	return 0;
}
#endif

#ifdef CONFIG_NET_MULTI
int board_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_SMC911X)
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
#endif

	return rc;
}
#endif



#ifdef CONFIG_CMD_MMC

struct fsl_esdhc_cfg esdhc_cfg[2] = {
	{MMC_SDHC1_BASE_ADDR, 1, 1},
#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
	{MMC_SDHC4_BASE_ADDR, 1, 1},
#else
	{MMC_SDHC3_BASE_ADDR, 1, 1},
#endif
};

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	return (soc_sbmr & 0x00300000)  ? 1 : 0;
}
#endif
#undef MX53_SDHC_PAD_CTRL
#define MX53_SDHC_PAD_CTRL      (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | \
		PAD_CTL_75K_PU | PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST)

int esdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;
	struct mmc *mmc;

	for (index = 0; index < CONFIG_SYS_FSL_ESDHC_NUM;
		++index) {
		switch (index) {
		case 0:
			mxc_request_iomux(MX53_PIN_SD1_CMD, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_CLK, IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA0,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA1,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA2,
						IOMUX_CONFIG_ALT0);
			mxc_request_iomux(MX53_PIN_SD1_DATA3,
						IOMUX_CONFIG_ALT0);

			mxc_iomux_set_pad(MX53_PIN_SD1_CMD, MX53_SDHC_PAD_CTRL);
			mxc_iomux_set_pad(MX53_PIN_SD1_CLK, MX53_SDHC_PAD_CTRL);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA0, MX53_SDHC_PAD_CTRL);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA1, MX53_SDHC_PAD_CTRL);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA2, MX53_SDHC_PAD_CTRL);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA3, MX53_SDHC_PAD_CTRL);
			break;
		case 1:
/* pull-up off */
#define WL12XX_IRQ_PAD_CTL	(PAD_CTL_HYS_ENABLE | \
		PAD_CTL_PUE_PULL | PAD_CTL_100K_PU | PAD_CTL_DRV_MEDIUM)
#define WL12XX_BTFUNC2_PAD_CTL	(PAD_CTL_HYS_ENABLE | \
		PAD_CTL_PUE_PULL | PAD_CTL_100K_PU | PAD_CTL_DRV_MEDIUM)
#define WL12XX_BTFUNC5_PAD_CTL	(PAD_CTL_HYS_ENABLE | \
		PAD_CTL_PUE_PULL | PAD_CTL_100K_PU | PAD_CTL_DRV_MEDIUM)

/*
 * WL1271(TiWi) wireless LAN/BT,
 */
#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
#ifdef CONFIG_K2
#define N53_WL1271_WL_EN	GPIO_NUMBER(2, 3)	/* ATA_DATA3, high active en */
#define N53_WL1271_BT_EN	GPIO_NUMBER(2, 2)	/* ATA_DATA2, high active en */
#define N53_WL1271_BT_FUNC5	GPIO_NUMBER(2, 0)	/* ATA_DATA0, input (HOST_WU) */
#define N53_WL1271_INT		GPIO_NUMBER(2, 1)	/* ATA_DATA1 - wlan_irq */
#define N53_EMMC_RESET		GPIO_NUMBER(7, 10)	/* ATA_CS_1 - eMMC reset */
#else
#define N53_WL1271_WL_EN	GPIO_NUMBER(3, 0)	/* EIM_DA0, high active en */
#define N53_WL1271_BT_EN	GPIO_NUMBER(3, 1)	/* EIM_DA1, high active en */
#define N53_WL1271_BT_FUNC2	GPIO_NUMBER(1, 9)	/* GPIO_9, output (BT_WU) */
#define N53_WL1271_BT_FUNC5	GPIO_NUMBER(3, 9)	/* EIM_DA9, input (HOST_WU) */
#define N53_WL1271_INT		GPIO_NUMBER(7, 9)	/* ATA_CS_0 - wlan_irq */
#define N53_EMMC_RESET		GPIO_NUMBER(3, 8)	/* EIM_DA8 - eMMC reset */
#endif

#else
#define N53_WL1271_WL_EN	GPIO_NUMBER(3, 0)	/* EIM_DA0, high active en */
#define N53_WL1271_BT_EN	GPIO_NUMBER(3, 1)	/* EIM_DA1, high active en */
#define N53_WL1271_BT_FUNC2	GPIO_NUMBER(5, 25)	/* CSI0_D7, output (BT_WU) */
#define N53_WL1271_BT_FUNC5	GPIO_NUMBER(1, 6)	/* GPIO_6, input (HOST_WU) */
#define N53_WL1271_INT		GPIO_NUMBER(2, 24)	/* EIM_CS1 - wlan_irq */

#define N53_EMMC_RESET		GPIO_NUMBER(5, 2)	/* EIM_A25 - eMMC reset */
#endif

			gpio_direction_output(N53_EMMC_RESET, 0);
			gpio_direction_input(N53_WL1271_INT);
			gpio_direction_output(N53_WL1271_WL_EN, 0);
#if CONFIG_MACH_TYPE != MACH_TYPE_MX53_NITROGEN_A
			gpio_direction_output(N53_WL1271_BT_EN, 0);
#endif
			gpio_direction_input(N53_WL1271_BT_FUNC5);
#ifdef N53_WL1271_BT_FUNC2
			gpio_direction_output(N53_WL1271_BT_FUNC2, 0);
#endif

#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
#ifdef CONFIG_K2
			/* EMMC Reset */
			mxc_request_iomux(MX53_PIN_ATA_CS_1, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_ATA_CS_1, 0);
			/* wl1271 wl_irq */
			mxc_request_iomux(MX53_PIN_ATA_DATA1, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA1, WL12XX_IRQ_PAD_CTL);
			/* wl1271 wl_en */
			mxc_request_iomux(MX53_PIN_ATA_DATA3, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA3, 0x0);
			/* wl1271 bt_en */
			mxc_request_iomux(MX53_PIN_ATA_DATA2, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA2, 0x0);
			/* wl1271 btfunc5 */
			mxc_request_iomux(MX53_PIN_ATA_DATA0, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA0, WL12XX_BTFUNC5_PAD_CTL);
			/* osc 32.768 */
			mxc_request_iomux(MX53_PIN_GPIO_10, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_GPIO_10, 0);
#else
			/* EMMC Reset */
			mxc_request_iomux(MX53_PIN_EIM_DA8, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_DA8, 0);

			/* wl1271 wl_irq */
			mxc_request_iomux(MX53_PIN_ATA_CS_0, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_ATA_CS_0, WL12XX_IRQ_PAD_CTL);
			/* wl1271 wl_en */
			mxc_request_iomux(MX53_PIN_EIM_DA0, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_DA0, 0x0);
			/* wl1271 bt_en */
			mxc_request_iomux(MX53_PIN_EIM_DA1, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_DA1, 0x0);
			/* wl1271 btfunc5 */
			mxc_request_iomux(MX53_PIN_EIM_DA9, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_DA9, WL12XX_BTFUNC5_PAD_CTL);
			/* wl1271 btfunc2 */
			mxc_request_iomux(MX53_PIN_GPIO_9, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_GPIO_9, WL12XX_BTFUNC2_PAD_CTL);
#endif
#else
			mxc_request_iomux(MX53_PIN_EIM_A25, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_A25, 0);

			/* wl1271 wl_irq */
			mxc_request_iomux(MX53_PIN_EIM_CS1, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_CS1, WL12XX_IRQ_PAD_CTL);
			/* wl1271 wl_en */
			mxc_request_iomux(MX53_PIN_EIM_DA0, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_DA0, 0x0);
			/* wl1271 bt_en */
			mxc_request_iomux(MX53_PIN_EIM_DA1, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_EIM_DA1, 0x0);
			/* wl1271 btfunc5 */
			mxc_request_iomux(MX53_PIN_GPIO_6, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_GPIO_6, WL12XX_BTFUNC5_PAD_CTL);
			/* wl1271 btfunc2 */
			mxc_request_iomux(MX53_PIN_CSI0_D7, IOMUX_CONFIG_ALT1);
			mxc_iomux_set_pad(MX53_PIN_CSI0_D7, WL12XX_BTFUNC2_PAD_CTL);
#endif

			/* esdhc3 */
			mxc_request_iomux(MX53_PIN_ATA_RESET_B,		/* CMD */
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_IORDY,		/* CLK */
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_DATA8,		/* D0 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA9,		/* D1 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA10,		/* D2 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA11,		/* D3 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA0,		/* D4 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA1,		/* D5 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA2,		/* D6 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA3,		/* D7 */
						IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_ATA_RESET_B, MX53_SDHC_PAD_CTRL);	/* CMD */
			mxc_iomux_set_pad(MX53_PIN_ATA_IORDY, MX53_SDHC_PAD_CTRL);	/* CLK */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA8, MX53_SDHC_PAD_CTRL);	/* D0 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA9, MX53_SDHC_PAD_CTRL);	/* D1 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA10, MX53_SDHC_PAD_CTRL);	/* D2 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA11, MX53_SDHC_PAD_CTRL);	/* D3 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA0, MX53_SDHC_PAD_CTRL);	/* D4 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA1, MX53_SDHC_PAD_CTRL);	/* D5 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA2, MX53_SDHC_PAD_CTRL);	/* D6 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA3, MX53_SDHC_PAD_CTRL);	/* D7 */

#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
			/* esdhc4 */
			mxc_request_iomux(MX53_PIN_ATA_DA_1,		/* CMD */
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_DA_2,		/* CLK */
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_DATA12,		/* D0 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA13,		/* D1 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA14,		/* D2 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA15,		/* D3 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA4,		/* D4 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA5,		/* D5 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA6,		/* D6 */
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA7,		/* D7 */
						IOMUX_CONFIG_ALT4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DA_1, MX53_SDHC_PAD_CTRL);	/* CMD */
			mxc_iomux_set_pad(MX53_PIN_ATA_DA_2, MX53_SDHC_PAD_CTRL);	/* CLK */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA12, MX53_SDHC_PAD_CTRL);	/* D0 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA13, MX53_SDHC_PAD_CTRL);	/* D1 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA14, MX53_SDHC_PAD_CTRL);	/* D2 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA15, MX53_SDHC_PAD_CTRL);	/* D3 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA4, MX53_SDHC_PAD_CTRL);	/* D4 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA5, MX53_SDHC_PAD_CTRL);	/* D5 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA6, MX53_SDHC_PAD_CTRL);	/* D6 */
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA7, MX53_SDHC_PAD_CTRL);	/* D7 */
#endif
			/* release eMMC reset */
			gpio_direction_output(N53_EMMC_RESET, 1);
			break;
		default:
			printf("Warning: you configured more ESDHC controller"
				"(%d) as supported by the board(2)\n",
				CONFIG_SYS_FSL_ESDHC_NUM);
			return status;
			break;
		}
		status |= fsl_esdhc_initialize(bis, &esdhc_cfg[index]);
	}
	mmc = find_mmc_device(1);
	if (mmc) {
#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
		mmc->voltages = MMC_VDD_165_195;
#endif
	}
	return status;
}

int board_mmc_init(bd_t *bis)
{
	if (!esdhc_gpio_init(bis))
		return 0;
	else
		return -1;
}

#endif

#ifdef CONFIG_I2C_MXC
#define PC_22K	(PAD_CTL_PKE | PAD_CTL_PUE | PAD_CTL_PUS_22K_UP |	\
	PAD_CTL_HYS | PAD_CTL_ODE | PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST)

#define PC_NONE	(PAD_CTL_PKE | PAD_CTL_PUE |	\
	PAD_CTL_HYS | PAD_CTL_ODE | PAD_CTL_DSE_HIGH | PAD_CTL_SRE_FAST)

#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_A
#define PC_I2C1	PC_22K
#define PC_I2C2	PC_NONE
#define PC_I2C3	PC_22K
#else
#define PC_I2C1	PC_22K
#define PC_I2C2	PC_NONE
#define PC_I2C3	PC_NONE
#endif


/* I2C1,  */
struct i2c_pads_info i2c_pad_info0 = {
       .scl = {
               .i2c_mode = NEW_PAD_CTRL(MX53_PAD_EIM_D21__I2C1_SCL, PC_I2C1),
               .gpio_mode = NEW_PAD_CTRL(MX53_PAD_EIM_D21__GPIO3_21, PC_I2C1),
               .gp = GPIO_NUMBER(3, 21)
       },
       .sda = {
               .i2c_mode = NEW_PAD_CTRL(MX53_PAD_EIM_D28__I2C1_SDA, PC_I2C1),
               .gpio_mode = NEW_PAD_CTRL(MX53_PAD_EIM_D28__GPIO3_28, PC_I2C1),
               .gp = GPIO_NUMBER(3, 28)
       }
};

/* I2C2,  */
struct i2c_pads_info i2c_pad_info1 = {
       .scl = {
               .i2c_mode = NEW_PAD_CTRL(MX53_PAD_EIM_EB2__I2C2_SCL, PC_I2C2),
               .gpio_mode = NEW_PAD_CTRL(MX53_PAD_EIM_EB2__GPIO2_30, PC_I2C2),
               .gp = GPIO_NUMBER(2, 30)
       },
       .sda = {
               .i2c_mode = NEW_PAD_CTRL(MX53_PAD_KEY_ROW3__I2C2_SDA, PC_I2C2),
               .gpio_mode = NEW_PAD_CTRL(MX53_PAD_KEY_ROW3__GPIO4_13, PC_I2C2),
               .gp = GPIO_NUMBER(4, 13)
       }
};

/* I2C3,  */
struct i2c_pads_info i2c_pad_info2 = {
       .scl = {
               .i2c_mode = NEW_PAD_CTRL(MX53_PAD_GPIO_3__I2C3_SCL, PC_I2C3),
               .gpio_mode = NEW_PAD_CTRL(MX53_PAD_GPIO_3__GPIO1_3, PC_I2C3),
               .gp = GPIO_NUMBER(1, 3)
       },
       .sda = {
               .i2c_mode = NEW_PAD_CTRL(MX53_PAD_GPIO_16__I2C3_SDA, PC_I2C3),
               .gpio_mode = NEW_PAD_CTRL(MX53_PAD_GPIO_16__GPIO7_11, PC_I2C3),
               .gp = GPIO_NUMBER(7, 11)
       }
};
#endif

static char watch_on_key;

int board_init(void)
{
#ifdef GP_LCD_BACKLIGHT
	/* Nitrogen A, disable 12V display power */
	gpio_direction_output(GP_LCD_BACKLIGHT, 0);
	mxc_request_iomux(MX53_PIN_EIM_A22, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_A22, PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE);	//pullup disabled
#endif

	#ifdef CONFIG_MFG
/* MFG firmware need reset usb to avoid host crash firstly */
#define USBCMD 0x140
	int val = readl(OTG_BASE_ADDR + USBCMD);
	val &= ~0x1; /*RS bit*/
	writel(val, OTG_BASE_ADDR + USBCMD);
#endif
	gpio_direction_output(GPIO_NUMBER(5, 0), 0);		//USB Hub reset, low power reset state
	mxc_request_iomux(MX53_PIN_EIM_WAIT, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_WAIT, PAD_CTL_HYS_ENABLE | PAD_CTL_DRV_HIGH);

	setup_boot_device();
	setup_soc_rev();
#ifdef CONFIG_IMX_ECSPI
	setup_spi();
#endif

	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

#ifdef CONFIG_I2C_MXC
	setup_i2c(0, CONFIG_SYS_I2C1_SPEED, 0x7f, &i2c_pad_info0);
#endif
	setup_uart();
	gd->bd->bi_arch_number = CONFIG_MACH_TYPE;	/* board id for linux */
#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_A
	if ((system_rev & 0xf00) == 0x100)
		gd->bd->bi_arch_number = MACH_TYPE_MX53_NITROGEN_AP;
	if (bus_i2c_write(DA90_I2C_BUS, DA90_I2C_ADDR, 0x3a, 1, (uchar *)"\x5f", 1))
		printf("reg 0x3a (LDO9) of DA9053 failed\n");
#endif


#define PAD_CTL_NORMAL_LOW_OUT	PAD_CTL_360K_PD		/* pull down disabled */

#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_A
	gpio_direction_output(I2C2_HUB_PIC16F616_TOUCH, 0);	/* Disable */
	gpio_direction_output(I2C2_HUB_CAMERA, 0);		/* Disable */
	gpio_direction_output(I2C2_HUB_TFP410_ACCEL, 0);		/* Disable */
	gpio_direction_output(I2C2_HUB_BATT_EDID, 0);		/* Disable */
	gpio_direction_output(I2C2_HUB_OLD_EMPTY, 0);		/* Disable */
	gpio_direction_output(I2C2_HUB_RTC_ISL1208, 0);		/* Disable */
	gpio_direction_output(I2C3_HUB_SC16IS7XX, 0);		/* Disable */
	gpio_direction_output(GP_LCD_3_3V_POWER_ENABLE, 0);	/* Disable */
	gpio_direction_output(GP_BT_RESET, 0);			/* Disable */

	mxc_request_iomux(MX53_PIN_EIM_DA7, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_EIM_DA10, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_EIM_DA11, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_NANDF_CS0, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_EIM_DA9, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_NANDF_WE_B, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_NANDF_RB0, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_ATA_DATA6, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_EIM_DA3, IOMUX_CONFIG_ALT1);

	mxc_iomux_set_pad(MX53_PIN_EIM_DA7, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA10, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA11, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_NANDF_CS0, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA9, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_NANDF_WE_B, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_NANDF_RB0, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_ATA_DATA6, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA3, PAD_CTL_NORMAL_LOW_OUT);
#endif
#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
	gpio_direction_output(N53K_I2C2_HUB_EDID, 0);		/* Disable */
	gpio_direction_output(N53K_I2C2_HUB_BATTERY, 0);	/* Disable */
	gpio_direction_output(N53K_I2C2_HUB_AMBIENT, 0);	/* Disable */
	gpio_direction_output(N53K_I2C2_HUB_CAMERA, 0);		/* Disable */
	gpio_direction_output(N53K_POWER_KEEP_ON, 1);		/* enable power */
	mxc_request_iomux(MX53_PIN_EIM_DA8, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_EIM_DA9, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_EIM_DA10, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_NANDF_RB0, IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX53_PIN_NANDF_WE_B, IOMUX_CONFIG_ALT1);

	mxc_iomux_set_pad(MX53_PIN_EIM_DA8, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA9, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_EIM_DA10, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_NANDF_RB0, PAD_CTL_NORMAL_LOW_OUT);
	mxc_iomux_set_pad(MX53_PIN_NANDF_WE_B, PAD_CTL_100K_PU);	/* Pull up disabled */
#endif

#ifdef CONFIG_I2C_MXC
	setup_i2c(1, CONFIG_SYS_I2C2_SPEED, 0x7f, &i2c_pad_info1);
#endif
#ifdef CONFIG_MXC_FEC
	setup_fec();
#endif

#ifdef CONFIG_I2C_MXC
	setup_i2c(2, CONFIG_SYS_I2C3_SPEED, 0x7f, &i2c_pad_info2);
	setup_core_voltages();
#endif
#ifdef CONFIG_BQ2416X_CHARGER
	bq2416x_init();
#endif
	watch_on_key = 1;
	return 0;
}


#ifdef CONFIG_ANDROID_RECOVERY
struct reco_envs supported_reco_envs[BOOT_DEV_NUM] = {
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC,
	 .args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC,
	 },
	{
	 .cmd = NULL,
	 .args = NULL,
	 },
};

int check_recovery_cmd_file(void)
{
	disk_partition_t info;
	ulong part_length;
	int filelen;
	char *env;

	/* For test only */
	/* When detecting android_recovery_switch,
	 * enter recovery mode directly */
	env = getenv("android_recovery_switch");
	if (!strcmp(env, "1")) {
		printf("Env recovery detected!\nEnter recovery mode!\n");
		return 1;
	}

	printf("Checking for recovery command file...\n");
	switch (get_boot_device()) {
	case MMC_BOOT:
	case SD_BOOT:
		{
			block_dev_desc_t *dev_desc = NULL;
			struct mmc *mmc = find_mmc_device(0);

			dev_desc = get_dev("mmc", 0);

			if (NULL == dev_desc) {
				puts("** Block device MMC 0 not supported\n");
				return 0;
			}

			mmc_init(mmc);

			if (get_partition_info(dev_desc,
					CONFIG_ANDROID_CACHE_PARTITION_MMC,
					&info)) {
				printf("** Bad partition %d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				return 0;
			}

			part_length = ext2fs_set_blk_dev(dev_desc,
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
			if (part_length == 0) {
				printf("** Bad partition - mmc 0:%d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				ext2fs_close();
				return 0;
			}

			if (!ext2fs_mount(part_length)) {
				printf("** Bad ext2 partition or "
					"disk - mmc 0:%d **\n",
					CONFIG_ANDROID_CACHE_PARTITION_MMC);
				ext2fs_close();
				return 0;
			}

			filelen = ext2fs_open(CONFIG_ANDROID_RECOVERY_CMD_FILE);

			ext2fs_close();
		}
		break;
	case NAND_BOOT:
		return 0;
		break;
	case SPI_NOR_BOOT:
		return 0;
		break;
	case UNKNOWN_BOOT:
	default:
		return 0;
		break;
	}

	return (filelen > 0) ? 1 : 0;

}
#endif

extern void mxc_fec_eth_set_mac_addr(const unsigned char *macaddr);

int parse_mac(char const *cmac, unsigned char *mac)
{
	int i = 0;
	char *end;
	int ret = (cmac) ? 0 : -1;

	do {
		mac[i++] = cmac ? simple_strtoul(cmac, &end, 16) : 0;
		if (i == 6)
			break;
		if (cmac) {
			cmac = (*end) ? end+1 : end;
			if ((*end != '-') && (*end != ':'))
				ret = -1;
		}
	} while (1);
	return ret;
}

static int get_env_mac(unsigned char *mac)
{
	char *cmac = getenv("ethaddr");
	if (cmac) {
		if (!parse_mac(cmac, mac)) {
			return 0;
		}
	}
	return -1;
}

#ifdef CONFIG_CMD_BMODE
static const struct boot_mode board_boot_modes[] = {
	/* 4 bit bus width */
	{"mmc0",	MAKE_CFGVAL(0x40, 0x20, 0x00, 0x12)},	/* esdhc1 */
#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K
	{"mmc1",	MAKE_CFGVAL(0x40, 0x20, 0x18, 0x12)},	/* esdhc4 */
#else
	{"mmc1",	MAKE_CFGVAL(0x40, 0x20, 0x10, 0x12)},	/* esdhc3 */
#endif
	{NULL,		0},
};
#endif

int misc_init_r(void)
{
	unsigned char macAddrROM[6];
	unsigned char macAddrEnv[6];
	unsigned found = 0 ;
	int rv ;

#ifdef CONFIG_CMD_BMODE
	add_board_boot_modes(board_boot_modes);
#endif
	/* gpio3[23] - KEEPON */
	mxc_request_iomux(MX53_PIN_EIM_D23, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_D23, PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE);	//pullup disabled
	gpio_direction_output(GPIO_NUMBER(3, 23), 1);

	rv = iim_read_mac_addr(macAddrROM);
	if (rv) {
		printf( "ROM mac address %02x:%02x:%02x:%02x:%02x:%02x\n",
			macAddrROM[0], macAddrROM[1], macAddrROM[2],
			macAddrROM[3], macAddrROM[4], macAddrROM[5] );
		found++ ;
	}
	else
		printf("Mac not programmed in IIM\n");
	rv = get_env_mac (macAddrEnv);
	if (0 == rv) {
		printf( "Env mac address %02x:%02x:%02x:%02x:%02x:%02x\n",
			macAddrEnv[0], macAddrEnv[1], macAddrEnv[2],
			macAddrEnv[3], macAddrEnv[4], macAddrEnv[5] );
		found++ ;
		memcpy (macAddrROM, macAddrEnv,sizeof (macAddrROM));
	} else {
		printf( "No mac address assigned in environment\n" );
		if (found) {
			char cMac[32];
			sprintf (cMac,
				 "%02x:%02x:%02x:%02x:%02x:%02x",
				 macAddrROM[0], macAddrROM[1], macAddrROM[2],
				 macAddrROM[3], macAddrROM[4], macAddrROM[5] );
			setenv ("ethaddr", cMac);
		}
	}
	if (found > 0) {
		printf ("setting mac address to %02x:%02x:%02x:%02x:%02x:%02x\n",
                        macAddrROM[0], macAddrROM[1], macAddrROM[2],
			macAddrROM[3], macAddrROM[4], macAddrROM[5] );
		mxc_fec_eth_set_mac_addr(macAddrROM);
		return 0 ;
	}
	return -1 ;
}
extern void setup_display(void);

int board_late_init(void)
{
	char buf[20];
	int i = 1;
	unsigned uart;

#ifdef CONFIG_I2C_MXC
	char *pmic_regs ;
	if (0 != (pmic_regs = getenv("PMICREGS"))) {
		char *save=strdup(pmic_regs);
		char *next = strtok(save,",");
		while (next) {
			unsigned regnum = simple_strtoul(next,0,16);
			char *cval = strchr(next,':');
			unsigned regval = cval ? simple_strtoul(cval+1,0,16)
						: -1UL ;
			if ((0 != cval)
			    &&
			    (regnum < 127)
			    &&
			    (regval <= 0x100)){
				printf ("PMIC[0x%02x] == 0x%02x\n", regnum, regval);
				if (bus_i2c_write(DA90_I2C_BUS, DA90_I2C_ADDR, regnum, 1, (uchar *)&regval, 1)) {
					printf("Error storing PMIC[0x%02x] == 0x%02x\n", regnum, regval);
				}
			}
			next = strtok(0,",");
		}
		free(save);
	}
#endif
#ifdef CONFIG_VIDEO_IMX5X
	setup_display();
#endif
	/* gpio3[22] - Power button */
	mxc_request_iomux(MX53_PIN_EIM_D22, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_D22, N53_BUTTON_100KPU_PAD_CTL);
	gpio_direction_input(GPIO_NUMBER(3, 22));

	uart = get_uart_base();
	if (uart == UART1_BASE_ADDR) {
		i = 0;	/* Nitrogen53_a rev 1 */
	} else if (uart == UART3_BASE_ADDR) {
		i = 2;	/* Nitrogen53_a rev 2 */
	}
	sprintf(buf, "ttymxc%d,115200", i);
	setenv("console", buf);
	return 0;
}

int checkboard(void)
{
	unsigned base = ESD_BASE;
	unsigned tapeout = (readl(base + ESD_ZQHWCTRL) >> 31) + 1;
	unsigned srev = readl(0x63f98024);
	printf("Board: " CONFIG_BOARD_NAME ", TO%d SREV=%x ", tapeout, srev);
	printf("Boot Reason: [");

	switch (__REG(SRC_BASE_ADDR + 0x8)) {
	case 0x0001:
		printf("POR");
		break;
	case 0x0009:
		printf("RST");
		break;
	case 0x0010:
	case 0x0011:
		printf("WDOG");
		break;
	default:
		printf("unknown");
	}
	printf("]\n");

	printf("Boot Device: ");
	switch (get_boot_device()) {
	case WEIM_NOR_BOOT:
		printf("NOR\n");
		break;
	case ONE_NAND_BOOT:
		printf("ONE NAND\n");
		break;
	case PATA_BOOT:
		printf("PATA\n");
		break;
	case SATA_BOOT:
		printf("SATA\n");
		break;
	case I2C_BOOT:
		printf("I2C\n");
		break;
	case SPI_NOR_BOOT:
		printf("SPI NOR\n");
		break;
	case SD_BOOT:
		printf("SD\n");
		break;
	case MMC_BOOT:
		printf("MMC\n");
		break;
	case NAND_BOOT:
		printf("NAND\n");
		break;
	case UNKNOWN_BOOT:
	default:
		printf("UNKNOWN\n");
		break;
	}
	return 0;
}

int do_macafter(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i ;
	unsigned char macAddress[6];
	if ((3 == argc) && (0 == parse_mac(argv[1],macAddress))) {
		char cMac[32];
		for (i = sizeof(macAddress); i > 0 ;) {
			--i ;
			if (0 != ++macAddress[i])
				break;
		}
		sprintf (cMac,
			 "%02x:%02x:%02x:%02x:%02x:%02x",
			 macAddress[0], macAddress[1], macAddress[2],
			 macAddress[3], macAddress[4], macAddress[5] );
		printf("%s => %s\n", argv[2],cMac);
		setenv (argv[2], cMac);
	} else
		cmd_usage(cmdtp);
	return 0 ;
}

U_BOOT_CMD(
	macafter, 3, 0, do_macafter,
	"Generate a mac address based on another",
	"Usage: macafter 00:01:02:03:04:05 mymac\n"
	"This example will set 'mymac' to 00:01:02:03:04:06\n"
);

int pwm(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (2 == argc) {
		unsigned long val = simple_strtoul(argv[1],0,0);
		if (255 >= val)
			writel(val, PWM2_BASE_ADDR + PWMSAR);
	} else
		cmd_usage(cmdtp);
	return 0 ;
}

U_BOOT_CMD(
	   pwm, 3, 0, pwm,
	   "Set PWM (backlight) value",
	   "Usage: pwm 0..255\n"
);

#if CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_A
#include <power_key.h>
#define POWER_KEY GPIO_NUMBER(3,22)
#define POWER_DOWN GPIO_NUMBER(3,23)

static int prev_power_key = -1 ;
static unsigned long when_pressed ;

void check_power_key(void)
{
	int newval = gpio_get_value(POWER_KEY)^1; /* high means not pressed */
	if (newval != prev_power_key) {
		prev_power_key = newval ;
		if (newval)
                        when_pressed = get_timer(0);
	} else if (1 == prev_power_key) {
		long long elapsed = get_timer(when_pressed);
		if (5000 <= elapsed) {
			printf( "power down\n");
			when_pressed = get_timer(0);
			gpio_set_value(POWER_DOWN, 0);
		}
	}
}
#endif

struct button_key {
       char const      *name;
       unsigned        gpnum;
       char            ident;
};

static struct button_key const buttons[] = {
       {"back",		GPIO_NUMBER(3, 26),	'B'},
       {"home",		GPIO_NUMBER(3, 29),	'H'},
       {"menu",		GPIO_NUMBER(3, 31),	'M'},
       {"search",	GPIO_NUMBER(3, 27),	'S'},
};

/*
 * generate a null-terminated string containing the buttons pressed
 * returns number of keys pressed
 */
static int read_keys(char *buf)
{
       int i, numpressed = 0;
       for (i = 0; i < ARRAY_SIZE(buttons); i++) {
               if (!gpio_get_value(buttons[i].gpnum))
                       buf[numpressed++] = buttons[i].ident;
       }
       buf[numpressed] = '\0';
       return numpressed;
}

static int do_kbd(cmd_tbl_t *cmdtp, int flag, int argc, char * argv[])
{
       char envvalue[ARRAY_SIZE(buttons)+1];
       int numpressed = read_keys(envvalue);
       setenv("keybd", envvalue);
       return numpressed == 0;
}

U_BOOT_CMD(
       kbd, 1, 1, do_kbd,
       "Tests for keypresses, sets 'keybd' environment variable",
       "Returns 0 (true) to shell if key is pressed."
);


struct da90_regname_t {
	char const *name ;
	int 	    regnum ;
	int	    bit ;
	int	    invert ;
	int	    default_val ;
};

static struct da90_regname_t const da90_regs[] = {
	{ "dcdet", 0x01, 5, 1, 0 }
,	{ "vbus",  0x01, 4, 1, 0 }
};

static int da90_readbit(char const *which) {
	int i ;
	for (i = 0 ; i < ARRAY_SIZE(da90_regs); i++) {
		struct da90_regname_t const *reg = da90_regs+i;
		if (0 == strcmp(which,reg->name)) {
			unsigned char buf[1];
			int ret = bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR, reg->regnum, 1, buf, sizeof (buf));
			if (0 == ret)
				return (0 != (buf[0]&(1<<reg->bit))) ^ reg->invert ;
			else
				printf("reg %d(%s) of DA9053 failed\n",reg->regnum,which);
			return reg->default_val ;
		}
	}
	printf("Invalid da90 register %s\n", which);
	return 0 ;
}

int do_readbit(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return da90_readbit(argv[0]);
}

U_BOOT_CMD(
	dcdet, 3, 0, do_readbit,
	"Detect DCIN",
	"Usage: dcdet\n"
	"Returns 0 (true) if DCIN is present\n"
);

U_BOOT_CMD(
	vbus, 3, 0, do_readbit,
	"Detect VBUS",
	"Usage: vbus\n"
	"Returns 0 (true) if VBUS is present\n"
);

struct da90_adc_name_t {
	char const *name ;
	unsigned    channel ;
	char	    units ;
	long	    (*conversion)(unsigned short val);
};

static long convert_vbat(unsigned short val)
{
	return ((val+1280)*1000)/512;
}

static long convert_vbbat(unsigned short val)
{
	return (5000*val)/1023 ;
}

static long convert_tjunc(unsigned short val)
{
        return (((1708 * val)/10) - 10600);
}

static long convert_ichg(unsigned short val)
{
	return ((3900*val)/1000);
}

static struct da90_adc_name_t const da90_adc_names[] = {
	{ "ADC_VDDOUT",	0, 'V',  convert_vbat}
,	{ "ADC_ICH",	1, 'A',  convert_ichg}
,	{ "ADC_TBAT",	2, '\0', 0}
,	{ "ADC_VBAT",	3, 'V',	 convert_vbat}
,	{ "ADC_ADCIN4",	4, '\0', 0}
,	{ "ADC_ADCIN5",	5, '\0', 0}
,	{ "ADC_ADCIN6",	6, '\0', 0}
,	{ "ADC_TSI",	7, '\0', 0}
,	{ "ADC_TJUNC",	8, 'C',  convert_tjunc}
,	{ "ADC_VBBAT",	9, 'V',	 convert_vbbat}
};

/*
 * returns 0 on failure
 */
int do_read_adc (unsigned channel)
{
	int rval ;
	u8 buf[1] = { 0 };

	rval = bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR,
			    DA9052_EVENTB_REG, 1,
			    buf, sizeof (buf));
	if (0 != rval)
		printf("%s: error %d reading EVENTB\n", __func__, rval );
	if (0 != (buf[0]&DA9052_EVENTB_EADCEOM)) {
                rval = bus_i2c_write (DA90_I2C_BUS, DA90_I2C_ADDR,
                                      DA9052_EVENTB_REG,  1,
				      buf, sizeof (buf));
		if (0 != rval)
			printf("%s: error %d clearing EVENTB\n", __func__, rval );
	}
	buf[0] = channel | DA9052_ADCMAN_MANCONV ;
	rval = bus_i2c_write (DA90_I2C_BUS, DA90_I2C_ADDR,
			      DA9052_ADCMAN_REG, 1,
			      buf, sizeof(buf));
	if (0 == rval) {
		int loops = 0 ;
		do {
			udelay(1000);
			rval = bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR,
					    DA9052_ADCMAN_REG,  1,
					    buf, sizeof (buf));
			if (0 == rval) {
				if (0 == (buf[0]&DA9052_ADCMAN_MANCONV)) {
					break;
				}
			} else
				printf("%s: error %d reading ADCMAN\n", __func__, rval);
		} while ( 10 > ++loops );
		if (10 > loops) {
			rval = bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR,
					    DA9052_ADCRESH_REG,  1,
					    buf, sizeof (buf));
			if (0 == rval) {
				unsigned short adc = buf[0] << 2 ;
				rval = bus_i2c_read (DA90_I2C_BUS, DA90_I2C_ADDR,
						     DA9052_ADCRESL_REG,  1,
						     buf, sizeof (buf));
				if (0 == rval) {
					adc |= (buf[0]&3);
					return adc ;
				} else
					printf("%s: error %d reading ADCRESL\n", __func__, rval);
			} else
                                printf("%s: error %d reading ADCRESH\n", __func__, rval);
		} else
			printf("%s: timeout waiting for ADC result\n", __func__ );
	} else
		printf ("%s: error %d writing ADCMAN_REG\n", __func__, rval);
	return 0 ;
}

int do_adc(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char const *which = argv[0];
	int i ;
	for (i = 0 ; i < ARRAY_SIZE(da90_adc_names); i++) {
		if (0 == strcmp(which,da90_adc_names[i].name)) {
			struct da90_adc_name_t const *adc = &da90_adc_names[i];
			unsigned short val = do_read_adc(adc->channel);
			printf("ADC channel(%s) == 0x%04x", which, val);
			if (adc->units && adc->conversion) {
				long milli = adc->conversion(val);
				int whole = milli/1000 ;
				int frac = (milli/10)%100 ;
				printf(" (%u.%02u%c)\n", whole, frac, adc->units);
			} else
				putc ('\n');
			return 0 ;
		}
	}
	printf("%s: adc channel %s is not valid\n", __func__, which);
	return -1 ;
}

U_BOOT_CMD(
	ADC_VBAT, 3, 0, do_adc,
	"Read ADC_VBAT",
	"Usage: ADC_VBAT\n"
	"Displays value of ADC_VBAT\n"
);

U_BOOT_CMD(
	   ADC_VDDOUT, 3, 0, do_adc,
	   "Read ADC_VDDOUT",
	   "Usage: ADC_VDDOUT\n"
	   "Displays value of ADC_VDDOUT\n"
);

U_BOOT_CMD(
	   ADC_ICH, 3, 0, do_adc,
	   "Read ADC_ICH",
	   "Usage: ADC_ICH\n"
	   "Displays value of ADC_ICH\n"
);

U_BOOT_CMD(
	   ADC_TBAT, 3, 0, do_adc,
	   "Read ADC_TBAT",
	   "Usage: ADC_TBAT\n"
	   "Displays value of ADC_TBAT\n"
);

U_BOOT_CMD(
	   ADC_ADCIN4, 3, 0, do_adc,
	   "Read ADC_ADCIN4",
	   "Usage: ADC_ADCIN4\n"
	   "Displays value of ADC_ADCIN4\n"
);

U_BOOT_CMD(
	   ADC_ADCIN5, 3, 0, do_adc,
	   "Read ADC_ADCIN5",
	   "Usage: ADC_ADCIN5\n"
	   "Displays value of ADC_ADCIN5\n"
);

U_BOOT_CMD(
	   ADC_ADCIN6, 3, 0, do_adc,
	   "Read ADC_ADCIN6",
	   "Usage: ADC_ADCIN6\n"
	   "Displays value of ADC_ADCIN6\n"
);

U_BOOT_CMD(
	   ADC_TSI, 3, 0, do_adc,
	   "Read ADC_TSI",
	   "Usage: ADC_TSI\n"
	   "Displays value of ADC_TSI\n"
);

U_BOOT_CMD(
	   ADC_TJUNC, 3, 0, do_adc,
	   "Read ADC_TJUNC",
	   "Usage: ADC_TJUNC\n"
	   "Displays value of ADC_TJUNC\n"
);

U_BOOT_CMD(
	   ADC_VBBAT, 3, 0, do_adc,
	   "Read ADC_VBBAT",
	   "Usage: ADC_VBBAT\n"
	   "Displays value of ADC_VBBAT\n"
);

#if (CONFIG_MACH_TYPE == MACH_TYPE_MX53_NITROGEN_K)
static unsigned char const poweroff_regs[] = {
	5, 0xff,	/* clear events */
	6, 0xff,
	7, 0xff,
	8, 0xff,
	9, 0xff,
#if 1
	/* deep sleep*/
	15, 0x7a,	/* deep sleep */
#else
	/* Shutdown */
	10, 0xff,	/* mask interrupts */
	11, 0xfe,	/* except nONKEY */
	12, 0xff,
	13, 0xff,
	15, 0xaa,	/* power-off */
#endif
	DA9052_STATUSA_REG, 0	/* select status_a register */
};

int poweroff(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rval ;
	u8 buf[1];

	rval = bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR,
			DA9052_CONTROLB_REG, 1,
			buf, 1);
	if (rval) {
		printf("%s: Cannot read control_b\n", __func__);
		return rval;
	}
	if (!(buf[0] & (1 << 5))) {
		/* enable repeated write mode */
		buf[0] |= (1 << 5);
		buf[0] &= ~(1 << 7);
		rval = bus_i2c_write(DA90_I2C_BUS, DA90_I2C_ADDR,
				DA9052_CONTROLB_REG, 1,
				buf, 1);
		if (rval) {
			printf("%s: Cannot set repeated write\n", __func__);
			return rval;
		}
	}

	rval = bus_i2c_write(DA90_I2C_BUS, DA90_I2C_ADDR, poweroff_regs[0], 1,
			&poweroff_regs[1], sizeof(poweroff_regs) - 1);
	if (rval)
		printf("%s: error writing power-down sequence\n", __func__);
	gpio_set_value(N53K_POWER_KEEP_ON, 0);	/* power off */
	/* 1/2 sec delay so that no device is in use */
	udelay(500000);
	printf("!!Should not get here\n");
	return rval;
}

U_BOOT_CMD(
	   poweroff, 3, 0, poweroff,
	   "Power system off",
	   "Usage: poweroff\n"
);

#include <power_key.h>

static unsigned long when_change;
static unsigned long when_tested;
static char prev_power_key = -1 ;
static char was_high_sometime;
static char prime_power_down;

/*
 * On-key isn't valid until sampled high for .1 seconds
 * Needed for holding button at power-on.
 */
void check_power_key(void)
{
	int rval;
	int newval;
	unsigned long cur_time;
	u8 buf[1] = { 0 };

	if (!watch_on_key)	/* Wait for I2C bus */
		return;
	watch_on_key = 0;	/* prevent recursion */
	cur_time = get_timer(0);
	if ((unsigned long)(cur_time - when_tested) < 10)
		goto exit;
	when_tested = cur_time;
	rval = bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR,
			    DA9052_STATUSA_REG, 1,
			    buf, sizeof (buf));
	if (0 != rval)
		goto exit;

#ifdef CONFIG_BQ2416X_WATCHDOG
	bq2416x_watchdog(cur_time);
#endif
	newval = (buf[0] & 1);		/* high means not pressed */
	if (prev_power_key != newval) {
		prev_power_key = newval;
		when_change = cur_time;
	} else {
		unsigned long elapsed = (unsigned long)(cur_time - when_change);
		if (!newval) {
			if (was_high_sometime && (elapsed >= 200)) {
				/* press for .2 sec to prime powerdown */
				prime_power_down = 1;
			}
		} else if (elapsed >= 100) {
			if (prime_power_down) {
				/* release for .1 sec to initiate powerdown */
				printf( "power down\n");
				poweroff(0,0,0,0);
				prime_power_down = 0;
			}
			was_high_sometime |= 1;
		}
	}
exit:
	watch_on_key = 1;
}
#endif
