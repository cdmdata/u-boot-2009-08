#define PLL1_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00080000)
#define PLL2_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00084000)
#define PLL3_BASE_ADDR		(AIPS2_BASE_ADDR + 0x00088000)

//MRS defines
#define MRS_DLL_RESET (1 << 8)

//EMRS(1) defines
#define EMRS1_DRIVE_STRENGTH_FULL	(0 << 1)	//A1-0 full strength
#define EMRS1_DRIVE_STRENGTH_REDUCED	(1 << 1)	//A1-1 reduced strength
#define EMRS1_ODT_TERM_OHM_50		((1 << 6) | (1 << 2))	//A2,A6
#define EMRS1_ODT_TERM_OHM_75		(1 << 2)
#define EMRS1_ODT_TERM_OHM_150		(1 << 6)
#define EMRS1_ODT_TERM_DISABLED		(0)

#define EMRS1_DQS_SINGLE_BIT	10
/*
 * pll2/10 = 66.5 MHz
 * 66500000 /16 /2 *16 /289 = 115051 baud
 * pll3/10 = 21.6 MHz
 * 21600000 /16 /4 *3413 /10000 = 115188 baud
 */
#define UART_PREDIV	4
#define UART_MULT_VAL	3413	//3413 - 1 = 0x0d54
#define UART_DIV_VAL	10000	//10000 - 1 = 0x270f

#ifdef ASM
	.macro init_l1cc
	.endm

	/*
	 * Disable L2 cache
	 */
	.macro init_l2cc
	mrc	p15, 1, r0, c9, c0, 2		// Read L2 aux control reg
	orr	r0, r0, #(1 << 25)		// Disable write combining (HW workaround)
	mcr	p15, 1, r0, c9, c0, 2		// Update L2 aux control reg
	.endm


	.equiv	I_STAT,		0x0000
	.equiv	I_STATM,	0x0004
	.equiv	I_ERR,		0x0008
	.equiv	I_EMASK,	0x000c
	.equiv	I_FCTL,		0x0010
	.equiv	I_UA,		0x0014		//top 6 bits of fuse bit #, (3 bits bank #, 11 bits within bank)
	.equiv	I_LA,		0x0018		//bottom 8 bits of 14 bit fuse #
	.equiv	I_SDAT,		0x001c
	.equiv	I_PREV,		0x0020
	.equiv	I_SREV,		0x0024
	.equiv	I_PREG_P,	0x0028

	.equiv	I_UID7,		0x0820
	.equiv	I_UID6,		0x0824
	.equiv	I_UID5,		0x0828
	.equiv	I_UID4,		0x082c
	.equiv	I_UID3,		0x0830
	.equiv	I_UID2,		0x0834
	.equiv	I_UID1,		0x0838
	.equiv	I_UID0,		0x083c

	.equiv	I_FBAC1,	0x0c00		//bank 1, FUSE bank Access Protection register, fuse 256-263
	.equiv	I_SJC_RESP6,	0x0c08
	.equiv	I_SJC_RESP5,	0x0c0c
	.equiv	I_SJC_RESP4,	0x0c10
	.equiv	I_SJC_RESP3,	0x0c14
	.equiv	I_SJC_RESP2,	0x0c18
	.equiv	I_SJC_RESP1,	0x0c1c
	.equiv	I_SJC_RESP0,	0x0c20
	.equiv	I_MAC_ADDR5,	0x0c24
	.equiv	I_MAC_ADDR4,	0x0c28
	.equiv	I_MAC_ADDR3,	0x0c2c
	.equiv	I_MAC_ADDR2,	0x0c30
	.equiv	I_MAC_ADDR1,	0x0c34
	.equiv	I_MAC_ADDR0,	0x0c38
//
//(SRC) System reset controller
	.equiv	SRC_SCR,	0x000
	.equiv	SRC_SBMR,	0x004
	.equiv	SRC_SRSR,	0x008
