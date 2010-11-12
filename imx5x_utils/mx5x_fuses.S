	.global assem_start
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
	.equiv	IIM_BASE,	0x83f98000	//weird 0x53ffc000 in documentation
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
	

	.equiv	I_FB,		0x0800		//bank 0, fuse 0-7
	.equiv	I_JTAG,		0x0804		//fuse 0x08-0x0f (8-15)
						//fuse 0x10-0x17 (16-23)
	.equiv	I_BT_SRC,	0x080c		//fuse 0x18-0x1f (24-31)
	.equiv	I_BT_HAB_TYPE,	0x0810		//fuse 0x20-0x27 (32-39)
	.equiv	I_BT_MEM_CTL,	0x0814		//fuse 0x28-0x2f (40-47) also has BT_MEM_TYPE(bits 4 & 5) BT_MEM_CTL(bits 1 & 2)
	.equiv	I_SJC_DISABLE,	0x0854
	.equiv	I_UID8,		0x081c
	.equiv	I_UID7,		0x0820
	.equiv	I_UID6,		0x0824
	.equiv	I_UID5,		0x0828
	.equiv	I_UID4,		0x082c
	.equiv	I_UID3,		0x0830
	.equiv	I_UID2,		0x0834
	.equiv	I_UID1,		0x0838
	.equiv	I_UID0,		0x083c

	.equiv	I_FBAC1,	0x0c00		//bank 1, FUSE bank Access Protection register
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

	.equiv	SRC_BASE,	0x73fd0000		//System reset controller
	.equiv	SBMR,		0x004
	.equiv	SBMR_BT_MEM_CTL,	0	//len 2, 
	.equiv	SBMR_BT_BUS_WIDTH,	2
	.equiv	SBMR_BT_PAGE_SIZE,	3	//len 2
	.equiv	SBMR_BT_SPARE,		6
	.equiv	SBMR_BT_MEM_TYPE,	7	//len 2
	.equiv	SBMR_BT_BMOD,		14	//len 2

	.equiv	UART_BASE, UART1_BASE

						//Bank 0
	.equiv	FUSE03_BT_SRC,		30	//30 & 31

	.equiv	FUSE04_HAB_TYPE,	32	//32 & 33 & 34
	.equiv	FUSE04_GPIO_BT_SEL,	35	//35	0 - use gpio, 1(blown) use fuse
	.equiv	FUSE04_BT_EEPROM_CFG,	36	//36
	.equiv	FUSE04_BT_PAGE_SIZE,	37	//37 & 38
	.equiv	FUSE04_VIRGIN,		39	//39
	.equiv	FUSE05_DIR_BT_DIS,	40	//40
	.equiv	FUSE05_BT_MEM_CTL,	41	//41 & 42
	.equiv	FUSE05_BT_BUS_WIDTH,	43	//43
	.equiv	FUSE05_BT_MEM_TYPE,	44	//44 & 45
	.equiv	FUSE05_BT_UART_SRC,	46	//46 & 47
#define FMASK(a) (1 << ((a) & 0x1f))
	.equiv	CCM_BASE,	0x73fd4000
	.equiv	CCM_CSCMR1,	0x1c
	.equiv	CCM_CSCDR1,	0x24
	.equiv	CCM_CGPR,	0x64
	.equiv	CCM_CCGR1, 	0x6c
#define rFNAME r8
#define rFOFF r9
#define rUART r10
#define rIIM r12

#include <asm/BigMacro.h>
#include <asm/char.inc>

#define L1(a)          ((CH_##a))
#define L2(a,b)     ((CH_##a)+(CH_##b<<8))
#define L3(a,b,c)   ((CH_##a)+(CH_##b<<8)+(CH_##c<<16))
#define L4(a,b,c,d) ((CH_##a)+(CH_##b<<8)+(CH_##c<<16)+(CH_##d<<24))

