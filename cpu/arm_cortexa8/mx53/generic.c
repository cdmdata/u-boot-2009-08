/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx53.h>
#include <asm/errno.h>
#include <asm/io.h>
#ifdef CONFIG_ARCH_CPU_INIT
#include <asm/cache-cp15.h>
#endif
#ifdef CONFIG_CMD_CLOCK
#include <asm/clock.h>
#endif
#include <div64.h>
#include "crm_regs.h"
#include <asm/cache.h>
#define __HCLK_FREQ CONFIG_MX53_HCLK_FREQ

enum pll_clocks {
	PLL1_CLK = MXC_DPLL1_BASE,
	PLL2_CLK = MXC_DPLL2_BASE,
	PLL3_CLK = MXC_DPLL3_BASE,
	PLL4_CLK = MXC_DPLL4_BASE,
};

enum pll_sw_clocks {
	PLL1_SW_CLK,
	PLL2_SW_CLK,
	PLL3_SW_CLK,
	PLL4_SW_CLK,
};

#define AHB_CLK_ROOT 133333333
#define IPG_CLK_ROOT 66666666
#define IPG_PER_CLK_ROOT 40000000

#ifdef CONFIG_CMD_CLOCK
#define SZ_DEC_1M       1000000
#define PLL_PD_MAX      16      /* Actual pd+1 */
#define PLL_MFI_MAX     15
#define PLL_MFI_MIN     5
#define ARM_DIV_MAX     8
#define IPG_DIV_MAX     4
#define AHB_DIV_MAX     8
#define EMI_DIV_MAX     8
#define NFC_DIV_MAX     8

struct fixed_pll_mfd {
    u32 ref_clk_hz;
    u32 mfd;
};

const struct fixed_pll_mfd fixed_mfd[4] = {
    {0,                   0},      /* reserved */
    {0,                   0},      /* reserved */
    {__HCLK_FREQ, 24 * 16},    /* 384 */
    {0,                   0},      /* reserved */
};

struct pll_param {
    u32 pd;
    u32 mfi;
    u32 mfn;
    u32 mfd;
};

#define PLL_FREQ_MAX(_ref_clk_) \
		(4 * _ref_clk_ * PLL_MFI_MAX)
#define PLL_FREQ_MIN(_ref_clk_) \
		((2 * _ref_clk_ * (PLL_MFI_MIN - 1)) / PLL_PD_MAX)
#define MAX_DDR_CLK     420000000
#define AHB_CLK_MAX     133333333
#define IPG_CLK_MAX     (AHB_CLK_MAX / 2)
#define NFC_CLK_MAX     34000000
#define HSP_CLK_MAX     133333333
#endif

static u32 __decode_pll(enum pll_clocks pll, u32 infreq)
{
	u32 mfi, mfd, pd, ctl;
	s32 mfn;
	u64 rate;

	mfn = __REG(pll + MXC_PLL_DP_MFN);
	/* Sign extend to 32-bits */
	mfn <<= 5;
	mfn >>= 5;
	mfd = __REG(pll + MXC_PLL_DP_MFD) + 1;	//high 5 bits are 0
	mfi = __REG(pll + MXC_PLL_DP_OP);
	ctl = __REG(pll + MXC_PLL_DP_CTL);
	pd = (mfi  & 0xF) + 1;
	mfi = (mfi >> 4) & 0xF;
	mfi = (mfi >= 5) ? mfi : 5;
	rate = (u32)(mfi * mfd + mfn);
	rate *= 2 * infreq;
	do_div(rate, (mfd * pd));
	if (ctl & MXC_PLL_DP_CTL_DPDCK0_2_EN)
		rate <<= 1;
	return rate;
}

static u32 __get_mcu_main_clk(void)
{
	u32 reg, freq;
	reg = (__REG(MXC_CCM_CACRR) & MXC_CCM_CACRR_ARM_PODF_MASK) >>
	    MXC_CCM_CACRR_ARM_PODF_OFFSET;
	freq = __decode_pll(PLL1_CLK, __HCLK_FREQ);
	return freq / (reg + 1);
}

static u32 __get_periph_clk(void)
{
	u32 reg;
	reg = __REG(MXC_CCM_CBCDR);
	if (reg & MXC_CCM_CBCDR_PERIPH_CLK_SEL) {
		reg = __REG(MXC_CCM_CBCMR);
		switch ((reg & MXC_CCM_CBCMR_PERIPH_CLK_SEL_MASK) >>
			MXC_CCM_CBCMR_PERIPH_CLK_SEL_OFFSET) {
		case 0:
			return __decode_pll(PLL1_CLK, __HCLK_FREQ);
		case 1:
			return __decode_pll(PLL3_CLK, __HCLK_FREQ);
		default:
			return 0;
		}
	}
	return __decode_pll(PLL2_CLK, __HCLK_FREQ);
}

static u32 __get_axi_a_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_AXI_A_PODF_MASK) \
			>> MXC_CCM_CBCDR_AXI_A_PODF_OFFSET;

	return  __get_periph_clk() / (pdf + 1);
}

static u32 __get_axi_b_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_AXI_B_PODF_MASK) \
			>> MXC_CCM_CBCDR_AXI_B_PODF_OFFSET;

	return  __get_periph_clk() / (pdf + 1);
}

static u32 __get_ahb_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_AHB_PODF_MASK) \
			>> MXC_CCM_CBCDR_AHB_PODF_OFFSET;

	return  __get_periph_clk() / (pdf + 1);
}

