#include <config.h>
//256 meg is fine if we're just writing sdmmc to the SDCard
#define USE_CSD1	//512MB board uses CS0 and CS1, we will disable CS1 if ram doesn't appear to work

//HAB API Jump Table Addresses
#define HAB_CSF_CHECK		0x40	//Verify signature of memory ranges
//hab_result hab_csf_check(UINT8 csf_count, UINT32 *csf_list);
#define HAB_ASSERT_VERIFICATION	0x44	//Check that memory range is secured
//hab_result hab_asser_verification(UINT8 *block_start, UINT32 block_length);
#define PU_IROM_BOOT_DECISION	0x50	//Serial bootloader
//void pu_irom_boot_decision(void);

#define STACK_HIGH	0x1fffffb8
#define RAM_BASE	0x90000000
#define ECSPI1_BASE	0x70010000
#define MM_SDMMC_BASE	0x70004000
#define UART1_BASE	0x73fbc000
#define UART2_BASE	0x73fc0000
#define UART3_BASE	0x7000c000

#define UART_INIT_MASK	0x3	/* Initialize UARTS 1, 2 */
#define UART_TX_MASK	0x1	/* Initialize UARTS 1 */
#define UART_RX_MASK	0x1	/* Initialize UARTS 1 */
#define UART_DEF_INDEX	0

#define AIPS2_BASE_ADDR 0x83F00000

#define IPU_CM_REG_BASE		0x5E000000
#define IPU_IDMAC_BASE		0x5e008000

#define GPIO1_BASE	0x73f84000
#define GPIO4_BASE	0x73f90000
#define WDOG_BASE	0x73f98000
#define CCM_BASE	0x73fd4000
#define M4IF_BASE	0x83FD8000

#define I2C1_BASE_ADDR	(AIPS2_BASE_ADDR + 0x000C8000)

#define ESD_BASE	0x83fd9000
#define ESD_CTL0  0x000
#define ESD_CFG0  0x004
#define ESD_CTL1  0x008
#define ESD_CFG1  0x00c
#define ESD_MISC  0x010
#define ESD_SCR   0x014
#define ESD_DLY1  0x020
#define ESD_DLY2  0x024
#define ESD_DLY3  0x028
#define ESD_DLY4  0x02c
#define ESD_DLY5  0x030
#define ESD_GPR	  0x034

// AIPS Constants
#define CSP_BASE_REG_PA_AIPS1	0x73F00000
#define CSP_BASE_REG_PA_AIPS2	0x83F00000

#define EMRS1_DRIVE_STRENGTH		EMRS1_DRIVE_STRENGTH_REDUCED
#define EMRS1_ODT_TERM			EMRS1_ODT_TERM_OHM_150
#define SCR_EMRS1_DEFAULT		(((EMRS1_DRIVE_STRENGTH | EMRS1_ODT_TERM) << 16) | 0x8019)
//emrs(1) - buffers enabled, RDQS disable, DQS enable, 0 latency, DLL enable
#define SDCS1	4


/* Platform choices */
#define I2C_BASE	I2C1_BASE_ADDR
#define ECSPI_BASE	ECSPI1_BASE

