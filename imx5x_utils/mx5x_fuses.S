	.global assem_start

#include <asm/BigMacro.h>
#include <asm/char.inc>
#define ASM
#include "imx.h"
#include "mx5x.h"
#ifdef mx51
#include "mx51.h"
#else
#include "mx53.h"
#endif

#define rTable r8
#define rOffset r9
#define rIIM r12


#define L1(a)          ((CH_##a))
#define L2(a,b)     ((CH_##a)+(CH_##b<<8))
#define L3(a,b,c)   ((CH_##a)+(CH_##b<<8)+(CH_##c<<16))
#define L4(a,b,c,d) ((CH_##a)+(CH_##b<<8)+(CH_##c<<16)+(CH_##d<<24))

#define C2(a,b)     ((CH_##a<<8)+(CH_##b))
#define C3(a,b,c)   ((CH_##a<<16)+(CH_##b<<8)+(CH_##c))
#define C4(a,b,c,d) ((CH_##a<<24)+(CH_##b<<16)+(CH_##c<<8)+(CH_##d))

	.section .header_mem,"x"
	header_macro assem_start

	.section .text
assem_start:
	hw_init	CSP_BASE_REG_PA_AIPS1, CSP_BASE_REG_PA_AIPS2
	BigMov	sp,STACK_HIGH
	bl	Basic_Init_IOMux
	config_uart_clocks
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

	bl	TransmitCRLF
	mov	r0,sp
	bl	print_hex
	bl	TransmitCRLF

	mov	r0,#0x10000
	mov	r3,#0x10000
23:	subs	r0,r0,#1
	sub	r3,r3,#1
	bne	23b

	cmp	r3,r0
	BigMov	r0,L1(N)
	blne	TransmitX
//	b	Test
	bl	TransmitCRLF


	BigMov	rIIM,IIM_BASE
	bl	Get_fuse_name_table
1:	ldrb	r0,[rTable]
	cmp	r0,#0xff
	BigMovEq  rIIM,SRC_BASE
	addeq	rTable, rTable, #1
	mov	r0, rTable
	bl	transmit_string
	mov	rTable, r0

	BigMov	r0,L1(OPEN_PAREN)
	bl	TransmitX
	ldrb	rOffset,[rTable], #1
	ldrb	r0,[rTable], #1
	orr	rOffset, rOffset, r0, LSL #8
	add	r0, rOffset, rIIM
	bl	print_hex
	BigMov	r0,L2(CLOSE_PAREN,COLON)
	bl	TransmitX
	ldr	r0,[rIIM,rOffset]
	bl	print_hex
	bl	TransmitCRLF
	ldrb	r0,[rTable]
	movs	r0,r0
	bne	1b

	add	rTable, rTable, #1
	BigMov	rIIM,IIM_BASE
	fuse_check_rev	rIIM
	bne	11f
1:
	mov	r0, rTable
	bl	transmit_string
	mov	rTable, r0
	ldrb	rOffset,[rTable], #1
	ldrb	r0,[rTable], #1
	orr	rOffset, rOffset, r0, LSL #8
	fuse_sense rIIM, rOffset

	fuse_load_mask rTable, r7, r1
	bics	r7, r7, r0
	beq	30f
//r7 has fuses to be burned

	BigMov	r5, CCM_BASE
	fuse_burn rIIM, r5, r7, rOffset, r6
	bne	8f
30:
	ldrb	r0,[rTable]
	movs	r0,r0
	bne	1b
	b	9f

11:
	adr	r0,unexpected_PREV_value_str
	b	12f

8:
	BigMov	r0,L1(OPEN_PAREN)
	bl	TransmitX
	mov	r0,r6
	bl	print_hex_short
	BigMov	r0,L2(CLOSE_PAREN,COLON)
	bl	TransmitX
	adr	r0,error_blow_str
	bl	transmit_string
9:
	adr	r0,done_str
12:	bl	transmit_string
20:	b	20b


//In: r0 @asciz str
//Out: r0 past str, r1-r3 trashed
transmit_string:
	mov	r3, lr
	mov	r2, r0
1:	ldrb	r0, [r2], #1
	movs	r0, r0
	moveq	r0, r2
	moveq	pc, r3
2:	bl	TransmitX
	b	1b

Get_fuse_name_table:
	adr	rTable,fuse_name_table
	mov	pc, lr

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
	mov	pc, lr
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

Basic_Init1:
	mov	r2,#0x1
Basic_Init:
10:	ldr	r0, [r3], #4
	cmp	r0,#0
	moveq	pc, lr

	mov	r1, r2
11:	subs	r1, r1, #1
	bne	11b

	ldr	r1, [r3], #4
	str	r1, [r0]
	b	10b

unexpected_PREV_value_str:
	.asciz	"Unexpected PREV value\n"

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
	fuse_table

	.align	2
Basic_Init_IOMux:
	choose_data	iomuxc_basic_init

	.section .text.uart_rtns,"x"

uart_bases:
	.word	UART1_BASE
	.word	UART2_BASE
	.word	UART3_BASE

TransmitSPACE:
	mov	r0,#L1(SPACE)
//	b	TransmitX

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
	mov	pc, lr

TransmitCRLF:
	BigMov	r0,L2(CR,LF)
	b	TransmitX


	.section .text.print_hex,"x"
//IN: r0 - value to print
//OUT: r0-r3 trashed
print_hex:
	mov	r2, r0, LSL #4
	orr	r2, r2, #1
print_hex1:
	mov	r0, r0, LSR #28
	mov	r3, lr
1:	cmp	r0,#10
	addcc	r0,r0,#'0'
	addcs	r0,r0,#'a'-10
	bl	TransmitX
	mov	r0, r2, LSR #28
	movs	r2, r2, LSL #4
	bne	1b
	mov	pc, r3

print_hex_byte:
	mov	r0, r0, LSL #24
	mov	r2, r0, LSL #4
	orr	r2, r2, #1 << 24
	b	print_hex1
print_hex_short:
	mov	r0, r0, LSL #16
	mov	r2, r0, LSL #4
	orr	r2, r2, #1 << 16
	b	print_hex1
