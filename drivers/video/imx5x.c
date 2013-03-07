#include <common.h>
#include <config.h>
#include <common.h>
#include <version.h>
#include <stdarg.h>
#include <lcd.h>
#include <lcd_panels.h>
#include <lcd_multi.h>
#include <asm/io.h>
#include <asm/arch/iomux.h>
#include <malloc.h>
#include <asm/clock.h>
unsigned get_pixel_clock(unsigned which);

#define CCDR			(CCM_BASE_ADDR + 0x004)
#define CBCMR			(CCM_BASE_ADDR + 0x018)
#define CLPCR			(CCM_BASE_ADDR + 0x054)
#define CCGR5			(CCM_BASE_ADDR + 0x07C)
#define CCGR6			(CCM_BASE_ADDR + 0x080)

#define IPU_CONF		(IPU_CM_REG_BASE + 0x000)
#define IPU_DISP_GEN		(IPU_CM_REG_BASE + 0x0C4)
#define IPU_MEM_RST 		(IPU_CM_REG_BASE + 0x0DC)
#define IPU_INT_STAT1		(IPU_CM_REG_BASE + 0x200)
#define IPU_INT_STAT3		(IPU_CM_REG_BASE + 0x208)

#define IDMAC_CONF		(IPU_IDMAC_BASE + 0x000)
#define IDMAC_CH_EN_1		(IPU_IDMAC_BASE + 0x004)
#define IDMAC_CH_EN_2		(IPU_IDMAC_BASE + 0x008)
#define IDMAC_WM_EN_1		(IPU_IDMAC_BASE + 0x01C)

#define DC_WR_CH_CONF_1		(IPU_DC_BASE + 0x01c)
#define DC_WR_CH_CONF_5		(IPU_DC_BASE + 0x05c)

#define DI0_GENERAL		(IPU_DI0_BASE + 0x000)
#define DI0_BS_CLKGEN0		(IPU_DI0_BASE + 0x004)

#define DI1_GENERAL		(IPU_DI1_BASE + 0x000)
#define DI1_BS_CLKGEN0		(IPU_DI1_BASE + 0x004)

#define DI0_DMACHAN	23
#define DI0_ENABLEBIT	6	/* bit in IPU_CONF */

#define DI1_DMACHAN	28
#define DI1_ENABLEBIT	7	/* bit in IPU_CONF */

#define IPU_INT_STAT3_MASK	((1<<DI0_DMACHAN)|(1<<DI1_DMACHAN))

DECLARE_GLOBAL_DATA_PTR;
#define DEBUG(arg...)

static unsigned end_of_ram;
#ifdef CONFIG_MX53
static unsigned lvds[2];
#define IOMUXC_GPR2	((IOMUXC_BASE_ADDR)+8)

#ifdef CONFIG_LVDS0_24BIT
#define GPR2_LVDS0_WIDTH	(1 << 5)
#else
#define GPR2_LVDS0_WIDTH	(0 << 5)
#endif

#ifdef CONFIG_LVDS0_JEIDA
#define GPR2_LVDS0_MAPPING	(1 << 6)
#else
#define GPR2_LVDS0_MAPPING	(0 << 6)
#endif

#ifdef CONFIG_LVDS1_24BIT
#define GPR2_LVDS1_WIDTH	(1 << 7)
#else
#define GPR2_LVDS1_WIDTH	(0 << 7)
#endif

#ifdef CONFIG_LVDS1_JEIDA
#define GPR2_LVDS1_MAPPING	(1 << 8)
#else
#define GPR2_LVDS1_MAPPING	(0 << 8)
#endif

#define GPR2_BASE_VALUE		(GPR2_LVDS0_WIDTH | GPR2_LVDS0_MAPPING | GPR2_LVDS1_WIDTH | GPR2_LVDS1_MAPPING)
#endif

struct display_channel_t {
	char const *name ;
	unsigned di_base_addr ;
	unsigned dma_chan ;
	unsigned clk_reg ;
	unsigned char enable_bit ; /* bit in IPU_CONF */
	char const *kernel_suffix ;
};

static struct display_channel_t const crt_channel = {
	"hdmi",
	IPU_DI0_BASE,
	DI0_DMACHAN,
        DI0_BS_CLKGEN0,
	DI0_ENABLEBIT,
	"888"
};

static struct display_channel_t const lcd_channel = {
	"prgb",
	IPU_DI1_BASE,
	DI1_DMACHAN,
        DI1_BS_CLKGEN0,
	DI1_ENABLEBIT,
	"565"
};

struct ipu_ch_param_word {
	unsigned data[5];
	unsigned res[3];
};

struct ipu_ch_param {
	struct ipu_ch_param_word word[2];
};


#define CCGR5_CG5_OFFSET	10
#define CCDR_HSC_HS_MASK	(0x1 << 18)
#define CCDR_IPU_HS_MASK	(0x1 << 17)

#define CLPCR_BYPASS_IPU_LPM_HS	(0x1 << 18)

