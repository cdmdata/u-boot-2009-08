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

#ifndef __NITROGEN53K_H
#define __NITROGEN53K_H

#include <configs/nitrogen53.h>
#define CONFIG_H5PS2G83AFR_S6	/* 1G total memory */
#define CONFIG_K2	/* Next rev of Nitrogenk */

#undef CONFIG_MACH_TYPE
#define CONFIG_MACH_TYPE	MACH_TYPE_MX53_NITROGEN_K
#undef CONFIG_BOARD_NAME
#define CONFIG_BOARD_NAME	"MX53-Nitrogen_K"

#undef CONFIG_TFP410_LDO10
#define CONFIG_TF410_OFF
#define CONFIG_W3
#define CONFIG_LCD_PINS_OFF	/* only LVDS panel used */

#undef CONFIG_PWM2_DUTY
#define CONFIG_PWM2_DUTY	40	/* was 230 out of 256 for previous rev */

#undef CONFIG_UART_BASE_ADDR
#undef CONFIG_TFP410_BUS

#define CONFIG_W3_SCL_PIN	MX53_PIN_EIM_A18
#define CONFIG_W3_SCL		GPIO_NUMBER(2, 20)	/* EIM_A18 */
#define CONFIG_W3_SDA_PIN	MX53_PIN_EIM_A19
#define CONFIG_W3_SDA		GPIO_NUMBER(2, 19)	/* EIM_A19 */

#define CONFIG_LVDS0_24BIT

#ifdef CONFIG_K2
#define CONFIG_UART_BASE_ADDR	UART3_BASE_ADDR
#define CONFIG_TFP410_BUS	((void *)I2C3_BASE_ADDR)
#define CONFIG_W3_CS_PIN	MX53_PIN_EIM_DA3
#define CONFIG_W3_CS		GPIO_NUMBER(3, 3)	/* EIM_DA3 */
#define CONFIG_UART_TX_MASK	0x4	/* UART 3 only */
#define CONFIG_UART_RX_MASK	0x4	/* UART 3 only */
#define CONFIG_BQ2416X_CHARGER
#define CONFIG_BQ2416X_WATCHDOG
#define CONFIG_BQ2416X_I2C_ENABLE	GPIO_NUMBER(3, 9)
#define CONFIG_BQ2416X_I2C_BUS	((void *)I2C2_BASE_ADDR)
#define CONFIG_BQ2416X_I2C_ADDR	0x6b
#undef N53_I2C_CONNECTOR_BUFFER_ENABLE

#else
#define CONFIG_UART_BASE_ADDR	UART1_BASE_ADDR
#define CONFIG_TFP410_BUS	((void *)I2C2_BASE_ADDR)
#define CONFIG_W3_CS_PIN	MX53_PIN_EIM_D24
#define CONFIG_W3_CS		GPIO_NUMBER(3, 24)	/* EIM_D24 */
#define CONFIG_UART_TX_MASK	0x1	/* UART 1 only */
#define CONFIG_UART_RX_MASK	0x1	/* UART 1 only */
#define CONFIG_DA9052_CHARGER_ENABLE
#endif

//#define CONFIG_POWER_KEY
#define CONFIG_EMMC_DDR_MODE

/* Indicate to esdhc driver which ports support 8-bit data */
#define CONFIG_MMC_8BIT_PORTS           0x2   /* dev1 esdhc4 */

#define CONFIG_POWER_KEY

#undef CONFIG_EXTRA_ENV_SETTINGS
#define	CONFIG_EXTRA_ENV_SETTINGS	\
	"ethprime=FEC0\0"		\
	"upgradeu=fatload mmc 0 70008000 n53_upgrade && source 70008000\0" \
	"clearenv=sf probe 1 && sf erase 0x5f000 0x1000 && echo 'environment reset to factory defaults';\0" \
	"panel=vesa:1024x600@60c\0" \
	"bootargs=earlyprintk ldb=single,di=0,ch0_map=SPWG\0" \
	"lvds=1,1\0"

#undef CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND	"if kbd ; then errmsg=\"Button pressed, boot stopped\" ; "				\
				"else "											\
					"if fatload mmc 0 70008000 nitrogen53_bootscript* ; then source 70008000 ; "	\
					"elif fatload mmc 1 70008000 nitrogen53_bootscript* ; then source 70008000 ; "	\
					"fi ; "										\
					"errmsg=\"Error running bootscript!\" ; "					\
				"fi ; "											\
				"lecho $errmsg ; echo $errmsg "
#endif
