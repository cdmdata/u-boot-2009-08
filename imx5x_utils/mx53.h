//#define USE_CSD1 1

#define CPU_2_BE_32(l) \
       ((((l) & 0x000000FF) << 24) | \
	(((l) & 0x0000FF00) << 8)  | \
	(((l) & 0x00FF0000) >> 8)  | \
	(((l) & 0xFF000000) >> 24))

#define MAKE_TAG(tag, len, v) CPU_2_BE_32(( ((tag) << 24) | ((len) << 8) | (v) ))
#define MXC_DCD_ITEM(addr, val)	.word CPU_2_BE_32(addr), CPU_2_BE_32(val)

#define STACK_HIGH	0xf805ff00
#define RAM_BASE	0x70000000
#define ECSPI1_BASE	0x50010000
#define MM_SDMMC_BASE	0x50004000
#define UART1_BASE	0x53fbc000
#define UART2_BASE	0x53fc0000
#define UART3_BASE	0x5000c000
#define UART_BASE	UART2_BASE

#define GPIO3_BASE	0x53f8c000
#define WDOG_BASE	0x53f98000
#define CCM_BASE	0x53fd4000
#define ESD_BASE	0x63fd9000

#define IOMUXC_BASE_ADDR (0x53FA8000)
#define ESDCTL_BASE_ADDR (0x63FD9000)

// AIPS Constants
#define CSP_BASE_REG_PA_AIPS1	0x53F00000
#define CSP_BASE_REG_PA_AIPS2	0x63F00000


#ifdef ASM
////////////////////////////
// fuse defines
////
	.equiv	IIM_BASE,	0x63f98000	//weird 0x53ffc000 in documentation
	.equiv	SRC_BASE,	0x53fd0000	//System reset controller

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
	.equiv	PREV_EXPECTED,	0xfa

	.equiv	I_SI_REV,	0x081c		//fuse 0x38-0x3f (56-63)
//below same as mx51
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

	.macro fuse_table
	fentry "PREV", I_PREV
	fentry "SREV", I_SREV
	fentry	"Fuse[7:0]", 0x0800		//0x0800 bank 0
	fentry	"Fuse[15:8]", 0x0804		//0x0804
	fentry	"BOOT_CFG1", 0x080c		//0x080c
	fentry	"BOOT_CFG2", 0x0810		//0x0810
	fentry	"BOOT_CFG3", 0x0814		//0x0814

	fentry	"UID0", I_UID0			//0x083c
	fentry	"UID1", I_UID1			//0x0838
	fentry	"UID2", I_UID2			//0x0834
	fentry	"UID3", I_UID3			//0x0830
	fentry	"UID4", I_UID4			//0x082c
	fentry	"UID5", I_UID5			//0x0828
	fentry	"UID6", I_UID6			//0x0824
	fentry	"UID7", I_UID7			//0x0820
//	fentry	"UID8", I_UID8			//0x081c

	fentry	"Fuse[263:256]", 0x0c00		//0x0c00 bank 1, FUSE bank Access Protection register
	fentry	"SJC_RESP0", I_SJC_RESP0	//0x0c20
	fentry	"SJC_RESP1", I_SJC_RESP1	//0x0c1c
	fentry	"SJC_RESP2", I_SJC_RESP2	//0x0c18
	fentry	"SJC_RESP3", I_SJC_RESP3	//0x0c14
	fentry	"SJC_RESP4", I_SJC_RESP4	//0x0c10
	fentry	"SJC_RESP5", I_SJC_RESP5	//0x0c0c
	fentry	"SJC_RESP6", I_SJC_RESP6	//0x0c08

	fentry	"MAC_ADDR0", I_MAC_ADDR0	//0x0c38
	fentry	"MAC_ADDR1", I_MAC_ADDR1	//0x0c34
	fentry	"MAC_ADDR2", I_MAC_ADDR2	//0x0c30
	fentry	"MAC_ADDR3", I_MAC_ADDR3	//0x0c2c
	fentry	"MAC_ADDR4", I_MAC_ADDR4	//0x0c28
	fentry	"MAC_ADDR5", I_MAC_ADDR5	//0x0c24
	.byte	0xff
	fentry	"SBMR", SRC_SBMR
	.byte	0