static u32 __get_emi_slow_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 emi_clk_sel = cbcdr & MXC_CCM_CBCDR_EMI_CLK_SEL;
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_EMI_PODF_MASK) \
			>> MXC_CCM_CBCDR_EMI_PODF_OFFSET;

	if (emi_clk_sel)
		return  __get_ahb_clk() / (pdf + 1);

	return  __get_periph_clk() / (pdf + 1);
}

static u32 __get_ipg_clk(void)
{
	u32 ahb_podf, ipg_podf;

	ahb_podf = __REG(MXC_CCM_CBCDR);
	ipg_podf = (ahb_podf & MXC_CCM_CBCDR_IPG_PODF_MASK) >>
			MXC_CCM_CBCDR_IPG_PODF_OFFSET;
	ahb_podf = (ahb_podf & MXC_CCM_CBCDR_AHB_PODF_MASK) >>
			MXC_CCM_CBCDR_AHB_PODF_OFFSET;
	return __get_periph_clk() / ((ahb_podf + 1) * (ipg_podf + 1));
}

static u32 __get_ldb_clk(int which){
	u32 reg = __REG(MXC_CCM_CSCMR2);
	u32 div = 0 != (reg & (MXC_CCM_CSCMR2_LBD_DI0_IPU_DIV<<which));
	u32 sel = 0 != (reg & (MXC_CCM_CSCMR2_LBD_DI0_CLK_SEL<<which));
        u32 freq ;
	freq = __decode_pll(sel ? PLL4_CLK : PLL3_CLK, __HCLK_FREQ);
	freq = (((div^1)+1) * freq) / 7 ;
	return freq ;
}

static u32 __get_ipu_di_clk(int which)
{
	u32 reg = __REG(MXC_CCM_CSCMR2);
	u32 sel = (reg >> MXC_CCM_CSCMR2_DI_CLK_SEL_OFFSET(which))&7;
	u32 parent_freq, div ;
	switch (sel) {
		case 0b000: //  derive clock from divided pll3. (Default)
			parent_freq = __decode_pll(PLL3_CLK, __HCLK_FREQ);
			reg = __REG(MXC_CCM_CDCDR) & MXC_CCM_CDCDR_DI1_CLK_PRED_MASK;
			div = (reg >> MXC_CCM_CDCDR_DI1_CLK_PRED_OFFSET) + 1;
			return parent_freq / div;
		case 0b001: //  derive clock from oscillator
			return __HCLK_FREQ;
		case 0b101: // derive clock from ldb_di1_clk
			return __get_ldb_clk(which);
		case 0b010: //  derive clock from ckih camp1 clock.
		case 0b011: //  derive clock from tve_di_clock - in this case tve will supply the di clock.
		case 0b100: //  derive clock from ipp_di1_clk - clock source will be generated from the IOMUX.
		default:
			printf("%s: Unsupported clock selector %x\n", __func__, sel );
			return 0 ;
	}
}

static u32 __get_ipg_per_clk(void)
{
	u32 pred1, pred2, podf;
	if (__REG(MXC_CCM_CBCMR) & MXC_CCM_CBCMR_PERCLK_IPG_CLK_SEL)
		return __get_ipg_clk();
	/* Fixme: not handle what about lpm */
	podf = __REG(MXC_CCM_CBCDR);
	pred1 = (podf & MXC_CCM_CBCDR_PERCLK_PRED1_MASK) >>
		MXC_CCM_CBCDR_PERCLK_PRED1_OFFSET;
	pred2 = (podf & MXC_CCM_CBCDR_PERCLK_PRED2_MASK) >>
		MXC_CCM_CBCDR_PERCLK_PRED2_OFFSET;
	podf = (podf & MXC_CCM_CBCDR_PERCLK_PODF_MASK) >>
		MXC_CCM_CBCDR_PERCLK_PODF_OFFSET;

	return __get_periph_clk() / ((pred1 + 1) * (pred2 + 1) * (podf + 1));
}

static u32 __get_cdcmr_axi_to_ahb_clk(int bit_offset)
{
	u32 ret_val = 0;
	u32 sel = (__REG(MXC_CCM_CBCMR) >> bit_offset) & 3;
	switch (sel) {
	case 0:
		ret_val =  __get_axi_a_clk();
		break;
	case 1:
		ret_val =  __get_axi_b_clk();
		break;
	case 2:
		ret_val =  __get_emi_slow_clk();
		break;
	case 3:
		ret_val =  __get_ahb_clk();
		break;
	default:
		break;
	}
	return ret_val;
}

static u32 __get_ddr_clk(void)
{
	unsigned cbcdr = __REG(MXC_CCM_CBCDR);
	if (cbcdr & MXC_CCM_CBCDR_DDR_HF_SEL) {
		unsigned div = ((cbcdr >> MXC_CCM_CBCDR_DDR_PODF_OFFSET) & 7) + 1;
		return __decode_pll(PLL1_CLK, __HCLK_FREQ) / div;
	}
	return __get_cdcmr_axi_to_ahb_clk(MXC_CCM_CBCMR_DDR_CLK_SEL_OFFSET);
}

/* Documentation differs from mach-mx5/clock.c on what values
 * of 2 and 3 mean in field ipu_hsp_clk_sel of register CBCMR
 */

static u32 __get_ipu_clk(void)
{
	return __get_cdcmr_axi_to_ahb_clk(MXC_CCM_CBCMR_IPU_HSP_CLK_SEL_OFFSET);
}

#define DI0_BS_CLKGEN0            (IPU_DI0_BASE + 0x004)
#define DI1_BS_CLKGEN0            (IPU_DI1_BASE + 0x004)

