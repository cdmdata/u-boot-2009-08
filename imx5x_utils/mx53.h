#include <config.h>
//Micron
//#define DDR_TYPE	MT47H128M8CF_3			//512MB, 333 MHz
//#define DDR_TYPE	MT47H128M8CF_3_REV1		//512MB, 333 MHz
//#define DDR_TYPE	MT47H128M8CF_25E		//512MB, 400 MHz
//#define DDR_TYPE	MT47H128M8CF_25E_REV1		//512MB, 400 MHz
//#define DDR_TYPE	MT47H128M8CF_5			//512MB, 200 MHz
//#define DDR_TYPE	MT47H64M8CF_25E			//256MB

//Hynix
//#define DDR_TYPE	H5PS1G83EFR_S6C			//512MB, 400 MHz
#ifdef CONFIG_H5PS2G83AFR_S6
#define DDR_TYPE	H5PS2G83AFR_S6			//1GB
#endif
//#define DDR_TYPE	H5PS2G83AFR_S5			//1GB

#ifdef CONFIG_H5PS1G83EFR_S6C
#define DDR_TYPE	H5PS1G83EFR_S6C			//512MB, 400 MHz
#endif

#ifndef DDR_TYPE
#define DDR_TYPE	H5PS1G83EFR_S6C			//512MB, 400 MHz
#endif

#define PMIC_DA9053_ADDR 0x48
//#define USE_CSD1 1
#define TO2
#define MT47H128M8CF_3		1	//512MB, 333 MHz
#define MT47H128M8CF_3_REV1	2	//512MB, 333 MHz
#define MT47H128M8CF_25E	3	//512MB, 400 MHz
#define MT47H128M8CF_25E_REV1	4	//512MB, 400 MHz
#define MT47H128M8CF_5		5	//512MB, 200 MHz
#define H5PS2G83AFR_S6		6	//1GB
#define H5PS2G83AFR_S5		7	//1GB
#define MT47H64M8CF_25E		8	//256MB
#define H5PS1G83EFR_S6C		9	//512MB, 400 MHz, CL 6, tRCD 6, tRP 6

#define HAB_RVT_HDR			0x94
#define HAB_RVT_ENTRY			0x98
#define HAB_RVT_EXIT			0x9c
#define HAB_RVT_CHECK_TARGET		0xa0
#define HAB_RVT_AUTHENTICATE_IMAGE	0xa4
#define HAB_RVT_RUN_DCD			0xa8
#define HAB_RVT_RUN_CSF			0xac
#define HAB_RVT_ASSERT			0xb0
#define HAB_RVT_REPORT_EVENT		0xb4
#define HAB_RVT_REPORT_STATUS		0xb8
#define HAB_RVT_FAIL_SAFE_VECT		0xbc

#define CPU_2_BE_32(l) \
       ((((l) << 24) & 0xff000000) | \
	(((l) <<  8) & 0x00ff0000) | \
	(((l) >>  8) & 0x0000ff00) | \
	(((l) >> 24) & 0x000000ff))

#define MAKE_TAG(tag, len, v) CPU_2_BE_32(( ((tag) << 24) | ((len) << 8) | (v) ))
#define MXC_DCD_ITEM(addr, val)	.word CPU_2_BE_32(addr), CPU_2_BE_32(val)

#define AIPS1_BASE_ADDR		0x53F00000
#define AIPS2_BASE_ADDR		0x63F00000

#define STACK_HIGH	0xf805ff00
#define RAM_BASE	0x70000000
#define ECSPI1_BASE	0x50010000
#define MM_SDMMC_BASE	0x50004000
#define UART1_BASE	(AIPS1_BASE_ADDR + 0x000bc000)
#define UART2_BASE	(AIPS1_BASE_ADDR + 0x000c0000)
#define UART3_BASE	0x5000c000

#define UART_INIT_MASK	0x7	/* Initialize UARTS 1, 2, 3 */

#ifdef CONFIG_UART_TX_MASK
#define UART_TX_MASK	CONFIG_UART_TX_MASK
#else
#define UART_TX_MASK	0x7	/* Tx UARTS 1, 2, 3 */
#endif

#ifdef CONFIG_UART_RX_MASK
#define UART_RX_MASK	CONFIG_UART_RX_MASK
#else
#define UART_RX_MASK	0x7	/* Rx UARTS 1, 2, 3 */
#endif

#ifdef CONFIG_UART_BASE_ADDR
#if CONFIG_UART_BASE_ADDR == UART1_BASE
#define UART_DEF_INDEX	0
#endif
#if CONFIG_UART_BASE_ADDR == UART2_BASE
#define UART_DEF_INDEX	1
#endif
#if CONFIG_UART_BASE_ADDR == UART3_BASE
#define UART_DEF_INDEX	2
#endif

#else
#define UART_DEF_INDEX	1
#endif

#define IPU_CM_REG_BASE		0x1E000000
#define IPU_IDMAC_BASE		0x1e008000

#define PLL4_BASE_ADDR		(AIPS2_BASE_ADDR + 0x0008C000)
#define M4IF_BASE		(AIPS2_BASE_ADDR + 0x000D8000)


#define GPIO1_BASE	(AIPS1_BASE_ADDR + 0x00084000)
#define GPIO2_BASE	(AIPS1_BASE_ADDR + 0x00088000)
#define GPIO3_BASE	(AIPS1_BASE_ADDR + 0x0008c000)
#define GPIO4_BASE	(AIPS1_BASE_ADDR + 0x00090000)
#define WDOG_BASE	(AIPS1_BASE_ADDR + 0x00098000)
#define CCM_BASE	(AIPS1_BASE_ADDR + 0x000d4000)
#define I2C1_BASE_ADDR  (AIPS2_BASE_ADDR + 0x000C8000)