#define FMASK(a) (1 << ((a) & 0x7))
//	fBurn	"UID6 FUSE:", 0x24*2, 0
//FMASK(FUSE05_DIR_BT_DIS)
#ifdef ECSPI_BURN
	fBurn	"BOOT_CFG1:", 3*8, (1<<5)|(1<<4)|(1<<3)
	fBurn	"BOOT_CFG2:", 4*8, (1<<5)	//24-bit address
	fBurn	"BOOT_CFG3:", 5*8, (1<<2)	//CS#1
#else
	fBurn	"BOOT_CFG1:", 3*8, (1<<6)
#endif
	fBurn	"Fuse[15:8] (12 is BT_FUSE_SEL):", 1*8, FMASK(12)
	.byte	0
	.endm


/* ************************************************** */
/* ************************************************** */
	.macro header_macro start_rtn
//offset 0x400 for SD/MMC
ivt_header:		.word MAKE_TAG(0xD1, 0x20, 0x40) /* 0x402000D1, Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:	.word (\start_rtn)
reserv1:		.word 0x0
dcd_ptr:		.word 0x0
boot_data_ptr:		.word boot_data
self_ptr:		.word ivt_header
app_code_csf:		.word 0x0
reserv2:		.word 0x0


boot_data:		.word 0xf8006000
image_len:		.word program_length
plugin:			.word 0x0
	.endm


/* DCD */
	.macro iomux_dcd_data
	.word	0x53FA8554, 0x00380000	/* DQM3: dse 7 - 43 30 34 20 ohm drive strength*/
	.word	0x53FA8558, 0x00380040	/* SDQS3: pue=1 but PKE=0, dse 7 */
	.word	0x53FA8560, 0x00380000	/* DQM2: dse 7 */
	.word	0x53FA8564, 0x00380040	/* SDODT1: pue=1 but PKE=0, dse 7 */
	.word	0x53FA8568, 0x00380040	/* SDQS2: pue=1 but PKE=0, dse 7 */
	.word	0x53FA8570, 0x00200000	/* SDCLK_1: dse 4 - 75 NA 60 NA */
	.word	0x53FA8574, 0x00380000	/* CAS: dse 7 */
	.word	0x53FA8578, 0x00200000	/* SDCLK_0: dse 4 */
	.word	0x53FA857c, 0x00380040	/* SDQS0: pue=1 but PKE=0, dse 7 */
	.word	0x53FA8580, 0x00380040	/* SDODT0: pue=1 but PKE=0, dse 7 */
	.word	0x53FA8584, 0x00380000	/* DQM0: dse 7 */
	.word	0x53FA8588, 0x00380000	/* RAS: dse 7 */
	.word	0x53FA8590, 0x00380040	/* SDQS1: pue=1 but PKE=0, dse 7 */
	.word	0x53FA8594, 0x00380000	/* DQM1: dse 7 */
	.word	0x53FA86f0, 0x00380000	/* GRP_ADDDS A0-A15 BA0-BA2: dse 7 */
	.word	0x53FA86f4, 0x00000200	/* GRP_DDRMODE_CTL: DDR2 SDQS0-3 */
	.word	0x53FA86fc, 0x00000000	/* GRP_DDRPKE: PKE=0 for A0-A15,CAS,CS0,CS1,D0-D31,DQM0-3,RAS,SDBA0-2,SDCLK_0-1,SDWE */
	.word	0x53FA8714, 0x00000000	/* GRP_DDRMODE: CMOS input type?????? D0-D31 */
	.word	0x53FA8718, 0x00380000	/* GRP_B0DS: dse 7 for D0-D7 */
	.word	0x53FA871c, 0x00380000	/* GRP_B1SD: dse 7 for D8-D15 */
	.word	0x53FA8720, 0x00380000	/* GRP_CTLDS: dse 7 for CS0-1, SDCKE0-1, SDWE*/
	.word	0x53FA8724, 0x06000000	/* GRP_DDR_TYPE: A0-A15,CAS,CS0-1,D0-D31,DQM0-3,RAS,SDBA0-2,SDCKE0-1,SDCLK_0-1,SDODT0-1,SDQS0-3,SDWE  3=20 ohms*/
	.word	0x53FA8728, 0x00380000	/* GRP_B2DS: dse 7 for D16-D23 */
	.word	0x53FA872c, 0x00380000	/* GRP_B3DS: dse 7 fro D24-D31 */
