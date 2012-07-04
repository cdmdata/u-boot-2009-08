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
#include <asm/errno.h>
#include <asm/io.h>

struct mxc_i2c_regs {
	uint32_t	iadr;
	uint32_t	ifdr;
	uint32_t	i2cr;
	uint32_t	i2sr;
	uint32_t	i2dr;
};

#define I2CR_IEN	(1 << 7)
#define I2CR_IIEN	(1 << 6)
#define I2CR_MSTA	(1 << 5)
#define I2CR_MTX	(1 << 4)
#define I2CR_TX_NO_AK	(1 << 3)
#define I2CR_RSTA	(1 << 2)

#define I2SR_ICF	(1 << 7)
#define I2SR_IBB	(1 << 5)
#define I2SR_IAL	(1 << 4)
#define I2SR_IIF	(1 << 1)
#define I2SR_RX_NO_AK	(1 << 0)

static u16 divisor_map[] = {
	  30,   32,   36,   42,   48,   52,   60,   72,
	  80,   88,  104,  128,  144,  160,  192,  240,
	 288,  320,  384,  480,  576,  640,  768,  960,
	1152, 1280, 1536, 1920, 2304, 2560, 3072, 3840,
	  22,   24,   26,   28,   32,   36,   40,   44,
	  48,   56,   64,   72,   80,   96,  112,  128,
	 160,  192,  224,  256,  320,  384,  448,  512,
	 640,  768,  896, 1024, 1280, 1536, 1792, 2048,
};

static inline void i2c_reset(struct mxc_i2c_regs *i2c_regs)
{
	writeb(0, &i2c_regs->i2cr);		/* Reset module */
	writeb(0, &i2c_regs->i2sr);
	writeb(I2CR_IEN, &i2c_regs->i2cr);
	udelay(100);
}

/*
 * Set I2C Bus speed
 */
int bus_i2c_set_bus_speed(void *base, int max_speed)
{
	int freq;
	int i;
	int min_div;
	int best_index = 0x1f;
	int best_div = divisor_map[best_index];
	struct mxc_i2c_regs *i2c_regs = (struct mxc_i2c_regs *)base;

#ifdef CONFIG_MX31
	freq = mx31_get_ipg_clk();
#else
	freq = mxc_get_clock(MXC_IPG_PERCLK);
#endif
	min_div = (freq + max_speed - 1) / max_speed;
	for (i = 0 ; i < ARRAY_SIZE(divisor_map); i++) {
		int cur_div = divisor_map[i];
		if ((cur_div < best_div) && (cur_div >= min_div)) {
			best_index = i;
			best_div = cur_div;
		}
	}

	printf("%s: root clock: %d, speed: %d index: %x\n",
		__func__, freq, freq / best_div, best_index);
	writeb(best_index, &i2c_regs->ifdr);
	i2c_reset(i2c_regs);
	return 0;
}

/*
 * Get I2C Speed
 */
unsigned int bus_i2c_get_bus_speed(void *base)
{
	int freq;
	struct mxc_i2c_regs *i2c_regs = (struct mxc_i2c_regs *)base;
	u8 clk_idx = readb(&i2c_regs->ifdr);

#ifdef CONFIG_MX31
	freq = mx31_get_ipg_clk();
#else
	freq = mxc_get_clock(MXC_IPG_PERCLK);
#endif
	return freq / divisor_map[clk_idx & 0x3f];
}

#define ST_BUS_IDLE (0 | (I2SR_IBB << 8))
#define ST_BUS_BUSY (I2SR_IBB | (I2SR_IBB << 8))
#define ST_BYTE_COMPLETE (I2SR_ICF | (I2SR_ICF << 8))
#define ST_BYTE_PENDING (0 | (I2SR_ICF << 8))
#define ST_IIF (I2SR_IIF | (I2SR_IIF << 8))

static int wait_for_sr_state(struct mxc_i2c_regs *i2c_regs, unsigned state)
{
	unsigned sr;
	ulong elapsed;
	ulong start_time = get_timer(0);
	for (;;) {
		sr = readb(&i2c_regs->i2sr);
		if (sr & I2SR_IAL) {
			writeb(sr & ~I2SR_IAL, &i2c_regs->i2sr);
			printf("%s: Arbitration lost sr=%x cr=%x state=%x\n",
				__func__, sr, readb(&i2c_regs->i2cr), state);
			return -ERESTART;
		}
		if ((sr & (state >> 8)) == (unsigned char)state)
			return sr;
		elapsed = get_timer(start_time);
		if (elapsed > (CONFIG_SYS_HZ / 10))	/* .1 seconds */
			break;
	}
	printf("%s: failed sr=%x cr=%x state=%x\n", __func__,
			sr, readb(&i2c_regs->i2cr), state);
	return -ETIMEDOUT;
}

