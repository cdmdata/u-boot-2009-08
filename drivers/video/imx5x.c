#include <common.h>
#include <config.h>
#include <common.h>
#include <version.h>
#include <stdarg.h>
#include <lcd.h>
#include <lcd_panels.h>
#include <lcd_multi.h>
#include <asm/io.h>
#include <asm/arch/mx51.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx51_pins.h>
#include <malloc.h>

DECLARE_GLOBAL_DATA_PTR;

#define IPU_CONF		0x5E000000
#define IPU_DISP_GEN		0x5E0000C4
#define IPU_DC_BASE		0x5E058000
#define DC_WR_CH_CONF_1		0x5e05801c
#define DC_WR_CH_CONF_5		0x5e05805c
#define IPU_CPMEM_REG_BASE	0x5F000000
#define IPU_INT_STAT1		0x5E000200
#define IPU_INT_STAT1_D0	0x00800000

#define DI0_BASE	0x5E040000
#define DI0_DMACHAN	23
#define DI0_ENABLEBIT	6	/* bit in IPU_CONF */

#define DI1_BASE	0x5E048000
#define DI1_DMACHAN	28
#define DI1_ENABLEBIT	7	/* bit in IPU_CONF */

#define DEBUG(arg...)

static unsigned end_of_ram = 0 ;

struct display_channel_t {
	unsigned di_base_addr ;
	unsigned dma_chan ;
	unsigned char enable_bit ; /* bit in IPU_CONF */
};

static struct display_channel_t const crt_channel = {
	DI0_BASE,
	DI0_DMACHAN,
	DI0_ENABLEBIT
};

static struct display_channel_t const lcd_channel = {
	DI1_BASE,
	DI1_DMACHAN,
	DI1_ENABLEBIT
};

struct ipu_ch_param_word {
	unsigned data[5];
	unsigned res[3];
};

struct ipu_ch_param {
	struct ipu_ch_param_word word[2];
};

#define IPU_CM_REG_BASE		0x5E000000
#define IDMAC_CONF		0x5E008000
#define IDMAC_CH_EN_1		0x5E008004
#define IDMAC_CH_EN_2		0x5E008008
#define IDMAC_WM_EN_1		0x5E00801C
#define CCGR5			0x73FD407C
#define CCGR5_CG5_OFFSET	10

#define CCDR			0x73FD4004
#define CCDR_HSC_HS_MASK	(0x1 << 18)
#define CCDR_IPU_HS_MASK	(0x1 << 17)

#define CLPCR			0x73FD4054
#define CLPCR_BYPASS_IPU_LPM_HS	(0x1 << 18)

#define CBCMR		0x73FD4018
#define CBCMR_IPU_SHIFT 6
#define CBCMR_IPU_MASK	(3<<CBCMR_IPU_SHIFT)

#define M4IF_CNTL_REG0	0x83FD808c
#define M4IF_CNTL_REG1	0x83FD8090

#define DI0_GENERAL		0x5E040000
#define DI1_GENERAL		0x5E048000

#define IPU_MEM_RST 		0x5E0000DC

static unsigned long const _720psettings[] = {
	0x5e058064, 0x05030000,			/* IPU_DC_BASE */
	0x5e05806c, 0x00000602,
	0x5e058074, 0x00000701,
	0x5e058064, 0x05030000,
	0x5e058068, 0x00000000,
	0x5e058068, 0x00000000,
	0x5e05806c, 0x00000602,
	0x5e058070, 0x00000000,
	0x5e058070, 0x00000000,
	0x5e05805c, 0x00000002,
	0x5e058060, 0x00000000,
	0x5e0580d4, 0x00000084,
	0x5e000000, 0x00000660,			/* IPU_CONF */
	0x5e040004, 0x00000018,			/* CLKGEN0 */
	0x5e040008, 0x00010000,			/* CLKGEN1 */
	0x5e040058, 0x00000000,			/* DI0_DW_GEN_0 */
	0x5e040058, 0x00000300,
	0x5e040118, 0x00020000,			/* DI0_DW_SET3_0 0x1E040118 */
	0x5e04000c, 0x33f90000,			/* DI0_SW_GEN0_1 0x1E04000C */
	0x5e040030, 0x10000000,
	0x5e040148, 0x00000000,
	0x5e040010, 0x33f90001,
	0x5e040034, 0x31001000,
	0x5e040148, 0x00000000,
	0x5e040014, 0x175a0000,
	0x5e040038, 0x300a2000,
	0x5e04014c, 0x00000000,
	0x5e040170, 0x000002eb,
	0x5e040018, 0x000300cb,
	0x5e04003c, 0x08000000,
	0x5e04014c, 0x02d00000,
	0x5e04001c, 0x00010a01,
	0x5e040040, 0x0a000000,
	0x5e040150, 0x00000500,
	0x5e040020, 0x00000000,
	0x5e040044, 0x00000000,
	0x5e040024, 0x00000000,
	0x5e040048, 0x00000000,
	0x5e040028, 0x00000000,
	0x5e04004c, 0x00000000,
	0x5e04002c, 0x00000000,
	0x5e040050, 0x00000000,
	0x5e040150, 0x00000500,
	0x5e040154, 0x00000000,
	0x5e040158, 0x00000000,
	0x5f080028, 0x00008885,			/* DC_TEMPLATE */
	0x5f08002c, 0x00000380,
	0x5f080030, 0x00008845,
	0x5f080034, 0x00000380,
	0x5f080038, 0x00008805,
	0x5f08003c, 0x00000380,
	0x5e040000, 0x00020004,			/* DI0_GENERAL */
	0x5e040054, 0x00004002,
	0x5e040164, 0x00000010,
	0x5e0580e8, 0x00000500,
	0x5f0005c0, 0x00000000,			/* CPMEM */
	0x5f0005c4, 0x00000000,
	0x5f0005c8, 0x00000000,
	0x5f0005cc, 0xe0001800,
	0x5f0005d0, 0x000b3c9f,
	0x5f0005d4, 0x00000000,
	0x5f0005d8, 0x00000000,
	0x5f0005dc, 0x00000000,
	0x5f0005e0, 0x12800000,
	0x5f0005e4, 0x02508000,
	0x5f0005e8, 0x00e3c000,
	0x5f0005ec, 0xf2c27fc0,
	0x5f0005f0, 0x00082ca0,
	0x5f0005f4, 0x00000000,
	0x5f0005f8, 0x00000000,
	0x5f0005fc, 0x00000000,
	0x5e060004, 0x00000088,			/* DMFC */
	0x5e060008, 0x202020f6,
	0x5e06000c, 0x00009694,
	0x5e060010, 0x2020f6f6,
	0x5e060014, 0x00000003,
	0x5f0005e8, 0x20e3c000,			/* CPMEM */
	0x5e000150, 0x00800000,			/* IPU_CH_DB_MODE_SEL_0 */
	0x5e00023c, 0x00800000,			/* IPU_CUR_BUF_0 */
	0x5e008004, 0x00800000,			/* IDMAC_CONF */
	0x5e00801c, 0x00800000,
	0x5e05801c, 0x00000004,			/* IPU_DC_BASE */
	0x5e05805c, 0x00000082,
};