#define ESD_BASE	(AIPS2_BASE_ADDR + 0x000d9000)
#define ESD_CTL		0x000
#define ESD_PDC		0x004
#define ESD_OTC		0x008
#define ESD_CFG0	0x00c
#define ESD_CFG1	0x010
#define ESD_CFG2	0x014
#define ESD_MISC	0x018
#define ESD_SCR		0x01c
#define ESD_REF		0x020
#define ESD_WCC		0x024
#define ESD_RCC		0x028
#define ESD_RWD		0x02c
#define ESD_OR		0x030
#define ESD_MRR		0x034
#define ESD_CFG3_LP	0x038
#define ESD_MR4		0x03c
#define ESD_ZQHWCTRL	0x040
#define ESD_ZQSWCTRL	0x044
#define ESD_WLGCR	0x048
#define ESD_WLDECTRL0	0x04c
#define ESD_WLDECTRL1	0x050
#define ESD_WLDLST	0x054
#define ESD_ODTCTRL	0x058
#define ESD_RDDQDLBY0	0x05c
#define ESD_RDDQDLBY1	0x060
#define ESD_RDDQDLBY2	0x064
#define ESD_RDDQDLBY3	0x068
#define ESD_WRDQDLBY0	0x06c
#define ESD_WRDQDLBY1	0x070
#define ESD_WRDQDLBY2	0x074
#define ESD_WRDQDLBY3	0x078
#define ESD_DGCTRL0	0x07c
#define ESD_DGCTRL1	0x080
#define ESD_DGDLST	0x084
#define ESD_RDDLCTL	0x088
#define ESD_RDDLST	0x08c
#define ESD_WRDLCTL	0x090
#define ESD_WRDLST	0x094
#define ESD_SDCTRL	0x098
#define ESD_ZQLP2CTL	0x09c
#define ESD_RDDLHWCTL	0x0a0
#define ESD_WRDLHWCTL	0x0a4
#define ESD_RDDLHWST0	0x0a8
#define ESD_RDDLHWST1	0x0ac
#define ESD_WRDLHWST0	0x0b0
#define ESD_WRDLHWST1	0x0b4
#define ESD_WLHWERR	0x0b8
#define ESD_DGHWST0	0x0bc
#define ESD_DGHWST1	0x0c0
#define ESD_DGHWST2	0x0c4
#define ESD_DGHWST3	0x0c8
#define ESD_PDCMPR1	0x0cc
#define ESD_PDCMPR2	0x0d0
#define ESD_SWDAR	0x0d4
#define ESD_SWDRDR0	0x0d8
#define ESD_SWDRDR1	0x0dc
#define ESD_SWDRDR2	0x0e0
#define ESD_SWDRDR3	0x0e4
#define ESD_SWDRDR4	0x0e8
#define ESD_SWDRDR5	0x0ec
#define ESD_SWDRDR6	0x0f0
#define ESD_SWDRDR7	0x0f4
#define ESD_MUR		0x0f8
#define ESD_WRCADL	0x0fc

#define IOMUXC_BASE_ADDR (AIPS1_BASE_ADDR + 0x000A8000)

// AIPS Constants
#define CSP_BASE_REG_PA_AIPS1	AIPS1_BASE_ADDR
#define CSP_BASE_REG_PA_AIPS2	AIPS2_BASE_ADDR

#define EMRS1_DRIVE_STRENGTH		EMRS1_DRIVE_STRENGTH_REDUCED
#define EMRS1_ODT_TERM			EMRS1_ODT_TERM_OHM_75
#define SCR_EMRS1_DEFAULT		(((EMRS1_DRIVE_STRENGTH | EMRS1_ODT_TERM) << 16) | 0x8031)
//emrs(1) - buffers enabled, RDQS disable, DQS enable, 0 latency, DLL enable

//ESD_SCR bits
#define DLL_RST0  (1 << 7)
#define DLL_RST1  (1 << 8)
#define SDCS0	0
#define SDCS1	8

/* Platform choices */
#define I2C_BASE	I2C1_BASE_ADDR
#define ECSPI_BASE	ECSPI1_BASE

#ifdef ASM
/* (4 bits) ddr type
 * (1 bit)tapeout - 1 or 2
 * ddr_freq - 200, 336 (more efficient than 333), 400
 * (1 bit)banks_bits - 2 (4 banks), or 3 (8 banks)
 * (3 bits)row_bits - 11-16
 * (2 bits)column_bits  - 9, 10, 11
 * (2 bits)rl - CAS read  latency 3-6
 * (2 bits)wl - CAS write latency 2-5
 * (3 bits)wr - write recovery for autoprecharge 1-6,  ddr2-400(2-3), ddr2-533(2-4), ddr2-667(2-5), ddr2-800(2-6)
 * (3 bits)rcd - 1-6
 * (3 bits)rp - 1-6
 * dgctrl0, dgctrl1, rddlctl, wrdlctl
 */
	.equiv	ddr_type_offs, 0x0
	.equiv	ddr_dp_op_offs, 0x3
	.equiv	ddr_dp_ctl_offs, 0x4
	.equiv	ddr_dp_mfd_offs, 0x6
	.equiv	ddr_dp_mfn_offs, 0x7
	.equiv	ddr_dgctrl0_offs, 0x8
	.equiv	ddr_dgctrl1_offs, 0xc
	.equiv	ddr_rddlctl_offs, 0x10
	.equiv	ddr_wrdlctl_offs, 0x14
	.equiv	ddr_data_size, 0x18

	.macro ddr_type	type, tapeout, ddr_freq, bank_bits, row_bits, column_bits, rl, wl, wr, rcd, rp, dgctrl0, dgctrl1, rddlctl, wrdlctl
	.word	(\type) | ((\tapeout - 1) << 4) | ((\bank_bits - 2) << 5) | ((\row_bits - 11) << 6) | ((\column_bits - 9) << 9) | ((\rl - 3) << 11) | ((\wl - 2) << 13) | ((\wr - 1) << 15) | ((\rcd - 1) << 18) | ((\rp - 1) << 21) | ((_DP_OP_\ddr_freq) << 24)
	.word	(_DP_MFN_\ddr_freq << 24) | (_DP_MFD_\ddr_freq << 16) | _DP_CTL_\ddr_freq
	.word	\dgctrl0
	.word	\dgctrl1
	.word	\rddlctl
	.word	\wrdlctl
	.endm

	.macro ddr_field rResult, result_bit, rfields, fstart, flen, fadd
	and	r0, \rfields, #(( 1 << \flen) - 1) << \fstart
	BigAdd2	r0, ((\fadd) << \fstart)
	.if (\result_bit > \fstart)
		orr	\rResult, \rResult, r0, LSL #\result_bit - \fstart
	.else
		orr	\rResult, \rResult, r0, LSR #\fstart - \result_bit
	.endif
	.endm

	.macro ddr_bank rResult, result_bit, rfields, fadd
	ddr_field \rResult, \result_bit, \rfields, 5, 1, (2 + \fadd)
	.endm
	.macro ddr_row rResult, result_bit, rfields, fadd
	ddr_field \rResult, \result_bit, \rfields, 6, 3, (11 + \fadd)
	.endm
	.macro ddr_column rResult, result_bit, rfields, fadd
	ddr_field \rResult, \result_bit, \rfields, 9, 2, (9 + \fadd)
	.endm
	.macro ddr_rl rResult, result_bit, rfields, fadd
	ddr_field \rResult, \result_bit, \rfields, 11, 2, (3 + \fadd)
	.endm
	.macro ddr_wl rResult, result_bit, rfields, fadd
	ddr_field \rResult, \result_bit, \rfields, 13, 2, (2 + \fadd)
	.endm
	.macro ddr_wr rResult, result_bit, rfields, fadd
	ddr_field \rResult, \result_bit, \rfields, 15, 3, (1 + \fadd)
	.endm
	.macro ddr_rcd rResult, result_bit, rfields, fadd
	ddr_field \rResult, \result_bit, \rfields, 18, 3, (1 + \fadd)
	.endm
	.macro ddr_rp rResult, result_bit, rfields, fadd
	ddr_field \rResult, \result_bit, \rfields, 21, 3, (1 + \fadd)
	.endm

	.macro ddr_data
