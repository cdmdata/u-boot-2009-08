/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX53-EVK Freescale board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __NITROGEN53A_REV1_H
#define __NITROGEN53A_REV1_H

#include <configs/nitrogen53.h>
#undef CONFIG_MACH_TYPE
#define CONFIG_MACH_TYPE	MACH_TYPE_MX53_NITROGEN_A
#undef CONFIG_BOARD_NAME
#define CONFIG_BOARD_NAME	"MX53-Nitrogen_A rev1"

/* This will damage DDR on new boards if enabled */
#define CONFIG_ALLOW_VBUCKCORE_BOOST

#undef CONFIG_UART_BASE_ADDR
#define CONFIG_UART_BASE_ADDR		UART3_BASE_ADDR
#define CONFIG_UART_DA9052_GP12		/* Low means new board - UART3, high means old board - UART1 (rev 0)*/

#undef CONFIG_EXTRA_ENV_SETTINGS
#define	CONFIG_EXTRA_ENV_SETTINGS	\
	"ethprime=FEC0\0"		\
	"machid=c62\0"			\

#define CONFIG_POWER_KEY

#undef CONFIG_PWM2_DUTY
#define CONFIG_PWM2_DUTY	235	/* 235 out of 256 */

#endif