unsigned get_pixel_clock(unsigned which) {
	unsigned external  = (__REG(which ? IPU_DI1_BASE : IPU_DI0_BASE) >> 20)&1 ;
	unsigned divisorReg = __REG(which ? DI1_BS_CLKGEN0 : DI0_BS_CLKGEN0);
	unsigned ipu_clock = external 
				? __get_ipu_di_clk(which)
				: __get_ipu_clk();
	unsigned divisor = divisorReg&0xffff ;
printf("%s: external(%d), ipu_clk %d, divisor %d\n", __func__, external, ipu_clock, divisor);
	if (0 == divisor) {
		printf ("%s: pixel clock divisor %u not set\n", __func__, which );
		divisor = 0x10 ;
	}

	return (ipu_clock*16)/divisor ;
}

void set_pixel_clock(int which, unsigned hz, int lvds)
{
	unsigned ipu_clock = __get_ipu_clk();
	unsigned divisorReg = (0 != which) 
				? DI1_BS_CLKGEN0
				: DI0_BS_CLKGEN0 ;
	unsigned divisor = lvds ? 0x10 : (hz ? (ipu_clock*16)/hz : 0);
	if (0 == divisor) {
		printf ("%s: pixel clock divisor %u not set\n", __func__, which );
		divisor = 0x10 ;
	}
	__REG(divisorReg) = divisor ;
	divisor >>= 4 ;
	divisor <<= 16 ;
	__REG(divisorReg+4) = divisor ;
        udelay(5000);
}

/*
 * This function returns the low power audio clock.
 */
static u32 __get_lp_apm(void)
{
	u32 ret_val = 0;
	u32 ccsr = __REG(MXC_CCM_CCSR);

	if (((ccsr >> MXC_CCM_CCSR_LP_APM_SEL_OFFSET) & 1) == 0)
		ret_val = __HCLK_FREQ;
	else
		ret_val = ((32768 * 1024));

	return ret_val;
}

/*
static u32 __get_perclk_lp_apm(void)
{
	u32 ret_val = 0;
	u32 cbcmr = __REG(MXC_CCM_CBCMR);
	u32 clk_sel = (cbcmr & MXC_CCM_CBCMR_PERCLK_LP_APM_CLK_SEL) \
			>> MXC_CCM_CBCMR_PERCLK_LP_APM_CLK_SEL_OFFSET;

	switch (clk_sel) {
	case 0:
		ret_val =  __get_periph_clk();
		break;
	case 1:
		ret_val =  __get_lp_apm();
		break;
	default:
		break;
	}

	return ret_val;
}
*/

static u32 __get_pll_from_choice(unsigned choice)
{
	u32 freq;
	switch (choice) {
	case 0x0:
		freq = __decode_pll(PLL1_CLK, __HCLK_FREQ);
		break;
	case 0x1:
		freq = __decode_pll(PLL2_CLK, __HCLK_FREQ);
		break;
	case 0x2:
		freq = __decode_pll(PLL3_CLK, __HCLK_FREQ);
		break;
	default:
		freq = __get_lp_apm();
	}
	return freq;
}

static u32 __get_cspi_clk(void)
{
	u32 clk = (__REG(MXC_CCM_CSCMR1) & MXC_CCM_CSCMR1_CSPI_CLK_SEL_MASK)
		>> MXC_CCM_CSCMR1_CSPI_CLK_SEL_OFFSET;
	u32 cscdr2 = __REG(MXC_CCM_CSCDR2);
	u32 pred = (cscdr2 & MXC_CCM_CSCDR2_CSPI_CLK_PRED_MASK) \
			>> MXC_CCM_CSCDR2_CSPI_CLK_PRED_OFFSET;
	u32 podf = (cscdr2 & MXC_CCM_CSCDR2_CSPI_CLK_PODF_MASK) \
			>> MXC_CCM_CSCDR2_CSPI_CLK_PODF_OFFSET;
	return __get_pll_from_choice(clk) / ((pred + 1) * (podf + 1));
}

static u32 __get_clk_cscdr1_pred_podf(int cscmr1_offset, int pred_offset, int podf_offset)
{
	u32 clk = (__REG(MXC_CCM_CSCMR1) >> cscmr1_offset) & 0x3;
	u32 cscdr1 = __REG(MXC_CCM_CSCDR1);
	u32 pred = (cscdr1 >> pred_offset) & 0x7;
	u32 podf = (cscdr1 >> podf_offset) & 0x7;
	return __get_pll_from_choice(clk) / ((pred + 1) * (podf + 1));
}

static u32 __get_nfc_clk(void)
{
	u32 cbcdr =  __REG(MXC_CCM_CBCDR);
	u32 pdf = (cbcdr & MXC_CCM_CBCDR_NFC_PODF_MASK) \
			>> MXC_CCM_CBCDR_NFC_PODF_OFFSET;

	return  __get_emi_slow_clk() / (pdf + 1);
}

static u32 __get_esdhc1_clk(void)
{
	return __get_clk_cscdr1_pred_podf(
			MXC_CCM_CSCMR1_ESDHC1_MSHC1_CLK_SEL_OFFSET,
			MXC_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PRED_OFFSET,
			MXC_CCM_CSCDR1_ESDHC1_MSHC1_CLK_PODF_OFFSET);
}

static u32 __get_esdhc3_clk(void)
{
	return __get_clk_cscdr1_pred_podf(
			MXC_CCM_CSCMR1_ESDHC3_MSHC2_CLK_SEL_OFFSET,
			MXC_CCM_CSCDR1_ESDHC3_MSHC2_CLK_PRED_OFFSET,
			MXC_CCM_CSCDR1_ESDHC3_MSHC2_CLK_PODF_OFFSET);
}

