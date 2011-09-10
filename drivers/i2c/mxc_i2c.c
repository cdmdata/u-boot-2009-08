/*
 * i2c driver for Freescale mx31
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * (C) Copyright 2008-2010 Freescale Semiconductor, Inc.
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
#include <asm/arch/mxc_i2c.h>

#if defined(CONFIG_HARD_I2C)

#define IADR	0x00
#define IFDR	0x04
#define I2CR	0x08
#define I2SR	0x0c
#define I2DR	0x10

#define I2CR_IEN	(1 << 7)
#define I2CR_IIEN	(1 << 6)
#define I2CR_MSTA	(1 << 5)
#define I2CR_MTX	(1 << 4)
#define I2CR_TX_NO_AK	(1 << 3)
#define I2CR_RSTA	(1 << 2)

#define I2SR_ICF	(1 << 7)
#define I2SR_IBB	(1 << 5)
#define I2SR_IIF	(1 << 1)
#define I2SR_RX_NO_AK	(1 << 0)

#define I2C_MAX_TIMEOUT		100000
#define I2C_TIMEOUT_TICKET	1

#undef DEBUG

#ifdef DEBUG
#define DPRINTF(args...)  printf(args)
#else
#define DPRINTF(args...)
#endif

static u16 div[] = { 30, 32, 36, 42, 48, 52, 60, 72, 80, 88, 104, 128, 144,
	160, 192, 240, 288, 320, 384, 480, 576, 640, 768, 960,
	1152, 1280, 1536, 1920, 2304, 2560, 3072, 3840
};

static inline void i2c_reset(unsigned base)
{
	__REG16(base + I2CR) = 0;	/* Reset module */
	__REG16(base + I2SR) = 0;
	__REG16(base + I2CR) = I2CR_IEN;
	udelay(100);
}

#define ST_BUS_IDLE (0 | (I2SR_IBB << 16))
#define ST_BUS_BUSY (I2SR_IBB | (I2SR_IBB << 16))
#define ST_BYTE_COMPLETE (I2SR_ICF | (I2SR_ICF << 16))
#define ST_BYTE_PENDING (0 | (I2SR_ICF << 16))

static unsigned wait_for_sr_state(unsigned base, unsigned state)
{
	int timeout = I2C_MAX_TIMEOUT;
	unsigned sr;
	for (;;) {
		sr = __REG16(base + I2SR);
		if ((sr & (state >> 16)) == (unsigned short)state)
			break;
		if (!(--timeout)) {
			printf("%s: failed sr=%x cr=%x state=%x\n", __func__,
					sr, __REG16(base + I2CR), state);
			return 0;
		}
		udelay(I2C_TIMEOUT_TICKET);
	}
	DPRINTF("%s:%x\n", __func__, sr);
	return sr | 0x10000;
}

static int tx_byte(unsigned base, u8 byte, int clear_wait)
{
	unsigned sr;
	__REG16(base + I2DR) = byte;
	if (clear_wait) {
		/* cannot use
		 * wait_for_sr_state(base, ST_BYTE_PENDING);
		 * because the time period low is very very small */
		udelay(10);
	}
	sr = wait_for_sr_state(base, ST_BYTE_COMPLETE);
	if ((!sr) || (sr & I2SR_RX_NO_AK)) {
		printf("%s: sr=%x byte=%x\n", __func__, sr, byte);
		return -1;
	}
	DPRINTF("%s:%x\n", __func__, byte);
	return 0;
}

static void toggle_i2c(unsigned base);

static int i2c_addr(unsigned base, uchar chip, uint addr, int alen)
{
	int i, retry = 0;
	for (retry = 0; retry < 3; retry++) {
		if (wait_for_sr_state(base, ST_BUS_IDLE))
			break;
		i2c_reset(base);
		for (i = 0; i < I2C_MAX_TIMEOUT; i++)
			udelay(I2C_TIMEOUT_TICKET);
		toggle_i2c(base);
	}
	if (retry >= 3) {
		printf("%s:bus is busy(%x)\n",
		       __func__, __REG16(base + I2SR));
		return -1;
	}
	__REG16(base + I2CR) = I2CR_IEN | I2CR_MSTA | I2CR_MTX;
	if (!wait_for_sr_state(base, ST_BUS_BUSY)) {
		printf("%s:trigger start fail(%x)\n", __func__,
				__REG16(base + I2SR));
		__REG16(base + I2CR) = I2CR_IEN;	/* send stop */
		udelay(1000);
		i2c_reset(base);
		toggle_i2c(base);
		__REG16(base + I2CR) = I2CR_IEN | I2CR_MSTA | I2CR_MTX;
		if (!wait_for_sr_state(base, ST_BUS_BUSY)) {
			printf("%s:toggle didn't work(%x)\n", __func__,
					__REG16(base + I2SR));
			return -1;
		}
	}
	if (tx_byte(base, chip << 1, 0)) {
		printf("%s:chip address cycle fail(%x)\n",
		       __func__, __REG16(base + I2SR));
		return -1;
	}
	while (alen--)
		if (tx_byte(base, (addr >> (alen * 8)) & 0xff, 0)) {
			printf("%s:device address cycle fail(%x)\n",
			       __func__, __REG16(base + I2SR));
			return -1;
		}
	return 0;
}

