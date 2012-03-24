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

#define ERROR_NO_HEADER		-9
#define ERROR_MEMORY_TEST	-10
#define ERROR_GP12_LOW		-11
#define ERROR_MAX		-12

#ifdef ASM
//
// ARM constants
//
	.equiv	ARM_CPSR_PRECISE,	(1 << 8)
	.equiv	ARM_CPSR_IRQDISABLE,	(1 << 7)
	.equiv	ARM_CPSR_FIQDISABLE,	(1 << 6)
	.equiv	ARM_CPSR_MODE_SVC,	0x13

	.equiv	ARM_CTRL_MMU,		(1 << 0)
	.equiv  ARM_CTRL_ALIGNMENT,	(1 << 1)
	.equiv	ARM_CTRL_DCACHE,	(1 << 2)
	.equiv	ARM_CTRL_FLOW,		(1 << 11)
	.equiv	ARM_CTRL_ICACHE,	(1 << 12)
	.equiv	ARM_CTRL_VECTORS,	(1 << 13)

	.equiv	ARM_CACR_FULL,		0x3

	.equiv	ARM_AUXCR_L2EN,		(1 << 1)

// VFP uses coproc 10 for single-precision instructions
	.equiv	ARM_VFP_SP_COP,		10
	.equiv	ARM_VFP_SP_ACCESS,	(ARM_CACR_FULL << (ARM_VFP_SP_COP*2))

// VFP uses coproc 11 for double-precision instructions
	.equiv	ARM_VFP_DP_COP,		11
	.equiv	ARM_VFP_DP_ACCESS,	(ARM_CACR_FULL << (ARM_VFP_DP_COP*2))

// Configure coprocessor access control
	.equiv	ARM_CACR_CONFIG,	(ARM_VFP_SP_ACCESS | ARM_VFP_DP_ACCESS)


	.macro hw_init aips1_base, aips2_base
	BigMov	r0, ARM_CPSR_MODE_SVC|ARM_CPSR_IRQDISABLE|ARM_CPSR_FIQDISABLE|ARM_CPSR_PRECISE
	msr	cpsr_xc, r0			// update CPSR control bits

	mrc	p15, 1, r0, c9, c0, 2		// Read L2 aux control reg
	orr	r0, r0, #(1 << 25)		// Disable write combining (HW workaround)
	mcr	p15, 1, r0, c9, c0, 2		// Update L2 aux control reg

// Disable MMU and both the instruction and data caches
	mrc	p15, 0, r0, c1, c0, 0		// r0 = system control reg
// disable ICache, disable DCache, disable MMU, set vector base to 0x00000000
	BigBic2	r0, ARM_CTRL_ICACHE | ARM_CTRL_DCACHE | ARM_CTRL_MMU | ARM_CTRL_VECTORS
	orr	 r0, r0, #ARM_CTRL_FLOW		// program flow prediction enabled
	mcr	 p15, 0, r0, c1, c0, 0		// update system control reg

// Configure ARM coprocessor access control register
	BigMov	 r0, ARM_CACR_CONFIG		// r0 = CACR configuration
	mcr	 p15, 0, r0, c1, c0, 2		// update CACR

	.word	 0xF57FF06F			// ISB

// Configure AHB<->IP-bus interface (AIPS) registers
	BigMov	 r1, \aips1_base
	add	 r2, r1, #\aips2_base - (\aips1_base)

// Except for AIPS regs, configure all peripherals as follows:
// unbuffered writes (MBW=0), disable supervisor protect (SP=0)
// disable write protect (WP=0), disable trusted protect (TP=0)
	mov	 r0, #0
	str	 r0, [r1, #AIPSREG_OPACR0_OFFSET]
	str	 r0, [r1, #AIPSREG_OPACR1_OFFSET]
	str	 r0, [r1, #AIPSREG_OPACR2_OFFSET]
	str	 r0, [r1, #AIPSREG_OPACR3_OFFSET]
	str	 r0, [r1, #AIPSREG_OPACR4_OFFSET]
	str	 r0, [r2, #AIPSREG_OPACR0_OFFSET]
	str	 r0, [r2, #AIPSREG_OPACR1_OFFSET]
	str	 r0, [r2, #AIPSREG_OPACR2_OFFSET]
	str	 r0, [r2, #AIPSREG_OPACR3_OFFSET]
	str	 r0, [r2, #AIPSREG_OPACR4_OFFSET]

// Set all MPRx to be non-bufferable, trusted for R/W,
// not forced to user-mode.
	ldr	 r0, =(0x77777777)
	str	 r0, [r1, #AIPSREG_MPR0_OFFSET]
	str	 r0, [r1, #AIPSREG_MPR1_OFFSET]
	str	 r0, [r2, #AIPSREG_MPR0_OFFSET]
	str	 r0, [r2, #AIPSREG_MPR1_OFFSET]
	.endm

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