static u32 __get_esdhc2_clk(void)
{
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 esdh2_clk_sel = cscmr1 & MXC_CCM_CSCMR1_ESDHC2_CLK_SEL;
	if (esdh2_clk_sel)
		return __get_esdhc3_clk();

	return __get_esdhc1_clk();
}

static u32 __get_esdhc4_clk(void)
{
	u32 cscmr1 = __REG(MXC_CCM_CSCMR1);
	u32 esdh4_clk_sel = cscmr1 & MXC_CCM_CSCMR1_ESDHC4_CLK_SEL;
	if (esdh4_clk_sel)
		return __get_esdhc3_clk();

	return __get_esdhc1_clk();
}

unsigned int mxc_get_clock(enum mxc_clock clk)
{
	switch (clk) {
	case MXC_ARM_CLK:
		return __get_mcu_main_clk();
	case MXC_PER_CLK:
		return __get_periph_clk();
	case MXC_AHB_CLK:
		return __get_ahb_clk();
	case MXC_IPG_CLK:
		return __get_ipg_clk();
	case MXC_IPG_PERCLK:
		return __get_ipg_per_clk();
	case MXC_UART_CLK:
		return __get_clk_cscdr1_pred_podf(
				MXC_CCM_CSCMR1_UART_CLK_SEL_OFFSET,
				MXC_CCM_CSCDR1_UART_CLK_PRED_OFFSET,
				MXC_CCM_CSCDR1_UART_CLK_PODF_OFFSET);
	case MXC_CSPI_CLK:
		return __get_cspi_clk();
	case MXC_AXI_A_CLK:
		return __get_axi_a_clk();
	case MXC_AXI_B_CLK:
		return __get_axi_b_clk();
	case MXC_EMI_SLOW_CLK:
		return __get_emi_slow_clk();
	case MXC_DDR_CLK:
		return __get_ddr_clk();
	case MXC_ESDHC_CLK:
		return __get_esdhc1_clk();
	case MXC_ESDHC2_CLK:
		return __get_esdhc2_clk();
	case MXC_ESDHC3_CLK:
		return __get_esdhc3_clk();
	case MXC_ESDHC4_CLK:
		return __get_esdhc4_clk();
	case MXC_SATA_CLK:
		return __get_ahb_clk();
	case MXC_IPU_CLK:
		return __get_ipu_clk();
	case MXC_IPU_DI0_CLK:
		return __get_ipu_di_clk(0);
	case MXC_IPU_DI1_CLK:
		return __get_ipu_di_clk(1);
	case MXC_NFC_CLK:
		return __get_nfc_clk();
	case MXC_LDB0_CLK:
		return __get_ldb_clk(0);
	case MXC_LDB1_CLK:
		return __get_ldb_clk(1);
	case MXC_PLL4_CLK:
		return __decode_pll(PLL4_CLK, __HCLK_FREQ);
	default:
		break;
	}
	return -1;
}