#ifdef ASM
	.macro ddr_init
	BigMov  r1,CCM_BASE
	ldr     r0,[r1, #CCM_CBCDR]
	bic	r0,r0,#(7 << 27)	//clear ddr podf
	orr	r0,r0,#(1 << 30) | ((4 - 1) << 27)	//ddr podf div to 4, select pll1
	str     r0,[r1, #CCM_CBCDR]
	bl	Basic_Init_ESD
	.endm

	.macro ddr_get_size
	BigMov	r0, ESD_BASE
	ldr	r1, [r0, #ESD_MISC];
	and	r2, r1, #(1 << 6)
	mov	r2, r2, LSR #6		//bank bits

	ldr	r1, [r0, #ESD_CTL1];
	add	r2, r1, LSR #31		//chip select 1 bit

	ldr	r1, [r0, #ESD_CTL0];
	and	r0, r1, #(7 << 24)
	add	r2, r0, LSR #24		//rows
	and	r0, r1, #(3 << 20)
	add	r2, r0, LSR #20		//columns
	and	r0, r1, #(1 << 17)
	add	r2, r0, LSR #17		//memory width
	add	r2, #11+8+2+1		//11 more rows, 8 more column, 2 more banks, 1 more width
	mov	r0, #1
	mov	r0, r0, LSL r2
	.endm

	.macro ddr_get_tapeout
	mov	r0, #1
	.endm

	.equiv	IIM_BASE,	0x83f98000	//weird 0x53ffc000 in documentation
	.equiv	SRC_BASE,	0x73fd0000		//System reset controller
	.equiv	PREV_EXPECTED,	0xf2

	.equiv	I_FB,		0x0800		//bank 0, fuse 0-7
	.equiv	I_JTAG,		0x0804		//fuse 0x08-0x0f (8-15)
						//fuse 0x10-0x17 (16-23)
	.equiv	I_BT_SRC,	0x080c		//fuse 0x18-0x1f (24-31)
	.equiv	I_BT_HAB_TYPE,	0x0810		//fuse 0x20-0x27 (32-39)
	.equiv	I_BT_MEM_CTL,	0x0814		//fuse 0x28-0x2f (40-47) also has BT_MEM_TYPE(bits 4 & 5) BT_MEM_CTL(bits 1 & 2)
	.equiv	I_SJC_DISABLE,	0x0854
	.equiv	I_UID8,		0x081c

	.equiv	SBMR_BT_MEM_CTL,	0	//len 2,
	.equiv	SBMR_BT_BUS_WIDTH,	2
	.equiv	SBMR_BT_PAGE_SIZE,	3	//len 2
	.equiv	SBMR_BT_SPARE,		6
	.equiv	SBMR_BT_MEM_TYPE,	7	//len 2
	.equiv	SBMR_BT_BMOD,		14	//len 2


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


	.macro fuse_table
	fentry "PREV", I_PREV
	fentry "SREV", I_SREV
	fentry	"FB", I_FB			//0x0800 bank 0
	fentry	"JTAG", I_JTAG			//0x0804
	fentry	"BT_SRC", I_BT_SRC		//0x080c
	fentry	"BT_HAB_TYPE", I_BT_HAB_TYPE	//0x0810
	fentry	"BT_MEM_CTL..", I_BT_MEM_CTL	//0x0814

	fentry	"SJC_DISABLE", I_SJC_DISABLE	//0x0854
	fentry	"UID0", I_UID0			//0x083c
	fentry	"UID1", I_UID1			//0x0838
	fentry	"UID2", I_UID2			//0x0834
	fentry	"UID3", I_UID3			//0x0830
	fentry	"UID4", I_UID4			//0x082c
	fentry	"UID5", I_UID5			//0x0828
	fentry	"UID6", I_UID6			//0x0824
	fentry	"UID7", I_UID7			//0x0820
	fentry	"UID8", I_UID8			//0x081c

	fentry	"FBAC1", I_FBAC1		//0x0c00 bank 1, FUSE bank Access Protection register
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
	fBurn	"UID6 FUSE:", 0x24*2, 0
#ifdef ECSPI_BURN
#define F5_MASK (FMASK(FUSE05_BT_MEM_CTL) | FMASK(FUSE05_BT_MEM_CTL+1) | FMASK(FUSE05_BT_MEM_TYPE) | FMASK(FUSE05_BT_MEM_TYPE+1) | FMASK(FUSE05_BT_BUS_WIDTH)) // 3 byte address SPI device
#else
//FMASK(FUSE05_DIR_BT_DIS)
#define F5_MASK (FMASK(FUSE05_BT_MEM_CTL) | FMASK(FUSE05_BT_MEM_CTL+1))
#endif
	fBurn	"FUSE 40-47:", (FUSE05_BT_MEM_CTL & ~0x7), F5_MASK
	fBurn	"FUSE 32-39 (39 is virgin):", (FUSE04_VIRGIN & ~0x7), FMASK(FUSE04_VIRGIN)
	.byte	0
	.endm


	.macro header_macro start_rtn
//offset 0x400 for SD/MMC
header:
		.word	(\start_rtn)	//0x00 app_start_addr
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
	.endm

	.macro header_chain_ivt start_rtn
header2:
		.word	(\start_rtn)	//0x00 app_start_addr
		.word	0xb1		//0x04 app_barker
		.word	0x00		//0x08 csf_ptr
		.word	dcd_ptr2	//0x0c dcd_ptr_ptr
		.word	0		//0x10 srk_ptr
dcd_ptr2:	.word	dcd		//0x14 dcd_ptr
		.word	image_base	//0x18 app_dest_ptr
	.endm

	.macro plug_entry_setup
	mov	r0, #1
	mov	lr, #0
	.endm

#define HEADER_SEARCH		mx51_header_search
#define HEADER_GET_RTN		mx51_header_get_rtn
#define HEADER_UPDATE_END	mx51_header_update_end

#define APP_BARKER	0xb1
	.macro test_for_header
	ldr	r0, [r0, #4]
	cmp	r0, #APP_BARKER
	moveq	r0,#1
	movne	r0,#0
	.endm

	.macro iomux_dcd_data
	.word	0x73fa8418, 0x000000e0	//SW_PAD_CTL_PAD_EIM_D26, usb OTG power off
	.word	0x73fa8084, 0x00000001	//SW_MUX_CTL_PAD_EIM_D26, ALT1 kpp column 7, GPIO on next board
#if 1
//babbage tape out 2, 56 DCD entries
	.word	0x73fa88a0, 0x00000200	//SW_PAD_CTL_GRP_INMODE1, ddr2 input type
	.word	0x73fa850c, 0x000020c3	//SW_PAD_CTL_PAD_EIM_SDODT1, 100K Pull Down, medium drive strength
	.word	0x73fa8510, 0x000020c3	//SW_PAD_CTL_PAD_EIM_SDODT0, 100K Pull Down, medium drive strength
	.word	0x73fa883c, 0x00000003	//SW_PAD_CTL_GRP_DDR_A0, (a0-a7)Medium drive strength
	.word	0x73fa8848, 0x00000003	//SW_PAD_CTL_GRP_DDR_A1, (a8-a14,ba0-ba2)Medium drive strength
	.word	0x73fa84b8, 0x000000e3	//SW_PAD_CTL_PAD_DRAM_SDCLK1, medium drive strength
	.word	0x73fa84bc, 0x00000043	//SW_PAD_CTL_PAD_DRAM_SDQS0, Disable pull down
	.word	0x73fa84c0, 0x00000043	//SW_PAD_CTL_PAD_DRAM_SDQS1, Disable pull down
	.word	0x73fa84c4, 0x00000043	//SW_PAD_CTL_PAD_DRAM_SDQS2, Disable pull down
	.word	0x73fa84c8, 0x00000043	//SW_PAD_CTL_PAD_DRAM_SDQS3, Disable pull down
	.word	0x73fa8820, 0x00000000	//SW_PAD_CTL_GRP_DDRPKS, select keeper
	.word	0x73fa84a4, 0x00000003	//SW_PAD_CTL_PAD_DRAM_RAS, Medium Drive Strength
	.word	0x73fa84a8, 0x00000003	//SW_PAD_CTL_PAD_DRAM_CAS, Medium Drive Strength
	.word	0x73fa84ac, 0x000000e3	//SW_PAD_CTL_PAD_DRAM_SDWE, Medium Drive Strength
	.word	0x73fa84b0, 0x000000e3	//SW_PAD_CTL_PAD_DRAM_SDCKE0, Medium Drive Strength
	.word	0x73fa84b4, 0x000000e3	//SW_PAD_CTL_PAD_DRAM_SDCKE1, Medium Drive Strength
	.word	0x73fa84cc, 0x000000e3	//SW_PAD_CTL_PAD_DRAM_CS0, Medium Drive Strength
	.word	0x73fa84d0, 0x000000e3	//SW_PAD_CTL_PAD_DRAM_CS1, Medium Drive Strength
//serial downloader doesn't have the next 4
	.word	0x73fa882c, 0x00000002	//SW_PAD_CTL_GRP_DRAM_B4 (D24-D31), Medium drive strength
	.word	0x73fa88a4, 0x00000002	//SW_PAD_CTL_GRP_DRAM_B0 (D0-D7), Medium drive strength
	.word	0x73fa88ac, 0x00000002	//SW_PAD_CTL_GRP_DRAM_B1 (D8-D15, Medium drive strength
	.word	0x73fa88b8, 0x00000002	//SW_PAD_CTL_GRP_DRAM_B2 (D16-D23), Medium drive strength

	.word	0x73fa8228, 0		//SW_MUX_CTL_PAD_UART1_RXD
	.word	0x73fa822c, 0		//SW_MUX_CTL_PAD_UART1_TXD
	.word	0x73fa8618, 0x1c5	//SW_PAD_CTL_UART1_RXD
	.word	0x73fa861c, 0x1c5	//SW_PAD_CTL_UART1_TXD
	.word	0
#endif
	.endm

#define ESDM_ODT_NONE	0
#define ESDM_ODT_150	1
#define ESDM_ODT_75	2
#define ESDM_ODT_50	3
#define ESDM_ODT	ESDM_ODT_150
#define MAKE_ODT(a)	(((a << 6) | (a << 4) | (a << 2) | a) << 20)
#define ESDMISC_INIT_DEFAULT	(0x000ad0d0 | MAKE_ODT(ESDM_ODT))	//ODT_EN should not be set yet
#define ESDMISC_RUN_DEFAULT	(0x000af6d0 | MAKE_ODT(ESDM_ODT))	//enable ODT, MIF3
/*
 * Bit 31 - CS0_RDY
 * Bit 30 - CS1_RDY
 * Bit 29 - ODT_IDLE
 * Bit 28 - SDCLK_EXT
 * Bit 27-26 - DQ[31-24] resistor for odt (0 - off, 1 - 150 ohm, 2 - 75 ohm, 3 - 50 ohm)
 * Bit 25-24 - DQ[23-16]
 * Bit 23-22 - DQ[15-8]
 * Bit 21-20 - DQ[7-0]
 * Bit 19-16 - must be 1010 for Jedec standard compliant devices
 * Bit 15 - differential DQS enable
 * Bit 14 - auto_dll_pause enable
 * Bit 13 - odt enable
 * Bit 12 - Bank interleave enable
 * Bit 11 - Force measurement
 * Bit 10-9 - MIF3_MODE, 0 disable, 3 enable all
 * Bit 8-7 - read additional Latency(RALAT) 01 means 1 cycle delay
 * Bit 6 - 1 means 8 bank device
 * Bit 5 - Latency hiding disable
 * Bit 4 - DDR2 enable
 * Bit 3 - DDR enable
 * Bit 2 - reserved
 * Bit 1 - reset
 * Bit 0 - reserved
 */
	.macro esd_dcd_data
#if 1
	.word	ESD_BASE + ESD_CTL0, 0x82a20000	//13 row, 10 col, 32 bit width
#ifdef USE_CSD1
	.word	ESD_BASE + ESD_CTL1, 0x82a20000
#endif
	.word	ESD_BASE + ESD_MISC, ESDMISC_INIT_DEFAULT
	.word	ESD_BASE + ESD_CFG0, 0x333584ab
//Samsung K4T1G164Q[E/F]-BCE6000 - E6 means DDR2-667, tCK, CL=3 : 5 - 8 ns
//					K4T1G164QF-BCE6000
//tRFC(refresh to any command)		127.5 ns (26 clocks)	Bits 31-28: 3 = 26 clocks (130 ns)(ESDCTL0[23] is double tRFC)
//tXSR(exit self refresh)		137.5 ns (28 clocks)	Bits 27-24: 3 = 28 clocks (140 ns)
//tXP (exit power down to command)	2 clocks		Bits 23-21: 1 = 2 clocks (10 ns)
//tWTR(write to read command)		7.5 ns (2 clocks)	Bits 20:    1 = 2 clocks (10 ns)
//tRP (row precharge)			15 ns (3 clocks)	Bits 19-18: 1 = 3 clocks (15 ns)
//tMRD(load mode register)		2 clocks		Bits 17-16: 1 = 2 clocks (10 ns)
//tRAS(Active to precharge Command)	45 ns (9 clocks)	Bits 15-12: 8 = 9 clocks (45 ns), was 7 = 8 clocks (40ns)
//tRRD(Active Bank A to Active B)	10 ns (2 clocks)	Bits 11-10: 1 = 2 clocks (10 ns)
//tWR (write to precharge)		15 ns (3 clocks)	Bits 7:     1 = 3 clocks (15 ns)
//tRCD(row to columnn delay)		15 ns (3 clocks)	Bits 6-4:   2 = 3 clocks (15 ns)
//tRC(ACTIVE to ACTIVE, same bank)	60 ns (12 clocks)	Bits 3-0:   0xb - 12 clocks(60ns), was 0xa = 11 clocks (55 ns)

#ifdef USE_CSD1
	.word	ESD_BASE + ESD_CFG1, 0x333584ab
#endif
//CSD0 always used
	.word	ESD_BASE + ESD_SCR, 0x04008008	//PRECHARGE ALL
	.word	ESD_BASE + ESD_SCR, 0x0000801a	//emrs(2)
	.word	ESD_BASE + ESD_SCR, 0x0000801b	//emrs(3)
	.word	ESD_BASE + ESD_SCR, SCR_EMRS1_DEFAULT	//emrs(1) - 50 ohms ODT
	.word	ESD_BASE + ESD_SCR, 0x05328018	//MRS (load mode register)
	.word	ESD_BASE + ESD_SCR, 0x04008008	//PRECHARGE ALL
	.word	ESD_BASE + ESD_SCR, 0x00008010	//auto-refresh
	.word	ESD_BASE + ESD_SCR, 0x00008010	//auto-refresh
	.word	ESD_BASE + ESD_SCR, 0x06328018	//MRS (load mode register)
	.word	ESD_BASE + ESD_SCR, 0x03800000 | SCR_EMRS1_DEFAULT	//emrs(1) - calibrate
	.word	ESD_BASE + ESD_SCR, SCR_EMRS1_DEFAULT	//OCD calibration mode exit
	.word	ESD_BASE + ESD_SCR, 0x00008000	//nop
#ifdef USE_CSD1
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x04008008	//ESDSCR, CSD1 !!!!
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x0000801a	//emrs(2)
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x0000801b	//emrs(3)
	.word	ESD_BASE + ESD_SCR, SDCS1 | SCR_EMRS1_DEFAULT	//emrs(1) - 50 ohms ODT vs 0x0000801d
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x07328018	//MRS (load mode register)
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x04008008	//PRECHARGE ALL
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x00008010	//auto-refresh
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x00008010	//auto-refresh
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x06328018	//MRS (load mode register)
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x03800000 | SCR_EMRS1_DEFAULT	//emrs(1) - calibrate vs 0x0380801d
	.word	ESD_BASE + ESD_SCR, SDCS1 | SCR_EMRS1_DEFAULT	//OCD calibration mode exit
	.word	ESD_BASE + ESD_SCR, SDCS1 | 0x00008000	//nop
#endif
	.word	ESD_BASE + ESD_CTL0, 0xb2a20000	//refresh 4 rows each refresh clock
#ifdef USE_CSD1
	.word	ESD_BASE + ESD_CTL1, 0xb2a20000
#endif
	.word	ESD_BASE + ESD_MISC, ESDMISC_RUN_DEFAULT
	.word	ESD_BASE + ESD_DLY1, 0x00f48c00	// D0-D7 read delay
	.word	ESD_BASE + ESD_DLY2, 0x00f48c00	// D8-D15 read delay
	.word	ESD_BASE + ESD_DLY3, 0x00f48c00	// D16-D23 read delay
	.word	ESD_BASE + ESD_DLY4, 0x00f48c00	// D24-D31 read delay
	.word	ESD_BASE + ESD_DLY5, 0x00f48000	// D0-D31 write delay
	.word	ESD_BASE + ESD_GPR, 0x88000000	// DQS gating delays
	.word	ESD_BASE + ESD_SCR, 0x00000000	//ESDSCR, AXI address readies normal operation
	.word	0
//CSD0 DDR - 0x90000000 (256M), CSD1 DDR - 0xa0000000 (256M)
#endif
	.endm

	.macro config_uart_clocks
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CCGR1]		//index 3 & 4 is UART1
	bic	r0,r0,#0xf<<(3*2)
	str	r0,[r1, #CCM_CCGR1]

	ldr	r0,[r1, #CCM_CSCMR1]
	bic	r0,r0,#3<<24
	orr	r0,r0,#2<<24			//pll3
	str	r0,[r1, #CCM_CSCMR1]

	ldr	r0,[r1, #CCM_CSCDR1]
	bic	r0,r0,#0x3f
	orr	r0,r0,#0x21			// podf /2, pred /5
	str	r0,[r1, #CCM_CSCDR1]

	ldr	r0,[r1, #CCM_CCGR1]		//index 3 & 4 is UART1
	orr	r0,r0,#0xf<<(3*2)
	str	r0,[r1, #CCM_CCGR1]
	.endm

	.macro test_watchdog_reset
	BigMov	r1, SRC_BASE
	ldr	r0, [r1, #SRC_SRSR]
	tst	r0,#(1 << 4)	//watchdog
	.endm

	.macro test_memory_active
	BigMov	r1, ESD_BASE
	ldr	r0, [r1, #ESD_CTL0]
	tst	r0,#(1 << 31)	//sdcs0
	.endm

	.macro esd_con_req rESD
	BigMov	r0, 0x00008000		//CON_REQ
	str	r0, [\rESD, #ESD_SCR]
91:	ldr	r0, [\rESD, #ESD_SCR]
	tst	r0, #1<<14
	beq	91b
	.endm

	.macro	esd_zqcalibrate
	.endm

	.macro Turn_off_CS1
	BigMov	r3, ESD_BASE
	BigMov	r1, 0x00008000		//CON_REQ
	str	r1, [r3, #ESD_SCR]
91:	ldr	r1, [r3, #ESD_SCR]
	tst	r1, #1<<14
	beq	91b
#if 1
	BigMov	r1, ESDMISC_INIT_DEFAULT | 0x02 //ESDMISC
	str	r1, [r3, #ESD_MISC]	// soft reset
	mov	r0,#0x1
91:	subs	r0,r0,#1
	bne	91b
	BigEor2	r1, (ESDMISC_INIT_DEFAULT | 0x02) ^ ESDMISC_RUN_DEFAULT
	str	r1, [r3, #ESD_MISC]	// release reset
#endif
	BigMov	r1, 0x04008008		//PRECHARGE ALL
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
90:	ldr	r0, [r7], #4
	cmp	r0,#0
	beq	93f
	ldr	r1, [r7], #4
	cmp	r0, r4		//ESD_CTL1
	cmpne	r0, r5		//ESD_CFG1
	beq	90b
	cmp	r0, r6		//ESD_SCR??
	bne	92f
	tst	r1,#4
	bne	90b
92:
	mov	r2,#0x100000
91:	subs	r2,r2,#1
	bne	91b
	str	r1, [r0]
	b	90b
#else
	mov	r1, #0		//normal operations again
	str	r1, [r3, #ESD_SCR]
#endif
93:
	.endm

#define ALT0	0x0
#define ALT1	0x1
#define ALT3	0x3
#define SION	0x10	//0x10 means force input path of BGA contact

#define SRE_FAST	(0x1 << 0)
#define DSE_LOW		(0x0 << 1)
#define DSE_MEDIUM	(0x1 << 1)
#define DSE_HIGH	(0x2 << 1)
#define DSE_MAX		(0x3 << 1)
#define OPENDRAIN_ENABLE (0x1 << 3)
#define R100K_PD	(0x0 << 4)
#define R47K_PU		(0x1 << 4)
#define R100K_PU	(0x2 << 4)
#define R22K_PU		(0x3 << 4)
#define KEEPER		(0x0 << 6)
#define PULL		(0x1 << 6)
#define PKE_ENABLE	(0x1 << 7)
#define HYS_ENABLE	(0x1 << 8)
#define VOT_LOW		(0x0 << 13)
#define VOT_HIGH	(0x1 << 13)

#define PAD_CSPI1_MOSI	(HYS_ENABLE | PKE_ENABLE | KEEPER | R100K_PU | DSE_HIGH | SRE_FAST)
#define PAD_CSPI1_MISO	(HYS_ENABLE | PKE_ENABLE | KEEPER | DSE_HIGH | SRE_FAST)
#define PAD_CSPI1_SS0	(HYS_ENABLE | PKE_ENABLE | KEEPER | DSE_HIGH | SRE_FAST)
#define PAD_CSPI1_SS1	(HYS_ENABLE | PKE_ENABLE | KEEPER | DSE_HIGH | SRE_FAST)
#define PAD_CSPI1_RDY	(HYS_ENABLE | PKE_ENABLE | KEEPER | DSE_LOW | SRE_FAST)
#define PAD_CSPI1_SCLK	(HYS_ENABLE | PKE_ENABLE | KEEPER | R100K_PU | DSE_HIGH | SRE_FAST)

	.macro ecspi1_iomux_dcd_data
	.word	0x73fa8600, PAD_CSPI1_MOSI	//SW_PAD_CTL_CSPI1_MOSI
	.word	0x73fa8604, PAD_CSPI1_MISO	//SW_PAD_CTL_CSPI1_MISO
	.word	0x73fa8608, PAD_CSPI1_SS0	//SW_PAD_CTL_CSPI1_SS0
	.word	0x73fa860c, PAD_CSPI1_SS1	//SW_PAD_CTL_CSPI1_SS1
	.word	0x73fa8614, PAD_CSPI1_SCLK	//SW_PAD_CTL_CSPI1_SCLK

	.word	0x73fa8210, 0		//SW_MUX_CTL_PAD_CSPI1_MOSI, gpio4[22], alt 3
	.word	0x73fa8214, 0		//SW_MUX_CTL_PAD_CSPI1_MISO, gpio4[23], alt 3
	.word	0x73fa8218, ALT3	//SW_MUX_CTL_PAD_CSPI1_SS0, gpio4[24], alt 3
	.word	0x73fa821c, ALT3	//SW_MUX_CTL_PAD_CSPI1_SS1, gpio4[25], alt 3
	.word	0x73fa8224, 0		//SW_MUX_CTL_PAD_CSPI1_SCLK, gpio4[27], alt 3
	.word	0
	.endm

	.macro i2c1_iomux_dcd_data
	.word	0
	.endm

	.macro ddr_single_iomux_dcd_data
	.word	0x73fa84bc, 0x000000c3	//SW_PAD_CTL_PAD_DRAM_SDQS0, Enable pull down
	.word	0x73fa84c0, 0x000000c3	//SW_PAD_CTL_PAD_DRAM_SDQS1, Enable pull down
	.word	0x73fa84c4, 0x000000c3	//SW_PAD_CTL_PAD_DRAM_SDQS2, Enable pull down
	.word	0x73fa84c8, 0x000000c3	//SW_PAD_CTL_PAD_DRAM_SDQS3, Enable pull down
	.word	0
	.endm

	.macro ddr_differential_iomux_dcd_data
	.word	0x73fa84bc, 0x00000043	//SW_PAD_CTL_PAD_DRAM_SDQS0, Disable pull down
	.word	0x73fa84c0, 0x00000043	//SW_PAD_CTL_PAD_DRAM_SDQS1, Disable pull down
	.word	0x73fa84c4, 0x00000043	//SW_PAD_CTL_PAD_DRAM_SDQS2, Disable pull down
	.word	0x73fa84c8, 0x00000043	//SW_PAD_CTL_PAD_DRAM_SDQS3, Disable pull down
	.word	0
	.endm

	.macro miso_gp_iomux_dcd_data
	.word	0x73fa8214, ALT3	//SW_MUX_CTL_PAD_CSPI1_MISO, gpio4[23], alt 3
	.word	0
	.endm

	.macro miso_ecspi_iomux_dcd_data
	.word	0x73fa8214, 0		//SW_MUX_CTL_PAD_CSPI1_MISO, gpio4[23], alt 3
	.word	0
	.endm

	.macro init_gps
	.endm

#define GP4_MISO_BIT	23
#define GP4_SS0_BIT	24
#define GP4_SS1_BIT	25

	.macro ecspi_gp_setup
	BigMov	r1, GPIO4_BASE
	ldr	r0, [r1, #GPIO_DR]
// make sure SS0 is low(pmic), SS1 is high
	bic	r0, r0, #(1 << GP4_SS0_BIT)
	orr	r0, r0, #(1 << GP4_SS1_BIT)
	str	r0, [r1, #GPIO_DR]

	ldr	r0, [r1, #GPIO_DIR]
// make sure MISO is input, SS0/SS1 are output
	bic	r0, r0, #(1 << GP4_MISO_BIT)
	orr	r0, r0, #(1 << GP4_SS0_BIT) | (1 << GP4_SS1_BIT)
	str	r0, [r1, #GPIO_DIR]
	.endm

	.macro ecspi_ss_make_active
	BigMov	r1, GPIO4_BASE
	ldr	r0, [r1, #GPIO_DR]
	bic	r0, r0, #(1 << GP4_SS1_BIT)	//make SS1 low
	str	r0, [r1, #GPIO_DR]
	.endm

	.macro ecspi_ss_make_inactive
	BigMov	r1, GPIO4_BASE
	ldr	r0, [r1, #GPIO_DR]
	orr	r0, r0, #(1 << GP4_SS1_BIT)	//make SS1 high
	str	r0, [r1, #GPIO_DR]
	.endm

	.macro ecspi_ss_get_miso
	BigMov	r1, GPIO4_BASE
	ldr	r0, [r1, #GPIO_PSR]
	and	r0, r0, #(1 << GP4_MISO_BIT)
	.endm

#define MUX_SD1_CMD		ALT0|SION	//0x0 is default
#define MUX_SD1_CLK		ALT0	//0x0 is default
#define MUX_SD1_DATA0		ALT0	//0x0 is default
#define MUX_SD1_DATA1		ALT0	//0x0 is default
#define MUX_SD1_DATA2		ALT0	//0x0 is default
#define MUX_SD1_DATA3		ALT0	//0x0 is default
#define MUX_SD1_CD		ALT1|SION	//0x0 is default, GPIO1_0
#define MUX_SD1_WP		ALT1|SION	//0x0 is default, GPIO1_1


#define PAD_SD1_CMD	(KEEPER | PKE_ENABLE | DSE_HIGH | R47K_PU | SRE_FAST)	//reset val: 0x20dc, 47K pull up
#define PAD_SD1_CLK	(KEEPER | PKE_ENABLE | DSE_HIGH | R47K_PU | SRE_FAST)	//reset val: 0x20d4, 47K pull up
#define PAD_SD1_DATA0	(KEEPER | PKE_ENABLE | DSE_HIGH | R47K_PU | SRE_FAST)	//reset val: 0x20d4, 47K pull up
#define PAD_SD1_DATA1	(KEEPER | PKE_ENABLE | DSE_HIGH | R47K_PU | SRE_FAST)	//reset val: 0x20d4, 47K pull up
#define PAD_SD1_DATA2	(KEEPER | PKE_ENABLE | DSE_HIGH | R47K_PU | SRE_FAST)	//reset val: 0x20d4, 47K pull up
#define PAD_SD1_DATA3	(KEEPER | PKE_ENABLE | DSE_HIGH | R47K_PU | SRE_FAST)	//reset val: 0x20c4, 100K pull down
#define PAD_SD1_CD	(HYS_ENABLE | R100K_PU)	//reset val: 0x01a5, 100K pull up
#define PAD_SD1_WP	(HYS_ENABLE | R100K_PU)	//reset val: 0x01a5, 100K pull up
	.macro mmc_iomux_dcd_data
	.word	0x73fa879c, PAD_SD1_CMD		//SW_PAD_CTL_SD1_CMD
	.word	0x73fa87a0, PAD_SD1_CLK		//SW_PAD_CTL_SD1_CLK
	.word	0x73fa87a4, PAD_SD1_DATA0	//SW_PAD_CTL_SD1_DATA0
	.word	0x73fa87a8, PAD_SD1_DATA1	//SW_PAD_CTL_SD1_DATA1
	.word	0x73fa87ac, PAD_SD1_DATA2	//SW_PAD_CTL_SD1_DATA2
	.word	0x73fa87b0, PAD_SD1_DATA3	//SW_PAD_CTL_SD1_DATA3
	.word	0x73fa87b4, PAD_SD1_CD		//SW_PAD_CTL_SD1_CD
	.word	0x73fa87b8, PAD_SD1_WP		//SW_PAD_CTL_SD1_WP


	.word	0x73fa8394, MUX_SD1_CMD		//SW_MUX_CTL_PAD_SD1_CMD
	.word	0x73fa8398, MUX_SD1_CLK		//SW_MUX_CTL_PAD_SD1_CLK, this is default setting
	.word	0x73fa839c, MUX_SD1_DATA0	//SW_MUX_CTL_PAD_SD1_DATA0
	.word	0x73fa83a0, MUX_SD1_DATA1	//SW_MUX_CTL_PAD_SD1_DATA1
	.word	0x73fa83a4, MUX_SD1_DATA2	//SW_MUX_CTL_PAD_SD1_DATA2
	.word	0x73fa83a8, MUX_SD1_DATA3	//SW_MUX_CTL_PAD_SD1_DATA3
	.word	0x73fa83ac, MUX_SD1_CD		//SW_MUX_CTL_PAD_CD, GPIO1_0 is card detect
	.word	0x73fa83b0, MUX_SD1_WP		//SW_MUX_CTL_PAD_WP, GPIO1_1 is write protect
	.word	0
	.endm

	.macro mmc_get_cd
	BigMov	r1, GPIO4_BASE
	ldr	r0, [r1, #GPIO_DIR]
	bic	r0, r0, #3	//make sure CD & WP are inputs
	str	r0, [r1, #GPIO_DIR]
	ldr	r0, [r1, #GPIO_DR]
	and	r0, r0, #1
	.endm

#endif
