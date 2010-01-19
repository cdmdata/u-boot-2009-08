/*
 * (C) Copyright 2009
 * Boundary Devices
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <common.h>
#include <command.h>
#include <spi.h>
#include <imx_spi.h>
#include <asm/arch/imx_spi_pmic.h>

enum {
	REG_INT_STATUS0 = 0,
	REG_INT_MASK0,
	REG_INT_SENSE0,
	REG_INT_STATUS1,
	REG_INT_MASK1,
	REG_INT_SENSE1,
	REG_PU_MODE_S,
	REG_IDENTIFICATION,
	REG_UNUSED0,
	REG_ACC0,
	REG_ACC1,		/*10 */
	REG_UNUSED1,
	REG_UNUSED2,
	REG_POWER_CTL0,
	REG_POWER_CTL1,
	REG_POWER_CTL2,
	REG_REGEN_ASSIGN,
	REG_UNUSED3,
	REG_MEM_A,
	REG_MEM_B,
	REG_RTC_TIME,		/*20 */
	REG_RTC_ALARM,
	REG_RTC_DAY,
	REG_RTC_DAY_ALARM,
	REG_SW_0,
	REG_SW_1,
	REG_SW_2,
	REG_SW_3,
	REG_SW_4,
	REG_SW_5,
	REG_SETTING_0,		/*30 */
	REG_SETTING_1,
	REG_MODE_0,
	REG_MODE_1,
	REG_POWER_MISC,
	REG_UNUSED4,
	REG_UNUSED5,
	REG_UNUSED6,
	REG_UNUSED7,
	REG_UNUSED8,
	REG_UNUSED9,		/*40 */
	REG_UNUSED10,
	REG_UNUSED11,
	REG_ADC0,
	REG_ADC1,
	REG_ADC2,
	REG_ADC3,
	REG_ADC4,
	REG_CHARGE,
	REG_USB0,
	REG_USB1,		/*50 */
	REG_LED_CTL0,
	REG_LED_CTL1,
	REG_LED_CTL2,
	REG_LED_CTL3,
	REG_UNUSED12,
	REG_UNUSED13,
	REG_TRIM0,
	REG_TRIM1,
	REG_TEST0,
	REG_TEST1,		/*60 */
	REG_TEST2,
	REG_TEST3,
	REG_TEST4,
	NUM_REGISTERS
};

static char const * const regnames[] = {
	"INT_STATUS0",
	"INT_MASK0",
	"INT_SENSE0",
	"INT_STATUS1",
	"INT_MASK1",
	"INT_SENSE1",
	"PU_MODE_S",
	"IDENTIFICATION",
	"UNUSED0",
	"ACC0",
	"ACC1",		/*10 */
	"UNUSED1",
	"UNUSED2",
	"POWER_CTL0",
	"POWER_CTL1",
	"POWER_CTL2",
	"REGEN_ASSIGN",
	"UNUSED3",
	"MEM_A",
	"MEM_B",
	"RTC_TIME",		/*20 */
	"RTC_ALARM",
	"RTC_DAY",
	"RTC_DAY_ALARM",
	"SW_0",
	"SW_1",
	"SW_2",
	"SW_3",
	"SW_4",
	"SW_5",
	"SETTING_0",		/*30 */
	"SETTING_1",
	"MODE_0",
	"MODE_1",
	"POWER_MISC",
	"UNUSED4",
	"UNUSED5",
	"UNUSED6",
	"UNUSED7",
	"UNUSED8",
	"UNUSED9",		/*40 */
	"UNUSED10",
	"UNUSED11",
	"ADC0",
	"ADC1",
	"ADC2",
	"ADC3",
	"ADC4",
	"CHARGE",
	"USB0",
	"USB1",		/*50 */
	"LED_CTL0",
	"LED_CTL1",
	"LED_CTL2",
	"LED_CTL3",
	"UNUSED12",
	"UNUSED13",
	"TRIM0",
	"TRIM1",
	"TEST0",
	"TEST1",		/*60 */
	"TEST2",
	"TEST3",
	"TEST4",
};

int do_pmic (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	struct spi_slave *slave = spi_pmic_probe();
	if ((2==argc) && (0 ==strcmp(argv[1],"all")) ){
		unsigned regnum ;
		for( regnum=0 ; regnum < NUM_REGISTERS ; regnum++ ) {
			if ((regnum<REG_ADC0) || (regnum>REG_ADC4)) {
				unsigned prev = pmic_reg(slave, regnum, 0, 0);
				printf( "0x%02x  %-20s 0x%06x\n", regnum, regnames[regnum], prev);
			} else {
				printf( "0x%02x  %-20s ********\n", regnum, regnames[regnum]);
			}
		}
	} else if (1 < argc) {
		unsigned reg = simple_strtoul(argv[1], NULL, 0);
		if (NUM_REGISTERS > reg) {
			int do_write = (2 < argc);
			unsigned val = (do_write ? simple_strtoul(argv[2],NULL,0) : 0 );

			unsigned prev = pmic_reg(slave, reg, val, do_write);
			printf( "0x%02x  %-20s 0x%06x", reg, regnames[reg], prev );
			if (do_write)
				printf(" --> 0x%06x\n", val );
			else
				printf("\n");
		}
		else
			printf("invalid register %s, max is %u (0x%x)\n", argv[1], NUM_REGISTERS-1, NUM_REGISTERS-1 );
	} else {
		show_pmic_info(slave);
	}
	return 0 ;
}

/***************************************************/

U_BOOT_CMD(
	pmic,	5,	1,	do_pmic,
	"pmic    - Read/write PMIC registers through SPI\n",
	"register [value]\n"
);