void mxc_dump_clocks(void)
{
	u32 freq;
	freq = __decode_pll(PLL1_CLK, __HCLK_FREQ);
	printf("mx53 pll1: %dMHz\n", freq / 1000000);
	freq = __decode_pll(PLL2_CLK, __HCLK_FREQ);
	printf("mx53 pll2: %dMHz\n", freq / 1000000);
	freq = __decode_pll(PLL3_CLK, __HCLK_FREQ);
	printf("mx53 pll3: %dMHz\n", freq / 1000000);
	freq = __decode_pll(PLL4_CLK, __HCLK_FREQ);
	printf("mx53 pll4: %dMHz\n", freq / 1000000);
	printf("ipu clock     : %dHz\n", mxc_get_clock(MXC_IPU_CLK));
	printf("ipg clock     : %dHz\n", mxc_get_clock(MXC_IPG_CLK));
	printf("ipg per clock : %dHz\n", mxc_get_clock(MXC_IPG_PERCLK));
	printf("uart clock    : %dHz\n", mxc_get_clock(MXC_UART_CLK));
	printf("cspi clock    : %dHz\n", mxc_get_clock(MXC_CSPI_CLK));
	printf("periph clock  : %dHz\n", __get_periph_clk());
	printf("ahb clock     : %dHz\n", mxc_get_clock(MXC_AHB_CLK));
	printf("axi_a clock   : %dHz\n", mxc_get_clock(MXC_AXI_A_CLK));
	printf("axi_b clock   : %dHz\n", mxc_get_clock(MXC_AXI_B_CLK));
	printf("emi_slow clock: %dHz\n", mxc_get_clock(MXC_EMI_SLOW_CLK));
	printf("ddr clock     : %dHz\n", mxc_get_clock(MXC_DDR_CLK));
	printf("esdhc1 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC_CLK));
	printf("esdhc2 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC2_CLK));
	printf("esdhc3 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC3_CLK));
	printf("esdhc4 clock  : %dHz\n", mxc_get_clock(MXC_ESDHC4_CLK));
	printf("nfc clock     : %dHz\n", mxc_get_clock(MXC_NFC_CLK));
	printf("ldb0 clock    : %dHz\n", mxc_get_clock(MXC_LDB0_CLK));
	printf("ldb1 clock    : %dHz\n", mxc_get_clock(MXC_LDB1_CLK));
	printf("ipu di0 clock : %dHz\n", mxc_get_clock(MXC_IPU_DI0_CLK));
	printf("ipu di1 clock : %dHz\n", mxc_get_clock(MXC_IPU_DI1_CLK));
	printf("pixel clock 0 : %dHz\n", get_pixel_clock(0));
	printf("pixel clock 1 : %dHz\n", get_pixel_clock(1));
}


#ifdef CONFIG_CMD_CLOCK
/* precondition: m>0 and n>0.  Let g=gcd(m,n). */
static int gcd(int m, int n)
{
	int t;
	while (m > 0) {
		if (n > m) {
			t = m;
			m = n;
			n = t;
		} /* swap */
		m -= n;
	}
	return n;
}

/*!
 * This is to calculate various parameters based on reference clock and
 * targeted clock based on the equation:
 *      t_clk = 2*ref_freq*(mfi + mfn/(mfd+1))/(pd+1)
 * This calculation is based on a fixed MFD value for simplicity.
 *
 * @param ref       reference clock freq in Hz
 * @param target    targeted clock in Hz
 * @param pll		pll_param structure.
 *
 * @return          0 if successful; non-zero otherwise.
 */
static int calc_pll_params(u32 ref, u32 target, struct pll_param *pll)
{
	u64 pd, mfi = 1, mfn, mfd, t1;
	u32 n_target = target;
	u32 n_ref = ref, i;

	/*
	 * Make sure targeted freq is in the valid range.
	 * Otherwise the following calculation might be wrong!!!
	 */
	if (n_target < PLL_FREQ_MIN(ref) ||
		n_target > PLL_FREQ_MAX(ref)) {
		printf("Targeted peripheral clock should be"
			"within [%d - %d]\n",
			PLL_FREQ_MIN(ref) / SZ_DEC_1M,
			PLL_FREQ_MAX(ref) / SZ_DEC_1M);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(fixed_mfd); i++) {
		if (fixed_mfd[i].ref_clk_hz == ref) {
			mfd = fixed_mfd[i].mfd;
			break;
		}
	}

	if (i == ARRAY_SIZE(fixed_mfd))
		return -1;

	/* Use n_target and n_ref to avoid overflow */
	for (pd = 1; pd <= PLL_PD_MAX; pd++) {
		t1 = n_target * pd;
		do_div(t1, (4 * n_ref));
		mfi = t1;
		if (mfi > PLL_MFI_MAX)
			return -1;
		else if (mfi < 5)
			continue;
		break;
	}
	/* Now got pd and mfi already */
	/*
	mfn = (((n_target * pd) / 4 - n_ref * mfi) * mfd) / n_ref;
	*/
	t1 = n_target * pd;
	do_div(t1, 4);
	t1 -= n_ref * mfi;
	t1 *= mfd;
	do_div(t1, n_ref);
	mfn = t1;
#ifdef CMD_CLOCK_DEBUG
	printf("%d: ref=%d, target=%d, pd=%d,"
			"mfi=%d,mfn=%d, mfd=%d\n",
			__LINE__, ref, (u32)n_target,
			(u32)pd, (u32)mfi, (u32)mfn,
			(u32)mfd);
#endif
	i = 1;
	if (mfn != 0)
		i = gcd(mfd, mfn);
	pll->pd = (u32)pd;
	pll->mfi = (u32)mfi;
	do_div(mfn, i);
	pll->mfn = (u32)mfn;
	do_div(mfd, i);
	pll->mfd = (u32)mfd;

	return 0;
}

struct map_clks_st
{
	const char *name;
	const unsigned mxc_clk;
};

const struct map_clks_st map_clks[] = {
	[CPU_CLK] = {"CPU", MXC_ARM_CLK},
	[PERIPH_CLK] = {"Peripheral", MXC_PER_CLK},
	[AHB_CLK] = {"AHB", MXC_AHB_CLK},
	[IPG_CLK] = {"IPG", MXC_IPG_CLK},
	[IPG_PERCLK] = {"IPG_PER", MXC_IPG_PERCLK},
	[UART_CLK] = {"UART", MXC_UART_CLK},
	[CSPI_CLK] = {"CSPI", MXC_CSPI_CLK},
	[DDR_CLK] = {"DDR", MXC_DDR_CLK},
	[NFC_CLK] = {"NFC", MXC_NFC_CLK},
	[LDB0_CLK] = {"LDB0", MXC_LDB0_CLK},
	[LDB1_CLK] = {"LDB1", MXC_LDB1_CLK},
	[PLL4] = {"PLL4", MXC_PLL4_CLK},
};

int clk_info(u32 clk_type)
{
	int freq = -1;
	if (clk_type < ARRAY_SIZE(map_clks)) {
		freq = mxc_get_clock(map_clks[clk_type].mxc_clk);
		printf("%s Clock: %dHz\n", map_clks[clk_type].name, freq);
	} else if (clk_type == ALL_CLK) {
		printf("cpu clock: %dMHz\n",
			mxc_get_clock(MXC_ARM_CLK) / SZ_DEC_1M);
		mxc_dump_clocks();
		freq = 0;
	} else {
		printf("Unsupported clock type! :(\n");
	}
	return freq;
}

#define calc_div(target_clk, src_clk, limit) ({	\
		u32 tmp = 0;	\
		if ((src_clk % target_clk) <= 100)	\
			tmp = src_clk / target_clk;	\
		else	\
			tmp = (src_clk / target_clk) + 1;	\
		if (tmp > limit)	\
			tmp = limit;	\
		(tmp - 1);	\
	})

static u32 calc_per_cbcdr_val(u32 per_clk, u32 cbcmr)
{
	u32 cbcdr = __REG(MXC_CCM_CBCDR);
	u32 tmp_clk = 0, div = 0, clk_sel = 0;

	cbcdr &= ~MXC_CCM_CBCDR_PERIPH_CLK_SEL;

	/* emi_slow_podf divider */
	tmp_clk = __get_emi_slow_clk();
	clk_sel = cbcdr & MXC_CCM_CBCDR_EMI_CLK_SEL;
	if (clk_sel) {
		div = calc_div(tmp_clk, per_clk, 8);
		cbcdr &= ~MXC_CCM_CBCDR_EMI_PODF_MASK;
		cbcdr |= (div << MXC_CCM_CBCDR_EMI_PODF_OFFSET);
	}

	/* axi_b_podf divider */
	tmp_clk = __get_axi_b_clk();
	div = calc_div(tmp_clk, per_clk, 8);
	cbcdr &= ~MXC_CCM_CBCDR_AXI_B_PODF_MASK;
	cbcdr |= (div << MXC_CCM_CBCDR_AXI_B_PODF_OFFSET);

	/* axi_b_podf divider */
	tmp_clk = __get_axi_a_clk();
	div = calc_div(tmp_clk, per_clk, 8);
	cbcdr &= ~MXC_CCM_CBCDR_AXI_A_PODF_MASK;
	cbcdr |= (div << MXC_CCM_CBCDR_AXI_A_PODF_OFFSET);

	/* ahb podf divider */
	tmp_clk = AHB_CLK_ROOT;
	div = calc_div(tmp_clk, per_clk, 8);
	cbcdr &= ~MXC_CCM_CBCDR_AHB_PODF_MASK;
	cbcdr |= (div << MXC_CCM_CBCDR_AHB_PODF_OFFSET);

	return cbcdr;
}

void change_pll_settings(enum pll_clocks base, struct pll_param *p)
{
	writel(0x0, base + PLL_DP_CONFIG);
	writel(((p->pd - 1) << 0) | (p->mfi << 4), base + PLL_DP_OP);
	writel(((p->pd - 1) << 0) | (p->mfi << 4), base + PLL_DP_HFS_OP);
	writel(p->mfd - 1, base + PLL_DP_MFD);
	writel(p->mfd - 1, base + PLL_DP_HFS_MFD);
	writel(p->mfn, base + PLL_DP_MFN);
	writel(p->mfn, base + PLL_DP_HFS_MFN);
	writel(0x1232, base + PLL_DP_CTL);
	while (!readl(base + PLL_DP_CTL) & 0x1)
		;
}

static int config_pll_clk(enum pll_clocks pll, struct pll_param *pll_param)
{
	u32 ccsr = readl(CCM_BASE_ADDR + CLKCTL_CCSR);
	u32 pll_base = pll;

	switch (pll) {
	case PLL1_CLK:
		/* Switch ARM to PLL2 clock */
		writel(ccsr | 0x4, CCM_BASE_ADDR + CLKCTL_CCSR);
		change_pll_settings(pll_base, pll_param);
		/* Switch back */
		writel(ccsr & ~0x4, CCM_BASE_ADDR + CLKCTL_CCSR);
		break;
	case PLL2_CLK:
		/* Switch to pll2 bypass clock */
		writel(ccsr | 0x2, CCM_BASE_ADDR + CLKCTL_CCSR);
		change_pll_settings(pll_base, pll_param);
		/* Switch back */
		writel(ccsr & ~0x2, CCM_BASE_ADDR + CLKCTL_CCSR);
		break;
	case PLL3_CLK:
		/* Switch to pll3 bypass clock */
		writel(ccsr | 0x1, CCM_BASE_ADDR + CLKCTL_CCSR);
		change_pll_settings(pll_base, pll_param);
		/* Switch back */
		writel(ccsr & ~0x1, CCM_BASE_ADDR + CLKCTL_CCSR);
		break;
	case PLL4_CLK:
		/* Switch to pll4 bypass clock */
		writel(ccsr | 0x20, CCM_BASE_ADDR + CLKCTL_CCSR);
		change_pll_settings(pll_base, pll_param);
		/* Switch back */
		writel(ccsr & ~0x20, CCM_BASE_ADDR + CLKCTL_CCSR);
		break;
	default:
		return -1;
	}

	return 0;
}

static int config_core_clk(u32 ref, u32 freq)
{
	int ret = 0;
	u32 pll = 0;
	struct pll_param pll_param;

	memset(&pll_param, 0, sizeof(struct pll_param));

	/* The case that periph uses PLL1 is not considered here */
	pll = freq;
	ret = calc_pll_params(ref, pll, &pll_param);
	if (ret != 0) {
		printf("Can't find pll parameters: %d\n",
			ret);
		return ret;
	}

	return config_pll_clk(PLL1_CLK, &pll_param);
}

static int config_pll4_clk(u32 ref, u32 freq)
{
	int ret = 0;
	struct pll_param pll_param = { 0 };

	/* The case that periph uses PLL1 is not considered here */
	ret = calc_pll_params(ref, freq, &pll_param);
	if (ret != 0) {
		printf("Can't find pll parameters: %d\n", ret);
		return ret;
	}

        ret = config_pll_clk(PLL4_CLK, &pll_param);
	return ret ;
}

static int config_nfc_clk(u32 nfc_clk)
{
	u32 reg;
	u32 parent_rate = __get_emi_slow_clk();
	u32 div = parent_rate / nfc_clk;

	if (nfc_clk <= 0)
		return -1;
	if (div == 0)
		div++;
	if (parent_rate / div > NFC_CLK_MAX)
		div++;
	reg = __REG(MXC_CCM_CBCDR);
	reg &= ~MXC_CCM_CBCDR_NFC_PODF_MASK;
	reg |= (div - 1) << MXC_CCM_CBCDR_NFC_PODF_OFFSET;
	writel(reg, MXC_CCM_CBCDR);
	while (readl(CCM_BASE_ADDR + CLKCTL_CDHIPR) != 0)
		;
	return 0;
}

static int config_ldb_clk(u32 ref, u32 freq, int which)
{
	u32 reg = __REG(MXC_CCM_CSCMR2) & ~MXC_CCM_CSCMR2_DI_CLK_SEL_MASK(which);
	config_pll4_clk(ref,freq*7);
	if (0 != freq) {
		reg |= 5 << MXC_CCM_CSCMR2_DI_CLK_SEL_OFFSET(which);
		__REG(MXC_CCM_CSCMR2) = reg ;
	}
	return 0;
}

static int config_periph_clk(u32 ref, u32 freq)
{
	int ret = 0;
	u32 pll = freq;
	struct pll_param pll_param;

	memset(&pll_param, 0, sizeof(struct pll_param));

	if (__REG(MXC_CCM_CBCDR) & MXC_CCM_CBCDR_PERIPH_CLK_SEL) {
		/* Actually this case is not considered here */
		ret = calc_pll_params(ref, pll, &pll_param);
		if (ret != 0) {
			printf("Can't find pll parameters: %d\n",
				ret);
			return ret;
		}
		switch ((__REG(MXC_CCM_CBCMR) & \
			MXC_CCM_CBCMR_PERIPH_CLK_SEL_MASK) >>
			MXC_CCM_CBCMR_PERIPH_CLK_SEL_OFFSET) {
		case 0:
			return config_pll_clk(PLL1_CLK, &pll_param);
			break;
		case 1:
			return config_pll_clk(PLL3_CLK, &pll_param);
			break;
		default:
			return -1;
		}
	} else {
		u32 old_cbcmr = readl(CCM_BASE_ADDR + CLKCTL_CBCMR);
		u32 new_cbcdr = calc_per_cbcdr_val(pll, old_cbcmr);
		u32 old_nfc = __get_nfc_clk();

		/* Switch peripheral to PLL3 */
		writel(0x00015154, CCM_BASE_ADDR + CLKCTL_CBCMR);
		writel(0x02888945, CCM_BASE_ADDR + CLKCTL_CBCDR);

		/* Make sure change is effective */
		while (readl(CCM_BASE_ADDR + CLKCTL_CDHIPR) != 0)
			;

		/* Setup PLL2 */
		ret = calc_pll_params(ref, pll, &pll_param);
		if (ret != 0) {
			printf("Can't find pll parameters: %d\n",
				ret);
			return ret;
		}
		config_pll_clk(PLL2_CLK, &pll_param);

		/* Switch peripheral back */
		writel(new_cbcdr, CCM_BASE_ADDR + CLKCTL_CBCDR);
		writel(old_cbcmr, CCM_BASE_ADDR + CLKCTL_CBCMR);

		/* Make sure change is effective */
		while (readl(CCM_BASE_ADDR + CLKCTL_CDHIPR) != 0)
			;
		/* restore to old NFC clock */
		config_nfc_clk(old_nfc);
		puts("\n");
	}

	return 0;
}

static int config_ddr_clk(u32 emi_clk)
{
	u32 clk_src;
	s32 shift = 0, clk_sel, div = 1;
	u32 cbcmr = readl(CCM_BASE_ADDR + CLKCTL_CBCMR);
	u32 cbcdr = readl(CCM_BASE_ADDR + CLKCTL_CBCDR);

	if (emi_clk > MAX_DDR_CLK) {
		printf("DDR clock should be less than"
			"%d MHz, assuming max value \n",
			(MAX_DDR_CLK / SZ_DEC_1M));
		emi_clk = MAX_DDR_CLK;
	}

	clk_src = __get_periph_clk();
	/* Find DDR clock input */
	clk_sel = (cbcmr >> 10) & 0x3;
	switch (clk_sel) {
	case 0:
		shift = 16;
		break;
	case 1:
		shift = 19;
		break;
	case 2:
		shift = 22;
		break;
	case 3:
		shift = 10;
		break;
	default:
		return -1;
	}

	if ((clk_src % emi_clk) == 0)
		div = clk_src / emi_clk;
	else
		div = (clk_src / emi_clk) + 1;
	if (div > 8)
		div = 8;

	cbcdr = cbcdr & ~(0x7 << shift);
	cbcdr |= ((div - 1) << shift);
	writel(cbcdr, CCM_BASE_ADDR + CLKCTL_CBCDR);
	while (readl(CCM_BASE_ADDR + CLKCTL_CDHIPR) != 0)
		;
	writel(0x0, CCM_BASE_ADDR + CLKCTL_CCDR);

	return 0;
}

static int config_di_clk(u32 ref, u32 freq, int which)
{
	printf ("%s(%d): %u -> %u\n", __func__, which, ref, freq );
	return 0 ;
}

/*!
 * This function assumes the expected core clock has to be changed by
 * modifying the PLL. This is NOT true always but for most of the times,
 * it is. So it assumes the PLL output freq is the same as the expected
 * core clock (presc=1) unless the core clock is less than PLL_FREQ_MIN.
 * In the latter case, it will try to increase the presc value until
 * (presc*core_clk) is greater than PLL_FREQ_MIN. It then makes call to
 * calc_pll_params() and obtains the values of PD, MFI,MFN, MFD based
 * on the targeted PLL and reference input clock to the PLL. Lastly,
 * it sets the register based on these values along with the dividers.
 * Note 1) There is no value checking for the passed-in divider values
 *         so the caller has to make sure those values are sensible.
 *      2) Also adjust the NFC divider such that the NFC clock doesn't
 *         exceed NFC_CLK_MAX.
 *      3) IPU HSP clock is independent of AHB clock. Even it can go up to
 *         177MHz for higher voltage, this function fixes the max to 133MHz.
 *      4) This function should not have allowed diag_printf() calls since
 *         the serial driver has been stoped. But leave then here to allow
 *         easy debugging by NOT calling the cyg_hal_plf_serial_stop().
 *
 * @param ref       pll input reference clock (24MHz)
 * @param freq		core clock in Hz
 * @param clk_type  clock type, e.g CPU_CLK, DDR_CLK, etc.
 * @return          0 if successful; non-zero otherwise
 */
int clk_config(u32 ref, u32 freq, u32 clk_type)
{
	if ((LDB0_CLK != clk_type) && (LDB1_CLK != clk_type))
		freq *= SZ_DEC_1M;

	switch (clk_type) {
	case CPU_CLK:
		if (config_core_clk(ref, freq))
			return -1;
		break;
	case PERIPH_CLK:
		if (config_periph_clk(ref, freq))
			return -1;
		break;
	case DDR_CLK:
		if (config_ddr_clk(freq))
			return -1;
		break;
	case NFC_CLK:
		if (config_nfc_clk(freq))
			return -1;
		break;
	case LDB0_CLK:
		if (config_ldb_clk(ref,freq,0))
			return -1;
		break;
	case LDB1_CLK:
		if (config_ldb_clk(ref,freq,1))
			return -1;
		break;
	case IPU_DI0_CLK:
		if (config_di_clk(ref,freq,0))
			return -1;
		break;
	case IPU_DI1_CLK:
		if (config_di_clk(ref,freq,1))
			return -1;
		break;
	case PLL4:
		if (config_pll4_clk(ref,freq))
			return -1;
		break;
	default:
		printf("Unsupported or invalid clock type! :(\n");
	}

	return 0;
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("CPU:   Freescale i.MX53 family %d.%dV at %d MHz\n",
	       (get_board_rev() & 0xFF) >> 4,
	       (get_board_rev() & 0xF),
		__get_mcu_main_clk() / 1000000);
	mxc_dump_clocks();
	return 0;
}
#endif

/*
 * Initializes on-chip ethernet controllers.
 * to override, implement board_eth_init()
 */
#if defined(CONFIG_MXC_FEC)
extern int mxc_fec_initialize(bd_t *bis);
extern void mxc_fec_set_mac_from_env(char *mac_addr);
#endif

int cpu_eth_init(bd_t *bis)
{
	int rc = -ENODEV;
#if defined(CONFIG_MXC_FEC)
	rc = mxc_fec_initialize(bis);
#endif
	return rc;
}

//#define DEBUG		//if enabled, also enable in start.S
#ifdef DEBUG
void TransmitX(char ch);
#define debug_putc(ch) TransmitX(ch)
#else
#define debug_putc(ch)
#endif

#if defined(CONFIG_ARCH_CPU_INIT)
int arch_cpu_init(void)
{
	debug("%s ", __func__);
	debug_putc('a');
	icache_enable();
	debug_putc('b');
	dcache_enable();
	debug_putc('c');
#ifdef CONFIG_L2_OFF
	l2_cache_disable();
#else
	l2_cache_enable();
#endif
	debug_putc('\n');
	return 0;
}
#endif

void set_usboh3_clk(void)
{
	unsigned int reg;

	reg = readl(MXC_CCM_CSCMR1) &
		 ~MXC_CCM_CSCMR1_USBOH3_CLK_SEL_MASK;
	reg |= 1 << MXC_CCM_CSCMR1_USBOH3_CLK_SEL_OFFSET;
	writel(reg, MXC_CCM_CSCMR1);

	reg = readl(MXC_CCM_CSCDR1);
	reg &= ~MXC_CCM_CSCDR1_USBOH3_CLK_PODF_MASK;
	reg &= ~MXC_CCM_CSCDR1_USBOH3_CLK_PRED_MASK;
	reg |= 4 << MXC_CCM_CSCDR1_USBOH3_CLK_PRED_OFFSET;
	reg |= 1 << MXC_CCM_CSCDR1_USBOH3_CLK_PODF_OFFSET;

	writel(reg, MXC_CCM_CSCDR1);
}

void set_usb_phy1_clk(void)
{
	unsigned int reg;

	reg = readl(MXC_CCM_CSCMR1);
	reg &= ~MXC_CCM_CSCMR1_USB_PHY_CLK_SEL;
	writel(reg, MXC_CCM_CSCMR1);
}

void enable_usboh3_clk(unsigned char enable)
{
	unsigned int reg;

	reg = readl(MXC_CCM_CCGR2);
	if (enable)
		reg |= 1 << MXC_CCM_CCGR2_CG14_OFFSET;
	else
		reg &= ~(1 << MXC_CCM_CCGR2_CG14_OFFSET);
	writel(reg, MXC_CCM_CCGR2);
}

#ifdef CONFIG_I2C_MXC
/* i2c_num can be from 0 - 2 */
int enable_i2c_clk(unsigned char enable, unsigned i2c_num)
{
	u32 reg;
	u32 mask;

	if (i2c_num > 2)
		return -EINVAL;
	mask = MXC_CCM_CCGR_CG_MASK << ((i2c_num + 9) << 1);
	reg = __raw_readl(MXC_CCM_CCGR1);
	if (enable)
		reg |= mask;
	else
		reg &= ~mask;
	__raw_writel(reg, MXC_CCM_CCGR1);
	return 0;
}
#endif

void enable_usb_phy1_clk(unsigned char enable)
{
	unsigned int reg;

	reg = readl(MXC_CCM_CCGR4);
	if (enable)
		reg |= 1 << MXC_CCM_CCGR4_CG5_OFFSET;
	else
		reg &= ~(1 << MXC_CCM_CCGR4_CG5_OFFSET);
	writel(reg, MXC_CCM_CCGR4);
}