////
	.macro fentry name,offset
	.asciz	"\name"
	.byte	 ((\offset) & 0xff), ((\offset) >> 8)
	.endm

	.macro fBurn name,offset,mask
	.asciz	"\name"
	.byte	 ((\offset) & 0xff), ((\offset) >> 8), (\mask)
	.endm

	.macro	fuse_check_rev	rIIM
	ldr	r0, [\rIIM, #I_PREV]
	cmp	r0, #PREV_EXPECTED
	.endm

	.macro fuse_load_mask rTable, rMask, rTmp
	ldrb	\rMask,[\rTable], #1
	.endm

#define PROGRAM_START 1
#define ESNS_N 8
#define PGR_LENGTH_MASK 0x70

//Out: r0 - fuse value, r1-r3 trashed
	.macro	fuse_sense, rIIM, rFuse
	strb	\rFuse,[\rIIM, #I_LA]
	mov	r0, \rFuse,LSR #8
	strb	r0, [\rIIM, #I_UA]

	mov	r0, #3
	strb	r0, [\rIIM, #I_STAT]
	ldrb	r0, [\rIIM, #I_FCTL]
	and	r0, r0, #PGR_LENGTH_MASK
	orr	r0, r0, #ESNS_N
	strb	r0, [\rIIM, #I_FCTL]
91:	ldrb	r0, [\rIIM, #I_STAT]
	tst	r0, #1
	beq	91b
	ldrb	r0, [\rIIM, #I_SDAT]
	bl	print_hex_byte
	bl	TransmitCRLF
	ldrb	r0, [\rIIM, #I_SDAT]
	.endm

//In: rFuse >= r4
//Out: r0 has error code, r0-r3 trashed
	.macro	fuse_burn_single rIIM, rCCM, rFuse
	ldr	r1, [\rCCM, #CCM_CGPR]
	orr	r1, r1, #0x10		//turn on programming supply
	str	r1, [\rCCM, #CCM_CGPR]

	mov	r1, #3
	strb	r1, [\rIIM, #I_STAT]
	mov	r1, #0xff
	strb	r1, [\rIIM, #I_ERR]

	strb	\rFuse, [\rIIM, #I_LA]
	mov	r0, \rFuse, LSR #8
	strb	r0, [\rIIM, #I_UA]

	ldrb	r0, [\rIIM, #I_FCTL]
	and	r0, r0, #PGR_LENGTH_MASK
	orr	r0, r0, #PROGRAM_START
#if 1
	mov	r1, #0xaa
	strb	r1, [\rIIM, #I_PREG_P]

	strb	r0, [\rIIM, #I_FCTL]
93:	ldrb	r0, [\rIIM, #I_STAT]
	tst	r0, #2		//wait for program done
	beq	93b
#endif
	mov	r0, #0x55
	strb	r0, [\rIIM, #I_PREG_P]

	ldr	r1, [\rCCM, #CCM_CGPR]
	bic	r1, r1, #0x10		//turn off programming supply
	str	r1, [\rCCM, #CCM_CGPR]

	ldrb	r0,[\rIIM, #I_ERR]
	.endm

//Out: rTmp - err code if z-0
	.macro fuse_burn rIIM, rCCM, rMask, rOffset, rTmp
	b	91f
90:
	mov	r1, #1
	bic	\rMask, r1, LSL r0
	add	r0, r0, \rOffset
//r0 - fuse #
//Out: r0 - r5, trashed, r6 - error code
	mov	\rTmp, r0
	adr	r0, blowing_str
	bl	transmit_string
	mov	r0, \rTmp
	bl	print_hex_byte
	bl	TransmitCRLF

	fuse_burn_single \rIIM, \rCCM, \rTmp
	movs	\rTmp, r0
	bne	92f
91:
	clz	r0, \rMask
	rsbs	r0, r0, #31
	bcs	90b
	movs	r0, #0
92:
	.endm

	.equiv	M4IF_CNTL_REG0, 0x08c

	.macro ecspi_clk_setup, ccm_base
	BigMov	r1,\ccm_base
	ldr	r0,[r1, #CCM_CCGR4]		//index 9 & 10 are ECSPI_IPG, ECSPI_PERCLK
	orr	r0,r0,#0xf<<(9*2)
	str	r0,[r1, #CCM_CCGR4]
	.endm

	.macro i2c1_clk_setup, ccm_base
	BigMov	r1,\ccm_base
	ldr	r0,[r1, #CCM_CCGR1]		//index 9 is i2c1_ser_clk_enable
	orr	r0,r0,#0x3<<(9*2)
	str	r0,[r1, #CCM_CCGR1]
	.endm

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

	.equiv	_DP_CTL_RESTART, 0x10
	.equiv	_DP_CTL_BRMO,	0x02
	.equiv	_DP_CTL_DIV1,	0x1000
	.equiv	_DP_CTL_DIV2,	0x0

//(41/48 + 8) * 96 / 1 = 850
	.equiv	_DP_OP_850,	MAKE_OP(8, 1)
	.equiv	_DP_MFN_850,	41
	.equiv	_DP_MFD_850,	(48 - 1)
	.equiv	_DP_CTL_850,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART | _DP_CTL_BRMO)

//(1/3 + 8) * 96 / 1 = 800
	.equiv	_DP_OP_800,	MAKE_OP(8, 1)
	.equiv	_DP_MFN_800,	1
	.equiv	_DP_MFD_800,	(3 - 1)
	.equiv	_DP_CTL_800,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART)

//(7/24 + 7) * 96 / 1 = 700
	.equiv	_DP_OP_700,	MAKE_OP(7, 1)
	.equiv	_DP_MFN_700,	7
	.equiv	_DP_MFD_700,	(24 - 1)
	.equiv	_DP_CTL_700,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART | _DP_CTL_BRMO)

//(89/96 + 6) * 96 / 1 = 665
	.equiv	_DP_OP_665,	MAKE_OP(6, 1)
	.equiv	_DP_MFN_665,	89
	.equiv	_DP_MFD_665,	(96 - 1)
	.equiv	_DP_CTL_665,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART | _DP_CTL_BRMO)

//(1/4 + 6) * 96 / 1 = 600
	.equiv	_DP_OP_600,	MAKE_OP(6, 1)
	.equiv	_DP_MFN_600,	1
	.equiv	_DP_MFD_600,	(4 - 1)
	.equiv	_DP_CTL_600,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART | _DP_CTL_BRMO)

//(13/24 + 5) * 96 / 1 = 532
	.equiv	_DP_OP_532,	MAKE_OP(5, 1)
	.equiv	_DP_MFN_532,	13
	.equiv	_DP_MFD_532,	(24 - 1)
	.equiv	_DP_CTL_532,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART | _DP_CTL_BRMO)

//(1/3 + 8) * 96 / 2 = 400
	.equiv	_DP_OP_400,	MAKE_OP(8, 2)
	.equiv	_DP_MFN_400,	1
	.equiv	_DP_MFD_400,	(3 - 1)
	.equiv	_DP_CTL_400,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART)

//(0/2 + 7) * 96 / 2 = 336
	.equiv	_DP_OP_336,	MAKE_OP(7, 2)
	.equiv	_DP_MFN_336,	0
	.equiv	_DP_MFD_336,	(2 - 1)
	.equiv	_DP_CTL_336,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART)

//(15/16 + 6) * 96 / 2 = 333
	.equiv	_DP_OP_333,	MAKE_OP(6, 2)
	.equiv	_DP_MFN_333,	15
	.equiv	_DP_MFD_333,	(16 - 1)
	.equiv	_DP_CTL_333,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART | _DP_CTL_BRMO)

//(7/8 + 6) * 96 / 2 = 330
	.equiv	_DP_OP_330,	MAKE_OP(6, 2)
	.equiv	_DP_MFN_330,	7
	.equiv	_DP_MFD_330,	(8 - 1)
	.equiv	_DP_CTL_330,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART | _DP_CTL_BRMO)

//(0/2 + 10) * 96 / 3 = 320
	.equiv	_DP_OP_320,	MAKE_OP(10, 3)
	.equiv	_DP_MFN_320,	0
	.equiv	_DP_MFD_320,	(2 - 1)
	.equiv	_DP_CTL_320,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART)

//(1/4 + 6) * 96 / 2 = 300
	.equiv	_DP_OP_300,	MAKE_OP(6, 2)
	.equiv	_DP_MFN_300,	1
	.equiv	_DP_MFD_300,	(4 - 1)
	.equiv	_DP_CTL_300,	(0x0220 | _DP_CTL_DIV1 | _DP_CTL_RESTART)

//(0/2 + 9) * 96 / 2 = 432
// 432 / 2 = 216
	.equiv	_DP_OP_216,	MAKE_OP(9, 2)
	.equiv	_DP_MFN_216,	0
	.equiv	_DP_MFD_216,	(2 - 1)
	.equiv	_DP_CTL_216,	(0x0220 | _DP_CTL_DIV2 | _DP_CTL_RESTART)

//(1/3 + 8) * 96 / 2 = 400
	.equiv	_DP_OP_200,	MAKE_OP(8, 2)
	.equiv	_DP_MFN_200,	1
	.equiv	_DP_MFD_200,	(3 - 1)
	.equiv	_DP_CTL_200,	(0x0220 | _DP_CTL_DIV2 | _DP_CTL_RESTART)


#define PLL_DP_CTL      0x00
#define PLL_DP_CONFIG   0x04
#define PLL_DP_OP       0x08
#define PLL_DP_MFD      0x0C
#define PLL_DP_MFN      0x10
#define PLL_DP_MFNMINUS 0x14
#define PLL_DP_MFNPLUS  0x18
#define PLL_DP_HFS_OP   0x1C
#define PLL_DP_HFS_MFD  0x20
#define PLL_DP_HFS_MFN  0x24
#define PLL_DP_TOGC     0x28
#define PLL_DP_DESTAT   0x2C
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

	.macro pll_freq	r_pll, freq
	pll_op_mfd_mfn	\r_pll, _DP_OP_\freq, _DP_MFD_\freq, _DP_MFN_\freq
	.endm

	.macro start_pll r_pll, dp_ctl
	ldr r0, =\dp_ctl
	str r0, [\r_pll, #PLL_DP_CTL] /* Set DPLL ON (set UPEN bit) */
1:	ldr r0, [\r_pll, #PLL_DP_CTL]
	tst r0, #0x1
	beq 1b
	.endm

	.macro start_pll_freq r_pll, freq
	start_pll	\r_pll, _DP_CTL_\freq
	.endm

	.macro divisor_change_wait r_clk
	/* make sure divider effective */
1:	ldr r0, [\r_clk, #CLKCTL_CDHIPR]
	cmp r0, #0x0
	bne 1b
	.endm

	.macro choose_data	function
	adr	r3, 99f
	b	Basic_Init1
99:	iod_\function
	.endm

	.macro	create_function function
	.section .text.\function,"x"
\function:
	choose_data	\function
	.endm
#endif
