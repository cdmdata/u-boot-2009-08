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
#include <asm/errno.h>
#include <imx_spi.h>
#include <netdev.h>

#if CONFIG_I2C_MXC
#include <i2c.h>
#endif

#ifdef CONFIG_CMD_MMC
#include <mmc.h>
#include <fsl_esdhc.h>
#endif

#ifdef CONFIG_ARCH_MMU
#include <asm/mmu.h>
#include <asm/arch/mmu.h>
#endif

#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM
#include <asm/imx_iim.h>
#endif

#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif

#ifdef CONFIG_ANDROID_RECOVERY
#include "../common/recovery.h"
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
#define MAKE_GP(port, offset) (((port - 1) << 5) | offset)
unsigned gp_base[] = {GPIO1_BASE_ADDR, GPIO2_BASE_ADDR, GPIO3_BASE_ADDR, GPIO4_BASE_ADDR,
		GPIO5_BASE_ADDR, GPIO6_BASE_ADDR, GPIO7_BASE_ADDR};
inline void Set_GPIO_output_val(unsigned gp, unsigned val)
{
	unsigned base = gp_base[gp >> 5];
	unsigned mask = (1 << (gp & 0x1f));
	unsigned reg = readl(base + GPIO_DR);
	if (val & 1)
		reg |= mask;	/* set high */
	else
		reg &= ~mask;	/* clear low */
	writel(reg, base + GPIO_DR);

	reg = readl(base + GPIO_DIR);
	reg |= mask;		/* configure GPIO line as output */
	writel(reg, base + GPIO_DIR);
}