#define CBCMR_IPU_SHIFT 6
#define CBCMR_IPU_MASK	(3<<CBCMR_IPU_SHIFT)



static unsigned long const _720psettings[] = {
	IPU_DC_BASE + 0x064, 0x05030000,			/* IPU_DC_BASE */
	IPU_DC_BASE + 0x06c, 0x00000602,
	IPU_DC_BASE + 0x074, 0x00000701,
	IPU_DC_BASE + 0x064, 0x05030000,
	IPU_DC_BASE + 0x068, 0x00000000,
	IPU_DC_BASE + 0x068, 0x00000000,
	IPU_DC_BASE + 0x06c, 0x00000602,
	IPU_DC_BASE + 0x070, 0x00000000,
	IPU_DC_BASE + 0x070, 0x00000000,
	IPU_DC_BASE + 0x05c, 0x00000002,
	IPU_DC_BASE + 0x060, 0x00000000,
	IPU_DC_BASE + 0x0d4, 0x00000084,
	IPU_CM_REG_BASE + 0x000, 0x00000660,			/* IPU_CONF */
	IPU_DI0_BASE + 0x004, 0x00000018,			/* CLKGEN0 */
	IPU_DI0_BASE + 0x008, 0x00010000,			/* CLKGEN1 */
	IPU_DI0_BASE + 0x058, 0x00000000,			/* DI0_DW_GEN_0 */
	IPU_DI0_BASE + 0x058, 0x00000300,
	IPU_DI0_BASE + 0x118, 0x00020000,			/* DI0_DW_SET3_0 0x1E040118 */
	IPU_DI0_BASE + 0x00c, 0x33f90000,			/* DI0_SW_GEN0_1 0x1E04000C */
	IPU_DI0_BASE + 0x030, 0x10000000,
	IPU_DI0_BASE + 0x148, 0x00000000,
	IPU_DI0_BASE + 0x010, 0x33f90001,
	IPU_DI0_BASE + 0x034, 0x31001000,
	IPU_DI0_BASE + 0x148, 0x00000000,
	IPU_DI0_BASE + 0x014, 0x175a0000,
	IPU_DI0_BASE + 0x038, 0x300a2000,
	IPU_DI0_BASE + 0x14c, 0x00000000,
	IPU_DI0_BASE + 0x170, 0x000002eb,
	IPU_DI0_BASE + 0x018, 0x000300cb,
	IPU_DI0_BASE + 0x03c, 0x08000000,
	IPU_DI0_BASE + 0x14c, 0x02d00000,
	IPU_DI0_BASE + 0x01c, 0x00010a01,
	IPU_DI0_BASE + 0x040, 0x0a000000,
	IPU_DI0_BASE + 0x150, 0x00000500,
	IPU_DI0_BASE + 0x020, 0x00000000,
	IPU_DI0_BASE + 0x044, 0x00000000,
	IPU_DI0_BASE + 0x024, 0x00000000,
	IPU_DI0_BASE + 0x048, 0x00000000,
	IPU_DI0_BASE + 0x028, 0x00000000,
	IPU_DI0_BASE + 0x04c, 0x00000000,
	IPU_DI0_BASE + 0x02c, 0x00000000,
	IPU_DI0_BASE + 0x050, 0x00000000,
	IPU_DI0_BASE + 0x150, 0x00000500,
	IPU_DI0_BASE + 0x154, 0x00000000,
	IPU_DI0_BASE + 0x158, 0x00000000,
	IPU_DC_TEMPLATE_BASE + 0x028, 0x00008885,			/* DC_TEMPLATE */
	IPU_DC_TEMPLATE_BASE + 0x02c, 0x00000380,
	IPU_DC_TEMPLATE_BASE + 0x030, 0x00008845,
	IPU_DC_TEMPLATE_BASE + 0x034, 0x00000380,
	IPU_DC_TEMPLATE_BASE + 0x038, 0x00008805,
	IPU_DC_TEMPLATE_BASE + 0x03c, 0x00000380,
	IPU_DI0_BASE + 0x000, 0x00020004,			/* DI0_GENERAL */
	IPU_DI0_BASE + 0x054, 0x00004002,
	IPU_DI0_BASE + 0x164, 0x00000010,
	IPU_DC_BASE + 0x0e8, 0x00000500,
	IPU_CPMEM_REG_BASE + 0x5c0, 0x00000000,			/* CPMEM */
	IPU_CPMEM_REG_BASE + 0x5c4, 0x00000000,
	IPU_CPMEM_REG_BASE + 0x5c8, 0x00000000,
	IPU_CPMEM_REG_BASE + 0x5cc, 0xe0001800,
	IPU_CPMEM_REG_BASE + 0x5d0, 0x000b3c9f,
	IPU_CPMEM_REG_BASE + 0x5d4, 0x00000000,
	IPU_CPMEM_REG_BASE + 0x5d8, 0x00000000,
	IPU_CPMEM_REG_BASE + 0x5dc, 0x00000000,
	IPU_CPMEM_REG_BASE + 0x5e0, 0x12800000,
	IPU_CPMEM_REG_BASE + 0x5e4, 0x02508000,
	IPU_CPMEM_REG_BASE + 0x5e8, 0x00e3c000,
	IPU_CPMEM_REG_BASE + 0x5ec, 0xf2c27fc0,
	IPU_CPMEM_REG_BASE + 0x5f0, 0x00082ca0,
	IPU_CPMEM_REG_BASE + 0x5f4, 0x00000000,
	IPU_CPMEM_REG_BASE + 0x5f8, 0x00000000,
	IPU_CPMEM_REG_BASE + 0x5fc, 0x00000000,
	IPU_DMFC_BASE + 0x004, 0x00000088,			/* DMFC */
	IPU_DMFC_BASE + 0x008, 0x202020f6,
	IPU_DMFC_BASE + 0x00c, 0x00009694,
	IPU_DMFC_BASE + 0x010, 0x2020f6f6,
	IPU_DMFC_BASE + 0x014, 0x00000003,
	IPU_CPMEM_REG_BASE + 0x5e8, 0x20e3c000,			/* CPMEM */
	IPU_CM_REG_BASE + 0x150, 0x00800000,			/* IPU_CH_DB_MODE_SEL_0 */
	IPU_CM_REG_BASE + 0x23c, 0x00800000,			/* IPU_CUR_BUF_0 */
	IPU_IDMAC_BASE + 0x004, 0x00800000,			/* IDMAC_CONF */
	IPU_IDMAC_BASE + 0x01c, 0x00800000,
	IPU_DC_BASE + 0x01c, 0x00000004,			/* IPU_DC_BASE */
	IPU_DC_BASE + 0x05c, 0x00000082,
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
	IPU_DC_BASE + 0x108, 0x00000000,				/* MAP conf */
	IPU_DC_BASE + 0x144, 0x000007ff,
	IPU_DC_BASE + 0x108, 0x00000000,
	IPU_DC_BASE + 0x144, 0x0fff07ff,
	IPU_DC_BASE + 0x108, 0x00000020,
	IPU_DC_BASE + 0x148, 0x000017ff,
	IPU_DC_BASE + 0x108, 0x00000820,
	IPU_DC_BASE + 0x108, 0x00000820,
	IPU_DC_BASE + 0x148, 0x05fc17ff,
	IPU_DC_BASE + 0x108, 0x00030820,
	IPU_DC_BASE + 0x14c, 0x00000bfc,
	IPU_DC_BASE + 0x108, 0x00830820,
	IPU_DC_BASE + 0x14c, 0x11fc0bfc,
	IPU_DC_BASE + 0x108, 0x14830820,
	IPU_DC_BASE + 0x10c, 0x00000000,
	IPU_DC_BASE + 0x150, 0x00000fff,
	IPU_DC_BASE + 0x10c, 0x00000006,
	IPU_DC_BASE + 0x150, 0x17ff0fff,
	IPU_DC_BASE + 0x10c, 0x000000e6,
	IPU_DC_BASE + 0x154, 0x000007ff,
	IPU_DC_BASE + 0x10c, 0x000020e6,
	IPU_DC_BASE + 0x10c, 0x000020e6,
	IPU_DC_BASE + 0x154, 0x04f807ff,
	IPU_DC_BASE + 0x10c, 0x000920e6,
	IPU_DC_BASE + 0x158, 0x00000afc,
	IPU_DC_BASE + 0x10c, 0x014920e6,
	IPU_DC_BASE + 0x158, 0x0ff80afc,
	IPU_DC_BASE + 0x10c, 0x2d4920e6,
	IPU_DC_BASE + 0x110, 0x00000000,
	IPU_DC_BASE + 0x15c, 0x000005fc,
	IPU_DC_BASE + 0x110, 0x0000000c,
	IPU_DC_BASE + 0x15c, 0x0dfc05fc,
	IPU_DC_BASE + 0x110, 0x000001ac,
	IPU_DC_BASE + 0x160, 0x000015fc,
	IPU_DC_BASE + 0x110, 0x000039ac,
	// _ipu_dmfc_init(DMFC_NORMAL, 1);
	IPU_DMFC_BASE + 0x01c, 0x00000002,
	IPU_DMFC_BASE + 0x004, 0x00000090,
	IPU_DMFC_BASE + 0x008, 0x202020f6,
	IPU_DMFC_BASE + 0x00c, 0x00009694,
	IPU_DMFC_BASE + 0x010, 0x2020f6f6,
        /* IDMAC_CHA_PRI */
        IPU_IDMAC_BASE + 0x014, 0x18800001,
	/* Set MCU_T to divide MCU access window into 2 */
	IPU_CM_REG_BASE + 0x0c4, 0x00600000,
	IPU_SRM_REG_BASE + 0x004, 0x80000000,			/* DP_GRAPH_WIND_CTRL_SYNC: set alpha value */
	IPU_SRM_REG_BASE + 0x000, 0x00000004,			/* DP_COM_CONF_SYNC: set global alpha */
};