#define C2(a,b)     ((CH_##a<<8)+(CH_##b))
#define C3(a,b,c)   ((CH_##a<<16)+(CH_##b<<8)+(CH_##c))
#define C4(a,b,c,d) ((CH_##a<<24)+(CH_##b<<16)+(CH_##c<<8)+(CH_##d))

	.section .header_mem,"x"

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
//		.word	4, 0x83fd9000, 0xb2a20000	//ESDCTL0, 4 rows refreshed/clk, 13 row, 10 col, 32 bit width
//		.word	4, 0x83fd9010, 0x000ad0d0	//ESDMISC, ddr2, 3 bank bits (13+10+3+2) = 28 bits = 256M per CS
#if 0
//babbage tape out 2, 56 DCD entries
	.word	4, 0x73fa88a0, 0x00000200	//IOMUXC_SW_PAD_CTL_GRP_INMODE1, ddr2 input type
	.word	4, 0x73fa850c, 0x000020c5	//IOMUXC_SW_PAD_CTL_PAD_EIM_SDODT1, 100K Pull Down, high drive strength
	.word	4, 0x73fa8510, 0x000020c5	//IOMUXC_SW_PAD_CTL_PAD_EIM_SDODT0, 100K Pull Down, high drive strength
	.word	4, 0x73fa883c, 0x00000002	//IOMUXC_SW_PAD_CTL_GRP_DDR_A0, Medium drive strength
	.word	4, 0x73fa8848, 0x00000002	//IOMUXC_SW_PAD_CTL_GRP_DDR_A1, Medium drive strength
	.word	4, 0x73fa84b8, 0x000000e7	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCLK1, Max drive strength
	.word	4, 0x73fa84bc, 0x00000045	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0, Disable pull down
	.word	4, 0x73fa84c0, 0x00000045	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1, Disable pull down
	.word	4, 0x73fa84c4, 0x00000045	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2, Disable pull down
	.word	4, 0x73fa84c8, 0x00000045	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3, Disable pull down
	.word	4, 0x73fa8820, 0x00000000	//IOMUXC_SW_PAD_CTL_GRP_DDRPKS, select keeper
	.word	4, 0x73fa84a4, 0x00000003	//IOMUXC_SW_PAD_CTL_PAD_DRAM_RAS, Medium Drive Strength
	.word	4, 0x73fa84a8, 0x00000003	//IOMUXC_SW_PAD_CTL_PAD_DRAM_CAS, Medium Drive Strength
	.word	4, 0x73fa84ac, 0x000000e3	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDWE, Medium Drive Strength
	.word	4, 0x73fa84b0, 0x000000e3	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCKE0, Medium Drive Strength
	.word	4, 0x73fa84b4, 0x000000e3	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCKE1, Medium Drive Strength
	.word	4, 0x73fa84cc, 0x000000e3	//IOMUXC_SW_PAD_CTL_PAD_DRAM_SDCKE1, Medium Drive Strength
	.word	4, 0x73fa84d0, 0x000000e2	//IOMUXC_SW_PAD_CTL_PAD_DRAM_CS0, Medium Drive Strength, slow slew rate
//serial downloader doesn't have the next 4
	.word	4, 0x73fa882c, 0x00000006	//IOMUXC_SW_PAD_CTL_GRP_DRAM_B4 (D24-D31), max drive strength
	.word	4, 0x73fa88a4, 0x00000006	//IOMUXC_SW_PAD_CTL_GRP_DRAM_B0 (D0-D7), max drive strength
	.word	4, 0x73fa88ac, 0x00000006	//IOMUXC_SW_PAD_CTL_GRP_DRAM_B1 (D8-D15, max drive strength
	.word	4, 0x73fa88b8, 0x00000006	//IOMUXC_SW_PAD_CTL_GRP_DRAM_B2 (D16-D23), max drive strength
#endif
#if 0
	.word	4, 0x83fd9000, 0x82a20000	//ESDCTL0, 13 row, 10 col, 32 bit width
#ifdef USE_CSD1
	.word	4, 0x83fd9008, 0x82a20000	//ESDCTL1
#endif
	.word	4, 0x83fd9010, 0x000ad0d0	//ESDMISC, ddr2, 3 bank bits (13+10+3+2) = 28 bits = 128M
	.word	4, 0x83fd9004, 0x333574aa	//ESDCFG0
#ifdef USE_CSD1
	.word	4, 0x83fd900c, 0x333574aa	//ESDCFG1