inline void Set_GPIO_input(unsigned gp)
{
	unsigned base = gp_base[gp >> 5];
	unsigned mask = (1 << (gp & 0x1f));
	unsigned reg;
	reg = readl(base + GPIO_DIR);
	reg &= ~mask;		/* configure GPIO line as input */
	writel(reg, base + GPIO_DIR);
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
void board_mmu_init(void)
{
	unsigned long ttb_base = PHYS_SDRAM_1 + 0x4000;
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

#define ESDCTL 0x63FD9000
#define ESDMISC 0x63FD9018

int dram_init(void)
{
///	.word	0x63FD9000, 0x83110000	/* ESDCTL, (3)14 rows, (1)10 columns (0)BL 4,  (1)32 bit width */

	unsigned shift;
	unsigned esdctl = readl(ESDCTL); 
	shift = (esdctl >> 30) & 1;		//0 = cs1 bit
	shift += ((esdctl >> 24) & 7) + 11;	//3+11 = 14 rows
	shift += ((esdctl >> 20) & 7) + 9;	//1+9 = 10 columns
	shift += ((esdctl >> 16) & 1) + 1;	//1+1 = 2 memory width (32 bit wide)
	shift += (((readl(ESDMISC) >> 5) & 1) ^ 1) + 2;	//1+2 = 3 bank bits
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

#define UART1_BASE	0x53fbc000
#define UART2_BASE	0x53fc0000
#define UART3_BASE	0x5000c000
#define UART_BASE	UART2_BASE

#define UFCR           0x0090
#define UBIR           0x00a4
#define UBMR           0x00a8
static void setup_uart(void)
{
	u32 uart = UART_BASE;
	u32 clk_src = mxc_get_clock(MXC_UART_CLK);
	u32 mult, div;
	unsigned int pad = PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
			 PAD_CTL_PUE_PULL | PAD_CTL_100K_PU | PAD_CTL_DRV_HIGH;
#define BAUDRATE 115200
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

//	mxc_request_iomux(MX51_PIN_UART1_RTS, IOMUX_CONFIG_ALT0);
//	mxc_iomux_set_pad(MX51_PIN_UART1_RTS, pad);
//	mxc_request_iomux(MX51_PIN_UART1_CTS, IOMUX_CONFIG_ALT0);
//	mxc_iomux_set_pad(MX51_PIN_UART1_CTS, pad);

	/* enable GPIO1_9 for CLK0 and GPIO1_8 for CLK02 */
//	writel(0x00000004, 0x73fa83e8);
//	writel(0x00000004, 0x73fa83ec);

	udelay(10);
	printf("setup_uart clk_src=%i mult=%i div=%i\n", clk_src, mult, div);
}


#ifdef CONFIG_I2C_MXC
static void setup_i2c(unsigned int module_base)
{
	switch (module_base) {
	case I2C1_BASE_ADDR:
		/* i2c1 SDA */
		mxc_request_iomux(MX53_PIN_EIM_D28,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_EIM_D28,
				PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		/* i2c1 SCL */
		mxc_request_iomux(MX53_PIN_EIM_D21,
				IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C1_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_EIM_D21,
				PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	case I2C2_BASE_ADDR:
		/* No device is connected via I2C2  */
		break;
	case I2C3_BASE_ADDR:
		/* GPIO_3 for I2C3_SCL */
		mxc_request_iomux(MX53_PIN_GPIO_3,
				IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C3_IPP_SCL_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_GPIO_3,
				PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		/* GPIO_6 for I2C3_SDA */
		mxc_request_iomux(MX53_PIN_GPIO_6,
				IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION);
		mxc_iomux_set_input(MUX_IN_I2C3_IPP_SDA_IN_SELECT_INPUT,
				INPUT_CTL_PATH1);
		mxc_iomux_set_pad(MX53_PIN_GPIO_6,
				PAD_CTL_PUE_PULL | PAD_CTL_PKE_ENABLE |
				PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
				PAD_CTL_HYS_ENABLE);
		break;
	default:
		printf("Invalid I2C base: 0x%x\n", module_base);
		break;
	}
}
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

void init_display_pins(void)
{
	unsigned int pad = PAD_CTL_HYS_NONE | PAD_CTL_DRV_MEDIUM | PAD_CTL_SRE_FAST ;
	int const *pins = di0_prgb_pins ;
	while (*pins) {
		mxc_request_iomux(*pins,IOMUX_CONFIG_ALT0);
		mxc_iomux_set_pad(*pins,pad);
		pins++ ;
	}

	Set_GPIO_output_val(MAKE_GP(2, 29), 0);		//tfp410 i2c sel
	mxc_request_iomux(MX53_PIN_EIM_EB1, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_EB1, PAD_CTL_HYS_ENABLE | PAD_CTL_DRV_HIGH);
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
	if ((0 == ch) && (0 != lvds)){
		pins = di0_prgb_pins ;
		while (*pins) {
			mxc_request_iomux(*pins,IOMUX_CONFIG_ALT1); /* GPIO */
			pins++ ;
		}
	}
}

void setup_core_voltages(void)
{
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);

#ifdef CONFIG_I2C_PMIC_ADDRESS
	{
		unsigned char buf[4] = { 0 };
		/* Set core voltage VDDGP to 1.05V for 800MHZ */
		buf[0] = 0x45;
		buf[1] = 0x4a;
		buf[2] = 0x52;
		if (i2c_write(CONFIG_I2C_PMIC_ADDRESS, 24, 1, buf, 3))
			return;

		/* Set DDR voltage VDDA to 1.25V */
		buf[0] = 0;
		buf[1] = 0x63;
		buf[2] = 0x1a;
		if (i2c_write(CONFIG_I2C_PMIC_ADDRESS, 26, 1, buf, 3))
			return;
	}
#endif

	/* Raise the core frequency to 800MHz */
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
		Set_GPIO_output_val(MAKE_GP(3, 19), active ? 0 : 1);
	}
}

void setup_spi(void)
{
	Set_GPIO_output_val(MAKE_GP(3, 19), 1);	/* SS1 low active */
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

#ifdef CONFIG_GET_FEC_MAC_ADDR_FROM_IIM

int fec_get_mac_addr(unsigned char *mac)
{
	u32 *iim1_mac_base =
		(u32 *)(IIM_BASE_ADDR + IIM_BANK_AREA_1_OFFSET +
			CONFIG_IIM_MAC_ADDR_OFFSET);
	int i;

	for (i = 0; i < 6; ++i, ++iim1_mac_base)
		mac[i] = (u8)readl(iim1_mac_base);

	return 0;
}
#endif

static void setup_fec(void)
{
	/* gp7[13] - low active reset pin*/
	Set_GPIO_output_val(MAKE_GP(7, 13), 0);
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
	Set_GPIO_output_val(MAKE_GP(7, 13), 1);
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
	mxc_request_iomux(MX53_PIN_KEY_ROW4, IOMUX_CONFIG_ALT0);

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
	{MMC_SDHC3_BASE_ADDR, 1, 1},
};

#ifdef CONFIG_DYNAMIC_MMC_DEVNO
int get_mmc_env_devno(void)
{
	uint soc_sbmr = readl(SRC_BASE_ADDR + 0x4);
	return (soc_sbmr & 0x00300000)  ? 1 : 0;
}
#endif


int esdhc_gpio_init(bd_t *bis)
{
	s32 status = 0;
	u32 index = 0;

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

			mxc_iomux_set_pad(MX53_PIN_SD1_CMD, 0x1E4);
			mxc_iomux_set_pad(MX53_PIN_SD1_CLK, 0xD4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA0, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA1, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA2, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_SD1_DATA3, 0x1D4);
			break;
		case 1:
			mxc_request_iomux(MX53_PIN_ATA_RESET_B,
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_IORDY,
						IOMUX_CONFIG_ALT2);
			mxc_request_iomux(MX53_PIN_ATA_DATA8,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA9,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA10,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA11,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA0,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA1,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA2,
						IOMUX_CONFIG_ALT4);
			mxc_request_iomux(MX53_PIN_ATA_DATA3,
						IOMUX_CONFIG_ALT4);

			mxc_iomux_set_pad(MX53_PIN_ATA_RESET_B, 0x1E4);
			mxc_iomux_set_pad(MX53_PIN_ATA_IORDY, 0xD4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA8, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA9, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA10, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA11, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA0, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA1, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA2, 0x1D4);
			mxc_iomux_set_pad(MX53_PIN_ATA_DATA3, 0x1D4);
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

int board_init(void)
{
#ifdef CONFIG_MFG
/* MFG firmware need reset usb to avoid host crash firstly */
#define USBCMD 0x140
	int val = readl(OTG_BASE_ADDR + USBCMD);
	val &= ~0x1; /*RS bit*/
	writel(val, OTG_BASE_ADDR + USBCMD);
#endif
	Set_GPIO_output_val(MAKE_GP(5, 0), 0);		//USB Hub reset, low power reset state
	mxc_request_iomux(MX53_PIN_EIM_WAIT, IOMUX_CONFIG_ALT1);
	mxc_iomux_set_pad(MX53_PIN_EIM_WAIT, PAD_CTL_HYS_ENABLE | PAD_CTL_DRV_HIGH);

	setup_boot_device();
	setup_soc_rev();
#ifdef CONFIG_IMX_ECSPI
	setup_spi();
#endif

	gd->bd->bi_arch_number = MACH_TYPE_MX53_NITROGEN;	/* board id for linux */
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	setup_uart();
#ifdef CONFIG_MXC_FEC
	setup_fec();
#endif

#ifdef CONFIG_I2C_MXC
	setup_i2c(CONFIG_SYS_I2C_PORT);
	setup_core_voltages();
#endif
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

static int get_rom_mac(unsigned char *mac)
{
	char *cmac = getenv("ethaddr");
	if (cmac) {
		if (!parse_mac(cmac, mac)) {
			mxc_fec_eth_set_mac_addr(mac);
			return 0;
		}
	}
	return -1;
}

int misc_init_r(void)
{
	unsigned char macAddr[6];
	if (!get_rom_mac(macAddr)) {
		printf( "Mac address %02x:%02x:%02x:%02x:%02x:%02x\n",
				macAddr[0], macAddr[1], macAddr[2],
				macAddr[3], macAddr[4], macAddr[5] );
	} else
		printf( "No mac address assigned\n" );
	return 0;
}
extern void setup_display(void);

int board_late_init(void)
{
#ifdef CONFIG_VIDEO_IMX5X
	setup_display();
#endif

	return 0;
}

int checkboard(void)
{
	printf("Board: MX53-Nitrogen");
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
