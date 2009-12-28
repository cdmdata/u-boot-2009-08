/*
 * watchdog.c - driver for MX51 on-chip watchdog
 *
 * Copyright (c) 2007-2008 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <watchdog.h>

#define WDOG_BASE 0x73f98000
#define MX51_WCR  WDOG_BASE+0x00
#define MX51_WSR  WDOG_BASE+0x02
#define MX51_WRSR WDOG_BASE+0x04
#define MX51_WICR WDOG_BASE+0x06
#define MX51_WMCR WDOG_BASE+0x08

#define __REG(x)     (*((volatile u16 *)(x)))

#ifdef CONFIG_HW_WATCHDOG
void hw_watchdog_reset(void)
{
	__REG(MX51_WSR) = 0x5555 ;
	__REG(MX51_WSR) = 0xAAAA ;
}

void hw_watchdog_init(void)
{
	__REG(MX51_WCR) = 0xFF8F ;
	hw_watchdog_reset();
}

#endif