//Micron Memory
	//MT47H128M8CF_3  DDR2-667(333MHz), tck=3ns(333Mhz), CL=5 wr=15 ns, rcd=15 ns, rp=15 ns
	//512MB = 3 bank bits(8 banks) + 14 row bits + 10 column bits + 2 bits(32 bit width) = 29 bits
	//		ddr type,	      to,freq, b,  r,  c,rl,wl,wr,rcd,rp,   dgctrl0,    dgctrl1,    rddlctl,    wrdlctl
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x015b015d, 0x01630163, 0x24242426, 0x534b5549	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x014a014a, 0x0148014c, 0x2a282a28, 0x4e4a5047	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x014b014f, 0x01520134, 0x29292929, 0x524d5248	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x0141013d, 0x01460149, 0x2a282a2a, 0x4d475143	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x0148014c, 0x014c0130, 0x2a2a2c28, 0x514b5249	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x014f014f, 0x01520132, 0x29292929, 0x524d5248	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x014e0151, 0x01510133, 0x28282828, 0x514c5348	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x0141013f, 0x01480149, 0x2a262a2a, 0x4f475345	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x01450145, 0x01480145, 0x2c2a2c28, 0x4f4b5245	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x014c0132, 0x015a0156, 0x29292929, 0x514b5349	//v
//	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x014b0132, 0x01520156, 0x2b252b2b, 0x514b544b	//v 2e:7b
//	ddr_type	MT47H128M8CF_3,        2, 333, 3, 14, 10, 5, 4, 5, 5, 5, 0x01490131, 0x01500156, 0x29272b2b, 0x4f4c534a	//v 2e:7b
	ddr_type	MT47H128M8CF_3,        2, 336, 3, 14, 10, 5, 4, 6, 5, 5, 0x01490131, 0x01520156, 0x29262b29, 0x4f4c534a	//v 2e:7b
	ddr_type	MT47H128M8CF_3,        1, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x0127012b, 0x0127012c, 0x282a2c2c, 0x42403b3b	//v
	ddr_type	MT47H128M8CF_3_REV1,   1, 336, 3, 14, 10, 5, 4, 5, 5, 5, 0x0165021a, 0x0154015b, 0x27292930, 0x5a4b615c

	//MT47H128M8CF_25E tck=2.5ns(400Mhz), CL=5, wr=15 ns, rcd=12.5 ns, rp=12.5 ns
	//		ddr type,	      to,freq, b,  r,  c,rl,wl,wr,rcd,rp,   dgctrl0,    dgctrl1,    rddlctl,    wrdlctl
//	ddr_type	MT47H128M8CF_25E,      2, 400, 3, 14, 10, 5, 4, 6, 5, 5, 0x01730205, 0x020a020a, 0x201e1e20, 0x57525f4d //v
//	ddr_type	MT47H128M8CF_25E,      2, 400, 3, 14, 10, 5, 4, 6, 5, 5, 0x016c016a, 0x013e013e, 0x23252523, 0x534e554a //v
	ddr_type	MT47H128M8CF_25E,      2, 400, 3, 14, 10, 5, 4, 6, 5, 5, 0x016e013d, 0x01750179, 0x24222626, 0x534e574c //v 2e:7b
	ddr_type	MT47H128M8CF_25E,      1, 400, 3, 14, 10, 5, 4, 6, 5, 5, 0x01780131, 0x0139017d, 0x27292930, 0x57504949
	ddr_type	MT47H128M8CF_25E_REV1, 1, 400, 3, 14, 10, 5, 4, 6, 5, 5, 0x013a013c, 0x013a0145, 0x34313634, 0x433e3e3e

	//MT47H128M8CF_5 tck=5ns(200Mhz), CL=3, wr=15 ns, rcd=15 ns, rp=15 ns
	//		ddr type,	      to,freq, b,  r,  c,rl,wl,wr,rcd,rp,   dgctrl0,    dgctrl1,    rddlctl,    wrdlctl
//	ddr_type	MT47H128M8CF_5,        2, 200, 3, 14, 10, 3, 2, 3, 3, 3, 0x01190117, 0x01160117, 0x2e2d2c2c, 0x5151584a
	ddr_type	MT47H128M8CF_5,        2, 200, 3, 14, 10, 3, 2, 3, 3, 3, 0x01100111, 0x01140113, 0x34343434, 0x4b484c46	//v
	ddr_type	MT47H128M8CF_5,        1, 200, 3, 14, 10, 3, 2, 3, 3, 3, 0x01190117, 0x01160117, 0x2e2d2c2c, 0x5151584a

	//MT47H64M8CF_25E, tck=2.5 ns(400Mhz), CL=5, WR=15ns, tRCD=12.5ns, tRP=12.5ns
	//256MB = 2 bank bits(4 banks) + 14 row bits + 10 column bits + 2 bits(32 bit width) = 28 bits
	//		ddr type,	      to,freq, b,  r,  c,rl,wl,wr,rcd,rp,   dgctrl0,    dgctrl1,    rddlctl,    wrdlctl
	ddr_type	MT47H64M8CF_25E,       2, 400, 2, 14, 10, 5, 4, 6, 5, 5, 0x01460204, 0x0146014a, 0x1f1c211f, 0x4f485646