#if 1
//uart1 rxd:PATA_DMACK, txd:PATA_DIOW
	.word	0x53FA8274, 3		//Mux: PATA_DMACK - UART1_RXD
	.word	0x53FA85F4, 0x1e4	//CTL: PATA_DMACK - UART1_RXD
	.word	0x53FA8878, 3		//Select: UART1_IPP_UART_RXD_MUX_SELECT_INPUT
	.word	0x53FA8270, 3		//MUX: PATA_DIOW - UART1_TXD
	.word	0x53FA85F0, 0x1e4	//CTL: PATA_DIOW - UART1_TXD
#endif
//uart2 rxd:PATA_BUFFER_EN, txd:PATA_DMARQ
	.word	0x53FA827C, 3		//Mux: PATA_BUFFER_EN - UART1_RXD
	.word	0x53FA85FC, 0x1e4	//CTL: PATA_BUFFER_EN - UART1_RXD
	.word	0x53FA8880, 3		//Select: UART2_IPP_UART_RXD_MUX_SELECT_INPUT
	.word	0x53FA8278, 3		//MUX: PATA_DMARQ - UART1_TXD
	.word	0x53FA85F8, 0x1e4	//CTL: PATA_DMARQ UART1_TXD
	.word	0
	.endm

	.macro esd_dcd_data
	.word	0x63FD9088, 0x2b2f3031	/* RDDLCTL */
	.word	0x63FD9090, 0x40363333	/* WRDLCTL */
	.word	0x63FD9098, 0x00000f00	/* SDCTRL */
	.word	0x63FD90f8, 0x00000800	/* MUR */
	.word	0x63FD907c, 0x01310132	/* DGCTRL0 */
	.word	0x63FD9080, 0x0133014b	/* DGCTRL1 */
	.word	0x63FD9018, 0x000016d0	/* ESDMISC, (0)8 banks, (2)DDR2 */

#ifdef USE_CSD1
//	.word	0x63FD9000, 0xc4110000	/* ESDCTL, (4)15 rows, (1)10 columns (0)BL 4,  (1)32 bit width*/
	.word	0x63FD9000, 0xc3110000	/* ESDCTL, (3)14 rows, (1)10 columns (0)BL 4,  (1)32 bit width*/
#else
//	.word	0x63FD9000, 0x84110000	/* ESDCTL, (4)15 rows, (1)10 columns (0)BL 4,  (1)32 bit width */
	.word	0x63FD9000, 0x83110000	/* ESDCTL, (3)14 rows, (1)10 columns (0)BL 4,  (1)32 bit width */
#endif
	.word	0x63FD900c, 0x4d5122d2	/* ESDCFG0 */
	.word	0x63FD9010, 0x92d18a22	/* ESDCFG1 */
	.word	0x63FD9014, 0x00c70092	/* ESDCFG2 */
	.word	0x63FD902c, 0x000026d2	/* ESDRWD */
	.word	0x63FD9030, 0x009f000e	/* ESDOR */
	.word	0x63FD9008, 0x12272000	/* ESDOTC */
	.word	0x63FD9004, 0x00030012	/* ESDPDC */

	.word	0x63FD901c, 0x04008010	/* ESDSCR */
	.word	0x63FD901c, 0x00008032	/* ESDSCR */
	.word	0x63FD901c, 0x00008033	/* ESDSCR */
	.word	0x63FD901c, 0x00008031	/* ESDSCR */
	.word	0x63FD901c, 0x0b5280b0	/* ESDSCR DLL_RST0 */
	.word	0x63FD901c, 0x04008010	/* ESDSCR */ //PRECHARGE ALL CS0
	.word	0x63FD901c, 0x00008020	/* ESDSCR */
	.word	0x63FD901c, 0x00008020	/* ESDSCR */
	.word	0x63FD901c, 0x0a528030	/* ESDSCR */
	.word	0x63FD901c, 0x03c68031	/* ESDSCR */
	.word	0x63FD901c, 0x00468031	/* ESDSCR */