static int find_later(unsigned long const *regs, u32 addr, unsigned startoffs,unsigned max ) {
	int i ;
	for (i = startoffs+2 ; i < max; i += 2) {
		if (addr == regs[i])
			return 1 ;
	}
	return 0 ;
}

static unsigned long const init_dc_mappings[] = {
	0x5e058108, 0x00000000,				/* MAP conf */
	0x5e058144, 0x000007ff,
	0x5e058108, 0x00000000,
	0x5e058144, 0x0fff07ff,
	0x5e058108, 0x00000020,
	0x5e058148, 0x000017ff,
	0x5e058108, 0x00000820,
	0x5e058108, 0x00000820,
	0x5e058148, 0x05fc17ff,
	0x5e058108, 0x00030820,
	0x5e05814c, 0x00000bfc,
	0x5e058108, 0x00830820,
	0x5e05814c, 0x11fc0bfc,
	0x5e058108, 0x14830820,
	0x5e05810c, 0x00000000,
	0x5e058150, 0x00000fff,
	0x5e05810c, 0x00000006,
	0x5e058150, 0x17ff0fff,
	0x5e05810c, 0x000000e6,
	0x5e058154, 0x000007ff,
	0x5e05810c, 0x000020e6,
	0x5e05810c, 0x000020e6,
	0x5e058154, 0x04f807ff,
	0x5e05810c, 0x000920e6,
	0x5e058158, 0x00000afc,
	0x5e05810c, 0x014920e6,
	0x5e058158, 0x0ff80afc,
	0x5e05810c, 0x2d4920e6,
	0x5e058110, 0x00000000,
	0x5e05815c, 0x000005fc,
	0x5e058110, 0x0000000c,
	0x5e05815c, 0x0dfc05fc,
	0x5e058110, 0x000001ac,
	0x5e058160, 0x000015fc,
	0x5e058110, 0x000039ac,
	0x5e00004c, 0xffffffff,		/* Enable error interrupts by default */
	0x5e000050, 0xffffffff,
	0x5e00005c, 0xffffffff,
	0x5e000060, 0xffffffff,
	// _ipu_dmfc_init(DMFC_NORMAL, 1);
	0x5e06001c, 0x00000002,
	0x5e060004, 0x00000090,
	0x5e060008, 0x202020f6,
	0x5e06000c, 0x00009694,
	0x5e060010, 0x2020f6f6,
        /* IDMAC_CHA_PRI */
        0x5e008014, 0x18800001,
	/* Set MCU_T to divide MCU access window into 2 */
	0x5e0000c4, 0x00600000,
	0x5f040004, 0x80000000,			/* DP_GRAPH_WIND_CTRL_SYNC: set alpha value */
	0x5f040000, 0x00000004,			/* DP_COM_CONF_SYNC: set global alpha */
};

/* For the purposes of U-Boot, all of the uses of _ipu_dc_link_event are
 * effectively constant, although they're initialized separately for each
 * display port (DI0/DI1). The channels are essentially fixed at MEM_BG_SYNC
 * so we're just making them constant from the get-go.
 */
