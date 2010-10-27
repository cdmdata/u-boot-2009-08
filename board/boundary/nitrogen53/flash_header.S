/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
	.global relocate
#include <config.h>
#include <asm/arch/mx53.h>

//#define USE_CSD1 1

#ifdef	CONFIG_FLASH_HEADER
#ifndef CONFIG_FLASH_HEADER_OFFSET
# error "Must define the offset of flash header"
#endif

#define CPU_2_BE_32(l) \
       ((((l) & 0x000000FF) << 24) | \
	(((l) & 0x0000FF00) << 8)  | \
	(((l) & 0x00FF0000) >> 8)  | \
	(((l) & 0xFF000000) >> 24))

#define MAKE_TAG(tag, len, v) CPU_2_BE_32(( ((tag) << 24) | ((len) << 8) | (v) ))
#define MXC_DCD_ITEM(i, addr, val)   \
dcd_node_##i:                        \
        .word CPU_2_BE_32(addr) ;     \
        .word CPU_2_BE_32(val)  ;     \

.section ".text.flasheader", "x"
_image_base:
	b	_start
	.org	CONFIG_FLASH_HEADER_OFFSET
ivt_header:       .word MAKE_TAG(0xD1, 0x20, 0x40) /* 0x402000D1, Tag=0xD1, Len=0x0020, Ver=0x40 */
app_code_jump_v:  .word _start
reserv1:          .word 0x0
dcd_ptr:          .word dcd_hdr
boot_data_ptr:	  .word boot_data
self_ptr:         .word ivt_header
app_code_csf:     .word 0x0
reserv2:          .word 0x0

boot_data:        .word TEXT_BASE
image_len:        .word __bss_start - TEXT_BASE
plugin:           .word 0x0

dcd_hdr:          .word	MAKE_TAG(0xD2, hdr_end - ., 0x40)
// 0x400802D2 /* Tag=0xD2, Len=64*8 + 4 + 4, Ver=0x40 */

write_dcd_cmd:    .word	MAKE_TAG(0xCC, cmd_end1 - ., 4) 
//0x040402CC /* Tag=0xCC, Len=64*8 + 4, Param=4 */