#ifdef USE_CSD1
	.word	0x63FD901c, 0x04008018	/* ESDSCR */
	.word	0x63FD901c, 0x0000803a	/* ESDSCR */
	.word	0x63FD901c, 0x0000803b	/* ESDSCR */
	.word	0x63FD901c, 0x00008039	/* ESDSCR */
	.word	0x63FD901c, 0x0b528138	/* ESDSCR DLL_RST1 */
	.word	0x63FD901c, 0x04008018	/* ESDSCR */ //PRECHARGE ALL CS1
	.word	0x63FD901c, 0x00008028	/* ESDSCR */
	.word	0x63FD901c, 0x00008028	/* ESDSCR */
	.word	0x63FD901c, 0x0a528038	/* ESDSCR */
	.word	0x63FD901c, 0x03c68039	/* ESDSCR */
	.word	0x63FD901c, 0x00468039	/* ESDSCR */
#endif

//	.word	0x63FD9020, 0x00005800	/* ESDREF, (1)32k refresh cycle rate, (3) 4 refreshes/cycle */
	.word	0x63FD9020, 0x00004800	/* ESDREF, (1)32k refresh cycle rate, (1) 2 refreshes/cycle */
	.word	0x63FD9058, 0x00033337	/* ODTCTRL */
	.word	0x63FD901c, 0x00000000	/* ESDSCR: AXI address readies normal operation */
	.word	0
	.endm

	.macro config_uart_clocks
#if 0
	BigMov	r0, ('1') | (':' << 8)
	bl	TransmitX
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CCGR1]
	bl	print_hex