/* For the purposes of U-Boot, all of the uses of _ipu_dc_link_event are
 * effectively constant, although they're initialized separately for each
 * display port (DI0/DI1). The channels are essentially fixed at MEM_BG_SYNC
 * so we're just making them constant from the get-go.
 */
static unsigned long const late_dc_mappings[] = {
	IPU_DC_BASE + 0x000, 0xffff0000,
	IPU_DC_BASE + 0x004, 0x00000000,
	IPU_DC_BASE + 0x008, 0x00000000,
	IPU_DC_BASE + 0x00c, 0x00000000,
	IPU_DC_BASE + 0x010, 0x00000000,
	IPU_DC_BASE + 0x014, 0x00000000,
	IPU_DC_BASE + 0x018, 0x00000000,
	IPU_DC_BASE + 0x01c, 0x0000008e,
	IPU_DC_BASE + 0x020, 0x00000000,
	IPU_DC_BASE + 0x024, 0x02030000,
	IPU_DC_BASE + 0x028, 0x00000000,
	IPU_DC_BASE + 0x02c, 0x00000302,
	IPU_DC_BASE + 0x030, 0x00000000,
	IPU_DC_BASE + 0x034, 0x00000401,
	IPU_DC_BASE + 0x038, 0x00000000,
	IPU_DC_BASE + 0x03c, 0x00000000,
	IPU_DC_BASE + 0x040, 0x00000000,
	IPU_DC_BASE + 0x044, 0x00000000,
	IPU_DC_BASE + 0x048, 0x00000000,
	IPU_DC_BASE + 0x04c, 0x00000000,
	IPU_DC_BASE + 0x050, 0x00000000,
	IPU_DC_BASE + 0x054, 0x00000000,
	IPU_DC_BASE + 0x058, 0x00000000,
	IPU_DC_BASE + 0x05c, 0x00000082,
	IPU_DC_BASE + 0x060, 0x00000000,
	IPU_DC_BASE + 0x064, 0x05030000,
	IPU_DC_BASE + 0x068, 0x00000000,
	IPU_DC_BASE + 0x06c, 0x00000602,
	IPU_DC_BASE + 0x070, 0x00000000,
	IPU_DC_BASE + 0x074, 0x00000701,
	IPU_DC_BASE + 0x078, 0x00000000,
	IPU_DC_BASE + 0x07c, 0x00000000,
	IPU_DC_BASE + 0x080, 0x00000000,
	IPU_DC_BASE + 0x084, 0x00000000,
	IPU_DC_BASE + 0x088, 0x00000000,
	IPU_DC_BASE + 0x08c, 0x00000000,
	IPU_DC_BASE + 0x090, 0x00000000,
	IPU_DC_BASE + 0x094, 0x00000000,
	IPU_DC_BASE + 0x098, 0x00000000,
	IPU_DC_BASE + 0x09c, 0x00000000,
	IPU_DC_BASE + 0x0a0, 0x00000000,
	IPU_DC_BASE + 0x0a4, 0x00000000,
	IPU_DC_BASE + 0x0a8, 0x00000000,
	IPU_DC_BASE + 0x0ac, 0x00000000,
	IPU_DC_BASE + 0x0b0, 0x00000000,
	IPU_DC_BASE + 0x0b4, 0x00000000,
	IPU_DC_BASE + 0x0b8, 0x00000000,
	IPU_DC_BASE + 0x0bc, 0x00000000,
	IPU_DC_BASE + 0x0c0, 0x00000000,
	IPU_DC_BASE + 0x0c4, 0x00000000,
	IPU_DC_BASE + 0x0c8, 0x00000000,
	IPU_DC_BASE + 0x0cc, 0x00000000,
	IPU_DC_BASE + 0x0d0, 0x00000000,
	IPU_DC_BASE + 0x0d4, 0x00000084,
	IPU_DC_BASE + 0x0d8, 0x00000042,
	IPU_DC_BASE + 0x0dc, 0x00000042,
	IPU_DC_BASE + 0x0e0, 0x00000042,
	IPU_DC_BASE + 0x0e4, 0x00000042,
	IPU_DC_BASE + 0x0e8, 0x00000500,
	IPU_DC_BASE + 0x0ec, 0x00000320,
	IPU_DC_BASE + 0x0f0, 0x00000000,
	IPU_DC_BASE + 0x0f4, 0x00000000,
	IPU_DC_BASE + 0x0f8, 0x00000000,
	IPU_DC_BASE + 0x0fc, 0x00000000,
	IPU_DC_BASE + 0x100, 0x00000000,
	IPU_DC_BASE + 0x104, 0x00000000,
	IPU_DC_BASE + 0x108, 0x14830820,
	IPU_DC_BASE + 0x10c, 0x2d4920e6,
	IPU_DC_BASE + 0x110, 0x000039ac,
	IPU_DC_BASE + 0x114, 0x00000000,
	IPU_DC_BASE + 0x118, 0x00000000,
	IPU_DC_BASE + 0x11c, 0x00000000,
	IPU_DC_BASE + 0x120, 0x00000000,
	IPU_DC_BASE + 0x124, 0x00000000,
	IPU_DC_BASE + 0x128, 0x00000000,
	IPU_DC_BASE + 0x12c, 0x00000000,
	IPU_DC_BASE + 0x130, 0x00000000,
	IPU_DC_BASE + 0x134, 0x00000000,
	IPU_DC_BASE + 0x138, 0x00000000,
	IPU_DC_BASE + 0x13c, 0x00000000,
	IPU_DC_BASE + 0x140, 0x00000000,
	IPU_DC_BASE + 0x144, 0x0fff07ff,
	IPU_DC_BASE + 0x148, 0x05fc17ff,
	IPU_DC_BASE + 0x14c, 0x11fc0bfc,
	IPU_DC_BASE + 0x150, 0x17ff0fff,
	IPU_DC_BASE + 0x154, 0x04f807ff,
	IPU_DC_BASE + 0x158, 0x0ff80afc,
	IPU_DC_BASE + 0x15c, 0x0dfc05fc,
	IPU_DC_BASE + 0x160, 0x000015fc,
	IPU_DC_BASE + 0x164, 0x00000000,
	IPU_DC_BASE + 0x168, 0x00000000,
	IPU_DC_BASE + 0x16c, 0x00000000,
	IPU_DC_BASE + 0x170, 0x00000000,
	IPU_DC_BASE + 0x174, 0x00000000,
	IPU_DC_BASE + 0x178, 0x00000000,
	IPU_DC_BASE + 0x17c, 0x00000000,
	IPU_DC_BASE + 0x180, 0x00000000,
	IPU_DC_BASE + 0x184, 0x00000000,
	IPU_DC_BASE + 0x188, 0x00000000,
	IPU_DC_BASE + 0x18c, 0x00000000,
	IPU_DC_BASE + 0x190, 0x00000000,
	IPU_DC_BASE + 0x194, 0x00000000,
	IPU_DC_BASE + 0x198, 0x00000000,
	IPU_DC_BASE + 0x19c, 0x00000000,
	IPU_DC_BASE + 0x1a0, 0x00000000,
	IPU_DC_BASE + 0x1a4, 0x00000000,
	IPU_DC_BASE + 0x1a8, 0x00000000,
	IPU_DC_BASE + 0x1ac, 0x00000000,
	IPU_DC_BASE + 0x1b0, 0x00000000,
	IPU_DC_BASE + 0x1b4, 0x00000000,
	IPU_DC_BASE + 0x1b8, 0x00000000,
	IPU_DC_BASE + 0x1bc, 0x00000000,
	IPU_DC_BASE + 0x1c0, 0x00000000,
	IPU_DC_BASE + 0x1c4, 0x00000000,
	IPU_DC_BASE + 0x1c8, 0x00000050,

	/* DC template registers for both displays */
	IPU_DC_TEMPLATE_BASE + 0x028, 0x00008885,
	IPU_DC_TEMPLATE_BASE + 0x02c, 0x00000380,
	IPU_DC_TEMPLATE_BASE + 0x030, 0x00008845,
	IPU_DC_TEMPLATE_BASE + 0x034, 0x00000380,
	IPU_DC_TEMPLATE_BASE + 0x038, 0x00008805,
	IPU_DC_TEMPLATE_BASE + 0x03c, 0x00000380,
	IPU_DC_TEMPLATE_BASE + 0x010, 0x00020885,
	IPU_DC_TEMPLATE_BASE + 0x014, 0x00000380,
	IPU_DC_TEMPLATE_BASE + 0x018, 0x00020845,
	IPU_DC_TEMPLATE_BASE + 0x01c, 0x00000380,
#ifdef CONFIG_MX53
        IPU_DC_TEMPLATE_BASE + 0x020, 0x00010805,
#else
        IPU_DC_TEMPLATE_BASE + 0x020, 0x00020805,
#endif
	IPU_DC_TEMPLATE_BASE + 0x024, 0x00000380,
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
	__REG(CCGR5) |= 3 << CCGR5_CG5_OFFSET ;	/* ipu clocks */

	/* from _clk_ipu_enable: Handshake with IPU... */
	__REG(CCDR) &= ~CCDR_IPU_HS_MASK ;
	__REG(CLPCR) &= ~CLPCR_BYPASS_IPU_LPM_HS ;
}

