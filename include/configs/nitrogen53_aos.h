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

#ifndef __NITROGEN53_1G_H
#define __NITROGEN53_1G_H

#include <configs/nitrogen53.h>
#undef CONFIG_BOARD_NAME
#define CONFIG_BOARD_NAME	"MX53-Nitrogen_aos"

#undef CONFIG_TFP410_LDO10
#undef CONFIG_TFP410_BUS
#define CONFIG_H5PS1G83EFR_S6C	/* 512MB, 400 MHz */

						/* dgctrl0,    dgctrl1,    rddlctl,    wrdlctl */
//#define CONFIG_H5PS1G83EFR_S6C_CALIBRATION	0x012f0126, 0x01260138, 0x2d2d2f2d, 0x4d4a5046
#define CONFIG_H5PS1G83EFR_S6C_CALIBRATION	0x013a016d, 0x016d016f, 0x24222424, 0x514d5549
#endif
