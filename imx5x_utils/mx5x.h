#define GPIO_DR 0
#define GPIO_DIR 4
#define GPIO_PSR 8

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

////
	.macro fentry name,offset
	.asciz	"\name"
	.byte	 ((\offset) & 0xff), ((\offset) >> 8)
	.endm

	.macro fBurn name,offset,mask
	.asciz	"\name"
	.byte	 ((\offset) & 0xff), ((\offset) >> 8), (\mask)
	.endm

	.equiv	CCM_CBCDR, 0x14
	.equiv	CCM_CSCMR1, 0x1c
	.equiv	CCM_CSCDR1, 0x24
	.equiv	CCM_CGPR, 0x64
	.equiv	CCM_CCGR1, 0x6c
	.equiv	CCM_CCGR4, 0x78

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


#endif
