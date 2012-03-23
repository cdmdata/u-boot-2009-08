#define GPIO_DR		0x00
#define GPIO_DIR	0x04
#define GPIO_PSR	0x08

#define WDOG_WCR  0x00
#define WDOG_WSR  0x02
#define WDOG_WRSR 0x04
#define WDOG_WICR 0x06
#define WDOG_WMCR 0x08

#define AIPSREG_MPR0_OFFSET	0x0000
#define AIPSREG_MPR1_OFFSET	0x0004
#define AIPSREG_OPACR0_OFFSET	0x0040
#define AIPSREG_OPACR1_OFFSET	0x0044
#define AIPSREG_OPACR2_OFFSET	0x0048
#define AIPSREG_OPACR3_OFFSET	0x004C
#define AIPSREG_OPACR4_OFFSET	0x0050

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

#define ERROR_NO_HEADER		-9
#define ERROR_MEMORY_TEST	-10
#define ERROR_GP12_LOW		-11
#define ERROR_MAX		-12

#ifdef ASM
	.equiv	URXD,	0x0000
	.equiv	UTXD,	0x0040
	.equiv	UCR1,	0x0080
	.equiv	UCR2,	0x0084
	.equiv	UCR3,	0x0088
	.equiv	UCR4,	0x008c
	.equiv	UFCR,	0x0090
	.equiv	USR1,	0x0094
	.equiv	USR2,	0x0098
	.equiv	UESC,	0x009c
	.equiv	UTIM,	0x00a0
	.equiv	UBIR,	0x00a4
	.equiv	UBMR,	0x00a8
	.equiv	UBRC,	0x00ac
	.equiv	ONEMS,	0x00b0
	.equiv	UTS,	0x00b4

	.macro	debug_ch ch
	stmdb	sp!,{r0,r1,lr}
	mov	r0, #\ch
	bl	TransmitX
	ldmia	sp!,{r0,r1,lr}
	.endm

	.macro	debug_hex reg
	stmdb	sp!,{r0,r1,r2,r3,lr}
	str	\reg, [sp, #-4]!
	mov	r0, #' '
	bl	TransmitX
	ldr	r0,[sp]
	bl	print_hex
	add	sp, sp, #4
	ldmia	sp!,{r0,r1,r2,r3,lr}
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

	.equiv	CCM_CCDR, 0x04
	.equiv	CCM_CBCDR, 0x14
	.equiv	CCM_CSCMR1, 0x1c
	.equiv	CCM_CSCDR1, 0x24
	.equiv	CCM_CLPCR, 0x054
	.equiv	CCM_CGPR, 0x64
	.equiv	CCM_CCGR1, 0x6c
	.equiv	CCM_CCGR2, 0x70
	.equiv	CCM_CCGR3, 0x74
	.equiv	CCM_CCGR4, 0x78
	.equiv	CCM_CCGR5, 0x7c
	.equiv	CCM_CCGR6, 0x80

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

#define IPU_CONF		0x000
#define IPU_DISP_GEN		0x0C4
#define IPU_CH_DB_MODE_SEL0	0x150
#define IPU_CH_DB_MODE_SEL1	0x154
#define IPU_CH_TRB_MODE_SEL0	0x178
#define IPU_CH_TRB_MODE_SEL1	0x17c

#define IDMAC_CONF		0x000
#define IDMAC_CH_EN_1		0x004
#define IDMAC_CH_EN_2		0x008
#define IDMAC_WM_EN_1		0x01C
#define IDMAC_WM_EN_2		0x020


.macro init_disable_ipu ipu_cm_reg_base, ipu_idmac_base
	mov	r1,#\ipu_cm_reg_base
	ldr	r0,[r1, #IPU_DISP_GEN]
	bic	r0, r0, #(3 << 24)	//clear DI0/DI1 counter release
	str	r0,[r1, #IPU_DISP_GEN]

	mov	r0, #0
	str	r0,[r1, #IPU_CONF]	//disable DI0/DI1
#if 0
	add	r2, r1, #\ipu_idmac_base - \ipu_cm_reg_base
	str	r0, [r2, #IDMAC_CH_EN_1]
	str	r0, [r2, #IDMAC_CH_EN_2]
	str	r0, [r2, #IDMAC_WM_EN_1]
	str	r0, [r2, #IDMAC_WM_EN_2]

	str	r0, [r1, #IPU_CH_DB_MODE_SEL0]
	str	r0, [r1, #IPU_CH_DB_MODE_SEL1]
	str	r0, [r1, #IPU_CH_TRB_MODE_SEL0]
	str	r0, [r1, #IPU_CH_TRB_MODE_SEL1]
#endif
.endm
#endif