int bus_i2c_read(unsigned base, uchar chip, uint addr, int alen, uchar *buf,
		int len)
{
	uint ret;
	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",
		__func__, chip, addr, alen, len);

	if (i2c_addr(base, chip, addr, alen)) {
		printf("i2c_addr failed\n");
		/* send stop */
		__REG16(base + I2CR) = I2CR_IEN | I2CR_TX_NO_AK;
		return -1;
	}

	__REG16(base + I2CR) = I2CR_IEN | I2CR_MSTA | I2CR_MTX | I2CR_RSTA;

	if (tx_byte(base, chip << 1 | 1, 1)) {
		printf("%s:Send 2th chip address fail(%x)\n",
		       __func__, __REG16(base + I2SR));
		/* send stop */
		__REG16(base + I2CR) = I2CR_IEN | I2CR_TX_NO_AK;
		return -1;
	}
	__REG16(base + I2CR) = I2CR_IEN | I2CR_MSTA |
		((len <= 1) ? I2CR_TX_NO_AK : 0);
	ret = __REG16(base + I2DR);	/* dummy read to clear ICF */
	DPRINTF("CR=%x\n", __REG16(base + I2CR));

	for (;;) {
		if (!wait_for_sr_state(base, ST_BYTE_COMPLETE)) {
			printf("%s: fail sr=%x cr=%x, len=%i\n", __func__,
					__REG16(base + I2SR),
					__REG16(base + I2CR), len);
			/* send stop */
			__REG16(base + I2CR) = I2CR_IEN | I2CR_TX_NO_AK;
			return -1;
		}
		if (len == 2) {
			__REG16(base + I2CR) = I2CR_IEN | I2CR_MSTA
					| I2CR_TX_NO_AK;
		}
		if (len <= 1) {
			/* send stop */
			__REG16(base + I2CR) = I2CR_IEN | I2CR_TX_NO_AK;
		}
		ret = __REG16(base + I2DR);
		DPRINTF("%s:%x\n", __func__, ret);
		if (len <= 0)
			break;
		*buf++ = ret;
		if (!(--len))
			break;
	}
	if (!wait_for_sr_state(base, ST_BUS_IDLE))
		printf("%s:trigger stop fail\n", __func__);
	return 0;
}

int bus_i2c_write(unsigned base, uchar chip, uint addr, int alen, uchar *buf,
		int len)
{
	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",
		__func__, chip, addr, alen, len);

	if (i2c_addr(base, chip, addr, alen)) {
		__REG16(base + I2CR) = I2CR_IEN;	/* send stop */
		return -1;
	}
	while (len--)
		if (tx_byte(base, *buf++, 0)) {
			__REG16(base + I2CR) = I2CR_IEN;	/* send stop */
			return -1;
		}

	__REG16(base + I2CR) = I2CR_IEN;

	if (!wait_for_sr_state(base, ST_BUS_IDLE))
		printf("%s:trigger stop fail\n", __func__);
	return 0;
}

int bus_i2c_probe(unsigned base, uchar chip)
{
	int ret;
	i2c_reset(base);
	ret = bus_i2c_write(base, chip, 0, 0, NULL, 0);
	if (ret)
		printf("probe failed\n");
	return ret;
}

int g_bus;

struct i2c_parms {
	unsigned base;
	void *toggle_data;
	toggle_i2c_fn toggle_fn;
};

struct i2c_parms g_parms[3];

void bus_i2c_init(unsigned base, int speed, int unused,
		toggle_i2c_fn toggle_fn, void *toggle_data)
{
	int freq;
	int i = 0;
	struct i2c_parms *p = g_parms;
	if (!base)
		return;
	for (;;) {
		if (!p->base || (p->base == base)) {
			p->base = base;
			p->toggle_data = toggle_data;
			p->toggle_fn = toggle_fn;
			break;
		}
		p++;
		i++;
		if (i >= ARRAY_SIZE(g_parms))
			return;
	}
#ifdef CONFIG_MX31
	freq = mx31_get_ipg_clk();
#else
	freq = mxc_get_clock(MXC_IPG_PERCLK);
#endif
	for (i = 0; i < 0x1f; i++)
		if (freq / div[i] <= speed)
			break;

	DPRINTF("%s: root clock: %d, speed: %d div: %x\n",
		__func__, freq, speed, i);

	__REG16(base + IFDR) = i;
	i2c_reset(base);
}

static struct i2c_parms *i2c_get_parms(unsigned module_base)
{
	int i = 0;
	struct i2c_parms *p = g_parms;
	while (i < ARRAY_SIZE(g_parms)) {
		if (p->base == module_base)
			return p;
		p++;
		i++;
	}
	printf("Invalid I2C base: 0x%x\n", module_base);
	return NULL;
}

static void toggle_i2c(unsigned base)
{
	struct i2c_parms *p = i2c_get_parms(base);
	if (!p)
		return;
	if (p->toggle_fn)
		p->toggle_fn(p->toggle_data);
}

int i2c_get_bus_num(void)
{
	return g_bus;
}

int i2c_set_bus_num(unsigned bus_idx)
{
	if (bus_idx >= ARRAY_SIZE(g_parms))
		return -1;
	if (!g_parms[bus_idx].base)
		return -1;
	g_bus = bus_idx;
	return 0;
}

int i2c_read(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	return bus_i2c_read(g_parms[g_bus].base, chip, addr, alen, buf, len);
}

int i2c_write(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	return bus_i2c_write(g_parms[g_bus].base, chip, addr, alen, buf, len);
}

int i2c_probe(uchar chip)
{
	return bus_i2c_probe(g_parms[g_bus].base, chip);
}

void i2c_init(int speed, int slave_addr)
{
	struct i2c_parms *p = &g_parms[g_bus];
	unsigned base = p->base;
#ifdef CONFIG_SYS_I2C_PORT
	if (!base)
		base = CONFIG_SYS_I2C_PORT;
#endif
	bus_i2c_init(base, speed, slave_addr, p->toggle_fn, p->toggle_data);
}
#endif				/* CONFIG_HARD_I2C */
