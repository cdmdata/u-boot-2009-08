	.global assem_start
//256 meg is fine if we're just writing sdmmc to the SDCard 
#define USE_CSD1	//512MB board uses CS0 and CS1, we will disable CS1 if ram doesn't appear to work

//HAB API Jump Table Addresses
#define HAB_CSF_CHECK		0x40	//Verify signature of memory ranges
//hab_result hab_csf_check(UINT8 csf_count, UINT32 *csf_list);
#define HAB_ASSERT_VERIFICATION	0x44	//Check that memory range is secured
//hab_result hab_asser_verification(UINT8 *block_start, UINT32 block_length);
#define PU_IROM_BOOT_DECISION	0x50	//Serial bootloader
//void pu_irom_boot_decision(void);

	.equiv	UART1_BASE, 0x73fbc000
	.equiv	UART2_BASE, 0x73fc0000
	.equiv	UART3_BASE, 0x7000c000
	.equiv	UART_BASE, UART1_BASE
	.equiv	UCR1,	0x0080

	.equiv	CCM_BASE,	0x73fd4000
	.equiv	ESD_BASE,	0x83fd9000
	.equiv	ESD_CTL0, 0x000
	.equiv	ESD_CFG0, 0x004
	.equiv	ESD_CTL1, 0x008
	.equiv	ESD_CFG1, 0x00c
	.equiv	ESD_MISC, 0x010
	.equiv	ESD_SCR,  0x014

#define rFNAME r8
#define rFOFF r9
#define rUART r10
#define rIIM r12

#include <asm/BigMacro.h>
#include <asm/char.inc>

//
// ARM constants
//
	.equiv	ARM_CPSR_PRECISE,	(1 << 8)
	.equiv	ARM_CPSR_IRQDISABLE,	(1 << 7)
	.equiv	ARM_CPSR_FIQDISABLE,	(1 << 6)
	.equiv	ARM_CPSR_MODE_SVC,	0x13

	.equiv	ARM_CTRL_MMU,		(1 << 0)
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

// AIPS Constants
	.equiv	CSP_BASE_REG_PA_AIPS1,	0x73F00000
	.equiv	CSP_BASE_REG_PA_AIPS2,	0x83F00000

	.equiv	AIPSREG_MPR0_OFFSET,	0x0000
	.equiv	AIPSREG_MPR1_OFFSET,	0x0004
	.equiv	AIPSREG_OPACR0_OFFSET,	0x0040
	.equiv	AIPSREG_OPACR1_OFFSET,	0x0044
	.equiv	AIPSREG_OPACR2_OFFSET,	0x0048
	.equiv	AIPSREG_OPACR3_OFFSET,	0x004C
	.equiv	AIPSREG_OPACR4_OFFSET,	0x0050

	.macro hw_init
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
	BigMov	 r1, CSP_BASE_REG_PA_AIPS1
	BigMov	 r2, CSP_BASE_REG_PA_AIPS2

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

	.section .header_mem,"x"

//offset 0x400 for SD/MMC
header:
		.word	assem_start	//0x00 app_start_addr
		.word	0xb1		//0x04 app_barker
		.word	0x00		//0x08 csf_ptr
		.word	dcd_ptr		//0x0c dcd_ptr_ptr
		.word	0		//0x10 srk_ptr
dcd_ptr:	.word	dcd		//0x14 dcd_ptr
		.word	image_base	//0x18 app_dest_ptr


	.section .dcd_mem,"x"
dcd:		.word	0xb17219e9	//0x1c
		.word	2f-1f		//0x20
1:
2:
	.word	program_length	//0x24 length

	.equiv	CCM_CSCMR1, 0x1c
	.equiv	CCM_CSCDR1, 0x24
	.equiv	CCM_CCGR1, 0x6c

	.equiv	IOMUXC_BASE, 0x73fa8000
	.equiv	SW_MUX_CTL_PAD_UART1_RXD, 0x228
	.equiv	SW_MUX_CTL_PAD_UART1_TXD, 0x22c
	.equiv	SW_PAD_CTL_UART1_RXD, 0x618
	.equiv	SW_PAD_CTL_UART1_TXD, 0x61c

	.section .text
assem_start:
	BigMov	sp,0x1fffffb8
	hw_init
	bl	Basic_Init_IOMux
