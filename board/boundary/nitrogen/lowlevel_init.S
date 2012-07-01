/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
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

#include <config.h>
#include <asm/arch/mx51.h>
#include "board-nitrogen.h"

.macro print_debug_ch ch
#if 0
	stmdb	sp!,{r0,r1,lr}
	mov	r0, #\ch
	bl	TransmitX
	ldmia	sp!,{r0,r1,lr}
#endif
.endm

.macro init_l1cc
	mov	r0, #0
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate I + D TLBs
	mcr	p15, 0, r0, c2, c0, 2	@ TTB control register

	mov	r0, #0
	mcr	p15, 2, r0, c0, c0, 0
	mrc	p15, 1, r0, c0, c0, 0

	ldr	r1, =0x7fff
	and	r2, r1, r0, lsr #13

	ldr	r1, =0x3ff

	and	r3, r1, r0, lsr #3	@ NumWays - 1
	add	r2, r2, #1		@ NumSets

	and	r0, r0, #0x7
	add	r0, r0, #4		@ SetShift

	clz	r1, r3			@ WayShift
	add	r4, r3, #1		@ NumWays
1:	sub	r2, r2, #1		@ NumSets--
	mov	r3, r4			@ Temp = NumWays
2:	subs	r3, r3, #1		@ Temp--
	mov	r5, r3, lsl r1
	mov	r6, r2, lsl r0
	orr	r5, r5, r6
@ Reg = (Temp<<WayShift)|(NumSets<<SetShift)
	mcr	p15, 0, r5, c7, c6, 2
	bgt	2b
	cmp	r2, #0
	bgt	1b
	dsb
	isb
.endm

/*
 * L2CC Cache setup/invalidation/disable
 */
.macro init_l2cc
	/* explicitly disable L2 cache */
	/* in Auxiliary Control Register */
	mrc	15, 0, r0, c1, c0, 1
	bic	r0, r0, #0x2
	mcr	15, 0, r0, c1, c0, 1

	/* reconfigure L2 cache aux control reg */
	mov	r0, #0xC4				/* tag/data RAM */
	/* disable write allocate, combine, delay */
	orr	r0, r0, #(1 << 22)|(1 << 23)|(1 << 24)

	/* Write L2 Cache Auxiliary Control Register */
	mcr	15, 1, r0, c9, c0, 2
.endm /* init_l2cc */

/* AIPS setup - Only setup MPROTx registers.
 * The PACR default values are good.*/
