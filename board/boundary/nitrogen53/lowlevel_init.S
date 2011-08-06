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

/*
 * L2CC Cache setup/invalidation/disable
 */
.macro init_l2cc
	/* explicitly disable L2 cache */
        mrc 15, 0, r0, c1, c0, 1
        bic r0, r0, #0x2
        mcr 15, 0, r0, c1, c0, 1

        /* reconfigure L2 cache aux control reg */
        mov r0, #0xC0 | 0x4		/* tag RAM, data RAM */
        orr r0, r0, #(1 << 24) | (1 << 23) | (1 << 22)	/* disable write allocate (delay, combine, allocate) */
	mcr 15, 1, r0, c9, c0, 2
.endm /* init_l2cc */

/* AIPS setup - Only setup MPROTx registers.
 * The PACR default values are good.*/
.macro init_aips
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	ldr r0, =AIPS1_BASE_ADDR
	ldr r1, =0x77777777
	str r1, [r0, #0x0]
	str r1, [r0, #0x4]
	ldr r0, =AIPS2_BASE_ADDR
	str r1, [r0, #0x0]
	str r1, [r0, #0x4]
.endm /* init_aips */

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
	mov r0, #\dp_op
	str r0, [\r_pll, #PLL_DP_OP]
	str r0, [\r_pll, #PLL_DP_HFS_OP]

	mov r0, #\dp_mfd
	str r0, [\r_pll, #PLL_DP_MFD]
	str r0, [\r_pll, #PLL_DP_HFS_MFD]

	mov r0, #\dp_mfn
	str r0, [\r_pll, #PLL_DP_MFN]
	str r0, [\r_pll, #PLL_DP_HFS_MFN]
.endm

.macro pll_op r_pll, freq
	pll_op_mfd_mfn \r_pll, _DP_OP_\freq, _DP_MFD_\freq, _DP_MFN_\freq
.endm

.macro start_pll r_pll, dp_ctl
	ldr r0, =\dp_ctl
	str r0, [\r_pll, #PLL_DP_CTL] /* Set DPLL ON (set UPEN bit) */
1:	ldr r0, [\r_pll, #PLL_DP_CTL]
	tst r0, #0x1
	beq 1b
.endm

.macro start_pllf r_pll, freq
	start_pll \r_pll, _DP_CTL_\freq
.endm

.macro divisor_change_wait r_clk
	/* make sure divider effective */
1:	ldr r0, [\r_clk, #CLKCTL_CDHIPR]
	cmp r0, #0x0
	bne 1b
.endm

#define rPLL1 r4
#define rPLL2 r5
#define rPLL3 r6
#define rClk  r7

.macro init_clock
	ldr	rClk, =CCM_BASE_ADDR

        /* Disable IPU handshake */
//	mov	r0, #(1<<21)
//	str	r0, [rClk, #CLKCTL_CCDR]
//	print_debug_ch '1'

	ldr	rPLL1, =PLL1_BASE_ADDR
	ldr	rPLL2, =PLL2_BASE_ADDR
	ldr	rPLL3, =PLL3_BASE_ADDR

//disable auto restart
	mov	r0, #0
	str     r0, [rPLL1, #PLL_DP_CONFIG]
	str     r0, [rPLL2, #PLL_DP_CONFIG]
	str     r0, [rPLL3, #PLL_DP_CONFIG]

	/* Switch ARM to step clock */
	mov r1, #0x4
	str r1, [rClk, #CLKCTL_CCSR]

	pll_op	rPLL1, CONFIG_SYS_PLL1_FREQ
	start_pllf rPLL1,CONFIG_SYS_PLL1_FREQ

	pll_op	rPLL3, 400
	start_pllf rPLL3, 400

        /* Switch peripheral to PLL3 */
        ldr r1, =0x0001515c		//periph_apm_sel - 01(pll3_sw_clk)
        str r1, [rClk, #CLKCTL_CBCMR]
        ldr r1, CCM_VAL_0x02888945
        orr r1, r1, #(1 << 16)
	ldr r2, [rClk, #CLKCTL_CBCDR]
        str r1, [rClk, #CLKCTL_CBCDR]
        /* make sure change is effective */
1:      ldr r1, [rClk, #CLKCTL_CDHIPR]
        cmp r1, #0x0
        bne 1b

#if 0
	pll_op	rPLL2, CONFIG_SYS_PLL2_FREQ
	start_pllf rPLL2, CONFIG_SYS_PLL2_FREQ

	/* Switch peripheral to PLL2 */
	ldr r1, CCM_VAL_0x00808145
	orr r1, r1, #(CONFIG_SYS_AHB_PODF << 10)
	orr r1, r1, #(CONFIG_SYS_AXIA_PODF << 16)
	orr r1, r1, #(CONFIG_SYS_AXIB_PODF << 19)
	str r1, [rClk, #CLKCTL_CBCDR]
#else
	ldr r1, CCM_VAL_0x00808145
	orr r1, r1, #(CONFIG_SYS_AHB_PODF << 10)
	orr r1, r1, #(CONFIG_SYS_AXIB_PODF << 19)
//	orr r1, r1, #(CONFIG_SYS_AXIA_PODF << 16)	//keep same AXI_A_PODF
	and r2, r2, #(7 << 16)				//keep same AXI_A_PODF
	orr r1, r1, r2
	str r1, [rClk, #CLKCTL_CBCDR]
#endif
	ldr r1, =0x00016154		//periph_apm_sel - 2(lp_apm)
	str r1, [rClk, #CLKCTL_CBCMR]

	/* make sure change is effective */
1:      ldr r1, [rClk, #CLKCTL_CDHIPR]
	cmp r1, #0x0
	bne 1b

	pll_op	rPLL3, 216
	start_pllf rPLL3, 216

	/* Set the platform clock dividers */
	ldr r0, PLATFORM_BASE_ADDR_W
	ldr r1, PLATFORM_CLOCK_DIV_W
	str r1, [r0, #PLATFORM_ICGC]

	mov r1, #1
	str r1, [rClk, #CLKCTL_CACRR]

	/* Switch ARM back to PLL 1. */
	mov r1, #0x0
	str r1, [rClk, #CLKCTL_CCSR]

	ldr r1, [rClk, #CLKCTL_CSCDR1]
	orr r1, r1, #0x3f
	eor r1, r1, #0x3f
	orr r1, r1, #0x21
	str r1, [rClk, #CLKCTL_CSCDR1]

	/* Restore the default values in the Gate registers */
	ldr r1, =0xFFFFFFFF
	str r1, [rClk, #CLKCTL_CCGR0]
	str r1, [rClk, #CLKCTL_CCGR1]
	str r1, [rClk, #CLKCTL_CCGR2]
	str r1, [rClk, #CLKCTL_CCGR3]
	str r1, [rClk, #CLKCTL_CCGR4]
	str r1, [rClk, #CLKCTL_CCGR5]
	str r1, [rClk, #CLKCTL_CCGR6]
	str r1, [rClk, #CLKCTL_CCGR7]

        mov r1, #0x00000
        str r1, [rClk, #CLKCTL_CCDR]

        /* for cko - for ARM div by 8 */
        mov r1, #0x000A0000
        add r1, r1, #0x00000F0
        str r1, [rClk, #CLKCTL_CCOSR]
.endm

.section ".text.init", "x"

	.align	5
.globl lowlevel_init
lowlevel_init:

#ifdef ENABLE_IMPRECISE_ABORT
        mrs r1, spsr            /* save old spsr */
        mrs r0, cpsr            /* read out the cpsr */
	bic r0, r0, #0x100      /* clear the A bit */
	msr spsr, r0            /* update spsr */
	add lr, pc, #0x8        /* update lr */
        movs pc, lr             /* update cpsr */
        nop
        nop
        nop
	nop
	msr spsr, r1            /* restore old spsr */
#endif

	/* ARM errata ID #468414 */
	mrc 15, 0, r1, c1, c0, 1
	orr r1, r1, #(1 << 5)    /* enable L1NEON bit */
	mcr 15, 0, r1, c1, c0, 1

	init_l2cc

	init_aips

	init_clock

	mov pc, lr

/* Board level setting value */
CCM_BASE_ADDR_W:        .word CCM_BASE_ADDR
CCM_VAL_0x00808145:     .word 0x00808145
CCM_VAL_0x02888945:     .word 0x02888945
PLATFORM_BASE_ADDR_W:   .word ARM_BASE_ADDR
PLATFORM_CLOCK_DIV_W:   .word 0x00000124