static void disable_ipu_clock(void)
{
        /* from .enable_reg and .enable_shift */
	__REG(CCGR5) = (__REG(CCGR5) & ~(3 << CCGR5_CG5_OFFSET));
	__REG(CCGR6) &= ~((0x0f << (5*2)) | (0x0f << (14*2)));		/* ipu DI0/1, ldb DI0/1 */

	/* No handshake with IPU whe dividers are changed
	 * as its not enabled. 
	 */
	__REG(CCDR) |= CCDR_IPU_HS_MASK ;
	__REG(CLPCR) |= CLPCR_BYPASS_IPU_LPM_HS ;
}

void backlight_state(int enable);
void init_display_pins(void);
void init_lvds_pins(int ch,int lvds);
void disable_lcd_panel(void);

void setup_display(void)
{
	disable_lcd_panel();	//needed so that go nnn(u-boot address, works with testing)
#ifdef MIPI_HSC_BASE_ADDR
        __REG(MIPI_HSC_BASE_ADDR) = 0xF00 ; // indeed!
#endif

DEBUG( "%s\n", __func__ );
	init_display_pins();
	writel(0,DI0_GENERAL);
	writel(0,DI1_GENERAL);

	enable_ipu_clock();
	writel(0x807FFFFF, IPU_MEM_RST);
	while (readl(IPU_MEM_RST) & 0x80000000) ;

#ifdef CONFIG_MX53
	__REG(IOMUXC_GPR2) = __REG(IOMUXC_GPR2) & ~0x1ff | GPR2_BASE_VALUE;
#endif
	write_regs(init_dc_mappings,ARRAY_SIZE(init_dc_mappings));
	check_regs("dcmap",init_dc_mappings,ARRAY_SIZE(init_dc_mappings));

	write_regs(late_dc_mappings,ARRAY_SIZE(late_dc_mappings));
	check_regs("late_dc",late_dc_mappings,ARRAY_SIZE(late_dc_mappings));

#ifdef CONFIG_MX53
	set_pixel_clock(0,0,0);
	set_pixel_clock(1,0,0);
#else
	set_pixel_clock(0,0);
	set_pixel_clock(1,0);
#endif	
	disable_ipu_clock();

	end_of_ram = gd->bd->bi_dram[0].start+gd->bd->bi_dram[0].size;
	printf("%s: end-of-ram == 0x%08x\n", __func__, end_of_ram );
}


