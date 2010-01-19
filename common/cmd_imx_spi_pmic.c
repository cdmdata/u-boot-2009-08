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

int do_pmic (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	struct spi_slave *slave = spi_pmic_probe();
	if (1 < argc) {
		unsigned reg = simple_strtoul(argv[1], NULL, 0);
		int do_write = (2 < argc);
		unsigned val = (do_write ? simple_strtoul(argv[2],NULL,0) : 0 );

		unsigned prev = pmic_reg(slave, reg, val, do_write);
		printf( "reg[0x%x]: 0x%x -> 0x%x\n", reg, prev, val );
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