/* DCD */
MXC_DCD_ITEM(1, IOMUXC_BASE_ADDR + 0x554, 0x00380000)	/* DQM3: dse 7 - 43 30 34 20 ohm drive strength*/
MXC_DCD_ITEM(2, IOMUXC_BASE_ADDR + 0x558, 0x00380040)	/* SDQS3: pue=1 but PKE=0, dse 7 */
MXC_DCD_ITEM(3, IOMUXC_BASE_ADDR + 0x560, 0x00380000)	/* DQM2: dse 7 */
MXC_DCD_ITEM(4, IOMUXC_BASE_ADDR + 0x564, 0x00380040)	/* SDODT1: pue=1 but PKE=0, dse 7 */
MXC_DCD_ITEM(5, IOMUXC_BASE_ADDR + 0x568, 0x00380040)	/* SDQS2: pue=1 but PKE=0, dse 7 */
MXC_DCD_ITEM(6, IOMUXC_BASE_ADDR + 0x570, 0x00200000)	/* SDCLK_1: dse 4 - 75 NA 60 NA */
MXC_DCD_ITEM(7, IOMUXC_BASE_ADDR + 0x574, 0x00380000)	/* CAS: dse 7 */
MXC_DCD_ITEM(8, IOMUXC_BASE_ADDR + 0x578, 0x00200000)	/* SDCLK_0: dse 4 */
MXC_DCD_ITEM(9, IOMUXC_BASE_ADDR + 0x57c, 0x00380040)	/* SDQS0: pue=1 but PKE=0, dse 7 */
MXC_DCD_ITEM(10, IOMUXC_BASE_ADDR + 0x580, 0x00380040)	/* SDODT0: pue=1 but PKE=0, dse 7 */
MXC_DCD_ITEM(11, IOMUXC_BASE_ADDR + 0x584, 0x00380000)	/* DQM0: dse 7 */
MXC_DCD_ITEM(12, IOMUXC_BASE_ADDR + 0x588, 0x00380000)	/* RAS: dse 7 */
MXC_DCD_ITEM(13, IOMUXC_BASE_ADDR + 0x590, 0x00380040)	/* SDQS1: pue=1 but PKE=0, dse 7 */
MXC_DCD_ITEM(14, IOMUXC_BASE_ADDR + 0x594, 0x00380000)	/* DQM1: dse 7 */
MXC_DCD_ITEM(15, IOMUXC_BASE_ADDR + 0x6f0, 0x00380000)	/* GRP_ADDDS A0-A15 BA0-BA2: dse 7 */
MXC_DCD_ITEM(16, IOMUXC_BASE_ADDR + 0x6f4, 0x00000200)	/* GRP_DDRMODE_CTL: DDR2 SDQS0-3 */
MXC_DCD_ITEM(17, IOMUXC_BASE_ADDR + 0x6fc, 0x00000000)	/* GRP_DDRPKE: PKE=0 for A0-A15,CAS,CS0,CS1,D0-D31,DQM0-3,RAS,SDBA0-2,SDCLK_0-1,SDWE */
MXC_DCD_ITEM(18, IOMUXC_BASE_ADDR + 0x714, 0x00000000)	/* GRP_DDRMODE: CMOS input type?????? D0-D31 */
MXC_DCD_ITEM(19, IOMUXC_BASE_ADDR + 0x718, 0x00380000)	/* GRP_B0DS: dse 7 for D0-D7 */
MXC_DCD_ITEM(20, IOMUXC_BASE_ADDR + 0x71c, 0x00380000)	/* GRP_B1SD: dse 7 for D8-D15 */
MXC_DCD_ITEM(21, IOMUXC_BASE_ADDR + 0x720, 0x00380000)	/* GRP_CTLDS: dse 7 for CS0-1, SDCKE0-1, SDWE*/
MXC_DCD_ITEM(22, IOMUXC_BASE_ADDR + 0x724, 0x06000000)	/* GRP_DDR_TYPE: A0-A15,CAS,CS0-1,D0-D31,DQM0-3,RAS,SDBA0-2,SDCKE0-1,SDCLK_0-1,SDODT0-1,SDQS0-3,SDWE  3=20 ohms*/
MXC_DCD_ITEM(23, IOMUXC_BASE_ADDR + 0x728, 0x00380000)	/* GRP_B2DS: dse 7 for D16-D23 */
MXC_DCD_ITEM(24, IOMUXC_BASE_ADDR + 0x72c, 0x00380000)	/* GRP_B3DS: dse 7 fro D24-D31 */
MXC_DCD_ITEM(25, ESDCTL_BASE_ADDR + 0x088, 0x2b2f3031)	/* RDDLCTL */
MXC_DCD_ITEM(26, ESDCTL_BASE_ADDR + 0x090, 0x40363333)	/* WRDLCTL */
MXC_DCD_ITEM(27, ESDCTL_BASE_ADDR + 0x098, 0x00000f00)	/* SDCTRL */
MXC_DCD_ITEM(28, ESDCTL_BASE_ADDR + 0x0f8, 0x00000800)	/* MUR */
MXC_DCD_ITEM(29, ESDCTL_BASE_ADDR + 0x07c, 0x01310132)	/* DGCTRL0 */
MXC_DCD_ITEM(30, ESDCTL_BASE_ADDR + 0x080, 0x0133014b)	/* DGCTRL1 */
MXC_DCD_ITEM(31, ESDCTL_BASE_ADDR + 0x018, 0x000016d0)	/* ESDMISC, (0)8 banks, (2)DDR2 */

#ifdef USE_CSD1
//MXC_DCD_ITEM(32, ESDCTL_BASE_ADDR + 0x000, 0xc4110000) /* ESDCTL, (4)15 rows, (1)10 columns (0)BL 4,  (1)32 bit width*/
MXC_DCD_ITEM(32, ESDCTL_BASE_ADDR + 0x000, 0xc3110000)	/* ESDCTL, (3)14 rows, (1)10 columns (0)BL 4,  (1)32 bit width*/
#else
//MXC_DCD_ITEM(32, ESDCTL_BASE_ADDR + 0x000, 0x84110000) /* ESDCTL, (4)15 rows, (1)10 columns (0)BL 4,  (1)32 bit width */
MXC_DCD_ITEM(32, ESDCTL_BASE_ADDR + 0x000, 0x83110000)	/* ESDCTL, (3)14 rows, (1)10 columns (0)BL 4,  (1)32 bit width */
#endif
MXC_DCD_ITEM(33, ESDCTL_BASE_ADDR + 0x00c, 0x4d5122d2)	/* ESDCFG0 */
MXC_DCD_ITEM(34, ESDCTL_BASE_ADDR + 0x010, 0x92d18a22)	/* ESDCFG1 */
MXC_DCD_ITEM(35, ESDCTL_BASE_ADDR + 0x014, 0x00c70092)	/* ESDCFG2 */
MXC_DCD_ITEM(36, ESDCTL_BASE_ADDR + 0x02c, 0x000026d2)	/* ESDRWD */
MXC_DCD_ITEM(37, ESDCTL_BASE_ADDR + 0x030, 0x009f000e)	/* ESDOR */
MXC_DCD_ITEM(38, ESDCTL_BASE_ADDR + 0x008, 0x12272000)	/* ESDOTC */
MXC_DCD_ITEM(39, ESDCTL_BASE_ADDR + 0x004, 0x00030012)	/* ESDPDC */