void disable_lcd_panel(void)
{
	DEBUG ("%s: disable both displays here\n", __func__ );
#ifdef CONFIG_MX53
	__REG(IOMUXC_GPR2) &= ~0x0f;
#endif
	__REG(IPU_DISP_GEN) &= ~(3 << 24);	// stop DI0/DI1
	__REG(IDMAC_WM_EN_1)  &= ~(3<<23);	// No watermarking
	__REG(IDMAC_CH_EN_1)  &= ~(3<<23); 	// Disable DMA channel
	__REG(IPU_CONF+0x150) &= ~(3<<23);	// DB_MODE_SEL
	__REG(IPU_CONF+0x23C) &= ~(3<<23);	// CUR_BUF

	disable_ipu_clock();
#ifdef CONFIG_MX53
	backlight_state(0);
#endif
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
#define DI_GEN_DI_CLK_EXT	(1 << 20)

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

static void ipu_read_sync_config
	( unsigned base_addr,
	  int wave_gen,
	  int *run_count, 
	  int *offset_count, 
	  int *repeat_count,
	  int *cnt_down)
{
	u32 reg;

	reg = __REG(base_addr+0x000C+(wave_gen-1)*4); // DI_SW_GEN0
	*run_count = reg >> 19 ;
	*offset_count = (reg & 0xfff8)>>3 ;
	
	reg = __REG(base_addr+0x0030+(wave_gen-1)*4);	// DI_SW_GEN1
        *cnt_down = (reg&0xfff0000)>>16 ;
	
	reg = __REG(base_addr+0x0148+((wave_gen-1)/2)*4);	// DI_STP_REP
	wave_gen = (wave_gen&1)^1 ;
	*repeat_count = (reg>>(16*wave_gen)) & 0xffff ;
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
                        bmp_color_table_entry_t *colors = (bmp_color_table_entry_t *)
							  ((unsigned)bmp + bmp->header.size + 14);
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

#ifdef CONFIG_MX53
static void init_lvds(void)
{
	char *lvds_str ;

	lvds[0] = lvds[1] = 0 ;

        lvds_str=getenv("lvds");
	if (0 != lvds_str) {
                char *flag ;
		lvds_str = strdup(lvds_str);
		flag = strtok(lvds_str,",");
		if (flag) {
			lvds[0] = simple_strtoul(flag,0,0);

			flag = strtok(0,",");
			if (flag)
                                lvds[1] = simple_strtoul(flag,0,0);
			else
				lvds[1] = 0 ;
		}
		free(lvds_str);
	}
	printf ("%s: lvds=%d,%d\n",__func__,lvds[0],lvds[1]);
	init_lvds_pins(0,lvds[0]);
	init_lvds_pins(1,lvds[1]);
}
#endif

static void init_display
	( struct lcd_t *lcd,
          struct display_channel_t const *channel )
{
	u32 div ;
	u16 mask ;
	u32 wgen ;
	unsigned disp = (IPU_DI0_BASE != (channel->di_base_addr));
	u32 h_total, v_total ;
	void *params ;
	u32 screen_size = lcd->info.xres*lcd->info.yres*2 ;
	u32 address ;
	unsigned ext_clk = 0;

#ifdef CONFIG_MX53
	init_lvds();
	backlight_state(1);

	if (lvds[disp]) {
		clk_config(CONFIG_MX53_HCLK_FREQ, lcd->info.pixclock, LDB0_CLK+disp);
		div = 1 ;
                printf ("%s: lvds clock %u, div %u\n", __func__, clk_info(LDB0_CLK+disp), div );
		ext_clk = DI_GEN_DI_CLK_EXT;
	} else 
#endif	
	{
                div = mxc_get_clock(MXC_IPU_CLK) / lcd->info.pixclock ;
                printf ("%s: !lvds clock %u, div %u\n", __func__, mxc_get_clock(MXC_IPU_CLK), div );
	}
	mask = div-1 ;
	wgen = (mask << DI_DW_GEN_ACCESS_SIZE_OFFSET) 
		 | (mask << DI_DW_GEN_COMPONENT_SIZE_OFFSET)
	         | 3 << 8 ;
printf("mask == %x, wgen == %x\n", mask, wgen);
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
#ifdef CONFIG_MX53
	set_pixel_clock(disp,lcd->info.pixclock,lvds[disp]);
#else
	set_pixel_clock(disp,lcd->info.pixclock);
#endif
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
					| DI_GEN_POLARITY_3*(0 != lcd->info.vsyn_acth)
					| ext_clk;

	__REG(channel->di_base_addr+0x0054) = ((DI_SYNC_VSYNC-1) << DI_VSYNC_SEL_OFFSET) | 0x00000002 ; // DI_SYNC_AS_GEN
	__REG(channel->di_base_addr+0x0164) &= ~(DI_POL_DRDY_DATA_POLARITY );	// DI_POL
	/*
	 * Backwards to match our panel definitions
	 */
	if (0 == lcd->info.oepol_actl)
		__REG(channel->di_base_addr+0x0164) |= DI_POL_DRDY_POLARITY_15 ;
	else
		__REG(channel->di_base_addr+0x0164) &= ~DI_POL_DRDY_POLARITY_15 ;

	__REG(IPU_DC_BASE+0x00E8 + (disp*4)) = lcd->info.xres ;

        params = (void *)(IPU_CPMEM_REG_BASE+(0x40*channel->dma_chan));

	ipu_ch_param_set_field(params, 0, 125, 13, lcd->info.xres - 1);
	ipu_ch_param_set_field(params, 0, 138, 12, lcd->info.yres - 1);
	ipu_ch_param_set_field(params, 1, 102, 14, lcd->stride - 1);

	/* EBA is 8-byte aligned */
	address = end_of_ram - screen_size ;
        lcd->fbAddr = ioremap_nocache(address,screen_size);
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

#ifdef CONFIG_MX53
	__REG(CCGR6) |= (0x0f << (5*2)) | (0x0f << (14*2));		/* ipu DI0/1, ldb DI0/1 */

	mask = 3<<(2*disp); // Route LVDS 1:1
	printf("%s: mask %x/%x\n", __func__, mask, ((lvds[disp] ? (disp ? 3 : 1) : 0)<<(2*disp)));
	__REG(IOMUXC_GPR2) = (__REG(IOMUXC_GPR2) & ~mask) | ((lvds[disp] ? (disp ? 3 : 1) : 0)<<(2*disp));
	printf("%s: GPR2(0x%08x) == %x\n", __func__, IOMUXC_GPR2, __REG(IOMUXC_GPR2));
#endif
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
	if (1 < argc) {
		unsigned long addr = simple_strtoul (argv[1],0,0);
		if (addr && (0 == (addr&3))) {
			unsigned long *longs = (unsigned long *)addr ;
			while (*longs) {
				unsigned long count ;
				addr = *longs++ ;
				count = *longs++ ;
				printf( "check %lu longs at 0x%08lx\n", count, addr );
				while (0 < count--) {
					unsigned expected = *longs++ ;
					unsigned val = __REG(addr); 
					if (val != expected) {
						printf( "%08lx: %08x not expected %08x\n", addr, val, expected );
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

static struct display_channel_t const *channels[] = {
	&crt_channel
,	&lcd_channel
};

int
do_panel_regs (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i ;
	for (i = 0 ; i < ARRAY_SIZE(channels) ; i++) {
                struct display_channel_t const *ch = channels[i];
		unsigned enabled = __REG(IPU_DISP_GEN) & (1<<(i+24));
		printf ("%s: %sabled\n", ch->name, enabled? "en" : "dis");
		if (enabled) {
			struct lcd_panel_info_t panel ;
			unsigned reg ;
			int run_count, offset_count, repeat_count, cnt_down ;
			unsigned htotal, vtotal ;
			memset(&panel,0,sizeof(panel));
			panel.name = ch->name ;
			panel.pixclock = get_pixel_clock(i);
			reg  =__REG(ch->di_base_addr);
			panel.pclk_redg = (0 != (DI_GEN_DI_PIXCLK_ACTH&reg));
			panel.hsyn_acth = (0 != (DI_GEN_POLARITY_2&reg));
			panel.vsyn_acth = (0 != (DI_GEN_POLARITY_3&reg));
			panel.oepol_actl = 0 ;

			ipu_read_sync_config(ch->di_base_addr,DI_SYNC_HSYNC,&run_count,&offset_count,&repeat_count,&cnt_down);
			panel.hsync_len = cnt_down/2 ;
                        htotal = run_count+1 ;

			ipu_read_sync_config(ch->di_base_addr,DI_SYNC_DE,&run_count,&offset_count,&repeat_count,&cnt_down);
			panel.xres = repeat_count ;

			panel.left_margin = offset_count - panel.hsync_len ;
			panel.right_margin = htotal - panel.xres - panel.left_margin - panel.hsync_len ;
			
			ipu_read_sync_config(ch->di_base_addr,DI_SYNC_VSYNC,&run_count,&offset_count,&repeat_count,&cnt_down);
			vtotal = run_count+1 ;
			panel.vsync_len = cnt_down/2 ;
			
			ipu_read_sync_config(ch->di_base_addr,DI_SYNC_OE,&run_count,&offset_count,&repeat_count,&cnt_down);

			panel.yres = repeat_count ;
			panel.upper_margin = offset_count-panel.vsync_len ;
			panel.lower_margin = vtotal-panel.vsync_len-panel.upper_margin-panel.yres ;
			panel.active = 1 ;
			panel.crt = (0 == i);          // 1 == CRT, not LCD
			panel.rotation = 0 ;
                        print_panel_info(&panel);
		}
	}
	return 0 ;
}

U_BOOT_CMD(
	panelregs, 1, 0,	do_panel_regs,
	"Read and display panel information from registers\n",
	NULL
);

char const *fixupPanelBootArg(char const *cmdline)
{
	unsigned i ;
	char tempbuf[512];
	char *modestr=getenv("panelmodes");
	unsigned panelmodes[2] = {0,0};
	char *nextOut ;
	int di1_primary = 0 ;

	if (0 != strstr(cmdline,"video=")) {
		printf( "Don't override specified bootarg in %s\n", cmdline );
		return cmdline ;
	}

	if (0 != modestr) {
		char *m = strtok(modestr,",");
		if (m) {
			panelmodes[0] = simple_strtoul(m,0,0);
			m = strtok(0,",");
			if (m)
                                panelmodes[1] = simple_strtoul(m,0,0);
		}
	}

	if (0 == getPanelCount())
		return cmdline ;

	nextOut = tempbuf ;
	*nextOut = '\0' ;

	for (i = 0 ; i < getPanelCount(); i++) {
		struct lcd_t *lcd = getPanel(i);
		unsigned di = (lcd->info.crt&1)^1 ;
		if ((1 == di) && (0 == i)) {
			di1_primary = 1 ;
		}

		nextOut += sprintf (nextOut, "video=mxcdi%ufb:", di );
		lcd->info.name = "raw" ;
		build_panel_name(nextOut,&lcd->info);
		nextOut += strlen(nextOut);
		*nextOut++ = ',' ;
		if (panelmodes[di]) {
			sprintf(nextOut,"%d",panelmodes[di]);
		} else
                        strcpy(nextOut,channels[di]->kernel_suffix);
		nextOut += strlen(nextOut);
		*nextOut++ = ' ' ;
		*nextOut = '\0' ;
	}

	if (di1_primary) {
		if (0 == strstr(cmdline,"di1_primary")) {
			strcpy(nextOut,"di1_primary ");
			nextOut += strlen(nextOut);
		}
	}
	nextOut-- ;
	*nextOut = '\0' ;

	nextOut = (char *)malloc(strlen(cmdline)+1+(nextOut-tempbuf)+2);
	if (nextOut) {
		strcpy(nextOut,cmdline);
		strcat(nextOut, " " );
		strcat(nextOut,tempbuf);
		cmdline = nextOut ;
	}

	return cmdline ;
}

int
do_timevsync(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
 {
	unsigned long long start ;
	unsigned long elapsed ;
	__REG(IPU_INT_STAT3) = IPU_INT_STAT3_MASK ;
	
	/* wait for one sync first */
	do {
	} while ( 0 == (__REG(IPU_INT_STAT3)&IPU_INT_STAT3_MASK) );

	/* once more to measure things */
        reset_timer();
        start = get_timer(0);
	__REG(IPU_INT_STAT3) = IPU_INT_STAT3_MASK ;
	do {
	} while ( 0 == (__REG(IPU_INT_STAT3)&IPU_INT_STAT3_MASK) );

	elapsed = (unsigned long)(get_timer(0)-start);
	printf( "%lu ticks: %lu Hz\n", elapsed, CONFIG_SYS_HZ/(elapsed?elapsed:1));
	return 0 ;
}

U_BOOT_CMD(
	timevsync, 1, 0,	do_timevsync,
	"timevsync - time vsync\n",
	""
);

