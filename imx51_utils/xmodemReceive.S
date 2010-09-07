#include <asm/BigMacro.h>
#include <asm/char.inc>
#include <asm/xmodemReceive.inc>
	.global	xmodem_load
	.global TransmitX

#define L1(a)          ((CH_##a))
#define L2(a,b)     ((CH_##a)+(CH_##b<<8))
#define L3(a,b,c)   ((CH_##a)+(CH_##b<<8)+(CH_##c<<16))
#define L4(a,b,c,d) ((CH_##a)+(CH_##b<<8)+(CH_##c<<16)+(CH_##d<<24))

#define C2(a,b)     ((CH_##a<<8)+(CH_##b))
#define C3(a,b,c)   ((CH_##a<<16)+(CH_##b<<8)+(CH_##c))
#define C4(a,b,c,d) ((CH_##a<<24)+(CH_##b<<16)+(CH_##c<<8)+(CH_##d))

	.equiv	UART1_BASE, 0x73fbc000
	.equiv	UART2_BASE, 0x73fc0000
	.equiv	UART3_BASE, 0x7000c000
	.equiv	UART_BASE, UART1_BASE

	.equiv  BAUDRATE, 115200

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
	.section .text.TransmitX,"x"

//IN: r0 - character(s) to transmit
TransmitX:
1:	BigMov	r1,UART1_BASE
	ldr	r1, [r1, #USR2]
	tst	r1, #(1<<3)
	beq	1b

	BigMov	r1,UART1_BASE
	str	r0, [r1, #UTXD]
	movs	r0,r0,LSR #8
	bne	1b
	mov	pc, lr

	.section .text.ReceiveX,"x"
//OUT:
//z-0 good data in r0
//z-1 timeout, r0 is 0
//r1 - loop cnt time remaining
ReceiveX:
	mov	r1,#0x80000
1:
	BigMov	r0,UART1_BASE
	ldr	r0,[r0,#USR2]
	tst	r0,#1
	BigMovNe r0,UART1_BASE
	ldrne	r0,[r0,#URXD]
	andne	r0,r0,#0xff
	movne	pc,lr			//return
	subs	r1,r1,#1
	bne	1b
	mov	r0,#0
	mov	pc,lr			//return

	.section .text.xmodem_load,"x"
//In: r0 - destination
//Out: r0 - length of file loaded
xmodem_load:
	stmdb	sp!,{r4,r5,r6,r7,r9,sl,fp,ip,lr}
	mov	ip, r0		//ip is only input to macro, unchanged on output
1:
	xmodemReceive r3,r4,r5,r6,r7,r9,sl,fp,ip,TransmitX,ReceiveX
	movne	r3, ip
	sub	r0, r3, ip
	ldmia	sp!,{r4,r5,r6,r7,r9,sl,fp,ip,pc}