MXC_DCD_ITEM(40, ESDCTL_BASE_ADDR + 0x01c, 0x04008010)	/* ESDSCR */
MXC_DCD_ITEM(41, ESDCTL_BASE_ADDR + 0x01c, 0x00008032)	/* ESDSCR */
MXC_DCD_ITEM(42, ESDCTL_BASE_ADDR + 0x01c, 0x00008033)	/* ESDSCR */
MXC_DCD_ITEM(43, ESDCTL_BASE_ADDR + 0x01c, 0x00008031)	/* ESDSCR */
MXC_DCD_ITEM(44, ESDCTL_BASE_ADDR + 0x01c, 0x0b5280b0)	/* ESDSCR DLL_RST0 */
MXC_DCD_ITEM(45, ESDCTL_BASE_ADDR + 0x01c, 0x04008010)	/* ESDSCR */
MXC_DCD_ITEM(46, ESDCTL_BASE_ADDR + 0x01c, 0x00008020)	/* ESDSCR */
MXC_DCD_ITEM(47, ESDCTL_BASE_ADDR + 0x01c, 0x00008020)	/* ESDSCR */
MXC_DCD_ITEM(48, ESDCTL_BASE_ADDR + 0x01c, 0x0a528030)	/* ESDSCR */
MXC_DCD_ITEM(49, ESDCTL_BASE_ADDR + 0x01c, 0x03c68031)	/* ESDSCR */
MXC_DCD_ITEM(50, ESDCTL_BASE_ADDR + 0x01c, 0x00468031)	/* ESDSCR */

#ifdef USE_CSD1
MXC_DCD_ITEM(51, ESDCTL_BASE_ADDR + 0x01c, 0x04008018)	/* ESDSCR */
MXC_DCD_ITEM(52, ESDCTL_BASE_ADDR + 0x01c, 0x0000803a)	/* ESDSCR */
MXC_DCD_ITEM(53, ESDCTL_BASE_ADDR + 0x01c, 0x0000803b)	/* ESDSCR */
MXC_DCD_ITEM(54, ESDCTL_BASE_ADDR + 0x01c, 0x00008039)	/* ESDSCR */
MXC_DCD_ITEM(55, ESDCTL_BASE_ADDR + 0x01c, 0x0b528138)	/* ESDSCR DLL_RST1 */
MXC_DCD_ITEM(56, ESDCTL_BASE_ADDR + 0x01c, 0x04008018)	/* ESDSCR */
MXC_DCD_ITEM(57, ESDCTL_BASE_ADDR + 0x01c, 0x00008028)	/* ESDSCR */
MXC_DCD_ITEM(58, ESDCTL_BASE_ADDR + 0x01c, 0x00008028)	/* ESDSCR */
MXC_DCD_ITEM(59, ESDCTL_BASE_ADDR + 0x01c, 0x0a528038)	/* ESDSCR */
MXC_DCD_ITEM(60, ESDCTL_BASE_ADDR + 0x01c, 0x03c68039)	/* ESDSCR */
MXC_DCD_ITEM(61, ESDCTL_BASE_ADDR + 0x01c, 0x00468039)	/* ESDSCR */
#endif

//MXC_DCD_ITEM(62, ESDCTL_BASE_ADDR + 0x020, 0x00005800)	/* ESDREF, (1)32k refresh cycle rate, (3) 4 refreshes/cycle */
MXC_DCD_ITEM(62, ESDCTL_BASE_ADDR + 0x020, 0x00004800)	/* ESDREF, (1)32k refresh cycle rate, (1) 2 refreshes/cycle */
MXC_DCD_ITEM(63, ESDCTL_BASE_ADDR + 0x058, 0x00033337)	/* ODTCTRL */
MXC_DCD_ITEM(64, ESDCTL_BASE_ADDR + 0x01c, 0x00000000)	/* ESDSCR: AXI address readies normal operation */
cmd_end1:

hdr_end:


relocate:				@ relocate U-Boot to RAM
	ldr	r1, boot_data		@ where we want to be
	adr	r0, _image_base + CONFIG_FLASH_HEADER_OFFSET	@ r0 <- current position of code
	sub	r0, r0, #CONFIG_FLASH_HEADER_OFFSET
	cmp	r0, r1
	beq	2f
	sub	lr, lr, r0
	add	lr, lr, r1

	ldr	r2,image_len
	add	r2, r0, r2		@ r2 <- source end address

1:					@ copy 32 bytes at a time
	ldmia	r0!, {r3 - r10}		@ copy from source address [r0]
	stmia	r1!, {r3 - r10}		@ copy to   target address [r1]
	cmp	r0, r2			@ until source end addreee [r2]
	blo	1b
2:
	mov	pc, lr
#endif