static unsigned long const late_dc_mappings[] = {
	0x5e058000, 0xffff0000,
	0x5e058004, 0x00000000,
	0x5e058008, 0x00000000,
	0x5e05800c, 0x00000000,
	0x5e058010, 0x00000000,
	0x5e058014, 0x00000000,
	0x5e058018, 0x00000000,
	0x5e05801c, 0x0000008e,
	0x5e058020, 0x00000000,
	0x5e058024, 0x02030000,
	0x5e058028, 0x00000000,
	0x5e05802c, 0x00000302,
	0x5e058030, 0x00000000,
	0x5e058034, 0x00000401,
	0x5e058038, 0x00000000,
	0x5e05803c, 0x00000000,
	0x5e058040, 0x00000000,
	0x5e058044, 0x00000000,
	0x5e058048, 0x00000000,
	0x5e05804c, 0x00000000,
	0x5e058050, 0x00000000,
	0x5e058054, 0x00000000,
	0x5e058058, 0x00000000,
	0x5e05805c, 0x00000082,
	0x5e058060, 0x00000000,
	0x5e058064, 0x05030000,
	0x5e058068, 0x00000000,
	0x5e05806c, 0x00000602,
	0x5e058070, 0x00000000,
	0x5e058074, 0x00000701,
	0x5e058078, 0x00000000,
	0x5e05807c, 0x00000000,
	0x5e058080, 0x00000000,
	0x5e058084, 0x00000000,
	0x5e058088, 0x00000000,
	0x5e05808c, 0x00000000,
	0x5e058090, 0x00000000,
	0x5e058094, 0x00000000,
	0x5e058098, 0x00000000,
	0x5e05809c, 0x00000000,
	0x5e0580a0, 0x00000000,
	0x5e0580a4, 0x00000000,
	0x5e0580a8, 0x00000000,
	0x5e0580ac, 0x00000000,
	0x5e0580b0, 0x00000000,
	0x5e0580b4, 0x00000000,
	0x5e0580b8, 0x00000000,
	0x5e0580bc, 0x00000000,
	0x5e0580c0, 0x00000000,
	0x5e0580c4, 0x00000000,
	0x5e0580c8, 0x00000000,
	0x5e0580cc, 0x00000000,
	0x5e0580d0, 0x00000000,
	0x5e0580d4, 0x00000084,
	0x5e0580d8, 0x00000042,
	0x5e0580dc, 0x00000042,
	0x5e0580e0, 0x00000042,
	0x5e0580e4, 0x00000042,
	0x5e0580e8, 0x00000500,
	0x5e0580ec, 0x00000320,
	0x5e0580f0, 0x00000000,
	0x5e0580f4, 0x00000000,
	0x5e0580f8, 0x00000000,
	0x5e0580fc, 0x00000000,
	0x5e058100, 0x00000000,
	0x5e058104, 0x00000000,
	0x5e058108, 0x14830820,
	0x5e05810c, 0x2d4920e6,
	0x5e058110, 0x000039ac,
	0x5e058114, 0x00000000,
	0x5e058118, 0x00000000,
	0x5e05811c, 0x00000000,
	0x5e058120, 0x00000000,
	0x5e058124, 0x00000000,
	0x5e058128, 0x00000000,
	0x5e05812c, 0x00000000,
	0x5e058130, 0x00000000,
	0x5e058134, 0x00000000,
	0x5e058138, 0x00000000,
	0x5e05813c, 0x00000000,
	0x5e058140, 0x00000000,
	0x5e058144, 0x0fff07ff,
	0x5e058148, 0x05fc17ff,
	0x5e05814c, 0x11fc0bfc,
	0x5e058150, 0x17ff0fff,
	0x5e058154, 0x04f807ff,
	0x5e058158, 0x0ff80afc,
	0x5e05815c, 0x0dfc05fc,
	0x5e058160, 0x000015fc,
	0x5e058164, 0x00000000,
	0x5e058168, 0x00000000,
	0x5e05816c, 0x00000000,
	0x5e058170, 0x00000000,
	0x5e058174, 0x00000000,
	0x5e058178, 0x00000000,
	0x5e05817c, 0x00000000,
	0x5e058180, 0x00000000,
	0x5e058184, 0x00000000,
	0x5e058188, 0x00000000,
	0x5e05818c, 0x00000000,
	0x5e058190, 0x00000000,
	0x5e058194, 0x00000000,
	0x5e058198, 0x00000000,
	0x5e05819c, 0x00000000,
	0x5e0581a0, 0x00000000,
	0x5e0581a4, 0x00000000,
	0x5e0581a8, 0x00000000,
	0x5e0581ac, 0x00000000,
	0x5e0581b0, 0x00000000,
	0x5e0581b4, 0x00000000,
	0x5e0581b8, 0x00000000,
	0x5e0581bc, 0x00000000,
	0x5e0581c0, 0x00000000,
	0x5e0581c4, 0x00000000,
	0x5e0581c8, 0x00000050,

	/* DC template registers for both displays */
	0x5f080028, 0x00008885,
	0x5f08002c, 0x00000380,
	0x5f080030, 0x00008845,
	0x5f080034, 0x00000380,
	0x5f080038, 0x00008805,
	0x5f08003c, 0x00000380,
	0x5f080010, 0x00020885,
	0x5f080014, 0x00000380,
	0x5f080018, 0x00020845,
	0x5f08001c, 0x00000380,
	0x5f080020, 0x00020805,
	0x5f080024, 0x00000380,
};

static void write_regs(unsigned long const *regs, unsigned array_size) {
	int i ;
	for (i = 0; i < array_size; i += 2) {
//		DEBUG( "0x%08lx ,0x%08lx\n", regs[i],regs[i+1]);
		writel(regs[i+1], regs[i]);
	}
}

