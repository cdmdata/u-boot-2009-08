	.global assem_start
	.global get_ecspi_base
	.global drive_power_on
	.global get_mmc_base
	.global iomuxc_setup_i2c1
	.global iomuxc_setup_ecspi
	.global iomuxc_miso_gp
	.global iomuxc_miso_ecspi
	.global iomuxc_setup_mmc
	.global ecspi_ss_active
	.global ecspi_ss_inactive
	.global ecspi_read_miso
	.global mmc_read_cd
	.global chain_ivt
	.global get_ram_base
	.global get_ram_size
	.global get_tapeout_version
	.global TransmitX
	.global ReceiveX
	.global hw_watchdog_reset
	.global header_search
	.global header_get_rtn
	.global header_update_end
	.global header_present
	.global iomuxc_ddr_single
	.global iomuxc_ddr_differential
	.global header_base

#define rFNAME r8
#define rFOFF r9
#define rIIM r12

#include <config.h>
#include <asm/BigMacro.h>
#include <asm/char.inc>
#define ASM
#include "imx.h"
#include "mx5x.h"
#ifdef CONFIG_MX51
#include "mx51.h"
#else
#include "mx53.h"
#endif
//#define DEBUG

	.section .header_mem,"x"
header_base:
	header_macro assem_start

	.section .text
assem_start:
#ifdef DEBUG
	mov	r3, lr
#endif
	plug_entry_setup
	tst	r0, #3
	tsteq	r1, #3
	tsteq	r2, #3
	beq	plug_start
#if 0
	BigMov	sp,STACK_HIGH - 12
