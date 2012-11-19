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

#ifndef __NITROGEN53A_REV2_H
#define __NITROGEN53A_REV2_H

#include <configs/nitrogen53.h>
#undef CONFIG_MACH_TYPE
#define CONFIG_MACH_TYPE	MACH_TYPE_MX53_NITROGEN_A
#undef CONFIG_BOARD_NAME
#define CONFIG_BOARD_NAME	"MX53-Nitrogen_A rev2"

#undef CONFIG_UART_BASE_ADDR
#define CONFIG_UART_BASE_ADDR		UART3_BASE_ADDR

#define CONFIG_H5PS1G83EFR_S6C	/* 512MB, 400 MHz */
						/* dgctrl0,    dgctrl1,    rddlctl,    wrdlctl */
#define CONFIG_H5PS1G83EFR_S6C_CALIBRATION	0x013e0143, 0x01430143, 0x24242622, 0x534d554b

#undef CONFIG_EXTRA_ENV_SETTINGS
#define	CONFIG_EXTRA_ENV_SETTINGS	\
	"ethprime=FEC0\0"		\
	"machid=c62\0"			\
	"panel=raw:63500000,1024,768,1,0,1,0,104,152,48,4,23,3,1,1\0" \
	"lvds=1,1\0" \
	"upgradeu=fatload mmc 0 70008000 n53_upgrade && source 70008000\0" \
	"clearenv=sf probe 1 && sf erase 0x5f000 0x1000 && echo 'environment reset to factory defaults'; \0" \

#define CONFIG_POWER_KEY

#define CONFIG_TFP410_HUB_EN	GPIO_NUMBER(3, 11)
#undef N53_I2C_CONNECTOR_BUFFER_ENABLE

#undef CONFIG_PWM2_DUTY
#define CONFIG_PWM2_DUTY	235	/* 235 out of 256 */

#undef CONFIG_BOOTDELAY
#define CONFIG_BOOTDELAY	0

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND	"for disk in 0 1 ; do fatload mmc $disk 70008000 nitrogen53_bootscript* && source 70008000 ; done ; errmsg=\"Error running bootscript!\" ; lecho $errmsg ; echo $errmsg ;"

#define CONFIG_BQ20Z75_I2C_BUS ((void *)I2C2_BASE_ADDR)

#endif