static void check_regs(char const *which, unsigned long const *regs, unsigned array_size) {
	int i ;
	/* read 'em back */
	for (i = 0; i < array_size; i += 2) {
		u32 value = readl(regs[i]);
		if ((value != regs[i+1])
		    &&
		    (0 == find_later(regs,regs[i],i,array_size))) {
			DEBUG ("%s: reg 0x%08lx ,0x%08x != 0x%08lx at offset %d\n", 
				__func__, regs[i],value,regs[i+1], i/2);
		}
	}
}

static void enable_ipu_clock(void)
{
	/* from mach-mx5/clock.c ipu_clk */

	/* from _clk_ipu_set_parent:
	 *
	 * We're setting the parent to ahb_clk, which is 2 (doc says 3) 
	 */
	__REG(CBCMR) = (__REG(CBCMR) & ~CBCMR_IPU_MASK) | (3<<CBCMR_IPU_SHIFT);

        /* from .enable_reg and .enable_shift */
	__REG(CCGR5) |= 3 << CCGR5_CG5_OFFSET ;

	/* from _clk_ipu_enable: Handshake with IPU... */
	__REG(CCDR) &= ~CCDR_IPU_HS_MASK ;
	__REG(CLPCR) &= ~CLPCR_BYPASS_IPU_LPM_HS ;

	/* from sdram_autogating.c::enable(). Registers not documented */
	
	/* Set the Fast arbitration Power saving timer */
	__REG(M4IF_CNTL_REG1) = (__REG(M4IF_CNTL_REG1) & ~0xff) | 0x09 ;
	
	/*Allow for automatic gating of the EMI internal clock.
	 * If this is done, emi_intr CCGR bits should be set to 11.
	 */
	__REG(M4IF_CNTL_REG0) &= ~5 ;
}

static void disable_ipu_clock(void)
{
	/*
	 * Disable SDRAM autogating
	 */
	__REG(M4IF_CNTL_REG0) |= 4 ;

        /* from .enable_reg and .enable_shift */
	__REG(CCGR5) = (__REG(CCGR5) & ~(3 << CCGR5_CG5_OFFSET));

	/* No handshake with IPU whe dividers are changed
	 * as its not enabled. 
	 */
	__REG(CCDR) |= CCDR_IPU_HS_MASK ;
	__REG(CLPCR) |= CLPCR_BYPASS_IPU_LPM_HS ;
}


void setup_display(void)
{
	unsigned int pad = PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST ;
	
        __REG(MIPI_HSC_BASE_ADDR) = 0xF00 ; // indeed!

DEBUG( "%s\n", __func__ );
	mxc_request_iomux(MX51_PIN_DISP1_DAT0,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT0,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT1,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT1,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT2,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT2,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT3,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT3,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT4,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT4,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT5,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT5,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT6,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT6,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT7,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT7,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT8,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT8,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT9,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT9,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT10,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT10,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT11,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT11,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT12,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT12,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT13,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT13,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT14,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT14,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT15,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT15,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT16,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT16,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT17,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT17,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT18,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT18,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT19,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT19,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT20,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT20,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT21,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT21,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT22,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT22,pad);
	mxc_request_iomux(MX51_PIN_DISP1_DAT23,IOMUX_CONFIG_ALT0);
	mxc_iomux_set_pad(MX51_PIN_DISP1_DAT23,pad);
	mxc_request_iomux(MX51_PIN_DI1_PIN11,IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX51_PIN_DI1_PIN12,IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX51_PIN_DI1_PIN13,IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX51_PIN_DI1_D0_CS,IOMUX_CONFIG_ALT1);
	mxc_request_iomux(MX51_PIN_DI1_D1_CS,IOMUX_CONFIG_ALT4);
	mxc_request_iomux(MX51_PIN_DI_GP4,IOMUX_CONFIG_ALT4);

	writel(0,DI0_GENERAL);
	writel(0,DI1_GENERAL);

	enable_ipu_clock();
	writel(0x807FFFFF, IPU_MEM_RST);
	while (readl(IPU_MEM_RST) & 0x80000000) ;

	write_regs(init_dc_mappings,ARRAY_SIZE(init_dc_mappings));
	check_regs("dcmap",init_dc_mappings,ARRAY_SIZE(init_dc_mappings));

	write_regs(late_dc_mappings,ARRAY_SIZE(late_dc_mappings));
	check_regs("late_dc",late_dc_mappings,ARRAY_SIZE(late_dc_mappings));

	set_pixel_clock(0,0);
	set_pixel_clock(1,0);
	disable_ipu_clock();

	end_of_ram = gd->bd->bi_dram[0].start+gd->bd->bi_dram[0].size;
	DEBUG ("%s: end-of-ram == 0x%08x\n", __func__, end_of_ram );
}


void disable_lcd_panel(void)
{
	DEBUG ("%s: disable both displays here\n", __func__ );
	__REG(IPU_DISP_GEN) &= ~(3 << 24);	// stop DI0/DI1
	__REG(IDMAC_WM_EN_1)  &= ~(3<<23);	// No watermarking
	__REG(IDMAC_CH_EN_1)  &= ~(3<<23); 	// Disable DMA channel
	__REG(IPU_CONF+0x150) &= ~(3<<23);	// DB_MODE_SEL
	__REG(IPU_CONF+0x23C) &= ~(3<<23);	// CUR_BUF

	disable_ipu_clock();
        end_of_ram = gd->bd->bi_dram[0].start+gd->bd->bi_dram[0].size;
}

