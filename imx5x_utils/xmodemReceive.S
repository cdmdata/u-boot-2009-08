#include <asm/BigMacro.h>
#include <asm/char.inc>
#include <asm/xmodemReceive.inc>
	.global	xmodem_load

#define L1(a)          ((CH_##a))
#define L2(a,b)     ((CH_##a)+(CH_##b<<8))
#define L3(a,b,c)   ((CH_##a)+(CH_##b<<8)+(CH_##c<<16))
#define L4(a,b,c,d) ((CH_##a)+(CH_##b<<8)+(CH_##c<<16)+(CH_##d<<24))

#define C2(a,b)     ((CH_##a<<8)+(CH_##b))
#define C3(a,b,c)   ((CH_##a<<16)+(CH_##b<<8)+(CH_##c))
#define C4(a,b,c,d) ((CH_##a<<24)+(CH_##b<<16)+(CH_##c<<8)+(CH_##d))

	.equiv  BAUDRATE, 115200

////
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