//1:	b	1b
	BigMov	rUART,UART_BASE	//0x28

	BigMov	r6,IOMUXC_BASE
	mov	r0,#0
	str	r0,[r6, #SW_MUX_CTL_PAD_UART1_RXD]
	str	r0,[r6, #SW_MUX_CTL_PAD_UART1_TXD]
	BigMov	r0, 0x1c5
	str	r0,[r6, #SW_PAD_CTL_UART1_RXD]
	str	r0,[r6, #SW_PAD_CTL_UART1_RXD]

	BigMov	r6,CCM_BASE
	ldr	r0,[r6, #CCM_CCGR1]		//index 3 & 4 is UART1
	bic	r0,r0,#0xf<<(3*2)
	str	r0,[r6, #CCM_CCGR1]
	
	ldr	r0,[r6, #CCM_CSCMR1]
	bic	r0,r0,#3<<24
	orr	r0,r0,#2<<24			//pll3
	str	r0,[r6, #CCM_CSCMR1]

	ldr	r0,[r6, #CCM_CSCDR1]
	bic	r0,r0,#0x3f
	orr	r0,r0,#0x21			// podf /2, pred /5
	str	r0,[r6, #CCM_CSCDR1]

	ldr	r0,[r6, #CCM_CCGR1]		//index 3 & 4 is UART1
	orr	r0,r0,#0xf<<(3*2)
	str	r0,[r6, #CCM_CCGR1]
////////	
	adr	r6,init_tab
	adr	r7,init_tab_end
	add	r1,rUART, #UCR1
1:	ldrh	r0,[r6],#2
	and	r2,r1,#0xff
	cmp	r2,#0xac	//UBRC
	strne	r0,[r1],#4
	addeq	r1,r1,#4
	cmp	r6,r7
	bne	1b
	b	uart_init_done
init_tab:
	.short	0x0001		//UCR1 (0x80)
	.short	0x4027		//UCR2 (0x84)
	.short	0x0704		//UCR3 (0x88)
	.short	0x8000		//UCR4 (0x8c)
	.short	0x0901		//UFCR (0x90)
	.short	0x2050		//USR1 (0x94)
	.short	0x5088		//USR2 (0x98)
	.short	0x002b		//UESC (0x9c)
	.short	0x0000		//UTIM (0xa0)
	.short	0x0d54		//UBIR (0xa4)
	.short	0x270f		//UBMR (0xa8)
	.short	0x0004		//UBRC (0xac), read only
	.short	0x0000		//ONEMS (0xb0)
	.short	0x0060		//UTS (0xb4)
init_tab_end:
	.align	2


uart_init_done:
	mov	r0,#0xc0000	//0x40000 gives garbage, 0x6000 garbage somtimes
1:	subs	r0,r0,#1	//delay for UART to power up, avoiding garbage characters transmitting
	bne	1b	
	bl	Basic_Init_ESD
	mov	r0,#0x10000
2:	subs	r0,r0,#1
	bne	2b	
#ifdef USE_CSD1
//	mov	r0, #'A'
//	bl	TransmitX

//See if DDR CS1 is available
	mov	r5, #0xa0000000
#define CMP_ITEM_COUNT 8
	mov	r6, #CMP_ITEM_COUNT
3:
//	mov	r0, #'B'
//	bl	TransmitX
	str	r5, [r5]
	add	r5, r5, #4
//	mov	r0, #'C'
//	bl	TransmitX
	subs	r6, r6, #1
	bne	3b

	mov	r5, #0xa0000000
	mov	r6, #CMP_ITEM_COUNT
4:
//	mov	r0, #'D'
//	bl	TransmitX
	ldr	r2, [r5]
	cmp	r2, r5
	add	r5, r5, #4
	bne	7f
	subs	r6, r6, #1
	bne	4b
	b	8f
//CS1 does NOT have SDRAM connected, only 256 M on board
//We need to turn off CS1
7:
	mov	r0, #'F'
	bl	TransmitX
///
	BigMov	r3, ESD_BASE
	BigMov	r1, 0x00008000		//CON_REQ
	str	r1, [r3, #ESD_SCR]
11:	ldr	r1, [r3, #ESD_SCR]
	tst	r1, #1<<14
	beq	11b
#if 1
	BigMov	r1, 0x000ad0d2		//ESDMISC, ddr2, 3 bank bits (13+10+3+2) = 28 bits = 256M
	str	r1, [r3, #ESD_MISC]	// soft reset
	bic	r1, r1, #2
	mov	r0,#0x1
11:	subs	r0,r0,#1
	bne	11b
	str	r1, [r3, #ESD_MISC]	// release reset
#endif
	BigMov	r1, 0x04008008	 	//PRECHARGE ALL
	str	r1, [r3, #ESD_SCR]
///
	ldr	r1, [r3, #ESD_CTL1]
	tst	r1, #1<<31
	bic	r1, r1, #(1<<31)
	strne	r1, [r3, #ESD_CTL1]	// disable CS1

#if 0
	add	r4, r3, #ESD_CTL1
	add	r5, r3, #ESD_CFG1
	add	r6, r3, #ESD_SCR
	adr	r7, ESD_TABLE
10:	ldr	r0, [r7], #4
	cmp	r0,#0
	beq	8f
	ldr	r1, [r7], #4
	cmp	r0, r4		//ESD_CTL1
	cmpne	r0, r5		//ESD_CFG1
	beq	10b
	cmp	r0, r6		//ESD_SCR??
	bne	12f
	tst	r1,#4
	bne	10b
12:
	mov	r2,#0x100000
11:	subs	r2,r2,#1
	bne	11b
	str	r1, [r0]
	b	10b
#else
	mov	r1, #0	 	//normal operations again
	str	r1, [r3, #ESD_SCR]
#endif
8:
//	mov	r0, #'G'
//	bl	TransmitX
#endif

	bl	main
	bl	main
	bl	main
3:	b	3b

Basic_Init_ESD:
	adr	r7, ESD_TABLE
	mov	r6,#1000
	b	Basic_Init
Basic_Init_IOMux:
	adr	r7, IOMUX_TABLE
	mov	r6,#0x1
Basic_Init:
10:	ldr	r0, [r7], #4
	cmp	r0,#0
	moveq	pc, lr
	ldr	r1, [r7], #4
12:
	mov	r2, r6
11:	subs	r2, r2, #1
	bne	11b
	str	r1, [r0]
	b	10b


IOMUX_TABLE:
#if 1
//babbage tape out 2, 56 DCD entries
	.word	0x73fa88a0, 0x00000200	//IOMUXC_SW_PAD_CTL_GRP_INMODE1, ddr2 input type
	.word	0x73fa850c, 0x000020c5	//IOMUXC_SW_PAD_CTL_PAD_EIM_SDODT1, 100K Pull Down, high drive strength
	.word	0x73fa8510, 0x000020c5	//IOMUXC_SW_PAD_CTL_PAD_EIM_SDODT0, 100K Pull Down, high drive strength
	.word	0x73fa883c, 0x00000002	//IOMUXC_SW_PAD_CTL_GRP_DDR_A0, Medium drive strength
	.word	0x73fa8848, 0x00000002	//IOMUXC_SW_PAD_CTL_GRP_DDR_A1, Medium drive strength
	.word	0x73fa84b8, 0x000000e7	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK1, Max drive strength
	.word	0x73fa84bc, 0x00000045	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0, Disable pull down
	.word	0x73fa84c0, 0x00000045	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1, Disable pull down
	.word	0x73fa84c4, 0x00000045	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2, Disable pull down
	.word	0x73fa84c8, 0x00000045	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3, Disable pull down
	.word	0x73fa8820, 0x00000000	//IOMUXC_SW_PAD_CTL_GRP_DDRPKS, select keeper
	.word	0x73fa84a4, 0x00000003	//IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS, Medium Drive Strength
	.word	0x73fa84a8, 0x00000003	//IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS, Medium Drive Strength
	.word	0x73fa84ac, 0x000000e3	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDWE, Medium Drive Strength
	.word	0x73fa84b0, 0x000000e3	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCKE0, Medium Drive Strength
	.word	0x73fa84b4, 0x000000e3	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCKE1, Medium Drive Strength
	.word	0x73fa84cc, 0x000000e3	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCKE1, Medium Drive Strength
	.word	0x73fa84d0, 0x000000e2	//IOMUXC_SW_PAD_CTL_PAD_DRAM_CS0, Medium Drive Strength, slow slew rate
//serial downloader doesn't have the next 4
	.word	0x73fa882c, 0x00000006	//IOMUXC_SW_PAD_CTL_GRP_DRAM_B4 (D24-D31), max drive strength
	.word	0x73fa88a4, 0x00000006	//IOMUXC_SW_PAD_CTL_GRP_DRAM_B0 (D0-D7), max drive strength
	.word	0x73fa88ac, 0x00000006	//IOMUXC_SW_PAD_CTL_GRP_DRAM_B1 (D8-D15, max drive strength
	.word	0x73fa88b8, 0x00000006	//IOMUXC_SW_PAD_CTL_GRP_DRAM_B2 (D16-D23), max drive strength
	.word	0
#endif

ESD_TABLE:
#if 1
	.word	0x83fd9000, 0x82a20000	//ESDCTL0, 13 row, 10 col, 32 bit width
#ifdef USE_CSD1
	.word	0x83fd9008, 0x82a20000	//ESDCTL1
#endif
	.word	0x83fd9010, 0x000ad0d0	//ESDMISC, ddr2, 3 bank bits (13+10+3+2) = 28 bits = 256M
	.word	0x83fd9004, 0x333574aa	//ESDCFG0 vs 333584ab
//bit 3-0 ACTIVE to ACTIVE, same bank, command delay : us 1010 - 11 clocks,   them 1011 - 12 clocks
//bit 15-12 Active to precharge Command : us 0111 8 clocks, them 1000 9 clocks

#ifdef USE_CSD1
	.word	0x83fd900c, 0x333574aa	//ESDCFG1
#endif
//CSD0 always used
	.word	0x83fd9014, 0x04008008	//ESDSCR, CSD0
	.word	0x83fd9014, 0x0000801a	//emrs(2)
	.word	0x83fd9014, 0x0000801b	//emrs(3)
	.word	0x83fd9014, 0x00448019	//emrs(1) - 50 ohms ODT
	.word	0x83fd9014, 0x07328018	//MRS
	.word	0x83fd9014, 0x04008008	//PRECHARGE ALL
	.word	0x83fd9014, 0x00008010	//auto-refresh
	.word	0x83fd9014, 0x00008010	//auto-refresh
	.word	0x83fd9014, 0x06328018	//MRS
	.word	0x83fd9014, 0x03c48019	//emrs(1) - calibrate vs 0x03808019
//
	.word	0x83fd9014, 0x00448019	//emrs(1) - buffers enabled, RDQS disable, DQS enable,
						//OCD calibration mode exit, 50 ohm termination
						//0 latency, Normal drive strength, DLL enable
						// vs 0x00408019
//
	.word	0x83fd9014, 0x00008000	//nop
#ifdef USE_CSD1
	.word	0x83fd9014, 0x0400800c	//ESDSCR, CSD1 !!!!
	.word	0x83fd9014, 0x0000801e	//emrs(2)
	.word	0x83fd9014, 0x0000801f	//emrs(3)
	.word	0x83fd9014, 0x0044801d	//emrs(1) - 50 ohms ODT vs 0x0000801d
//
	.word	0x83fd9014, 0x0732801c	//MRS
	.word	0x83fd9014, 0x0400800c	//PRECHARGE ALL
	.word	0x83fd9014, 0x00008014	//auto-refresh
	.word	0x83fd9014, 0x00008014	//auto-refresh
	.word	0x83fd9014, 0x0632801c	//MRS
	.word	0x83fd9014, 0x03c4801d	//emrs(1) - calibrate vs 0x0380801d
//
	.word	0x83fd9014, 0x0044801d	//emrs(1) - buffers enabled, RDQS disable, DQS enable,
						//OCD calibration mode exit, 50 ohm termination
						//0 latency, Normal drive strength, DLL enable
						//vs 0x0040801d
//
	.word	0x83fd9014, 0x00008004	//nop
#endif
	.word	0x83fd9000, 0xb2a20000	//ESDCTL0, refresh 4 rows each refresh clock
#ifdef USE_CSD1
	.word	0x83fd9008, 0xb2a20000	//ESDCTL1, ...
#endif
	.word	0x83fd9010, 0x000ad6d0	//ESDMISC
	.word	0x83fd9034, 0x90000000	//ESDGPR DQS delays
	.word	0x83fd9014, 0x00000000	//ESDSCR, AXI address readies normal operation
	.word	0
//CSD0 DDR - 0x90000000 (256M), CSD1 DDR - 0xa0000000 (256M)
#endif