#define DI_DW_GEN_ACCESS_SIZE_OFFSET	24
#define DI_DW_GEN_COMPONENT_SIZE_OFFSET	16

#define DI_GEN_DI_PIXCLK_ACTH 0x20000
#define DI_GEN_POLARITY_1	0x00000001
#define DI_GEN_POLARITY_2	0x00000002
#define DI_GEN_POLARITY_3	0x00000004
#define DI_GEN_POLARITY_4	0x00000008
#define DI_GEN_POLARITY_5	0x00000010
#define DI_GEN_POLARITY_6	0x00000020
#define DI_GEN_POLARITY_7	0x00000040
#define DI_GEN_POLARITY_8	0x00000080

#define DI_VSYNC_SEL_OFFSET		13
#define DI_POL_DRDY_DATA_POLARITY	0x00000080
#define DI_POL_DRDY_POLARITY_15		0x00000010

static void _ipu_di_sync_config
	( unsigned base_addr,
	  int wave_gen,
	  int run_count, 
	  int run_src,
	  int offset_count, 
	  int offset_src,
	  int repeat_count, 
	  int cnt_clr_src,
	  int cnt_polarity_gen_en,
	  int cnt_polarity_clr_src,
	  int cnt_polarity_trigger_src,
	  int cnt_up, 
	  int cnt_down)
{
	u32 reg;

	if ((run_count >= 0x1000) || (offset_count >= 0x1000) || (repeat_count >= 0x1000) || (cnt_up >= 0x400) || (cnt_down >= 0x400)) {
		printf ("display counters out of range.\n");
		return;
	}

	reg = (run_count << 19) | (++run_src << 16) | (offset_count << 3) | ++offset_src;
	__REG(base_addr+0x000C+(wave_gen-1)*4) = reg ; // DI_SW_GEN0
	
	reg = (cnt_polarity_gen_en << 29) | (++cnt_clr_src << 25) | (++cnt_polarity_trigger_src << 12) | (++cnt_polarity_clr_src << 9);
	reg |= (cnt_down << 16) | cnt_up;
	if (repeat_count == 0) {
		/* Enable auto reload */
		reg |= 0x10000000;
	}
	__REG(base_addr+0x0030+(wave_gen-1)*4) = reg ;	// DI_SW_GEN1
	
	reg = __REG(base_addr+0x0148+((wave_gen-1)/2)*4);	// DI_STP_REP
	reg &= ~(0xFFFF << (16 * ((wave_gen - 1) & 0x1)));
	reg |= repeat_count << (16 * ((wave_gen - 1) & 0x1));
	__REG(base_addr+0x0148+((wave_gen-1)/2)*4) = reg ;
}

enum di_sync_wave {
	DI_SYNC_NONE = -1,
	DI_SYNC_CLK = 0,
	DI_SYNC_INT_HSYNC = 1,
	DI_SYNC_HSYNC = 2,
	DI_SYNC_VSYNC = 3,
	DI_SYNC_OE = 4,
	DI_SYNC_DE = 5,
};

inline static void ipu_ch_param_set_field(void *base, unsigned char w, unsigned start_bit, unsigned num_bits, unsigned value)
{
	char *addr = (char *)base + (w*0x20);
	unsigned startw = start_bit/32 ;
	unsigned endw = (start_bit+num_bits-1)/32 ;
        unsigned max = (1<<num_bits)-1 ;
	unsigned *words = ((unsigned *)addr) + startw ;

	if (startw == endw) {
		unsigned shifted = value << (start_bit & 31);
		unsigned mask = ~(max << (start_bit&31));
		unsigned oldval = words[0] & ~mask ;
		if( shifted != oldval ) {
                        words[0] = (words[0]&mask) | shifted ;
		}
	}
	else { 
		start_bit &= 31 ;
		unsigned oldlow = words[0];
		unsigned lowbits = 32-start_bit ;
		unsigned lowval = value & ((1<<lowbits)-1);
		unsigned newlow = (oldlow&((1<<start_bit)-1)) | (lowval<<start_bit);
		unsigned oldhigh = words[1];
		unsigned highbits = num_bits-lowbits;
		unsigned highval = value >> lowbits ;
		unsigned newhigh = (oldhigh&(~((1<<highbits)-1))) | highval ;
		words[0] = newlow ;
		words[1] = newhigh ;
	} 
}

static inline void _ipu_ch_params_set_packing(struct ipu_ch_param *p,
					      int red_width, int red_offset,
					      int green_width, int green_offset,
					      int blue_width, int blue_offset,
					      int alpha_width, int alpha_offset)
{
	/* Setup red width and offset */
	ipu_ch_param_set_field(p, 1, 116, 3, red_width - 1);
	ipu_ch_param_set_field(p, 1, 128, 5, red_offset);
	/* Setup green width and offset */
	ipu_ch_param_set_field(p, 1, 119, 3, green_width - 1);
	ipu_ch_param_set_field(p, 1, 133, 5, green_offset);
	/* Setup blue width and offset */
	ipu_ch_param_set_field(p, 1, 122, 3, blue_width - 1);
	ipu_ch_param_set_field(p, 1, 138, 5, blue_offset);
	/* Setup alpha width and offset */
	ipu_ch_param_set_field(p, 1, 125, 3, alpha_width - 1);
	ipu_ch_param_set_field(p, 1, 143, 5, alpha_offset);
}

