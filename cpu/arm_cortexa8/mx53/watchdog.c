/*
 * watchdog.c - driver for MX53 on-chip watchdog
 *
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <watchdog.h>

#define WDOG_BASE 0x53f98000
#define WDOG_WCR  0x00
#define WDOG_WSR  0x02
#define WDOG_WRSR 0x04
#define WDOG_WICR 0x06
#define WDOG_WMCR 0x08

void hw_watchdog_reset(void)
{
	__REG16(WDOG_BASE + WDOG_WSR) = 0x5555 ;
	__REG16(WDOG_BASE + WDOG_WSR) = 0xAAAA ;
}

void hw_watchdog_init(void)
{
	__REG16(WDOG_BASE + WDOG_WCR) = 0xFF8F ;
	hw_watchdog_reset();
}