.macro init_aips	rAips1, rAips2
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	ldr	r0, =0x77777777
	str	r0, [\rAips1, #0x0]
	str	r0, [\rAips1, #0x4]
	str	r0, [\rAips2, #0x0]
	str	r0, [\rAips2, #0x4]
	/*
	 * Clear the on and off peripheral modules Supervisor Protect bit
	 * for SDMA to access them. Did not change the AIPS control registers
	 * (offset 0x20) access type
	 */
.endm /* init_aips */

/* M4IF setup */
.macro init_m4if	rM4if
	/* VPU and IPU given higher priority (0x4)
	 * IPU accesses with ID=0x1 given highest priority (=0xA)
	 */
	ldr	r0, =0x00000203
	str	r0, [\rM4if, #0x40]

	mov	r0, #0x0
	str	r0, [\rM4if, #0x44]

	ldr	r0, =0x00120125
	str	r0, [\rM4if, #0x9C]

	ldr	r0, =0x001901A3
	str	r0, [\rM4if, #0x48]
.endm /* init_m4if */

#define IPU_CM_REG_BASE		0x5E000000
#define IPU_CONF		0x000
#define IPU_DISP_GEN		0x0C4
#define IPU_CH_DB_MODE_SEL0	0x150
#define IPU_CH_DB_MODE_SEL1	0x154
#define IPU_CH_TRB_MODE_SEL0	0x178
#define IPU_CH_TRB_MODE_SEL1	0x17c

#define IPU_IDMAC_BASE		0x5e008000
#define IDMAC_CONF		0x000
#define IDMAC_CH_EN_1		0x004
#define IDMAC_CH_EN_2		0x008
#define IDMAC_WM_EN_1		0x01C
#define IDMAC_WM_EN_2		0x020
.macro init_disable_ipu
	mov	r1,#IPU_CM_REG_BASE
	ldr	r0,[r1, #IPU_DISP_GEN]
	bic	r0, r0, #(3 << 24)	/* clear DI0/DI1 counter release */
	str	r0,[r1, #IPU_DISP_GEN]

	mov	r0, #0
	str	r0,[r1, #IPU_CONF]	/* disable DI0/DI1 */
	
	add	r2, r1, #IPU_IDMAC_BASE - IPU_CM_REG_BASE
	str	r0, [r2, #IDMAC_CH_EN_1]
	str	r0, [r2, #IDMAC_CH_EN_2]
	str	r0, [r2, #IDMAC_WM_EN_1]
	str	r0, [r2, #IDMAC_WM_EN_2]

	str	r0, [r1, #IPU_CH_DB_MODE_SEL0]
	str	r0, [r1, #IPU_CH_DB_MODE_SEL1]
	str	r0, [r1, #IPU_CH_TRB_MODE_SEL0]
	str	r0, [r1, #IPU_CH_TRB_MODE_SEL1]
.endm

	.equiv	_DP_OP_1000,	DP_OP_1000
	.equiv	_DP_MFN_1000,	DP_MFN_1000
	.equiv	_DP_MFD_1000,	DP_MFD_1000
	.equiv	_DP_CTL_1000,	DP_CTL_1000

	.equiv	_DP_OP_850,	DP_OP_850
	.equiv	_DP_MFN_850,	DP_MFN_850
	.equiv	_DP_MFD_850,	DP_MFD_850
	.equiv	_DP_CTL_850,	DP_CTL_850

	.equiv	_DP_OP_800,	DP_OP_800
	.equiv	_DP_MFN_800,	DP_MFN_800
	.equiv	_DP_MFD_800,	DP_MFD_800
	.equiv	_DP_CTL_800,	DP_CTL_800

	.equiv	_DP_OP_700,	DP_OP_700
	.equiv	_DP_MFN_700,	DP_MFN_700
	.equiv	_DP_MFD_700,	DP_MFD_700
	.equiv	_DP_CTL_700,	DP_CTL_700

	.equiv	_DP_OP_665,	DP_OP_665
	.equiv	_DP_MFN_665,	DP_MFN_665
	.equiv	_DP_MFD_665,	DP_MFD_665
	.equiv	_DP_CTL_665,	DP_CTL_665

	.equiv	_DP_OP_600,	DP_OP_600
	.equiv	_DP_MFN_600,	DP_MFN_600
	.equiv	_DP_MFD_600,	DP_MFD_600
	.equiv	_DP_CTL_600,	DP_CTL_600

	.equiv	_DP_OP_532,	DP_OP_532
	.equiv	_DP_MFN_532,	DP_MFN_532
	.equiv	_DP_MFD_532,	DP_MFD_532
	.equiv	_DP_CTL_532,	DP_CTL_532

	.equiv	_DP_OP_400,	DP_OP_400
	.equiv	_DP_MFN_400,	DP_MFN_400
	.equiv	_DP_MFD_400,	DP_MFD_400
	.equiv	_DP_CTL_400,	DP_CTL_400

	.equiv	_DP_OP_216,	DP_OP_216
	.equiv	_DP_MFN_216,	DP_MFN_216
	.equiv	_DP_MFD_216,	DP_MFD_216
	.equiv	_DP_CTL_216,	DP_CTL_216

.macro pll_op_mfd_mfn r_pll, dp_op, dp_mfd, dp_mfn
	mov	r0, #\dp_op
	str	r0, [\r_pll, #PLL_DP_OP]
	str	r0, [\r_pll, #PLL_DP_HFS_OP]

	mov	r0, #\dp_mfd
	str	r0, [\r_pll, #PLL_DP_MFD]
	str	r0, [\r_pll, #PLL_DP_HFS_MFD]

	mov	r0, #\dp_mfn
	str	r0, [\r_pll, #PLL_DP_MFN]
	str	r0, [\r_pll, #PLL_DP_HFS_MFN]
.endm

.macro pll_op r_pll, freq
	pll_op_mfd_mfn \r_pll, _DP_OP_\freq, _DP_MFD_\freq, _DP_MFN_\freq
.endm

.macro start_pll r_pll, dp_ctl
	ldr	r0, =\dp_ctl
	str	r0, [\r_pll, #PLL_DP_CTL] /* Set DPLL ON (set UPEN bit) */
1:	ldr	r0, [\r_pll, #PLL_DP_CTL]
	tst	r0, #0x1
	beq	1b
.endm

.macro start_pllf r_pll, freq
	start_pll	\r_pll, _DP_CTL_\freq
.endm

.macro divisor_change_wait r_clk
	/* make sure divider effective */
1:	ldr	r0, [\r_clk, #CLKCTL_CDHIPR]
	cmp	r0, #0x0
	bne	1b
.endm

.macro init_clock	rClk, rPll1, rPll2, rPll3

	/* Disable IPU and HSC handshake */
	mov	r0, #0x00060000
	str	r0, [\rClk, #CLKCTL_CCDR]

	ldr	r0, =0x1fff
	str	r0, [\rClk,#CLKCTL_CCR]	/* Turn on oscillator */

waitosc:
	ldr	r0, [\rClk, #CLKCTL_CSR]
	and	r0, r0, #0x22
	cmp	r0, #0x22		/* wait for Oscillator and FPM ready */
	bne	waitosc
	print_debug_ch '1'

/* disable auto restart */
	mov	r0, #0
	str	r0, [\rPll1, #PLL_DP_CONFIG]
	str	r0, [\rPll2, #PLL_DP_CONFIG]
	str	r0, [\rPll3, #PLL_DP_CONFIG]
	print_debug_ch '2'

	/* Gate of clocks to the peripherals first */
	mvn	r0, #~0x3FFFFFFF
	str	r0, [\rClk, #CLKCTL_CCGR0]
	mov	r0, #0x0
	str	r0, [\rClk, #CLKCTL_CCGR1]
	str	r0, [\rClk, #CLKCTL_CCGR2]
	str	r0, [\rClk, #CLKCTL_CCGR3]

	mov	r0, #0x00030000
	str	r0, [\rClk, #CLKCTL_CCGR4]
	ldr	r0, =0x00FFF030
	str	r0, [\rClk, #CLKCTL_CCGR5]
	mov	r0, #0x00000300
	str	r0, [\rClk, #CLKCTL_CCGR6]

	/* Make sure to switch the DDR away from PLL 1 */
	/* keep the initial ddr podf value */
	ldr	r2, [\rClk, #CLKCTL_CBCDR]
	ldr	r3, =0x19239145
	str	r3, [\rClk, #CLKCTL_CBCDR]
	divisor_change_wait \rClk

	/* Switch ARM to step clock (clock source 4) */
	/* deselect step clock for glitch free update */
	ldr	r0, [\rClk, #CLKCTL_CCSR]
	bic	r0,r0,#4
	str	r0, [\rClk, #CLKCTL_CCSR]
	mov	r0, #0
	str	r0, [\rClk, #CLKCTL_CCSR]
	/* select clock source 4 as source for step clock */
	/* switch arm to step clock */
	mov	r0, #0x4
	str	r0, [\rClk, #CLKCTL_CCSR]
	pll_op	\rPll1, CONFIG_SYS_PLL1_FREQ
	start_pllf \rPll1, CONFIG_SYS_PLL1_FREQ

	/* Switch ARM back to PLL 1 */
	mov	r0, #0
	str	r0, [\rClk, #CLKCTL_CCSR]

	pll_op	\rPll3, 665
	start_pllf \rPll3, 665

	/* Switch peripheral to PLL 3 */
	ldr	r0, =0x000010C0
	str	r0, [\rClk, #CLKCTL_CBCMR]
	/* [25] 0 -> 1 (periph was pll2, now apm) */
	orr	r0, r3, #1<<25
	str	r0, [\rClk, #CLKCTL_CBCDR]
	divisor_change_wait \rClk

	pll_op	\rPll2, 665
	start_pllf \rPll2, 665

	/* Switch peripheral to PLL2 */
	/* [25] 1 -> 0 (periph was apm, now pll2) */
	str	r3, [\rClk, #CLKCTL_CBCDR]

	ldr	r0, =0x000020C0
	str	r0, [\rClk, #CLKCTL_CBCMR]

	pll_op	\rPll3, 216
	start_pllf \rPll3, 216

	/* Set the platform clock dividers */
	ldr	r1, =ARM_BASE_ADDR
	ldr	r0, =0x00000725
	str	r0, [r1, #0x14]

	/*
	 * Run Tapeout 3.0 at Full speed,
	 * for other TO's wait till we increase VDDGP
	 */
	mov	r0, #CONFIG_SYS_ARM_PODF
	str	r0, [\rClk, #CLKCTL_CACRR]

	/* setup the rest */
	/* Use lp_apm (24MHz) source for perclk */
	ldr	r0, =0x000020C2
	str	r0, [\rClk, #CLKCTL_CBCMR]
	/* ddr clock from PLL 1, all perclk dividers are 1 since using 24MHz */
	ldr	r0, =0x41E35100
	/* keep the initial ddr podf value */
	and	r2, r2, #(7 << 27)
	orr	r0, r0, r2
	str	r0, [\rClk, #CLKCTL_CBCDR]

	/* Use PLL2 for UART clock, get 66.5MHz from it */
	ldr	r0, =0xA5A2A020
	str	r0, [\rClk, #CLKCTL_CSCMR1]
	ldr	r0, =0x00C30321
	str	r0, [\rClk, #CLKCTL_CSCDR1]

	divisor_change_wait \rClk

	/* Restore the default values in the Gate registers */
	mvn	r0, #~0xFFFFFFFF
	str	r0, [\rClk, #CLKCTL_CCGR0]
	str	r0, [\rClk, #CLKCTL_CCGR1]
	str	r0, [\rClk, #CLKCTL_CCGR2]
	str	r0, [\rClk, #CLKCTL_CCGR3]
	str	r0, [\rClk, #CLKCTL_CCGR4]
	str	r0, [\rClk, #CLKCTL_CCGR5]
	str	r0, [\rClk, #CLKCTL_CCGR6]

	mov	r0, #0x0
	str	r0, [\rClk, #CLKCTL_CCDR]

	/* for cko - for ARM div by 8 */
	mov	r0, #0x000A0000
	add	r0, r0, #0x00000F0
	str	r0, [\rClk, #CLKCTL_CCOSR]
.endm

.section ".text.init", "x"

#define GPIO_DR 0
#define GPIO_DIR 4

.globl lowlevel_init
lowlevel_init:
	mov	r8, lr
	
	print_debug_ch 'a'
/* make gp1[23] a high output */
	ldr	r1, =GPIO1_BASE_ADDR
	ldr	r0, [r1, #GPIO_DR]
	orr	r0, r0, #(1 << 23)
	str	r0, [r1, #GPIO_DR]
	ldr	r0, [r1, #GPIO_DIR]
	orr	r0, r0, #(1 << 23)
	str	r0, [r1, #GPIO_DIR]

#ifdef ENABLE_IMPRECISE_ABORT
	mrs	r1, spsr		/* save old spsr */
	mrs	r0, cpsr		/* read out the cpsr */
	bic	r0, r0, #0x100		/* clear the A bit */
	msr	spsr, r0		/* update spsr */
	add	lr, pc, #0x8		/* update lr */
	movs	pc, lr			/* update cpsr */
	nop
	nop
	nop
	nop
	msr	spsr, r1		/* restore old spsr */
#endif

	/* ARM errata ID #468414 */
	mrc	15, 0, r1, c1, c0, 1
	orr	r1, r1, #(1 << 5)	/* enable L1NEON bit */
	mcr	15, 0, r1, c1, c0, 1

	print_debug_ch 'b'
	init_l1cc

	print_debug_ch 'c'
	init_l2cc

	print_debug_ch 'd'
	ldr	r4, =AIPS1_BASE_ADDR
	ldr	r5, =AIPS2_BASE_ADDR
	init_aips	r4, r5

	print_debug_ch 'e'
	ldr	r4, =M4IF_BASE_ADDR
	init_m4if	r4

//	print_debug_ch 'g'
//	init_disable_ipu

	print_debug_ch 'h'

	ldr	r4, =CCM_BASE_ADDR
	ldr	r5, =PLL1_BASE_ADDR
	ldr	r6, =PLL2_BASE_ADDR
	ldr	r7, =PLL3_BASE_ADDR
	init_clock	r4, r5, r6, r7
	bl	setup_uart

	print_debug_ch '\n'
	mov	pc, r8