static void disable_imx_display(void)
{
	printf ("%s: disable display here... Don't I need a display reference???\n", __func__ );
	disable_lcd_panel();
}

#define to565(r,g,b) \
    ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

static int display_bitmap(struct lcd_t *panel, bmp_image_t *bmp, int startx, int starty)
{
	uchar *bmap;
	ushort padded_line;
	unsigned long width, height;
	unsigned long pwidth = panel->info.xres ;
	unsigned colors,bpix;
	unsigned long compression;
	if (!((bmp->header.signature[0]=='B') &&
		(bmp->header.signature[1]=='M'))) {
		printf ("Error: no valid bmp image at %p\n", bmp);
		return 1;
	}
	if ((startx >= panel->info.xres)||(starty >= panel->info.yres)) {
		printf("%s: nothing to display\n", __func__ );
		return 1 ;
	}

	width = le32_to_cpu (bmp->header.width);
	height = le32_to_cpu (bmp->header.height);
	colors = 1<<le16_to_cpu (bmp->header.bit_count);
	compression = le32_to_cpu (bmp->header.compression);
	padded_line = (width&0x3) ? ((width&~0x3)+4) : (width);
	if ((startx + width)>pwidth)
		width = pwidth - startx;
	if ((starty + height)>panel->info.yres)
		height = panel->info.yres - starty;

	bpix = le16_to_cpu(bmp->header.bit_count);

	if ((startx + width)>pwidth)
		width = pwidth - startx;
	if ((starty + height)>panel->info.yres)
		height = panel->info.yres - starty;

	bmap = (uchar *)bmp + le32_to_cpu (bmp->header.data_offset);
	switch (bpix) {
		case 8: {
                        bmp_color_table_entry_t *colors = bmp->color_table ;
			unsigned short rgb16 = 0xffff ;
			int prevpalette = -1 ;
			unsigned char *row = ((unsigned char *)panel->fbAddr + ((starty+height-1)*panel->stride));
			unsigned y ;
			for (y = 0 ; y < height ; y++) {
				unsigned char *nextOut = row ;
				unsigned char *nextIn = bmap ;
				unsigned x ;
				for (x = 0 ; x < width ; x++ ) {
					unsigned char pidx = *nextIn++ ;
					if (pidx != prevpalette) {
						prevpalette = pidx ;
						bmp_color_table_entry_t cte ;
						memcpy(&cte,colors+pidx,sizeof(cte));
						rgb16 = to565(cte.red,cte.green,cte.blue);
					}
					memcpy(nextOut,&rgb16,sizeof(rgb16));
					nextOut += sizeof(rgb16);
				}
                                bmap += padded_line ;
				row -= panel->stride ;
			}

			break;
		}
		default:
			printf ("%s: %u bpp images not yet supported\n", __func__, bpix );
			return -1 ;
	}

	return (0);
}

static int checked_transp = 0 ;
static void drawchar16(struct lcd_t *lcd,unsigned char const *src, unsigned w, unsigned h)
{
	int transparent_back = 0 ;
	unsigned char *dest = (uchar *)lcd->fbAddr + (lcd->stride*h*lcd->y) + lcd->x*w*2 ;
	unsigned row ;

        if( 0 == checked_transp ){
           transparent_back = (0 != getenv("transp_lcdtext") );
           checked_transp = 1 ;
        }
	for (row=0;  row < h;  ++row, dest += lcd->stride, src++ ){
		uchar mask = '\x80' ;
		uchar bits = *src ;
		uchar *d = dest ;
		while( mask ){
			if(bits & mask)
				memcpy(d,&lcd->fg,2);
			else if(!transparent_back)
				memcpy(d,&lcd->bg,2);
			d += 2 ;
			mask >>= 1 ;
		}
	}
}

static void init_display
	( struct lcd_t *lcd,
          struct display_channel_t const *channel )
{
	u32 div = mxc_get_clock(MXC_IPU_CLK) / lcd->info.pixclock ;
	u16 mask = div-1 ;
	u32 wgen = (mask << DI_DW_GEN_ACCESS_SIZE_OFFSET) 
		 | (mask << DI_DW_GEN_COMPONENT_SIZE_OFFSET)
	         | 3 << 8 ;
	unsigned disp = (DI0_BASE != (channel->di_base_addr));
	u32 h_total, v_total ;
	void *params ;
	u32 screen_size = lcd->info.xres*lcd->info.yres*2 ;
	u32 address ;

	lcd->fbAddr = 0 ;
	lcd->fbMemSize = screen_size ;
	lcd->stride = (2*lcd->info.xres);
	lcd->x = lcd->y = 0 ;
	lcd->colorCount = 65536 ;
	lcd->fg 	= 0 ;		// black
	lcd->bg 	= 0xffff ;	// white
	lcd->disable = disable_imx_display ;
	lcd->display_bmp = display_bitmap ;
	lcd->drawchar = drawchar16 ;

	enable_ipu_clock();
	set_pixel_clock(disp,lcd->info.pixclock);

	h_total = lcd->info.xres + lcd->info.hsync_len + lcd->info.left_margin + lcd->info.right_margin;
	v_total = lcd->info.yres + lcd->info.vsync_len + lcd->info.upper_margin + lcd->info.lower_margin;