//Hynix Memory
	//H5PS2G83AFR_S6  DDR2-800(400Mhz), RL=6, WR=15ns, tRCD=6, tRP=6
	//1GB   = 3 bank bits(8 banks) + 15 row bits + 10 column bits + 2 bits(32 bit width) = 30 bits
	//		ddr type,	      to,freq, b,  r,  c,rl,wl,wr,rcd,rp,   dgctrl0,    dgctrl1,    rddlctl,    wrdlctl
						//dgctrl0,    dgctrl1,    rddlctl,    wrdlctl
#ifndef CONFIG_H5PS2G83AFR_S6_CALIBRATION
#define CONFIG_H5PS2G83AFR_S6_CALIBRATION	0x0173017b, 0x01400200, 0x1f1d221f, 0x504b5546
#endif
	ddr_type	H5PS2G83AFR_S6,        2, 400, 3, 15, 10, 6, 5, 6, 6, 6, CONFIG_H5PS2G83AFR_S6_CALIBRATION

	//H5PS2G83AFR_S5  DDR2-800(400Mhz), RL=5, WR=15ns, tRCD=5, tRP=5
	//1GB   = 3 bank bits(8 banks) + 15 row bits + 10 column bits + 2 bits(32 bit width) = 30 bits
	//		ddr type,	      to,freq, b,  r,  c,rl,wl,wr,rcd,rp,   dgctrl0,    dgctrl1,    rddlctl,    wrdlctl
	ddr_type	H5PS2G83AFR_S5,        2, 400, 3, 15, 10, 5, 4, 6, 5, 5, 0x0173017b, 0x01400200, 0x1f1d221f, 0x504b5546

	//H5PS1G83EFR_S6C DDR2-800(400Mhz), CL=6, tRCD=6, tRP=6
	//512MB = 3 bank bits(8 banks) + 14 Row bits, 10 Column bits, + 2 bits(32 bit width) = 29 bits
	//		ddr type,	      to,freq, b,  r,  c,rl,wl,wr,rcd,rp,   dgctrl0,    dgctrl1,    rddlctl,    wrdlctl