static int tx_byte(struct mxc_i2c_regs *i2c_regs, u8 byte)
{
	int ret;

	writeb(0, &i2c_regs->i2sr);
	writeb(byte, &i2c_regs->i2dr);
	ret = wait_for_sr_state(i2c_regs, ST_IIF);
	if (ret < 0)
		return ret;
	if (ret & I2SR_RX_NO_AK)
		return -ENODEV;
	return 0;
}

/*
 * Stop I2C transaction
 */
static void i2c_imx_stop(struct mxc_i2c_regs *i2c_regs)
{
	int ret;
	unsigned int temp = readb(&i2c_regs->i2cr);

	temp &= ~(I2CR_MSTA | I2CR_MTX);
	writeb(temp, &i2c_regs->i2cr);
	ret = wait_for_sr_state(i2c_regs, ST_BUS_IDLE);
	if (ret < 0)
		printf("%s:trigger stop fail\n", __func__);
}

/*
 * Send start signal, chip address and
 * write register address
 */
static int i2c_init_transfer_(struct mxc_i2c_regs *i2c_regs, uchar chip,
		uint addr, int alen)
{
	int ret;

	/* Enable I2C controller */
	if (!(readb(&i2c_regs->i2cr) & I2CR_IEN)) {
		writeb(I2CR_IEN, &i2c_regs->i2cr);
		/* Wait for controller to be stable */
		udelay(50);
	}
	if (readb(&i2c_regs->iadr) == (chip << 1))
		writeb((chip << 1) ^ 2, &i2c_regs->iadr);
	writeb(0, &i2c_regs->i2sr);
	ret = wait_for_sr_state(i2c_regs, ST_BUS_IDLE);
	if (ret < 0)
		return ret;

	/* Start I2C transaction */
	writeb(I2CR_IEN | I2CR_MSTA | I2CR_MTX, &i2c_regs->i2cr);
	ret = wait_for_sr_state(i2c_regs, ST_BUS_BUSY);
	if (ret < 0)
		return ret;

	ret = tx_byte(i2c_regs, chip << 1);
	if (ret < 0)
		return ret;

	while (alen--) {
		ret = tx_byte(i2c_regs, (addr >> (alen * 8)) & 0xff);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int toggle_i2c(void *base);

static int i2c_init_transfer(struct mxc_i2c_regs *i2c_regs,
		uchar chip, uint addr, int alen)
{
	int retry;
	int ret;
	for (retry = 0; retry < 3; retry++) {
		ret = i2c_init_transfer_(i2c_regs, chip, addr, alen);
		if (ret >= 0)
			return 0;
		i2c_imx_stop(i2c_regs);
		if (ret == -ENODEV)
			return ret;

		printf("%s: failed for chip 0x%x retry=%d\n", __func__, chip,
				retry);
		if (ret != -ERESTART)
			i2c_reset(i2c_regs);
		udelay(100);
		if (toggle_i2c(i2c_regs) < 0)
			break;
	}
	printf("%s: give up i2c_regs=%p\n", __func__, i2c_regs);
	return ret;
}

/*
 * Read data from I2C device
 */
int bus_i2c_read(void *base, uchar chip, uint addr, int alen, uchar *buf,
		int len)
{
	int ret;
	struct mxc_i2c_regs *i2c_regs = (struct mxc_i2c_regs *)base;

	ret = i2c_init_transfer(i2c_regs, chip, addr, alen);
	if (ret < 0) {
		printf("i2c_init_transfer failed\n");
		return ret;
	}

	writeb(I2CR_IEN | I2CR_MSTA | I2CR_MTX | I2CR_RSTA, &i2c_regs->i2cr);

	ret = tx_byte(i2c_regs, (chip << 1) | 1);
	if (ret < 0) {
		printf("%s:Send 2nd chip address fail(%x)\n",
		       __func__, readb(&i2c_regs->i2sr));
		/* send stop */
		writeb(I2CR_IEN | I2CR_TX_NO_AK, &i2c_regs->i2cr);
		return ret;
	}
	writeb(I2CR_IEN | I2CR_MSTA | ((len <= 1) ? I2CR_TX_NO_AK : 0),
			&i2c_regs->i2cr);
	ret = readb(&i2c_regs->i2dr);		/* dummy read to clear ICF */

	for (;;) {
		ret = wait_for_sr_state(i2c_regs, ST_BYTE_COMPLETE);
		if (ret < 0) {
			printf("%s: fail sr=%x cr=%x, len=%i\n", __func__,
					readb(&i2c_regs->i2sr),
					readb(&i2c_regs->i2cr), len);
			/* send stop */
			writeb(I2CR_IEN | I2CR_TX_NO_AK, &i2c_regs->i2cr);
			return ret;
		}
		if (len == 2) {
			writeb(I2CR_IEN | I2CR_MSTA | I2CR_TX_NO_AK,
					&i2c_regs->i2cr);
		}
		if (len <= 1) {
			i2c_imx_stop(i2c_regs);
		}
		ret = readb(&i2c_regs->i2dr);
		if (len <= 0)
			break;
		*buf++ = ret;
		if (!(--len))
			break;
	}
	i2c_imx_stop(i2c_regs);
	return 0;
}

int bus_i2c_write(void *base, uchar chip, uint addr, int alen,
		const uchar *buf, int len)
{
	int ret;
	struct mxc_i2c_regs *i2c_regs = (struct mxc_i2c_regs *)base;

	ret = i2c_init_transfer(i2c_regs, chip, addr, alen);
	if (ret < 0)
		return ret;

	while (len--) {
		ret = tx_byte(i2c_regs, *buf++);
		if (ret < 0) {
			writeb(I2CR_IEN, &i2c_regs->i2cr);	/* send stop */
			return ret;
		}
	}
	i2c_imx_stop(i2c_regs);
	return 0;
}

struct i2c_parms {
	void *base;
	void *toggle_data;
	int (*toggle_fn)(void *p);
};

struct sram_data {
	unsigned curr_i2c_bus;
	struct i2c_parms i2c_data[3];
};

/*
 * For SPL boot some boards need i2c before SDRAM is initialized so force
 * variables to live in SRAM
 */
static struct sram_data __attribute__((section(".data"))) srdata;

void *get_base(void)
{
#ifdef CONFIG_SYS_I2C_BASE
#ifdef CONFIG_I2C_MULTI_BUS
	void *ret = srdata.i2c_data[srdata.curr_i2c_bus].base;
	if (ret)
		return ret;
#endif
	return (void *)CONFIG_SYS_I2C_BASE;
#elif defined(CONFIG_I2C_MULTI_BUS)
	return srdata.i2c_data[srdata.curr_i2c_bus].base;
#else
	return srdata.i2c_data[0].base;
#endif
}

static struct i2c_parms *i2c_get_parms(void *base)
{
	int i = 0;
	struct i2c_parms *p = srdata.i2c_data;
	while (i < ARRAY_SIZE(srdata.i2c_data)) {
		if (p->base == base)
			return p;
		p++;
		i++;
	}
	printf("Invalid I2C base: %p\n", base);
	return NULL;
}

static int toggle_i2c(void *base)
{
	struct i2c_parms *p = i2c_get_parms(base);
	if (p && p->toggle_fn)
		return p->toggle_fn(p->toggle_data);
	return 0;
}

#ifdef CONFIG_I2C_MULTI_BUS
unsigned int i2c_get_bus_num(void)
{
	return srdata.curr_i2c_bus;
}

int i2c_set_bus_num(unsigned bus_idx)
{
	if (bus_idx >= ARRAY_SIZE(srdata.i2c_data))
		return -1;
	if (!srdata.i2c_data[bus_idx].base)
		return -1;
	srdata.curr_i2c_bus = bus_idx;
	return 0;
}
#endif

int i2c_read(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	return bus_i2c_read(get_base(), chip, addr, alen, buf, len);
}

int i2c_write(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	return bus_i2c_write(get_base(), chip, addr, alen, buf, len);
}

/*
 * Try if a chip add given address responds (probe the chip)
 */
int i2c_probe(uchar chip)
{
	return bus_i2c_write(get_base(), chip, 0, 0, NULL, 0);
}

void bus_i2c_init(void *base, int speed, int unused,
		int (*toggle_fn)(void *p), void *toggle_data)
{
	int i = 0;
	struct i2c_parms *p = srdata.i2c_data;
	if (!base)
		return;
	for (;;) {
		if (!p->base || (p->base == base)) {
			p->base = base;
			if (toggle_fn) {
				p->toggle_fn = toggle_fn;
				p->toggle_data = toggle_data;
			}
			break;
		}
		p++;
		i++;
		if (i >= ARRAY_SIZE(srdata.i2c_data))
			return;
	}
	bus_i2c_set_bus_speed(base, speed);
}

/*
 * Init I2C Bus
 */
void i2c_init(int speed, int unused)
{
	bus_i2c_init(get_base(), speed, unused, NULL, NULL);
}

/*
 * Set I2C Speed
 */
int i2c_set_bus_speed(unsigned int speed)
{
	return bus_i2c_set_bus_speed(get_base(), speed);
}

/*
 * Get I2C Speed
 */
unsigned int i2c_get_bus_speed(void)
{
	return bus_i2c_get_bus_speed(get_base());
}