	DEBUG ("%s: init display here, ram == 0x%08x/%p/%p\n", __func__, end_of_ram, gd, gd->bd );
	if (0 == end_of_ram) {
		printf( "no ram: bailing out: %p\n", gd );
	}
	__REG(channel->di_base_addr + 0x0058) = wgen ;
	__REG(channel->di_base_addr + 0x118) = div<<17 ;

	/* Setup internal HSYNC waveform */
	_ipu_di_sync_config(channel->di_base_addr,
			    DI_SYNC_INT_HSYNC, 
			    h_total - 1, 
			    DI_SYNC_CLK,
                            0, 
			    DI_SYNC_NONE, 
			    0, 
			    DI_SYNC_NONE, 
			    0, 
			    DI_SYNC_NONE,
                            DI_SYNC_NONE, 
			    0, 
			    0);

	/* Setup external (delayed) HSYNC waveform */
	_ipu_di_sync_config(channel->di_base_addr,
			    DI_SYNC_HSYNC, 
			    h_total - 1,
			    DI_SYNC_CLK, 
			    0, 
			    DI_SYNC_CLK,
			    0, 
			    DI_SYNC_NONE, 
			    1, 
			    DI_SYNC_NONE,
			    DI_SYNC_CLK, 
			    0, 
			    lcd->info.hsync_len * 2);

	/* Setup VSYNC waveform */
	_ipu_di_sync_config(channel->di_base_addr,
			    DI_SYNC_VSYNC, 
			    v_total - 1,
			    DI_SYNC_INT_HSYNC, 
			    0, 
			    DI_SYNC_NONE, 
			    0,
			    DI_SYNC_NONE, 
			    1, 
			    DI_SYNC_NONE,
			    DI_SYNC_INT_HSYNC, 
			    0, 
			    lcd->info.vsync_len * 2);

	__REG(channel->di_base_addr+0x0170) = v_total - 1 ;

	/* Setup active data waveform to sync with DC */
	_ipu_di_sync_config(channel->di_base_addr,
			    DI_SYNC_OE, 
			    0, 
			    DI_SYNC_HSYNC,
			    lcd->info.vsync_len + lcd->info.upper_margin, 
			    DI_SYNC_HSYNC, 
			    lcd->info.yres,
			    DI_SYNC_VSYNC, 
			    0, 
			    DI_SYNC_NONE,
			    DI_SYNC_NONE, 
			    0, 
			    0);
	_ipu_di_sync_config(channel->di_base_addr,
			    DI_SYNC_DE, 
			    0, 
			    DI_SYNC_CLK,
			    lcd->info.hsync_len + lcd->info.left_margin, 
			    DI_SYNC_CLK,
			    lcd->info.xres, 
			    4, 
			    0, 
			    DI_SYNC_NONE, 
			    DI_SYNC_NONE, 
			    0,
			    0);

	/* reset all unused counters */
	__REG(channel->di_base_addr+0x000C+(6-1)*4) = 0 ; // DI_SW_GEN0
	__REG(channel->di_base_addr+0x0030+(6-1)*4) = 0 ; // DI_SW_GEN1
	__REG(channel->di_base_addr+0x000C+(7-1)*4) = 0 ; // DI_SW_GEN0
	__REG(channel->di_base_addr+0x0030+(7-1)*4) = 0 ; // DI_SW_GEN1
	__REG(channel->di_base_addr+0x000C+(8-1)*4) = 0 ; // DI_SW_GEN0
	__REG(channel->di_base_addr+0x0030+(8-1)*4) = 0 ; // DI_SW_GEN1
	__REG(channel->di_base_addr+0x000C+(9-1)*4) = 0 ; // DI_SW_GEN0
	__REG(channel->di_base_addr+0x0030+(9-1)*4) = 0 ; // DI_SW_GEN1
        __REG(channel->di_base_addr+0x0148+((6-1)/2)*4) &= 0x0000FFFF; // DI_STP_REP
        __REG(channel->di_base_addr+0x0148+((7-1)/2)*4) = 0 ;
        __REG(channel->di_base_addr+0x0148+((9-1)/2)*4) = 0 ;

	__REG(channel->di_base_addr) = DI_GEN_DI_PIXCLK_ACTH*(0 != lcd->info.pclk_redg)
					| DI_GEN_POLARITY_2*(0 != lcd->info.hsyn_acth)
					| DI_GEN_POLARITY_3*(0 != lcd->info.vsyn_acth);
	__REG(channel->di_base_addr+0x0054) = ((DI_SYNC_VSYNC-1) << DI_VSYNC_SEL_OFFSET) | 0x00000002 ; // DI_SYNC_AS_GEN
	__REG(channel->di_base_addr+0x0164) &= ~(DI_POL_DRDY_DATA_POLARITY );	// DI_POL
	__REG(channel->di_base_addr+0x0164) |= DI_POL_DRDY_POLARITY_15 ;

	__REG(IPU_DC_BASE+0x00E8 + (disp*4)) = lcd->info.xres ;

        params = (void *)(IPU_CPMEM_REG_BASE+(0x40*channel->dma_chan));

	ipu_ch_param_set_field(params, 0, 125, 13, lcd->info.xres - 1);
	ipu_ch_param_set_field(params, 0, 138, 12, lcd->info.yres - 1);
	ipu_ch_param_set_field(params, 1, 102, 14, lcd->stride - 1);