#endif
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CCGR1]		//index 3 & 4 is UART1, 5&6 UART2
	bic	r0,r0,#0xff<<(3*2)
	str	r0,[r1, #CCM_CCGR1]

	ldr	r0,[r1, #CCM_CSCMR1]
	bic	r0,r0,#3<<24
	orr	r0,r0,#2<<24			//pll3
	str	r0,[r1, #CCM_CSCMR1]

	ldr	r0,[r1, #CCM_CSCDR1]
	bic	r0,r0,#0x3f
	orr	r0,r0,#0x21			// podf /2, pred /5
	str	r0,[r1, #CCM_CSCDR1]

	ldr	r0,[r1, #CCM_CCGR1]		//index 3 & 4 is UART1, 5&6 UART2
	orr	r0,r0,#0xff<<(3*2)
	str	r0,[r1, #CCM_CCGR1]
#if 0
	BigMov	r0, (' ') | ('R' << 8) | (':' << 16)
	bl	TransmitX
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CBCDR]
	bl	print_hex
#endif

#if 1	//slow ddr2
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CBCDR]
	orr	r0, r0, #(1 << 16)		//slow ddr2
	str	r0,[r1, #CCM_CBCDR]
#endif

#if 0
	BigMov	r0, ('R') | (':' << 8)
	bl	TransmitX
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CBCDR]
	bl	print_hex
#endif
	.endm

#define ESD_CTL  0x000
#define ESD_MISC  0x018
#define ESD_SCR   0x01c

	.macro test_watchdog_reset
	BigMov	r1, SRC_BASE
	ldr	r0, [r1, #SRC_SRSR]
	tst	r0,#(1 << 4)	//watchdog
	.endm

	.macro test_memory_active
	BigMov	r1, ESD_BASE
	ldr	r0, [r1, #ESD_CTL]
	tst	r0,#(1 << 31)	//sdcs0
	.endm

	.macro esd_con_req
	BigMov	r3, ESD_BASE
	BigMov	r1, 0x00008000		//CON_REQ
	str	r1, [r3, #ESD_SCR]
91:	ldr	r1, [r3, #ESD_SCR]
	tst	r1, #1<<14
	beq	91b
	.endm

	.macro Turn_off_CS1
	BigMov	r3, ESD_BASE
	BigMov	r1, 0x00008000		//CON_REQ
	str	r1, [r3, #ESD_SCR]
91:	ldr	r1, [r3, #ESD_SCR]
	tst	r1, #1<<14
	beq	91b
#if 1
	BigMov	r1, 0x000016d2		/* ESDMISC, (0)8 banks, (2)DDR2 */
	str	r1, [r3, #ESD_MISC]	// soft reset
	bic	r1, r1, #2
	mov	r0,#0x1
91:	subs	r0,r0,#1
	bne	91b
	str	r1, [r3, #ESD_MISC]	// release reset
#endif
	BigMov	r1, 0x04008010		//PRECHARGE ALL, CS0
	str	r1, [r3, #ESD_SCR]
///
	ldr	r1, [r3, #ESD_CTL]
	tst	r1, #1<<30
	bic	r1, r1, #(1<<30)
	strne	r1, [r3, #ESD_CTL]	// disable CS1

	mov	r1, #0		//normal operations again
	str	r1, [r3, #ESD_SCR]
93:
	.endm

#define ALT0	0x0
#define ALT1	0x1
#define ALT3	0x3
#define ALT4	0x4
#define SION	0x10	//0x10 means force input path of BGA contact

#define DSE_LOW		(0x0 << 1)
#define DSE_MEDIUM	(0x1 << 1)
#define DSE_HIGH	(0x2 << 1)
#define DSE_MAX		(0x3 << 1)
#define OPENDRAIN_ENABLE (0x1 << 3)
#define R360K_PD	(0x0 << 4)
#define R75K_PU		(0x1 << 4)
#define R100K_PU	(0x2 << 4)
#define R22K_PU		(0x3 << 4)
#define KEEPER		(0x0 << 6)
#define PULL		(0x1 << 6)
#define PKE_ENABLE	(0x1 << 7)
#define HYS_ENABLE	(0x1 << 8)
#define VOT_LOW		(0x0 << 13)
#define VOT_HIGH	(0x1 << 13)

#define PAD_CSPI1_MOSI	(HYS_ENABLE | DSE_HIGH)
#define PAD_CSPI1_MISO	(HYS_ENABLE | DSE_HIGH)
#define PAD_CSPI1_SS0	(HYS_ENABLE | DSE_HIGH)
#define PAD_CSPI1_SS1	(HYS_ENABLE | DSE_HIGH)
#define PAD_CSPI1_RDY	(HYS_ENABLE | DSE_LOW)
#define PAD_CSPI1_SCLK	(HYS_ENABLE | DSE_HIGH)

	.macro ecspi1_iomux_dcd_data
	.word	0x53fa8468, PAD_CSPI1_MOSI	//SW_PAD_CTL_EIM_D18 - ECSPI1_MOSI
	.word	0x53fa8464, PAD_CSPI1_MISO	//SW_PAD_CTL_EIM_D17 - ECSPI1_MISO
	.word	0x53fa846c, PAD_CSPI1_SS1	//SW_PAD_CTL_EIM_D19 - ECSPI1_SS1
	.word	0x53fa8460, PAD_CSPI1_SCLK	//SW_PAD_CTL_EIM_D16 - ECSPI1_SCLK

	.word	0x53fa8120, ALT4		//SW_MUX_CTL_PAD_EIM_D18, alt4 ECSPI1_MOSI, alt1 gpio3[18]
	.word	0x53fa87a4, 3		//mosi input select
	.word	0x53fa811c, ALT4		//SW_MUX_CTL_PAD_EIM_D17, alt4 ECSPI1_MISO, alt1 gpio3[17]
	.word	0x53fa87a0, 3		//miso input select
	.word	0x53fa8124, ALT1		//SW_MUX_CTL_PAD_EIM_D19, alt4 ECSPI1_SS1, alt1 gpio3[19]
	.word	0x53fa87ac, 2		//ss1 input select
	.word	0x53fa8118, ALT4		//SW_MUX_CTL_PAD_EIM_D16, alt4 ECSPI1_SCLK, alt1 gpio3[16]
	.word	0x53fa879c, 3		//clk input select
	.word	0
	.endm

	.macro miso_gp_iomux_dcd_data
	.word	0x53fa811c, 1		//SW_MUX_CTL_PAD_EIM_D17, alt4 ECSPI1_MISO, alt1 gpio3[17]
	.word	0
	.endm

	.macro miso_ecspi_iomux_dcd_data
	.word	0x53fa811c, 4		//SW_MUX_CTL_PAD_EIM_D17, alt4 ECSPI1_MISO, alt1 gpio3[17]
	.word	0
	.endm

#define GP3_MISO_BIT	17
#define GP3_SS1_BIT	19

	.macro ecspi_gp_setup
	BigMov	r1, GPIO3_BASE
	ldr	r0, [r1, #GPIO_DR]
// make sure SS1 is high
	orr	r0, r0, #(1 << GP3_SS1_BIT)
	str	r0, [r1, #GPIO_DR]

	ldr	r0, [r1, #GPIO_DIR]
// make sure MISO is input, SS1 is output
	bic	r0, r0, #(1 << GP3_MISO_BIT)
	orr	r0, r0, #(1 << GP3_SS1_BIT)
	str	r0, [r1, #GPIO_DIR]
	.endm

	.macro ecspi_ss_make_active
	BigMov	r1, GPIO3_BASE
	ldr	r0, [r1, #GPIO_DR]
	bic	r0, r0, #(1 << GP3_SS1_BIT)	//make SS1 low
	str	r0, [r1, #GPIO_DR]
	.endm

	.macro ecspi_ss_make_inactive
	BigMov	r1, GPIO3_BASE
	ldr	r0, [r1, #GPIO_DR]
	orr	r0, r0, #(1 << GP3_SS1_BIT)	//make SS1 high
	str	r0, [r1, #GPIO_DR]
	.endm

	.macro ecspi_ss_get_miso
	BigMov	r1, GPIO3_BASE
	ldr	r0, [r1, #GPIO_PSR]
	and	r0, r0, #(1 << GP3_MISO_BIT)
	.endm

#define PAD_SD1_CMD	(KEEPER | PKE_ENABLE | DSE_HIGH | R75K_PU)	//reset val: 0x20dc, 47K pull up
#define PAD_SD1_CLK	(KEEPER | PKE_ENABLE | DSE_HIGH | R75K_PU)	//reset val: 0x20d4, 47K pull up
#define PAD_SD1_DATA0	(KEEPER | PKE_ENABLE | DSE_HIGH | R75K_PU)	//reset val: 0x20d4, 47K pull up
#define PAD_SD1_DATA1	(KEEPER | PKE_ENABLE | DSE_HIGH | R75K_PU)	//reset val: 0x20d4, 47K pull up
#define PAD_SD1_DATA2	(KEEPER | PKE_ENABLE | DSE_HIGH | R75K_PU)	//reset val: 0x20d4, 47K pull up
#define PAD_SD1_DATA3	(KEEPER | PKE_ENABLE | DSE_HIGH | R75K_PU)	//reset val: 0x20c4, 100K pull down
#define PAD_SD1_CD	(HYS_ENABLE | R100K_PU)	//reset val: 0x01a5, 100K pull up
#define PAD_SD1_WP	(HYS_ENABLE | R100K_PU)	//reset val: 0x01a5, 100K pull up
	.macro mmc_iomux_dcd_data
	.word	0x53fa866c, PAD_SD1_DATA0	//SW_PAD_CTL_SD1_DATA0
	.word	0x53fa8670, PAD_SD1_DATA1	//SW_PAD_CTL_SD1_DATA1
	.word	0x53fa8674, PAD_SD1_CMD		//SW_PAD_CTL_SD1_CMD
	.word	0x53fa8678, PAD_SD1_DATA2	//SW_PAD_CTL_SD1_DATA2
	.word	0x53fa867c, PAD_SD1_CLK		//SW_PAD_CTL_SD1_CLK
	.word	0x53fa8680, PAD_SD1_DATA3	//SW_PAD_CTL_SD1_DATA3

	.word	0x53fa82e4, 0			//SW_MUX_CTL_PAD_SD1_DATA0
	.word	0x53fa82e8, 0			//SW_MUX_CTL_PAD_SD1_DATA1
	.word	0x53fa82ec, ALT0|SION		//SW_MUX_CTL_PAD_SD1_CMD
	.word	0x53fa82f0, 0			//SW_MUX_CTL_PAD_SD1_DATA2
	.word	0x53fa82f4, 0			//SW_MUX_CTL_PAD_SD1_CLK
	.word	0x53fa82f8, 0			//SW_MUX_CTL_PAD_SD1_DATA3
	.endm

	.macro mmc_get_cd
	mov	r0,#0	//always card present
	.endm

#define HEADER_SEARCH		mx53_header_search
#define EXEC_PROGRAM		mx53_exec_program
#define EXEC_DL			mx53_exec_dl
#define HEADER_UPDATE_END	mx53_header_update_end

#define IVT_BARKER 0x402000d1
	.macro test_for_header
	BigMov	r1, IVT_BARKER
	ldr	r0, [r0, #4]
	cmp	r0, r1
	moveq	r0,#1
	movne	r0,#0
	.endm

#endif

