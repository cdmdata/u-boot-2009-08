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
#define CONFIG_H5PS2G83AFR_S6	/* 1G total memory */
						/* dgctrl0,    dgctrl1,    rddlctl,    wrdlctl */
//#define CONFIG_H5PS2G83AFR_S6_CALIBRATION	0x015c0137, 0x013b013b, 0x29292626, 0x524c544a
//#define CONFIG_H5PS2G83AFR_S6_CALIBRATION	0x01370139, 0x0167013d, 0x27252723, 0x534a5548
//#define CONFIG_H5PS2G83AFR_S6_CALIBRATION	0x0138013c, 0x016a013c, 0x27242420, 0x544e5849	//oven calibrate
//#define CONFIG_H5PS2G83AFR_S6_CALIBRATION	0x01680165, 0x0168013e, 0x22222724  0x524b544b	//oven, 120 ohm term, 1st rev
//#define CONFIG_H5PS2G83AFR_S6_CALIBRATION	0x01320136, 0x0161013a, 0x26262624, 0x534b5549
#define CONFIG_H5PS2G83AFR_S6_CALIBRATION	0x015b0139, 0x0161016a, 0x28282626, 0x534c5548
#undef CONFIG_TFP410_LDO10
#undef CONFIG_TFP410_BUS

#endif