	/* EBA is 8-byte aligned */
	address = end_of_ram - screen_size ;
        lcd->fbAddr = (void *)address ;
	end_of_ram = address ;
	DEBUG( "screen size is %u\n", screen_size );
	DEBUG( "frame buffer at 0x%08x/0x%08x -> 0x%08x/0x%08x\n", address, address, address>>3, address>>3 );
	ipu_ch_param_set_field(params, 1, 0, 29, address >> 3);
	ipu_ch_param_set_field(params, 1, 29, 29, address >> 3);
	ipu_ch_param_set_field(params, 0, 107, 3, 3);	/* bits/pixel */
	ipu_ch_param_set_field(params, 1, 85, 4, 7);	/* pix format */
	ipu_ch_param_set_field(params, 1, 78, 7, 15);	/* burst size */
	ipu_ch_param_set_field(params, 1, 93, 2, 1); 	/* high priority */
	_ipu_ch_params_set_packing(params, 5, 0, 6, 5, 5, 11, 8, 16);

	__REG(IPU_CONF+0x150) |= (1<<channel->dma_chan);      // DB_MODE_SEL 
        __REG(IPU_CONF+0x23C) |= (1<<channel->dma_chan);      // CUR_BUF     

	__REG(IDMAC_CH_EN_1)  |= (1<<channel->dma_chan);
	__REG(IDMAC_WM_EN_1)  |= (1<<channel->dma_chan);

	__REG(DC_WR_CH_CONF_1) |= 1<<2 ;

        __REG(DC_WR_CH_CONF_1) |= 4 << 5 ;  /* set normal display type */
	__REG(IPU_DISP_GEN) |= 1 << (disp+24);
	__REG(IPU_CONF) |= 0x620 + (1<<(6+disp));
	memset((void *)address, 0xff, screen_size);
}

static void init_imx_crt( struct lcd_t *lcd )
{
	DEBUG ("%s: Use this device on CRT (HDMI) channel\n", __func__ );
	init_display(lcd,&crt_channel);
	check_regs("720p", _720psettings,ARRAY_SIZE(_720psettings));
}

static void init_imx_lcd( struct lcd_t *lcd )
{
	DEBUG ("%s: Use this device on LCD (PRGB) channel\n", __func__ );
	__REG(PWM1_BASE_ADDR) = 0x03c60001 ; // turn on backlight
	init_display(lcd,&lcd_channel);
}

struct lcd_t *newPanel( struct lcd_panel_info_t const *info )
{
   unsigned idx ;
   unsigned num_lcd = 0 ;
   unsigned num_crt = 0 ;
   struct lcd_t *rval = 0 ;

   /*
    * See what else is instantiated
    */
   for( idx = 0 ; idx < getPanelCount(); idx++ ){
      struct lcd_t *oldp = getPanel(idx);
      if( oldp ){
         if( oldp->info.crt )
            num_crt++ ;
         else
            num_lcd++ ;
      }
   }

   rval = (struct lcd_t *)malloc(sizeof(struct lcd_t));
   memset( rval, 0, sizeof(*rval) );
   memcpy( &rval->info, info, sizeof(rval->info) );

   if( info->crt ){
      if( 0 == num_crt ){
         init_imx_crt(rval);
      }
      else {
         printf( "Only one CRT is supported on i.MX5x!\n" );
         free(rval); rval = 0 ;
      }
   } else {
      if( 0 == num_lcd ){
         init_imx_lcd(rval);
      } else {
         printf( "Only one raw LCD panel is supported on i.MX5x!\n" );
         free(rval); rval = 0 ;
      }
   }

   return rval ;
}

int
do_ckregs(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	DEBUG("%s\n", __func__ );

	if (1 < argc) {
		unsigned long addr = simple_strtoul (argv[1],0,0);
		if (addr && (0 == (addr&3))) {
			unsigned long *longs = (unsigned long *)addr ;
			while (*longs) {
				unsigned long count ;
				addr = *longs++ ;
				count = *longs++ ;
				DEBUG( "check %lu longs at 0x%08lx\n", count, addr );
				while (0 < count--) {
					unsigned expected = *longs++ ;
					unsigned val = __REG(addr); 
					if (val != expected) {
						DEBUG( "%08lx: %08x not expected %08x\n", addr, val, expected );
					}
                                        addr += 4 ;
				}
			}
		}
	} else {
		check_regs("720p", _720psettings,ARRAY_SIZE(_720psettings));
	}
	return 0 ;
}

U_BOOT_CMD(
	ckregs, 2, 0,	do_ckregs,
	"ckregs - check 720P regs\n",
	""
);

int
do_timevsync(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned long long start ;
	unsigned long elapsed ;
	__REG(IPU_INT_STAT1) = IPU_INT_STAT1_D0 ;
	
	/* wait for one sync first */
	do {
	} while ( 0 == (__REG(IPU_INT_STAT1)&IPU_INT_STAT1_D0) );

	/* once more to measure things */
        start = get_ticks();
	__REG(IPU_INT_STAT1) = IPU_INT_STAT1_D0 ;
	do {
	} while ( 0 == (__REG(IPU_INT_STAT1)&IPU_INT_STAT1_D0) );

	elapsed = (unsigned long)(get_ticks()-start);
	printf( "%lu ticks: %lu Hz\n", elapsed, CONFIG_SYS_HZ/(elapsed?elapsed:1));
	return 0 ;
}

U_BOOT_CMD(
	timevsync, 1, 0,	do_timevsync,
	"timevsync - time vsync\n",
	""
);