#else
	str	lr,[sp, #-4]!
	sub	sp, sp, #12
#endif
	add	r0, sp, #0
	add	r1, sp, #4
	add	r2, sp, #8
	bl	plug_start
1:
	cmp	r0, #1
	bne	2f		//branch if error
#if 0
	ldr	lr,[sp, #12]
	movs	lr, lr
	addne	sp, sp, #16	//return if able
#if 1
	bne	rvt_reset
#else
	bxne	lr
#endif
#endif
	ldmia	sp, {r0-r2}
	add	r0, r0, r2
	bl	header_get_rtn
	movs	r3, r0
	addne	r0, sp, #0
	addne	r1, sp, #4
	addne	r2, sp, #8
	blxne	r3
	b	1b
#define LL4(a,b,c,d) ((a)+(b<<8)+(c<<16)+(d<<24))
2:
#if 1
//delay before reset to help prevent USB windows trouble with mfgtool
	mov	r3,#0xc00000
3:	subs	r3,r3,#1
	bne	3b
#endif
	ldr	r0, [sp, #12]
	bl	print_hex
	mov	r0, #0x20
	bl	TransmitX
	BigMov	r0, LL4('E','R','R','\n')
	bl	TransmitX
	bl	flush_uart
	ldr	lr, [sp, #12]
	mov	r0, #0
	add	sp, sp, #16
#if 0
	movs	lr, lr
	bxne	lr
#endif

rvt_reset:
#ifdef HAB_RVT_FAIL_SAFE_VECT
	BigMov	r0, LL4('R','V','T','\n')
	bl	TransmitX
	bl	flush_uart
	mov	r0, #HAB_RVT_ENTRY
	ldr	lr, [r0]
	blx	lr
	mov	r0, #HAB_RVT_FAIL_SAFE_VECT
	ldr	lr, [r0]
	blx	lr
#endif
wdg_reset:
	BigMov	r0, LL4('W','D','R','\n')
	bl	TransmitX
	bl	flush_uart
wdg_reset2:
	BigMov	r1, WDOG_BASE
	BigMov	r0, 0x4
	strh	r0,[r1, #WDOG_WCR]
98:	b	98b

#if 0
hb:	.word	header_base
pe:	.word	program_end
#endif

plug_start:
	/* Save the return address and the function arguments */
	push	{r0-r12, lr}
	hw_init CSP_BASE_REG_PA_AIPS1, CSP_BASE_REG_PA_AIPS2
	config_uart_clocks
	init_disable_ipu IPU_CM_REG_BASE, IPU_IDMAC_BASE
	bl	Basic_Init_IOMux
	init_gps

#if (UART_INIT_MASK & 1)
	BigMov	r3,UART1_BASE
	bl	setup_uart
#endif
#if (UART_INIT_MASK & 2)
	BigMov	r3,UART2_BASE
	bl	setup_uart
#endif
#if (UART_INIT_MASK & 4)
	BigMov	r3,UART3_BASE
	bl	setup_uart
#endif
////////
	mov	r0,#0xc0000	//0x40000 gives garbage, 0x6000 garbage somtimes
1:	subs	r0,r0,#1	//delay for UART to power up, avoiding garbage characters transmitting
	bne	1b
//	mov	r0, #'A'
//	bl	TransmitX

#if 0
//Dump program for comparsion
	ldr	r4, hb
	ldr	r5, pe
10:
	debug_hex r4
	mov	r0, #' '
	bl	TransmitX
	mov	r6,#4
11:	ldr	r0, [r4], #4
	debug_hex r0
	subs	r6, r6, #1
	bne	11b
	BigMov	r0, 0x0a0d
	bl	TransmitX
	cmp	r4, r5
	blo	10b
#endif

#ifdef PMIC_DA9053_ADDR
	ldr	r0, =I2C_BASE
	mov	r1, #PMIC_DA9053_ADDR
	bl	power_up_ddr
#endif
ddr_setup:
	ddr_init
	mov	r0,#0x10000
2:	subs	r0,r0,#1
	bne	2b
	esd_zqcalibrate
//	mov	r0, #'X'
//	bl	TransmitX
#ifdef USE_CSD1

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
#if 0	//change to if 1 to force 256M of memory
	b	7f
#endif
	b	8f
//CS1 does NOT have SDRAM connected, only 256 M on board
//We need to turn off CS1
7:
	mov	r0, #'F'
	bl	TransmitX
///
	Turn_off_CS1
8:
#else
//	mov	r0, #'F'
//	bl	TransmitX
//	Turn_off_CS1
//	mov	r0, #'-'
//	bl	TransmitX
#endif
//	mov	r0, #'G'
//	bl	TransmitX
	/* reload the function arguments */
	ldmia	sp, {r0-r2}
	bl	plug_main
	mov	r4, r0
#ifdef PMIC_DA9053_ADDR
	cmn	r4, #-ERROR_MEMORY_TEST
	bne	1f
//	bl	get_ram_base
//	bl	ram_test
//	cmp	r0, #0
//	bne	1f		//br if ram test passed

	ldr	r0, =I2C_BASE
	mov	r1, #PMIC_DA9053_ADDR
	bl	vbuckcore_boost
	movs	r0,r0
	beq	ddr_setup
1:
#endif

#ifdef DEBUG
	mov	r0, #' '
	bl	TransmitX
	ldr	r0, [sp, #13*4]
	bl	print_hex
	mov	r0, #' '
	bl	TransmitX
	ldr	r0, [sp, #3*4]
	bl	print_hex
	mov	r0, #' '
	bl	TransmitX
	add	r0, sp, #13*4
	bl	print_hex
	BigMov	r0, 0x0a0d
	bl	TransmitX
#endif
	bl	flush_uart
	cmp	r4, #1
	movne	r4, #0	//anything but 1 is failure
	str	r4, [sp, #0]
#ifdef DEBUG
	pop	{r0-r12, lr}
	sub	r2, sp, #0x300
	mvn	r1, #1
1:	str	r1, [r2], #4
	cmp	r2, sp
	bne	1b
	bx	lr
#else
	pop	{r0-r12, pc}
#endif

setup_uart:
	adr	r6,init_tab
	adr	r7,init_tab_end
	add	r1,r3, #UCR1
1:	ldrh	r0,[r6],#2
	and	r2,r1,#0xff
	cmp	r2,#0xac	//UBRC
	strne	r0,[r1],#4
	addeq	r1,r1,#4
	cmp	r6,r7
	bne	1b
	bx	lr
init_tab:
	.short	0x0001		//UCR1 (0x80)
	.short	0x4027		//UCR2 (0x84)
	.short	0x0704		//UCR3 (0x88)
	.short	0x8000		//UCR4 (0x8c)
#if (UART_PREDIV == 7)
#define TO_PREDIV(a)	6
#else
#define TO_PREDIV(a)	(6 - a)
#endif
	.short	0x0801 | (TO_PREDIV(UART_PREDIV) << 7)	//UFCR (0x90)
	.short	0x2050		//USR1 (0x94)
	.short	0x5088		//USR2 (0x98)
	.short	0x002b		//UESC (0x9c)
	.short	0x0000		//UTIM (0xa0)
	.short	UART_MULT_VAL-1	//UBIR (0xa4)
	.short	UART_DIV_VAL-1	//UBMR (0xa8)
	.short	0x0004		//UBRC (0xac), read only
	.short	0x0000		//ONEMS (0xb0)
	.short	0x0060		//UTS (0xb4)
init_tab_end:
	.align	2


#ifdef mx51
Basic_Init_ESD:
	test_memory_active
	bxne	lr
//	test_watchdog_reset
//	bxne	lr
	BigMov	r3, ESD_BASE
	esd_con_req r3
	adr	r3, ESD_TABLE
	mov	r2,#1000
	b	Basic_Init
#endif
Basic_Init_IOMux:
	choose_data	iomuxc_basic_init

Basic_Init1:
	mov	r2,#0x1
Basic_Init:
10:	ldr	r0, [r3], #4
	cmp	r0,#0
	bxeq	lr

	mov	r1, r2
11:	subs	r1, r1, #1
	bne	11b

	ldr	r1, [r3], #4
	str	r1, [r0]
#if 0
	mov	r0, #'.'
	cmp	r2,#1000
	str	lr,[sp, #-4]!
	bleq	TransmitX
	ldr	lr,[sp], #4
#endif
	b	10b


#ifdef mx51
ESD_TABLE:
	esd_dcd_data
#endif

	.section .text.chain_ivt,"x"
chain_ivt:
	header_chain_ivt plug_main2

	.section .text.get_ram_base,"x"
get_ram_base:
	BigMov	r0,RAM_BASE
	bx	lr

	.section .text.get_ram_size,"x"
get_ram_size:
	ddr_get_size
	bx	lr

	.section .text.get_tapeout_version,"x"
get_tapeout_version:
	ddr_get_tapeout
	bx	lr

	.section .text.get_ecspi_base,"x"
get_ecspi_base:
	BigMov	r0,ECSPI_BASE
	bx	lr

	.section .text.drive_power_on,"x"
drive_power_on:
	keep_power_on
	bx	lr

	.section .text.get_mmc_base,"x"
get_mmc_base:
	BigMov	r0,MM_SDMMC_BASE
	bx	lr

	.section .text.iomuxc_setup_ecspi,"x"
iomuxc_setup_ecspi:
	ecspi_gp_setup
	ecspi_clk_setup CCM_BASE
	choose_data	iomuxc_setup_ecspi
///
	.section .text.iomuxc_setup_i2c1,"x"
iomuxc_setup_i2c1:
	i2c1_clk_setup CCM_BASE
	choose_data	iomuxc_setup_i2c1
///
	create_function iomuxc_setup_mmc
	create_function iomuxc_ddr_single
	create_function iomuxc_ddr_differential
	create_function iomuxc_miso_gp
	create_function iomuxc_miso_ecspi

	.section .text.ecspi_ss_active,"x"
ecspi_ss_active:
	ecspi_ss_make_active
	bx	lr

	.section .text.ecspi_ss_inactive,"x"
ecspi_ss_inactive:
	ecspi_ss_make_inactive
	bx	lr

	.section .text.ecspi_read_miso,"x"
ecspi_read_miso:
	ecspi_ss_get_miso
	bx	lr

	.section .text.mmc_read_cd,"x"
mmc_read_cd:
	mmc_get_cd
	bx	lr

	.global uart_bases
	.global uart_index
	.section .text.uart_rtns,"x"
uart_bases:
	.word	UART1_BASE
	.word	UART2_BASE
	.word	UART3_BASE
uart_index:
	.word	UART_DEF_INDEX

//OUT:
//z-0 good data in r0
//z-1 timeout, r0 is 0
//r1 - loop cnt time remaining
ReceiveX:
	BigMov	r1, WDOG_BASE
	BigMov	r0, 0x5555
	strh	r0,[r1, #WDOG_WSR]
	mov	r0, r0, LSL #1
	strh	r0,[r1, #WDOG_WSR]
	mov	r1,#0x80000
1:
#if (UART_RX_MASK & 1)
	ldr	r0,uart_bases
	ldr	r0,[r0,#USR2]
	tst	r0,#1
	movne	r0,#0
	strne	r0,uart_index
	ldrne	r0,uart_bases
#if (UART_RX_MASK & 6)
	bne	2f
#endif
#endif
#if (UART_RX_MASK & 2)
	ldr	r0,uart_bases+4
	ldr	r0,[r0,#USR2]
	tst	r0,#1
	movne	r0,#1
	strne	r0,uart_index
	ldrne	r0,uart_bases+4
#if (UART_RX_MASK & 4)
	bne	2f
#endif
#endif
#if (UART_RX_MASK & 4)
	ldr	r0,uart_bases+8
	ldr	r0,[r0,#USR2]
	tst	r0,#1
	movne	r0,#2
	strne	r0,uart_index
	ldrne	r0,uart_bases+8
#endif
2:
	ldrne	r0,[r0,#URXD]
	andne	r0,r0,#0xff
	bxne	lr			//return
	subs	r1,r1,#1
	bne	1b
	mov	r0,#0
	bx	lr			//return

//IN: r0 - character(s) to transmit, r1 - trashed
TransmitX:
1:
#if (UART_TX_MASK & 1)
	ldr	r1,uart_bases
	ldr	r1, [r1, #USR2]
	tst	r1, #(1<<3)
	beq	1b

	ldr	r1,uart_bases
	str	r0, [r1, #UTXD]
#endif
#if (UART_TX_MASK & 2)
2:
	ldr	r1,uart_bases+4
	ldr	r1, [r1, #USR2]
	tst	r1, #(1<<3)
	beq	2b

	ldr	r1,uart_bases+4
	str	r0, [r1, #UTXD]
#endif
#if (UART_TX_MASK & 4)
3:
	ldr	r1,uart_bases+8
	ldr	r1, [r1, #USR2]
	tst	r1, #(1<<3)
	beq	3b

	ldr	r1,uart_bases+8
	str	r0, [r1, #UTXD]
#endif
	movs	r0,r0,LSR #8
	bne	1b
	bx	lr

	.section .text.print_hex,"x"
print_hex:
	mov	r3, lr
	mov	r2, r0, LSL #4
	orr	r2, r2, #1
	mov	r0, r0, LSR #28
1:	cmp	r0,#10
	addcc	r0,r0,#'0'
	addcs	r0,r0,#'a'-10
	bl	TransmitX
	mov	r0, r2, LSR #28
	movs	r2, r2, LSL #4
	bne	1b
	bx	r3

	.section .text.hw_watchdog_reset,"x"
hw_watchdog_reset:
	BigMov	r1, WDOG_BASE
	BigMov	r0, 0x5555
	strh	r0,[r1, #WDOG_WSR]
	mov	r0, r0, LSL #1
	strh	r0,[r1, #WDOG_WSR]
	bx	lr			//return

	.section .text.header_search,"x"
header_search:
	b HEADER_SEARCH

	.section .text.header_get_rtn,"x"
header_get_rtn:
	b HEADER_GET_RTN

	.section .text.header_update_end,"x"
header_update_end:
	b HEADER_UPDATE_END

	.section .text.header_present,"x"
header_present:
	test_for_header
	bx	lr			//return

