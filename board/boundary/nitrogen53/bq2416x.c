/*
 * Copyright (C) 2012, Boundary Devivces
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
#include "bq2416x.h"
void bus_i2c_init(unsigned base, int speed, int unused);
int bus_i2c_write(unsigned base, uchar chip, uint addr, int alen, const uchar *buf, int len);
int bus_i2c_read(unsigned base, uchar chip, uint addr, int alen, uchar *buf, int len);

#define BQ24163_STATUS_CTL	0
#define BQ24163_SUPPLY_STATUS	1
#define BQ24163_CONTROL		2
#define BQ24163_BATT_VOLTAGE	3
#define BQ24163_ID		4
#define BQ24163_BATT_CURRENT	5
#define BQ24163_DPPM		6
#define BQ24163_SAFETY_TIMER	7

#define BIT(n) (1 << n)

int bq2416x_reg_write(unsigned reg, unsigned char val)
{
	int ret;
	ret = bus_i2c_write(CONFIG_BQ2416X_I2C_BUS, CONFIG_BQ2416X_I2C_ADDR, reg, 1, &val, 1);
	if (ret)
		printf("%s: reg %x, val=%x failed\n", __func__, reg, val);
	return ret;
}

int bq2416x_reg_read(unsigned reg)
{
	int ret;
	unsigned char val;
	ret = bus_i2c_read(CONFIG_BQ2416X_I2C_BUS, CONFIG_BQ2416X_I2C_ADDR, reg, 1, &val, 1);
	if (ret)
		printf("%s: reg %x failed\n", __func__, reg);
	else
		ret = val;
	return ret;
}

unsigned char bq2416x_init_values[] = {
	0x80, 0x00, 0x0c, 0x8e, 0x00, 0x32, 0x00, 0x28
};

int bq2416x_init(void)
{
	int i;
	int ret;
	/* disable charging while parameters are changed */
	ret = bq2416x_reg_write(BQ24163_CONTROL, bq2416x_init_values[BQ24163_CONTROL] | BIT(1));
	for (i = BQ24163_CONTROL + 1; i < 8; i++) {
		if (i != BQ24163_ID) {
			ret = bq2416x_reg_write(i, bq2416x_init_values[i]);
			if (ret < 0)
				return ret;
		}
	}
	/* Last write reenables charging */
	for (i = 0; i <= BQ24163_CONTROL; i++) {
		ret = bq2416x_reg_write(i, bq2416x_init_values[i]);
		if (ret < 0)
			break;
	}
	return ret;
}

static unsigned long reset_time;

int bq2416x_watchdog(unsigned long cur_time)
{
	int ret;
	int val;
	int desired;
	if ((unsigned long)(cur_time - reset_time) < 25000)
		return 0;
	ret = bq2416x_reg_write(BQ24163_STATUS_CTL, BIT(7));
	if (ret >= 0)
		reset_time = cur_time;
	reset_time = cur_time;

	val = bq2416x_reg_read(BQ24163_BATT_VOLTAGE);
	desired = bq2416x_init_values[BQ24163_BATT_VOLTAGE];
	if ((val >= 0) && (val != desired))
		bq2416x_reg_write(BQ24163_BATT_VOLTAGE, desired);
	return ret;
}