#ifndef CONFIG_H5PS1G83EFR_S6C_CALIBRATION
#define CONFIG_H5PS1G83EFR_S6C_CALIBRATION	0x01770172, 0x0177017b, 0x25232523, 0x5250564b
#endif
	ddr_type	H5PS1G83EFR_S6C,       2, 400, 3, 14, 10, 6, 5, 6, 6, 6, CONFIG_H5PS1G83EFR_S6C_CALIBRATION //v 2e:84

	.word		0
	.endm

	.macro ddr_find_data rFind, rSearch, rResult
	BigMov	r0, ESD_BASE
	ldr	r0, [r0, #ESD_ZQHWCTRL]
	tst	r0, #1 << 31
	orrne	\rFind, \rFind, #0x10		//tapeout 2
	mov	\rResult, \rSearch
	b	98f
97:	eors	r0, r0, \rFind
	moveq	\rResult, \rSearch
	beq	99f
	cmp	r0, #0x10
	moveq	\rResult, \rSearch	//use other tapeout setting
	add	\rSearch, \rSearch, #ddr_data_size
98:
	ldr	r0, [\rSearch, #ddr_type_offs]
	ands	r0, r0, #0x1f
	bne	97b
99:
	.endm

	.macro ddr_pll r_pll, r_ddr
	ldrb r0, [\r_ddr, #ddr_dp_op_offs]
	str r0, [\r_pll, #PLL_DP_OP]
	str r0, [\r_pll, #PLL_DP_HFS_OP]

	ldrb r0, [\r_ddr, #ddr_dp_mfd_offs]
	str r0, [\r_pll, #PLL_DP_MFD]
	str r0, [\r_pll, #PLL_DP_HFS_MFD]

	ldrb r0, [\r_ddr, #ddr_dp_mfn_offs]
	str r0, [\r_pll, #PLL_DP_MFN]
	str r0, [\r_pll, #PLL_DP_HFS_MFN]
	.endm

	.macro ddr_start_pll r_pll, r_ddr
	ldrh r0, [\r_ddr, #ddr_dp_ctl_offs]
	str r0, [\r_pll, #PLL_DP_CTL] /* Set DPLL ON (set UPEN bit) */
1:	ldr r0, [\r_pll, #PLL_DP_CTL]
	tst r0, #0x1
	beq 1b
	.endm

	.macro esd_con_req rESD
	BigMov	r0, 0x00008000		//CON_REQ
	str	r0, [\rESD, #ESD_SCR]
91:	ldr	r0, [\rESD, #ESD_SCR]
	tst	r0, #1<<14
	beq	91b
	.endm

#ifdef USE_CSD1
#define CS_ENABLES (3 << 30)
#else
#define CS_ENABLES (2 << 30)
#endif
#define RALAT 4	//was 3

	.macro ddr2_lmr_init	sdcs, dll_rst
	ldr	r1, =(\sdcs | 0x04008010)
	str	r1, [r4, #ESD_SCR]
	BigMov	r1, (\sdcs | 0x00008032)
	str	r1, [r4, #ESD_SCR]
	orr	r1, r1, #1		//0x8033/0x803b
	str	r1, [r4, #ESD_SCR]
	bic	r1, r1, #2		//0x8031/0x8039
	str	r1, [r4, #ESD_SCR]
	ldr	r2, =(\sdcs | 0x00028030)
	ddr_wr	r2, 25, r5, -1
	ddr_rl	r2, 20, r5, 0
	BigOrr	r0, r2, ((MRS_DLL_RESET << 16) | \dll_rst)
	str	r0, [r4, #ESD_SCR]
	ldr	r1, =(\sdcs | 0x04008010)	//PRECHARGE ALL CS0
	str	r1, [r4, #ESD_SCR]
	BigMov	r1, (\sdcs | 0x00008020)
	str	r1, [r4, #ESD_SCR]
	str	r1, [r4, #ESD_SCR]
	str	r2, [r4, #ESD_SCR]

	ldr	r1, =(\sdcs | SCR_EMRS1_DEFAULT)
	orr	r0, r1, #0x03800000
	str	r0, [r4, #ESD_SCR]	/* ESD_SCR, EMR1: OCD Calibration */
	str	r1, [r4, #ESD_SCR]	/* ESD_SCR, EMR1: OCD Cal exit */
	.endm

	.macro ddr_init
	bl	get_ddr_type_addr	//r2 gets address
	ldr		r1, [r2], #4
	ddr_find_data	r1, r2, r3

//Disable SDRAM autogating
	BigMov	r1, M4IF_BASE
	ldr	r0, [r1, #M4IF_CNTL_REG0]
	orr	r0, r0, #4
	str	r0, [r1, #M4IF_CNTL_REG0]

//disable PLL auto-restart
	BigMov	r2,PLL2_BASE_ADDR
	BigMov	r1,CCM_BASE
	mov	r0, #0
	str	r0, [r2, #PLL_DP_CONFIG]

//Gate IPU clocks
	ldr	r0, [r1, #CCM_CCGR6]
	bic	r0, r0, #(0xF << (5*2))		//Disable IPU di0/di1 clocks
	str	r0, [r1, #CCM_CCGR6]

	ldr	r0, [r1, #CCM_CCGR5]
	bic	r0, r0, #(0x3 << (5*2))		//Disable IPU clock
	str	r0, [r1, #CCM_CCGR5]
//Disable HSC clocks
	ldr	r0, [r1, #CCM_CCGR4]
	bic	r0, r0, #(0x0f << (1*2))	//Disable sim (index 1,2)
	bic	r0, r0, #(0xff << (3*2))	//Disable HSC (index 3-6)
	bic	r0, r0, #(0xff << (9*2))	//Disable ecspi1/2 (index 9-12)
	str	r0, [r1, #CCM_CCGR4]

// Disable CCM-HSC/IPU handshake on clock mux switching
	mov	r0, #0x00060000
	str	r0, [r1, #CCM_CCDR]
#if 1
	ldr	r0, [r1, #CCM_CLPCR]
	orr	r0, r0, #(1 << 18)	//Bypass IPU handshake
	str	r0, [r1, #CCM_CLPCR]
#endif

	BigMov	r4, ESD_BASE
	esd_con_req r4
	ldr	r0, [r4, #ESD_MISC]
	orr	r0, r0, #2
	str	r0, [r4, #ESD_MISC]	//software reset

	mov	r0, #0
	str	r0, [r4, #ESD_ODTCTRL]

	ddr_pll		r2, r3
	ddr_start_pll	r2, r3

	ldr	r5, [r3, #ddr_type_offs]

	ldr	r0, [r3, #ddr_dgctrl0_offs]
	str	r0, [r4, #ESD_DGCTRL0]
	ldr	r0, [r3, #ddr_dgctrl1_offs]
	str	r0, [r4, #ESD_DGCTRL1]
	ldr	r0, [r3, #ddr_rddlctl_offs]
	str	r0, [r4, #ESD_RDDLCTL]
	ldr	r0, [r3, #ddr_wrdlctl_offs]
	str	r0, [r4, #ESD_WRDLCTL]

	mov	r0, #0	/* was 0x0f00, add 3 logic unit of delay to sdclk */
	str	r0, [r4, #ESD_SDCTRL]
	mov	r0, #0x00000800
	str	r0, [r4, #ESD_MUR]	//force remeasure

	mov	r1, #0
	ddr_bank r1, 5, r5, -1
	and	r1, r1, #1 << 5		//(0)8 banks
	BigOrr2	r1, (0x00001610 | (RALAT << 6))
	str	r1, [r4, #ESD_MISC]

/* 0x84110000, 0xc4110000 ESDCTL, (4)15 rows, (1)10 columns (0)BL 4,  (1)32 bit width*/
/* 0x83110000, 0xc3110000 ESDCTL, (3)14 rows, (1)10 columns (0)BL 4,  (1)32 bit width*/
	BigMov	r1, (0x00010000 | CS_ENABLES)
	ddr_row	r1, 24, r5, -11
	ddr_column r1, 20, r5, -9
	str	r1, [r4, #ESD_CTL]

	ldr	r1, =0x4d5122d0
	ddr_rl	r1, 0, r5, -3
	str	r1, [r4, #ESD_CFG0]
	ldr	r1, =0x02d18a20		//tRC=10110=0x16= 23 cycles * 2.5ns = 57.5 ns
	ddr_rcd r1, 29, r5, -1
	ddr_rp  r1, 26, r5, -1
	ddr_wl  r1, 0, r5, -2
	str	r1, [r4, #ESD_CFG1]
	BigMov	r1, 0x00c70092
	str	r1, [r4, #ESD_CFG2]
	BigMov	r1, 0x000026d2
	str	r1, [r4, #ESD_RWD]
	BigMov	r1, 0x009f000e
	str	r1, [r4, #ESD_OR]

	ldr	r1, =0x12272000
	str	r1, [r4, #ESD_OTC]
	BigMov	r1, 0x00030012
	str	r1, [r4, #ESD_PDC]

	ddr2_lmr_init	SDCS0, DLL_RST0
#ifdef USE_CSD1
	ddr2_lmr_init	SDCS1, DLL_RST1
#endif

	mov	r1, #0
	ddr_row r1, 0, r5, -13
	mov	r2, #0x00004000
	cmp	r1, #3
	movls	r0, #1
	movls	r0, r0, LSL r1
	subls	r0, r0, #1
	orrls	r2, r2, r0, LSL #11	//orr if rows is 13-16
	str	r2, [r4, #ESD_REF] /* ESDREF, (1)32k refresh cycle rate, n+1 refreshes/cycle */
	BigMov	r1, 0x00033337
	str	r1, [r4, #ESD_ODTCTRL]
	mov	r1, #0
	str	r1, [r4, #ESD_SCR]		/* ESD_SCR: AXI address readies normal operation */
	.endm

	.macro ddr_get_size
	BigMov	r0, ESD_BASE
	ldr	r1, [r0, #ESD_MISC];
	mvn	r2, r1, LSR #5
	and	r2, r2, #1		//bank bits

	ldr	r1, [r0, #ESD_CTL];
	and	r0, r1, #(1 << 30)
	add	r2, r0, LSR #30		//chip select 1 bit
	and	r0, r1, #(7 << 24)
	add	r2, r0, LSR #24		//rows
	and	r0, r1, #(7 << 20)
	add	r2, r0, LSR #20		//columns
	and	r0, r1, #(1 << 16)
	add	r2, r0, LSR #16		//memory width
	add	r2, #11+9+2+1		//11 more rows, 9 more column, 2 more banks, 1 more width
	mov	r0, #1
	mov	r0, r0, LSL r2
	.endm

	.macro ddr_get_tapeout
	BigMov	r0, ESD_BASE
	ldr	r0, [r0, #ESD_ZQHWCTRL]
	mov	r0, r0, LSR #31
	add	r0, r0, #1
	.endm
////////////////////////////
// fuse defines
////
	.equiv	IIM_BASE,	0x63f98000	//weird 0x53ffc000 in documentation
	.equiv	SRC_BASE,	0x53fd0000	//System reset controller

	.equiv	PREV_EXPECTED,	0xfa

	.equiv	I_SI_REV,	0x081c		//fuse 0x38-0x3f (56-63)

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
	.macro header_macro plug_rtn
//offset 0x400 for SD/MMC
ivt_header:		.word MAKE_TAG(0xD1, 0x20, 0x40) /* 0x402000D1, Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:	.word mfgtool_start
reserv1:		.word 0x0
dcd_ptr:		.word 0x0
boot_data_ptr:		.word boot_data
self_ptr:		.word ivt_header
app_code_csf:		.word 0x0
reserv2:		.word 0x0

mfgtool_start:		b	\plug_rtn	//stupid mfgtool assumes start is after reserv2
boot_data:		.word image_base
image_len:		.word program_length
plugin:			.word 0x1
ddr_type:		.word DDR_TYPE
			ddr_data
get_ddr_type_addr:
	adr	r2,ddr_type
	mov	pc, lr
	.endm

	.macro header_chain_ivt rtn
ivt_header2:		.word MAKE_TAG(0xD1, 0x20, 0x40) /* 0x402000D1, Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v2:	.word mfgtool_start2
			.word 0x0
dcd_ptr2:		.word 0x0
boot_data_ptr2:		.word boot_data
self_ptr2:		.word ivt_header2
app_code_csf2:		.word 0x0
			.word 0x0
mfgtool_start2:		b	\rtn		//stupid mfgtool assumes start is after reserv2
	.endm

	.macro plug_entry_setup
	.endm

#define HEADER_SEARCH		mx53_header_search
#define HEADER_GET_RTN		mx53_header_get_rtn
#define HEADER_UPDATE_END	mx53_header_update_end

#define IVT_BARKER 0x402000d1
	.macro test_for_header
	BigMov	r1, IVT_BARKER
	ldr	r0, [r0]
	cmp	r0, r1
	moveq	r0,#1
	movne	r0,#0
	.endm

#define ALT0	0x0
#define ALT1	0x1
#define ALT3	0x3
#define ALT4	0x4
#define ALT5	0x5
#define SION	0x10	//0x10 means force input path of BGA contact

#define SRE_FAST	(0x1 << 0)
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

/* DCD */
#define TO1_DQVAL 0x00380000	/* dse 7 - 43 30 34 20 ohm drive strength*/
#define TO2_DQVAL 0x00200000	/* dse 4 - 75 NA 60 NA ohm drive strength*/

#define TO1_DDR_TYPE 0x06000000		/* 3=20 ohms */
//#define TO2_DDR_TYPE 0x02000000	/* 1=37.5, 33, 30 ohms */
#define TO2_DDR_TYPE 0x06000000		/* 3=20  ohms */

#ifdef TO2
#define DQVAL TO2_DQVAL
#define IOMUXC_DDR_TYPE TO2_DDR_TYPE
#else
#define DQVAL TO1_DQVAL
#define IOMUXC_DDR_TYPE TO1_DDR_TYPE
#endif
#if 0
//old values
#define CAS_VAL		0x00380000	//dse 7
#define RAS_VAL		0x00380000	//dse 7
#define ADDDS_VAL	0x00380000	//dse 7
#define CTLDS_VAL	0x00380000	//dse 7
#else
//new values
#define CAS_VAL		0x00280000	//dse 5
#define RAS_VAL		0x00280000	//dse 5
#define ADDDS_VAL	0x00280000	//dse 5
#define CTLDS_VAL	0x00280000	//dse 5
#endif

#define _IOM	IOMUXC_BASE_ADDR
	.macro ddr_init_table
	.word	_IOM+0x554, DQVAL	/* DQM3: */
	.word	_IOM+0x558, DQVAL | 0x40	/* SDQS3: pue=1 but PKE=0 */
	.word	_IOM+0x560, DQVAL	/* DQM2: */
	.word	_IOM+0x564, DQVAL | 0x40	/* SDODT1: pue=1 but PKE=0 */
	.word	_IOM+0x568, DQVAL | 0x40	/* SDQS2: pue=1 but PKE=0 */
	.word	_IOM+0x570, 0x00200000	/* SDCLK_1: dse 4 - 75 NA 60 NA */
	.word	_IOM+0x574, CAS_VAL	/* CAS: */
	.word	_IOM+0x578, 0x00200000	/* SDCLK_0: dse 4 */
	.word	_IOM+0x57c, DQVAL | 0x40	/* SDQS0: pue=1 but PKE=0 */
	.word	_IOM+0x580, DQVAL | 0x40	/* SDODT0: pue=1 but PKE=0 */
	.word	_IOM+0x584, DQVAL	/* DQM0: */
	.word	_IOM+0x588, RAS_VAL	/* RAS: */
	.word	_IOM+0x590, DQVAL | 0x40	/* SDQS1: pue=1 but PKE=0 */
	.word	_IOM+0x594, DQVAL	/* DQM1: */
	.word	_IOM+0x6f0, ADDDS_VAL	/* GRP_ADDDS A0-A15 BA0-BA2: */
	.word	_IOM+0x6f4, 0x00000200	/* GRP_DDRMODE_CTL: DDR2 SDQS0-3 */
	.word	_IOM+0x6fc, 0x00000000	/* GRP_DDRPKE: PKE=0 for A0-A15,CAS,CS0,CS1,D0-D31,DQM0-3,RAS,SDBA0-2,SDCLK_0-1,SDWE */
	.word	_IOM+0x714, 0x00000000	/* GRP_DDRMODE: CMOS input type?????? D0-D31 */
	.word	_IOM+0x718, DQVAL	/* GRP_B0DS: D0-D7 */
	.word	_IOM+0x71c, DQVAL	/* GRP_B1SD: D8-D15 */
	.word	_IOM+0x720, CTLDS_VAL	/* GRP_CTLDS:  CS0-1, SDCKE0-1, SDWE*/
	.word	_IOM+0x724, IOMUXC_DDR_TYPE	/* GRP_DDR_TYPE: A0-A15,CAS,CS0-1,D0-D31,DQM0-3,RAS,SDBA0-2,SDCKE0-1,SDCLK_0-1,SDODT0-1,SDQS0-3,SDWE */
	.word	_IOM+0x728, DQVAL	/* GRP_B2DS: D16-D23 */
	.word	_IOM+0x72c, DQVAL	/* GRP_B3DS: D24-D31 */
	.endm

	.macro iod_iomuxc_basic_init
	ddr_init_table
#if 1
//uart1 rxd:PATA_DMACK, txd:PATA_DIOW
//Note: Serial downloader assumes UART1_TXD is CSI0_DAT10, UART1_RXD is CSI0_DAT11
	.word	_IOM+0x274, 3		//Mux: PATA_DMACK - UART1_RXD
	.word	_IOM+0x5F4, 0x1e4	//CTL: PATA_DMACK - UART1_RXD
//This line screws up bootloader downloads
//when we need to download a second file via HAB_RVT_FAIL_SAFE
//	.word	_IOM+0x878, 3		//Select: UART1_IPP_UART_RXD_MUX_SELECT_INPUT
	.word	_IOM+0x270, 3		//MUX: PATA_DIOW - UART1_TXD
	.word	_IOM+0x5F0, 0x1e4	//CTL: PATA_DIOW - UART1_TXD
#endif
//uart2 rxd:PATA_BUFFER_EN, txd:PATA_DMARQ
	.word	_IOM+0x27C, 3		//Mux: PATA_BUFFER_EN - UART1_RXD
	.word	_IOM+0x5FC, 0x1e4	//CTL: PATA_BUFFER_EN - UART1_RXD
	.word	_IOM+0x880, 3		//Select: UART2_IPP_UART_RXD_MUX_SELECT_INPUT
	.word	_IOM+0x278, 3		//MUX: PATA_DMARQ - UART1_TXD
	.word	_IOM+0x5F8, 0x1e4	//CTL: PATA_DMARQ UART1_TXD
#if 1
//uart3 rxd:EIM_D25, txd:EIM_D24
	.word	_IOM+0x140, 2		//Mux: EIM_D25 - UART1_RXD
	.word	_IOM+0x488, 0x1e4	//CTL: EIM_D25 - UART1_RXD
//This line screws up bootloader downloads
//when we need to download a second file via HAB_RVT_FAIL_SAFE
//	.word	_IOM+0x888, 1		//Select: UART3_IPP_UART_RXD_MUX_SELECT_INPUT
	.word	_IOM+0x13c, 2		//MUX: EIM_D24 - UART1_TXD
	.word	_IOM+0x484, 0x1e4	//CTL: EIM_D24 - UART1_TXD
#endif

	.word	_IOM+0x198, 1		//EIM_EB1: mux gpio2[29]
	.word	_IOM+0x4e8, 0x104
	.word	0
	.endm

	.macro iod_iomuxc_ddr_single
	/* Enable pull down */
	.word	_IOM+0x57c, DQVAL | 0x40 | 0x80	/* SDQS0: pue=1 PKE=1 */
	.word	_IOM+0x590, DQVAL | 0x40 | 0x80	/* SDQS1: pue=1 PKE=1 */
	.word	_IOM+0x568, DQVAL | 0x40 | 0x80	/* SDQS2: pue=1 PKE=1 */
	.word	_IOM+0x558, DQVAL | 0x40 | 0x80	/* SDQS3: pue=1 PKE=1 */
	.word	0
	.endm

	.macro iod_iomuxc_ddr_differential
	/* Disable pull down */
	.word	_IOM+0x57c, DQVAL | 0x40	/* SDQS0: pue=1 but PKE=0 */
	.word	_IOM+0x590, DQVAL | 0x40	/* SDQS1: pue=1 but PKE=0 */
	.word	_IOM+0x568, DQVAL | 0x40	/* SDQS2: pue=1 but PKE=0 */
	.word	_IOM+0x558, DQVAL | 0x40	/* SDQS3: pue=1 but PKE=0 */
	.word	0
	.endm

#define CCG1_UART_MASK	(0xfff<<(3*2))

	.macro config_uart_clocks
#if 0
	BigMov	r0, ('1') | (':' << 8)
	bl	TransmitX
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CCGR1]
	bl	print_hex
#endif
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CCGR1]		//index 3 & 4 is UART1, 5&6 UART2, 7&8 UART3
	BigBic2	r0,CCG1_UART_MASK
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
	BigOrr2	r0,CCG1_UART_MASK
	str	r0,[r1, #CCM_CCGR1]
#if 0
	BigMov	r0, (' ') | ('R' << 8) | (':' << 16)
	bl	TransmitX
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CBCDR]
	bl	print_hex
#endif
#if 0
	BigMov	r1,CCM_BASE
        /* Disable IPU and HSC handshake */
	mov r0, #0x00060000
	str r0, [r1, #CCM_CCDR]
#endif
#if 0
	BigMov	r0, ('R') | (':' << 8)
	bl	TransmitX
	BigMov	r1,CCM_BASE
	ldr	r0,[r1, #CCM_CBCDR]
	bl	print_hex
#endif
	.endm

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


	.macro	esd_zqcalibrate
	BigMov	r3, ESD_BASE
//	esd_con_req r3
#ifdef TO2
	ldr	r1, [r3, #ESD_ZQHWCTRL]
	and	r1, r1, #0xf8000000
	BigOrr2	r1, 0x04b80003	//0x04b80003
	str	r1, [r3, #ESD_ZQHWCTRL]
	BigMov	r0, 0x00194005	//PLL1P2_VREG (bits 16-12 of reg) from 0x10 (1.2V) to 0x14 (1.3V)
	BigMov	r1, IOMUXC_BASE_ADDR
#define IOMUXC_GPR1 0x04
	str	r0, [r1, #IOMUXC_GPR1]
//91:	ldr	r1, [r3, #ESD_ZQHWCTRL]
//	tst	r1, #1<<16
//	bne	91b
//	BigMov	r1, (1<<13)|(31<<7)|(30<<2)
//	BigMov	r1, (1<<13)|(28<<7)|(6<<2)
//	str	r1, [r3, #ESD_ZQSWCTRL]
#endif
//	mov	r1, #0		//normal operations again
//	str	r1, [r3, #ESD_SCR]
	.endm

	.macro Turn_off_CS1
	BigMov	r3, ESD_BASE
	esd_con_req r3
#if 1
	BigMov	r1, 0x000016d2 | (((DDR2_BANK_BITS - 1) & 1) << 5)	/* ESD_MISC, (0)8 banks, (2)DDR2 */
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

#define PAD_CSPI1_MOSI	(HYS_ENABLE | DSE_HIGH)
#define PAD_CSPI1_MISO	(HYS_ENABLE | DSE_HIGH)
#define PAD_CSPI1_SS0	(HYS_ENABLE | DSE_HIGH)
#define PAD_CSPI1_SS1	(HYS_ENABLE | DSE_HIGH)
#define PAD_CSPI1_RDY	(HYS_ENABLE | DSE_LOW)
#define PAD_CSPI1_SCLK	(HYS_ENABLE | DSE_HIGH)

	.macro iod_iomuxc_setup_ecspi
	.word	_IOM+0x468, PAD_CSPI1_MOSI	//SW_PAD_CTL_EIM_D18 - ECSPI1_MOSI
	.word	_IOM+0x464, PAD_CSPI1_MISO	//SW_PAD_CTL_EIM_D17 - ECSPI1_MISO
	.word	_IOM+0x46c, PAD_CSPI1_SS1	//SW_PAD_CTL_EIM_D19 - ECSPI1_SS1
	.word	_IOM+0x460, PAD_CSPI1_SCLK	//SW_PAD_CTL_EIM_D16 - ECSPI1_SCLK

	.word	_IOM+0x120, ALT4		//SW_MUX_CTL_PAD_EIM_D18, alt4 ECSPI1_MOSI, alt1 gpio3[18]
	.word	_IOM+0x7a4, 3		//mosi input select
	.word	_IOM+0x11c, ALT4		//SW_MUX_CTL_PAD_EIM_D17, alt4 ECSPI1_MISO, alt1 gpio3[17]
	.word	_IOM+0x7a0, 3		//miso input select
	.word	_IOM+0x124, ALT1		//SW_MUX_CTL_PAD_EIM_D19, alt4 ECSPI1_SS1, alt1 gpio3[19]
	.word	_IOM+0x7ac, 2		//ss1 input select
	.word	_IOM+0x118, ALT4		//SW_MUX_CTL_PAD_EIM_D16, alt4 ECSPI1_SCLK, alt1 gpio3[16]
	.word	_IOM+0x79c, 3		//clk input select
	.word	0
	.endm

	.macro iod_iomuxc_setup_i2c1
	.word	_IOM+0x14c, ALT5 | SION	//Mux: EIM_D28, SDA of i2c1
	.word	_IOM+0x494, HYS_ENABLE | PKE_ENABLE | PULL | R22K_PU | OPENDRAIN_ENABLE | DSE_HIGH | SRE_FAST
	.word	_IOM+0x818, 1		//Select EIM_D28
	.word	_IOM+0x12c, ALT5 | SION	//Mux: EIM_D21, SCL of i2c1
	.word	_IOM+0x474, HYS_ENABLE | PKE_ENABLE | PULL | R22K_PU | OPENDRAIN_ENABLE | DSE_HIGH | SRE_FAST
	.word	_IOM+0x814, 1		//Select EIM_D21
	.word	0
	.endm

	.macro iod_iomuxc_miso_gp
	.word	_IOM+0x11c, 1		//SW_MUX_CTL_PAD_EIM_D17, alt4 ECSPI1_MISO, alt1 gpio3[17]
	.word	0
	.endm

	.macro iod_iomuxc_miso_ecspi
	.word	_IOM+0x11c, 4		//SW_MUX_CTL_PAD_EIM_D17, alt4 ECSPI1_MISO, alt1 gpio3[17]
	.word	0
	.endm

	.macro init_gps
	BigMov	r1, GPIO2_BASE
	ldr	r0, [r1, #GPIO_DR]
// make sure gp2[29] is high, i2c_sel for tfp410
	orr	r0, r0, #(1 << 29)
	str	r0, [r1, #GPIO_DR]

	ldr	r0, [r1, #GPIO_DIR]
	orr	r0, r0, #(1 << 29)
	str	r0, [r1, #GPIO_DIR]
	.endm

	.macro keep_power_on
//for nitrogen53_a
	BigMov	r1, GPIO3_BASE
	ldr	r0, [r1, #GPIO_DR]
// turn on power
	orr	r0, r0, #(1 << 23)
	str	r0, [r1, #GPIO_DR]

	ldr	r0, [r1, #GPIO_DIR]
	orr	r0, r0, #(1 << 23)
	str	r0, [r1, #GPIO_DIR]
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
	.macro iod_iomuxc_setup_mmc
	.word	_IOM+0x66c, PAD_SD1_DATA0	//SW_PAD_CTL_SD1_DATA0
	.word	_IOM+0x670, PAD_SD1_DATA1	//SW_PAD_CTL_SD1_DATA1
	.word	_IOM+0x674, PAD_SD1_CMD		//SW_PAD_CTL_SD1_CMD
	.word	_IOM+0x678, PAD_SD1_DATA2	//SW_PAD_CTL_SD1_DATA2
	.word	_IOM+0x67c, PAD_SD1_CLK		//SW_PAD_CTL_SD1_CLK
	.word	_IOM+0x680, PAD_SD1_DATA3	//SW_PAD_CTL_SD1_DATA3

	.word	_IOM+0x2e4, 0			//SW_MUX_CTL_PAD_SD1_DATA0
	.word	_IOM+0x2e8, 0			//SW_MUX_CTL_PAD_SD1_DATA1
	.word	_IOM+0x2ec, ALT0|SION		//SW_MUX_CTL_PAD_SD1_CMD
	.word	_IOM+0x2f0, 0			//SW_MUX_CTL_PAD_SD1_DATA2
	.word	_IOM+0x2f4, 0			//SW_MUX_CTL_PAD_SD1_CLK
	.word	_IOM+0x2f8, 0			//SW_MUX_CTL_PAD_SD1_DATA3
	.word	0
	.endm

	.macro mmc_get_cd
	mov	r0,#0	//always card present
	.endm


#endif