#endif
//CSD0 always used
	.word	4, 0x83fd9014, 0x04008008	//ESDSCR, CSD0
	.word	4, 0x83fd9014, 0x0000801a	//emrs(2)
	.word	4, 0x83fd9014, 0x0000801b	//emrs(3)
	.word	4, 0x83fd9014, 0x00448019	//emrs(1) - 50 ohms ODT
	.word	4, 0x83fd9014, 0x07328018	//MRS
	.word	4, 0x83fd9014, 0x04008008	//PRECHARGE ALL
	.word	4, 0x83fd9014, 0x00008010	//auto-refresh
	.word	4, 0x83fd9014, 0x00008010	//auto-refresh
	.word	4, 0x83fd9014, 0x06328018	//MRS
	.word	4, 0x83fd9014, 0x03c48019	//emrs(1) - calibrate
	.word	4, 0x83fd9014, 0x00448019	//emrs(1) - buffers enabled, RDQS disable, DQS enable,
						//OCD calibration mode exit, 50 ohm termination
						//0 latency, Normal drive strength, DLL enable
	.word	4, 0x83fd9014, 0x00008000	//nop
#ifdef USE_CSD1
	.word	4, 0x83fd9014, 0x0400800c	//ESDSCR, CSD1 !!!!
	.word	4, 0x83fd9014, 0x0000801e	//emrs(2)
	.word	4, 0x83fd9014, 0x0000801f	//emrs(3)
	.word	4, 0x83fd9014, 0x0044801d	//emrs(1) - 50 ohms ODT
	.word	4, 0x83fd9014, 0x0732801c	//MRS
	.word	4, 0x83fd9014, 0x0400800c	//PRECHARGE ALL
	.word	4, 0x83fd9014, 0x00008014	//auto-refresh
	.word	4, 0x83fd9014, 0x00008014	//auto-refresh
	.word	4, 0x83fd9014, 0x0632801c	//MRS
	.word	4, 0x83fd9014, 0x03c4801d	//emrs(1) - calibrate
	.word	4, 0x83fd9014, 0x0044801d	//emrs(1) - buffers enabled, RDQS disable, DQS enable,
						//OCD calibration mode exit, 50 ohm termination
						//0 latency, Normal drive strength, DLL enable
	.word	4, 0x83fd9014, 0x00008004	//nop
#endif
	.word	4, 0x83fd9000, 0xb2a20000	//ESDCTL0, refresh 4 rows each refresh clock
	.word	4, 0x83fd9008, 0xb2a20000	//ESDCTL1, ...
	.word	4, 0x83fd9010, 0x000ad6d0	//ESDMISC
	.word	4, 0x83fd9034, 0x90000000	//ESDGPR DQS delays
	.word	4, 0x83fd9014, 0x00000000	//ESDSCR, AXI address readies normal operation
//CSD0 DDR - 0x90000000 (256M), CSD1 DDR - 0xa0000000 (256M)
#endif

#if 0
//babbage tape out 1, 20 DCD entries
	.word	4, 0x83fd9000, 0x80000000	//1 ESDCTL0
	.word	4, 0x83fd9014, 0x04008008	//2 ESDSCR
	.word	4, 0x83fd9014, 0x00008010	//3 ESDSCR
	.word	4, 0x83fd9014, 0x00008010	//4 ESDSCR
	.word	4, 0x83fd9014, 0x00338018	//5 ESDSCR
	.word	4, 0x83fd9000, 0xc3220000	//6 ESDCTL0 - 8 rows refreshed/clk, 14 row, 10 col, 32 bit width
	.word	4, 0x83fd9004, 0xb08567a9	//7 ESDCFG0
	.word	4, 0x83fd9010, 0x000a0104	//8 ESDMISC
	.word	4, 0x83fd9014, 0x00000000	//9 ESDSCR

	.word	4, 0x83fd9008, 0x80000000	//10 ESDCTL1
	.word	4, 0x83fd9014, 0x0400800c	//11 ESDSCR
	.word	4, 0x83fd9014, 0x00008014	//12 ESDSCR
	.word	4, 0x83fd9014, 0x00008014	//13 ESDSCR
	.word	4, 0x83fd9014, 0x0033801c	//14 ESDSCR
	.word	4, 0x83fd9008, 0xc3220000	//15 ESDCTL1
	.word	4, 0x83fd900c, 0xb08567a9	//16 ESDCFG1
	.word	4, 0x83fd9010, 0x000a0104	//17 ESDMISC - 2 bank bits (14+10+2+2) = 28 bits = 128M
	.word	4, 0x83fd9014, 0x00000004	//18 ESDSCR
	.word	4, 0x90000000, 0x00000000	//19
	.word	4, 0xa0000000, 0x00000000	//20
