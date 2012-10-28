/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mx53.h>

#ifndef CONFIG_SYS_PLL1_FREQ
#define CONFIG_SYS_PLL1_FREQ 800
#endif

.macro print_debug_ch ch
#ifdef DEBUG
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

.macro init_arm_erratum
	/* ARM erratum ID #468414 */
	mrc 15, 0, r1, c1, c0, 1
	orr r1, r1, #(1 << 5)    /* enable L1NEON bit */
	mcr 15, 0, r1, c1, c0, 1
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
	orr	r0, r0, #(1 << 22) | (1 << 23) | (1 << 24)

	/* Write L2 Cache Auxiliary Control Register */
	mcr	15, 1, r0, c9, c0, 2
.endm /* init_l2cc */

/* AIPS setup - Only setup MPROTx registers.
 * The PACR default values are good.*/
.macro init_aips
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	ldr	r0, =AIPS1_BASE_ADDR
	ldr	r1, =0x77777777
	str	r1, [r0, #0x0]
	str	r1, [r0, #0x4]
	ldr	r0, =AIPS2_BASE_ADDR
	str	r1, [r0, #0x0]
	str	r1, [r0, #0x4]
.endm /* init_aips */

/* M4IF setup */
.macro init_m4if	rM4if
#ifdef CONFIG_MX51
	ldr	\rM4if, =M4IF_BASE_ADDR
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
#endif
.endm /* init_m4if */

/* Assuming 24MHz input clock with doubler ON
 * fdck_2 = 4 * fref * (MFI + (MFN/MFD))/PDF
 * fdck_2 = (MFN/MFD + MFI) * 96 /PDF
 *
 * (MFI >= 5) && (MFI <= 15)
 * (PDF >= 1) && (PDF <= 16)
 * (MFD >= 1) && (MFD <= 0x3fffffe)
 * (MFN >= -0x3fffffe) && (MFN <= 0x3fffffe) && (|MFN| <= MFD)
 *
 * BRM = 0 if MFD < 8
 * BRM = 1 if MFD >= 8
 */
#define MAKE_OP(mfi, pdf) (((mfi) << 4) | (pdf - 1))

#define DP_CTL_RESTART	0x10
#define DP_CTL_BRMO	0x02

/* (5/12 + 10) * 96 / 1 = 1000 */
	.equiv	_DP_OP_1000,	MAKE_OP(10, 1)
	.equiv	_DP_MFN_1000,	5
	.equiv	_DP_MFD_1000,	(12 - 1)
	.equiv	_DP_CTL_1000,	(0x1220 | DP_CTL_RESTART | DP_CTL_BRMO)

/* (180/180 + 8) * 96 / 1 = 864 */
	.equiv	_DP_OP_864,	MAKE_OP(8, 1)
	.equiv	_DP_MFN_864,	180
	.equiv	_DP_MFD_864,	(180 - 1) /* PL Dither mode */
	.equiv	_DP_CTL_864,	(0x1220 | DP_CTL_RESTART | DP_CTL_BRMO)

/* (41/48 + 8) * 96 / 1 = 850 */
	.equiv	_DP_OP_850,	MAKE_OP(8, 1)
	.equiv	_DP_MFN_850,	41
	.equiv	_DP_MFD_850,	(48 - 1)
	.equiv	_DP_CTL_850,	(0x1220 | DP_CTL_RESTART | DP_CTL_BRMO)

#define DP_MFN_800_DIT	60 /* PL Dither mode */
/* (1/3 + 8) * 96 / 1 = 800 */
	.equiv	_DP_OP_800,	MAKE_OP(8, 1)
	.equiv	_DP_MFN_800,	1
	.equiv	_DP_MFD_800,	(3 - 1)
	.equiv	_DP_CTL_800,	(0x1220 | DP_CTL_RESTART)

/* (7/24 + 7) * 96 / 1 = 700 */
	.equiv	_DP_OP_700,	MAKE_OP(7, 1)
	.equiv	_DP_MFN_700,	7
	.equiv	_DP_MFD_700,	(24 - 1)
	.equiv	_DP_CTL_700,	(0x1220 | DP_CTL_RESTART | DP_CTL_BRMO)

/* (89/96 + 6) * 96 / 1 = 665 */
	.equiv	_DP_OP_665,	MAKE_OP(6, 1)
	.equiv	_DP_MFN_665,	89
	.equiv	_DP_MFD_665,	(96 - 1)
	.equiv	_DP_CTL_665,	(0x1220 | DP_CTL_RESTART | DP_CTL_BRMO)

/* (1/4 + 6) * 96 / 1 = 600 */
	.equiv	_DP_OP_600,	MAKE_OP(6, 1)
	.equiv	_DP_MFN_600,	1
	.equiv	_DP_MFD_600,	(4 - 1)
	.equiv	_DP_CTL_600,	(0x1220 | DP_CTL_RESTART | DP_CTL_BRMO)

/* (13/24 + 5) * 96 / 1 = 532 */
	.equiv	_DP_OP_532,	MAKE_OP(5, 1)
	.equiv	_DP_MFN_532,	13
	.equiv	_DP_MFD_532,	(24 - 1)
	.equiv	_DP_CTL_532,	(0x1220 | DP_CTL_RESTART | DP_CTL_BRMO)

/* (23/48 + 9) * 96 / 2 = 455 */
	.equiv	_DP_OP_455,	MAKE_OP(9, 2)
	.equiv	_DP_MFN_455,	23
	.equiv	_DP_MFD_455,	(48 - 1)
	.equiv	_DP_CTL_455,	(0x1220 | DP_CTL_RESTART | DP_CTL_BRMO)

/* (1/3 + 8) * 96 / 2 = 400 */
	.equiv	_DP_OP_400,	MAKE_OP(8, 2)
	.equiv	_DP_MFN_400,	1
	.equiv	_DP_MFD_400,	(3 - 1)
	.equiv	_DP_CTL_400,	(0x1220 | DP_CTL_RESTART)

/* (1/4 + 6) * 96 / 2 = 300 */
	.equiv	_DP_OP_300,	MAKE_OP(6, 2)
	.equiv	_DP_MFN_300,	1
	.equiv	_DP_MFD_300,	(4 - 1)
	.equiv	_DP_CTL_300,	(0x1220 | DP_CTL_RESTART)

#if 0
/*
 * Too low, output must be 300 - 800
 * (3/4 + 6) * 96 / 3 = 216
 */
	.equiv	_DP_OP_216,	MAKE_OP(6, 3)
	.equiv	_DP_MFN_216,	3
	.equiv	_DP_MFD_216,	(4 - 1)
	.equiv	_DP_CTL_216,	(0x1220 | DP_CTL_RESTART)
#else
/*
 * (0/2 + 9) * 96 / 2 = 432
 * 432 / 2 = 216
 */
	.equiv	_DP_OP_216,	MAKE_OP(9, 2)
	.equiv	_DP_MFN_216,	0
	.equiv	_DP_MFD_216,	(2 - 1)
	.equiv	_DP_CTL_216,	(0x0220 | DP_CTL_RESTART)
#endif

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

.macro setup_pll_errata r_pll, mfn
	start_pll	\r_pll, 0x00001236

	mov	r0, #\mfn
	str	r0, [\r_pll, #PLL_DP_MFN]	/* Modify MFN value */
	str	r0, [\r_pll, #PLL_DP_HFS_MFN]

	mov	r0, #0x1
	str	r0, [\r_pll, #PLL_DP_CONFIG]	/* Reload MFN value */

2:	ldr	r0, [\r_pll, #PLL_DP_CONFIG]
	tst	r0, #1
	bne	2b

	ldr	r0, =100			/* Wait at least 4 us */
3:	subs	r0, r0, #1
	bge	3b
.endm

.macro init_clock	rClk, rPll1, rPll2, rPll3
	ldr	\rClk, =CCM_BASE_ADDR

#ifdef CONFIG_MX51
	/* Disable IPU and HSC handshake */
	mov	r0, #0x00060000
#else
	/* Disable IPU handshake */
	mov	r0, #(1<<21)
#endif
	str	r0, [\rClk, #CLKCTL_CCDR]
	print_debug_ch '1'

	ldr	\rPll1, =PLL1_BASE_ADDR
	ldr	\rPll2, =PLL2_BASE_ADDR
	ldr	\rPll3, =PLL3_BASE_ADDR

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
	str	r0, [\rClk, #CLKCTL_CCGR7]

	mov	r0, #0x00030000
	str	r0, [\rClk, #CLKCTL_CCGR4]
	ldr	r0, =0x00FFF030
	str	r0, [\rClk, #CLKCTL_CCGR5]
#ifdef CONFIG_MX51
	mov	r0, #0x00000300
	str	r0, [\rClk, #CLKCTL_CCGR6]

	/* Make sure to switch the DDR away from PLL 1 */
	/* keep the initial ddr podf value */
	ldr	r2, [\rClk, #CLKCTL_CBCDR]
	ldr	r3, =0x19239145
	str	r3, [\rClk, #CLKCTL_CBCDR]
	divisor_change_wait \rClk
#else
	ldr	r0, =0x0303
	str	r0, [\rClk, #CLKCTL_CCGR6]
#endif

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

#ifdef CONFIG_MX51_PLL_ERRATA
	pll_op	\rPll1, 864
	start_pllf \rPll1,864
	setup_pll_errata \rPll1, DP_MFN_800_DIT
#else
	pll_op	\rPll1, CONFIG_SYS_PLL1_FREQ
	start_pllf \rPll1, CONFIG_SYS_PLL1_FREQ
#endif

	/* Switch ARM back to PLL 1 */
	mov	r0, #0
	str	r0, [\rClk, #CLKCTL_CCSR]

#ifdef CONFIG_MX51
	pll_op	\rPll3, 665
	start_pllf \rPll3, 665

	/* Switch peripheral to PLL 3 */
	ldr	r0, =0x000010C0
#if CONFIG_SYS_DDR_CLKSEL
	orr	r0,r0,#CONFIG_SYS_DDR_CLKSEL
#endif
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
#if CONFIG_SYS_DDR_CLKSEL
	orr	r0,r0,#CONFIG_SYS_DDR_CLKSEL
#endif
	str	r0, [\rClk, #CLKCTL_CBCMR]
#endif

#if 0
	pll_op	\rPll3, 400
	start_pllf \rPll3, 400

	/* Switch peripheral to PLL 3 */
	ldr	r0, =0x0001515c		/* periph_apm_sel - 01(pll3_sw_clk) */
	str	r0, [\rClk, #CLKCTL_CBCMR]
	ldr	r0, =0x02898945
	ldr	r2, [\rClk, #CLKCTL_CBCDR]
	str	r0, [\rClk, #CLKCTL_CBCDR]
	divisor_change_wait \rClk

	pll_op	\rPll2, CONFIG_SYS_PLL2_FREQ
	start_pllf \rPll2, CONFIG_SYS_PLL2_FREQ

	/* Switch peripheral to PLL2 */
	ldr	r0, =0x00808145
	orr	r0, r0, #(CONFIG_SYS_AHB_PODF << 10)
	orr	r0, r0, #(CONFIG_SYS_AXIA_PODF << 16)
	orr	r0, r0, #(CONFIG_SYS_AXIB_PODF << 19)
	str	r0, [\rClk, #CLKCTL_CBCDR]
#else
	ldr	r2, [\rClk, #CLKCTL_CBCDR]
	ldr	r0, =0x00808145
	orr	r0, r0, #(CONFIG_SYS_AHB_PODF << 10)
	orr	r0, r0, #(CONFIG_SYS_AXIB_PODF << 19)
//	orr	r0, r0, #(CONFIG_SYS_AXIA_PODF << 16)	/* keep same AXI_A_PODF */
	and	r2, r2, #(7 << 16)			/* keep same AXI_A_PODF */
	orr	r0, r0, r2
	str	r0, [\rClk, #CLKCTL_CBCDR]
#endif
	ldr	r0, =0x00016154		/* periph_apm_sel - 2(lp_apm) */
	str	r0, [\rClk, #CLKCTL_CBCMR]
	divisor_change_wait \rClk

	pll_op	\rPll3, 216
	start_pllf \rPll3, 216

	/* Set the platform clock dividers */
	ldr	r2, =ARM_BASE_ADDR
	mov	r0, #0x0124
	str	r0, [r2, #PLATFORM_ICGC]

#ifdef CONFIG_MX51
	/*
	 * Run Tapeout 3.0 at Full speed,
	 * for other TO's wait till we increase VDDGP
	 */
	mov	r0, #0x0
#if 0
	mov	r0, #0x0
	ldr	r1, [r0, #ROM_SI_REV]
	cmp	r1, #0x10
	movls	r0, #0x1
	movhi	r0, #0
#endif
#else
	mov	r0, #1
#endif
	str	r0, [\rClk, #CLKCTL_CACRR]

#ifdef CONFIG_MX51
	/* setup the rest */
	/* Use lp_apm (24MHz) source for perclk */
	ldr	r0, =0x000020C2
#if CONFIG_SYS_DDR_CLKSEL
	orr	r0,r0,#CONFIG_SYS_DDR_CLKSEL
#endif
	str	r0, [\rClk, #CLKCTL_CBCMR]
	/* ddr clock from PLL 1, all perclk dividers are 1 since using 24MHz */
#if 1
	ldr	r0, =CONFIG_SYS_CLKTL_CBCDR & ~(7 << 27)
	/* keep the initial ddr podf value */
	and	r2, r2, #(7 << 27)
	orr	r0, r0, r2
#else
	ldr	r0, =CONFIG_SYS_CLKTL_CBCDR
#endif
	str	r0, [\rClk, #CLKCTL_CBCDR]

	/* Use PLL2 for UART's clock, get 66.5MHz from it */
	ldr	r0, =0xA5A2A020
	str	r0, [\rClk, #CLKCTL_CSCMR1]
	ldr	r0, =0x00C30321
	str	r0, [\rClk, #CLKCTL_CSCDR1]
#endif

#ifdef CONFIG_MX53
	ldr	r0, [\rClk, #CLKCTL_CSCDR1]
	bic	r0, r0, #0x3f
	orr	r0, r0, #0x21
	str	r0, [\rClk, #CLKCTL_CSCDR1]
#endif
	/* Restore the default values in the Gate registers */
	mvn	r0, #~0xFFFFFFFF
	str	r0, [\rClk, #CLKCTL_CCGR0]
	str	r0, [\rClk, #CLKCTL_CCGR1]
	str	r0, [\rClk, #CLKCTL_CCGR2]
	str	r0, [\rClk, #CLKCTL_CCGR3]
	str	r0, [\rClk, #CLKCTL_CCGR4]
	str	r0, [\rClk, #CLKCTL_CCGR5]
	str	r0, [\rClk, #CLKCTL_CCGR6]
#ifdef CONFIG_MX53
	str	r0, [\rClk, #CLKCTL_CCGR7]
#endif
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
#ifdef CONFIG_MX51
/* make gp1[23] a high output */
	ldr	r1, =GPIO1_BASE_ADDR
	ldr	r0, [r1, #GPIO_DR]
	orr	r0, r0, #1 << 23
	str	r0, [r1, #GPIO_DR]
	ldr	r0, [r1, #GPIO_DIR]
	orr	r0, r0, #1 << 23
	str	r0, [r1, #GPIO_DIR]
#endif

	init_arm_erratum

	print_debug_ch 'b'
	init_l1cc

	print_debug_ch 'c'
	init_l2cc

	print_debug_ch 'd'
	init_aips

	print_debug_ch 'e'
	init_m4if	r4

	print_debug_ch 'h'
	.balignl	32,0xe1a00000   /* nop code */
	init_clock	r7, r4, r5, r6

	print_debug_ch 'i'
	print_debug_ch '\n'
	mov	pc, r8