#endif

2:
	.word	program_length	//0x24 length

	.section .text
assem_start:
	mov	sp,#0x20000000
	BigMov	rUART,UART_BASE	//0x28

	.equiv	IOMUXC_BASE, 0x73fa8000
	.equiv	SW_MUX_CTL_PAD_UART1_RXD, 0x228
	.equiv	SW_MUX_CTL_PAD_UART1_TXD, 0x22c
	.equiv	SW_PAD_CTL_UART1_RXD, 0x618
	.equiv	SW_PAD_CTL_UART1_TXD, 0x61c
#if 0
	BigMov	r6,IOMUXC_BASE
	ldr	r0,[r6, #SW_MUX_CTL_PAD_UART1_RXD]	//0 serial bootloader
	bl	PrintHex
	bl	TransmitCRLF
	ldr	r0,[r6, #SW_MUX_CTL_PAD_UART1_TXD]	//0 serial bootloader
	bl	PrintHex
	bl	TransmitCRLF
	ldr	r0,[r6, #SW_PAD_CTL_UART1_RXD]		//0x1c5 serial bootloader
	bl	PrintHex
	bl	TransmitCRLF
	ldr	r0,[r6, #SW_PAD_CTL_UART1_TXD]		//0x1c5 serial bootloader
	bl	PrintHex
	bl	TransmitCRLF
#else
	BigMov	r6,IOMUXC_BASE
	mov	r0,#0
	str	r0,[r6, #SW_MUX_CTL_PAD_UART1_RXD]
	str	r0,[r6, #SW_MUX_CTL_PAD_UART1_TXD]
	BigMov	r0, 0x1c5
	str	r0,[r6, #SW_PAD_CTL_UART1_RXD]
	str	r0,[r6, #SW_PAD_CTL_UART1_RXD]
#endif

#if 0
	BigMov	r6,CCM_BASE
	ldr	r0,[r6, #CCM_CSCMR1]		//0xA662A020, pll3
	bl	PrintHex
	bl	TransmitCRLF
	ldr	r0,[r6, #CCM_CSCDR1]		//0x00C30461, podf /2, pred /5
	bl	PrintHex
	bl	TransmitCRLF
	ldr	r0,[r6, #CCM_CCGR1]		//index 3 & 4 is UART1
	bl	PrintHex
	bl	TransmitCRLF
	
#else
	BigMov	r6,CCM_BASE
	ldr	r0,[r6, #CCM_CCGR1]		//index 3 & 4 is UART1
	bic	r0,r0,#0xf<<6
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
	orr	r0,r0,#0xf<<6
	str	r0,[r6, #CCM_CCGR1]
#endif
	
#if 0
	add	r6, rUART, #UCR1
	add	r7, rUART, #UTS+4
1:	mov	r0,r6
	bl	PrintHex
	BigMov	r0,L2(COLON,SPACE)
	bl	Transmit
	ldr	r0,[r6], #4
	bl	PrintHex
	bl	TransmitCRLF
	cmp	r6,r7
	bne	1b
#else
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
	b	2f
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
2:	
#endif
	bl	TransmitCRLF
	mov	r0,sp
	bl	PrintHex
	bl	TransmitCRLF

	mov		r0,#0x10000
	mov		r3,#0x10000
23:	subs	r0,r0,#1
	sub		r3,r3,#1
	bne		23b

	cmp		r3,r0
	BigMov	r0,L1(N)
	strne	r0,[rUART,#UTXD]
//	b	Test
	bl	TransmitCRLF

	.if 0
//setings for 115200,8,o,2
	mov		r0,#0x01
	str		r0,[r4,#UCR1]
	mov		r0,#0x20
	str		r0,[r4,#UTS]
	BigMov	r0,0x0a81
	str		r0,[r4,#UFCR]
	mov		r0,#2
//	str		r0,[r4,#TXTL]

	mov		r0,#3
//	str		r0,[r4,#RFDIV]
	mov		r0,#1
//	str		r0,[r4,#RXTL]
	BigMov	r0,0x000f
	str		r0,[r4,#UBIR]
	BigMov	r0,0x01a9		//49M/115200 = 425.34
	str		r0,[r4,#UBMR]
	BigMov	r0,0x61e7
	str		r0,[r4,#UCR2]

	mov		r0,#4
	str		r0,[r4,#UCR3]
	BigMov	r0,0x8000
	str		r0,[r4,#UCR4]
	mov		r0,#0x20
//	str		r0,[r4,CTSTL]

	.endif


	BigMov	rIIM,IIM_BASE
	bl	Get_fuse_name_table
	adr	rFOFF,fuse_offset_table
1:	bl	transmit_string

	BigMov	r0,L1(OPEN_PAREN)
	bl	Transmit
	ldrh	r0,[rFOFF]
	bl	PrintHexShort
	BigMov	r0,L2(CLOSE_PAREN,COLON)
	bl	Transmit
	ldrh	r4,[rFOFF],#2
	ldr	r0,[rIIM,r4]
	bl	PrintHex
	bl	TransmitCRLF
	ldrh	r4,[rFOFF]
	movs	r4,r4
	bne	1b
	
	BigMov	rIIM,SRC_BASE
	bl	transmit_string	//SBMR

	BigMov	r0,L1(OPEN_PAREN)
	bl	Transmit
	add	r0,rIIM,#SBMR
	bl	PrintHex
	BigMov	r0,L2(CLOSE_PAREN,COLON)
	bl	Transmit
	ldr	r0,[rIIM, #SBMR]
	bl	PrintHex
	bl	TransmitCRLF
	
	
	BigMov	rIIM,IIM_BASE
	bl	transmit_string	//UID6 FUSE
	mov	r0,#0x24*2	//UID6 fuse 72
	bl	sense_fuse

	bl	transmit_string	//virgin
	mov	r0,#FUSE04_VIRGIN & ~0x7
	bl	sense_fuse
	ldrb	r7,[rIIM, #I_SDAT]

	bl	transmit_string	//FUSE 40-47
	mov	r0,#FUSE05_BT_MEM_CTL & ~0x7
	bl	sense_fuse

	ldrb	r6,[rIIM, #I_SDAT]
	orr	r7, r7, r6, LSL #8
	
	tst	r7,#FMASK(FUSE05_DIR_BT_DIS)
	bne	2f
//	mov	r0,#FUSE05_DIR_BT_DIS
//	bl	burn_fuse
//	bne	8f
2:	
	tst	r7,#FMASK(FUSE05_BT_MEM_CTL)
	bne	3f
	mov	r0,#FUSE05_BT_MEM_CTL
	bl	burn_fuse
	bne	8f
3:
	tst	r7,#FMASK(FUSE05_BT_MEM_CTL+1)
	bne	4f
	mov	r0,#FUSE05_BT_MEM_CTL+1
	bl	burn_fuse
	bne	8f
4:
#ifdef ECSPI_BURN
	tst	r7,#FMASK(FUSE05_BT_MEM_TYPE)
	bne	4f
	mov	r0,#FUSE05_BT_MEM_TYPE
	bl	burn_fuse			//serial rom via SPI
	bne	8f
4:
	tst	r7,#FMASK(FUSE05_BT_MEM_TYPE+1)
	bne	4f
	mov	r0,#FUSE05_BT_MEM_TYPE+1
	bl	burn_fuse			//serial rom via SPI
	bne	8f
4:
	tst	r7,#FMASK(FUSE05_BT_BUS_WIDTH)
	bne	4f
	mov	r0,#FUSE05_BT_BUS_WIDTH		//3 byte address SPI device
	bl	burn_fuse
	bne	8f
4:
#endif
	tst	r7,#FMASK(FUSE04_VIRGIN)
	bne	9f
	mov	r0,#FUSE04_VIRGIN
	bl	burn_fuse
	beq	9f

8:
	BigMov	r0,L1(OPEN_PAREN)
	bl	Transmit
	mov	r0,r6
	bl	PrintHexShort
	BigMov	r0,L2(CLOSE_PAREN,COLON)
	bl	Transmit
	adr	rFNAME,error_blow_str
	bl	transmit_string
9:
	adr	rFNAME,done_str
	bl	transmit_string
20:	b	20b

#define PROGRAM_START 1
#define ESNS_N 8
#define PGR_LENGTH_MASK 0x70

//r0 - fuse #
sense_fuse:
	mov	r5, lr
	strb	r0,[rIIM, #I_LA]
	mov	r0,r0,LSR #8
	strb	r0,[rIIM, #I_UA]

	mov	r0,#3
	strb	r0,[rIIM, #I_STAT]
	ldrb	r0,[rIIM, #I_FCTL]
	and	r0, r0, #PGR_LENGTH_MASK
	orr	r0, r0, #ESNS_N
	strb	r0,[rIIM, #I_FCTL]
1:	ldrb	r0,[rIIM, #I_STAT]
	tst	r0,#1
	beq	1b
	ldrb	r0,[rIIM, #I_SDAT]
	bl	PrintHexByte
	bl	TransmitCRLF
	mov	pc, r5

//r0 - fuse #
burn_fuse:
	mov	r5, lr
	mov	r6, r0
	adr	rFNAME, blowing_str
	bl	transmit_string
	mov	r0, r6
	bl	PrintHexByte
	bl	TransmitCRLF

	BigMov	r2, CCM_BASE
	ldr	r1, [r2, #CCM_CGPR]
	orr	r1, r1, #0x10		//turn on programming supply
	str	r1, [r2, #CCM_CGPR]

	mov	r1,#3
	strb	r1,[rIIM, #I_STAT]
	mov	r1,#0xff
	strb	r1,[rIIM, #I_ERR]
	mov	r1,#0xaa
	strb	r1,[rIIM, #I_PREG_P]

	strb	r6,[rIIM, #I_LA]
	mov	r6,r6,LSR #8
	strb	r6,[rIIM, #I_UA]

	ldrb	r0,[rIIM, #I_FCTL]
	and	r0, r0, #PGR_LENGTH_MASK
	orr	r0, r0, #PROGRAM_START
	strb	r0,[rIIM, #I_FCTL]
1:	ldrb	r0,[rIIM, #I_STAT]
	tst	r0,#2		//wait for program done
	beq	1b

	mov	r0,#0x55
	strb	r0,[rIIM, #I_PREG_P]

	ldr	r1, [r2, #CCM_CGPR]
	bic	r1, r1, #0x10		//turn off programming supply
	str	r1, [r2, #CCM_CGPR]

	ldrb	r6,[rIIM, #I_ERR]
	tst	r6,#0xff
	mov	pc, r5

//IN: r0 - value to print
//OUT: r0-r4 trashed
PrintHex:
	mov	r2,#8
	mov	r4,r0
PrintHex1:
	mov	r3,lr
1:	mov	r4,r4,ROR #28
	and	r0,r4,#0xf
	cmp	r0,#0xA
	addcs	r0,r0,#L1(A)-0x0a
	addcc	r0,r0,#L1(0)
	bl	Transmit
	subs	r2,r2,#1
	bne	1b
	mov	pc,r3

PrintHexByte:
	mov	r4,r0,LSL #24
	mov	r2,#2
	b	PrintHex1
PrintHexShort:
	mov	r4,r0,LSL #16
	mov	r2,#4
	b	PrintHex1

TransmitCRLF:
	BigMov	r0,L2(CR,LF)
	b	Transmit

TransmitSPACE:
	mov	r0,#L1(SPACE)
//	b Transmit

Transmit:
1:
	str	r0,[rUART,#UTXD]
2:	ldr	r1,[rUART,#USR2]
	tst	r1,#1<<3
	beq	2b
	movs	r0,r0,LSR #8
	bne	1b
	mov	pc,lr

//In: rFNAME @asciz str
//Out: rFNAME past str
transmit_string:
1:	ldrb	r0,[rFNAME],#1
	movs	r0,r0
	moveq	pc,lr
2:	ldr	r1,[rUART,#USR2]
	tst	r1,#1<<3
	beq	2b
	str	r0,[rUART,#UTXD]
	b	1b

fuse_offset_table:
	.short	I_FB		//0x0800 bank 0
	.short	I_JTAG		//0x0804
	.short	I_BT_SRC	//0x080c
	.short	I_BT_HAB_TYPE	//0x0810
	.short	I_BT_MEM_CTL	//0x0814

	.short	I_SJC_DISABLE	//0x0854
	.short	I_UID0		//0x083c
	.short	I_UID1		//0x0838
	.short	I_UID2		//0x0834
	.short	I_UID3		//0x0830
	.short	I_UID4		//0x082c
	.short	I_UID5		//0x0828
	.short	I_UID6		//0x0824
	.short	I_UID7		//0x0820
	.short	I_UID8		//0x081c

	.short	I_FBAC1		//0x0c00 bank 1, FUSE bank Access Protection register
	.short	I_SJC_RESP0	//0x0c20
	.short	I_SJC_RESP1	//0x0c1c
	.short	I_SJC_RESP2	//0x0c18
	.short	I_SJC_RESP3	//0x0c14
	.short	I_SJC_RESP4	//0x0c10
	.short	I_SJC_RESP5	//0x0c0c
	.short	I_SJC_RESP6	//0x0c08

	.short	I_MAC_ADDR0	//0x0c38
	.short	I_MAC_ADDR1	//0x0c34
	.short	I_MAC_ADDR2	//0x0c30
	.short	I_MAC_ADDR3	//0x0c2c
	.short	I_MAC_ADDR4	//0x0c28
	.short	I_MAC_ADDR5	//0x0c24
	.short	0

Get_fuse_name_table:
	adr	rFNAME,fuse_name_table
	mov	pc, lr

	.align	2
blowing_str:
	.asciz	"Blowing fuse 0x"
	.align	2
error_blow_str:
	.asciz	"Error blowing fuse \n"
	.align	2
done_str:
	.asciz	"done\n"
	.align	2
fuse_name_table:
	.asciz	"FB"		//0x0800 bank 0
	.asciz	"JTAG"		//0x0804
	.asciz	"BT_SRC"	//0x080c
	.asciz	"BT_HAB_TYPE"	//0x0810
	.asciz	"BT_MEM_CTL.."	//0x0814

	.asciz	"SJC_DISABLE"	//0x0854
	.asciz	"UID0"		//0x083c
	.asciz	"UID1"		//0x0838
	.asciz	"UID2"		//0x0834
	.asciz	"UID3"		//0x0830
	.asciz	"UID4"		//0x082c
	.asciz	"UID5"		//0x0828
	.asciz	"UID6"		//0x0824
	.asciz	"UID7"		//0x0820
	.asciz	"UID8"		//0x081c

	.asciz	"FBAC1"		//0x0c00 bank 1, FUSE bank Access Protection register
	.asciz	"SJC_RESP0"	//0x0c20
	.asciz	"SJC_RESP1"	//0x0c1c
	.asciz	"SJC_RESP2"	//0x0c18
	.asciz	"SJC_RESP3"	//0x0c14
	.asciz	"SJC_RESP4"	//0x0c10
	.asciz	"SJC_RESP5"	//0x0c0c
	.asciz	"SJC_RESP6"	//0x0c08

	.asciz	"MAC_ADDR0"	//0x0c38
	.asciz	"MAC_ADDR1"	//0x0c34
	.asciz	"MAC_ADDR2"	//0x0c30
	.asciz	"MAC_ADDR3"	//0x0c2c
	.asciz	"MAC_ADDR4"	//0x0c28
	.asciz	"MAC_ADDR5"	//0x0c24
	.asciz	"SBMR"
	.asciz	"UID6 FUSE:"
	.asciz	"FUSE 32-39 (39 is virgin):"
	.asciz	"FUSE 40-47:"
end_addr:
