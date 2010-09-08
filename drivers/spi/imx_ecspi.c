/*
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>
#include <spi.h>
#include <asm/errno.h>
#include <linux/types.h>
#include <asm/io.h>
#include <malloc.h>

#include <imx_spi.h>

extern s32 spi_get_cfg(struct imx_spi_dev_t *dev);

static inline struct imx_spi_dev_t *to_imx_spi_slave(struct spi_slave *slave)
{
	return container_of(slave, struct imx_spi_dev_t, slave);
}

static s32 spi_reset(struct spi_slave *slave)
{
	u32 clk_src = mxc_get_clock(MXC_CSPI_CLK);
	u32 pre_div = 0, post_div = 0, reg_ctrl, reg_config;
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
	struct spi_reg_t *reg = &(dev->reg);

	if (dev->freq == 0) {
		printf("Error: desired clock is 0\n");
		return 1;
	}

	reg_ctrl = readl(dev->base + SPI_CON_REG);
	reg_config = readl(dev->base + SPI_CFG_REG);
	/* Reset spi */
	writel(0, dev->base + SPI_CON_REG);
	writel((reg_ctrl | 0x1), dev->base + SPI_CON_REG);

	/* Control register setup */
	pre_div = (clk_src + dev->freq - 1)/ dev->freq;
	while (pre_div > 16) {
		pre_div = (pre_div + 1) >> 1;
		post_div++;
	}
	if (post_div > 0x0f) {
		printf("Error: no divider can meet the freq: %d\n", dev->freq);
		return -1;
	}
	if (pre_div)
		pre_div--;

	printf("pre_div = %d, post_div=%d, clk_src=%d, spi_freq=%d\n", pre_div, post_div, clk_src, (clk_src/(pre_div + 1)) >> post_div);
	reg_ctrl &= ~((3 << 18) | (0xF << 12) | (0xF << 8));
	reg_ctrl |= (dev->ss << 18);
	reg_ctrl |= (pre_div << 12);
	reg_ctrl |= (post_div << 8);
	reg_ctrl |= (1 << (dev->ss + 4));	/* always set to master mode !!!! */
	reg_ctrl |= 1;

	/* configuration register setup */
	reg_config &= ~(0x111111 << dev->ss);
	reg_config |= (dev->in_sctl << (dev->ss + 20));
	reg_config |= (dev->in_dctl << (dev->ss + 16));
	reg_config |= (dev->ss_pol << (dev->ss + 12));
	reg_config |= (dev->ssctl << (dev->ss + 8));
	reg_config |= (dev->sclkpol << (dev->ss + 4));
	reg_config |= (dev->sclkpha << (dev->ss));

	if (dev->ss)
		reg_config |= (1 << 12);	/* pmic is high active */
	debug("ss%x, reg_config = 0x%x\n", dev->ss, reg_config);
	writel(reg_config, dev->base + SPI_CFG_REG);
	debug("ss%x, reg_ctrl = 0x%x\n", dev->ss, reg_ctrl);
	writel(reg_ctrl & ~1, dev->base + SPI_CON_REG);

	/* save config register and control register */
	reg->cfg_reg  = reg_config;
	reg->ctrl_reg = reg_ctrl;

	/* clear interrupt reg */
	writel(0, dev->base + SPI_INT_REG);
	writel(3 << 6, dev->base + SPI_STAT_REG);
	return 0;
}

void spi_init(void)
{
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode)
{
	struct imx_spi_dev_t *imx_spi_slave = NULL;

	if (!spi_cs_is_valid(bus, cs))
		return NULL;

	imx_spi_slave = (struct imx_spi_dev_t *)calloc(sizeof(struct imx_spi_dev_t), 1);
	if (!imx_spi_slave)
		return NULL;

	imx_spi_slave->slave.bus = bus;
	imx_spi_slave->slave.cs = cs;

	spi_get_cfg(imx_spi_slave);

	if(max_hz < imx_spi_slave->freq) {
		imx_spi_slave->freq = max_hz ;
	}
	spi_io_init(imx_spi_slave, 0);

	spi_reset(&(imx_spi_slave->slave));

	return &(imx_spi_slave->slave);
}

void spi_free_slave(struct spi_slave *slave)
{
	struct imx_spi_dev_t *imx_spi_slave;

	if (slave) {
		imx_spi_slave = to_imx_spi_slave(slave);
		free(imx_spi_slave);
	}
}

int spi_claim_bus(struct spi_slave *slave)
{
	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{

}

/*
 * SPI transfer:
 *
 * See include/spi.h and http://www.altera.com/literature/ds/ds_nios_spi.pdf
 * for more informations.
 */
int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
	int ret_val = 0;
	u32 rem = 0;
	u32 rem_bits = 0;
	u32 rem_in_bits = 0;
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
	struct spi_reg_t *spi_reg = &(dev->reg);
	u32 temp_buf[MAX_SPI_BYTES >> 2];
	if (!slave)
		return -1;

#ifdef CONFIG_SPI_FLASH_IMX_ATMEL
	flags |= SPI_XFER_NOREORDER;
#endif
	if ((flags & SPI_XFER_NOREORDER) &&
			((((unsigned)dout) | ((unsigned)din)) & 3)) {
		printf("Error: dout/din must be word aligned\n");
		return -1;
	}

	if (spi_reg->ctrl_reg == 0) {
		printf("Error: spi(base=0x%x) has not been initialized yet\n",
				dev->base);
		return -1;
	}
	while (bitlen) {
		s32 loop_cnt = SPI_RETRY_TIMES;
		u32 *pin = (u32 *)din;
		u32 *pout = (u32 *)dout;
		s32 current_len = bitlen;
		if (current_len > 64) {
			current_len &= ~0x1f;
			if (current_len > (MAX_SPI_BYTES * 8))
				current_len = (MAX_SPI_BYTES * 8);
		}
		if (!(flags & SPI_XFER_NOREORDER)) {
			if (din) pin = temp_buf;
			if (dout) {
				u32 val = 0;
				u32 v;
				u32 *p = temp_buf;
				u32 len = current_len;
				u32 l = len & 0x1f;
				if (rem_bits | l) {
					if (!l) l = 32;
					do {
						len -= l;
						val = *((u8 *)dout++);
						val = (val << 8) | *((u8 *)dout++);
						val = (val << 8) | *((u8 *)dout++);
						val = (val << 8) | *((u8 *)dout++);
						if (rem_bits) {
							v = rem;
							if (rem_bits <= l) {
								l -= rem_bits;
								rem_bits = 0;
							} else {
								rem_bits -= l;
								l = 0;
								v >>= rem_bits; 
							}
						} else
							v = 0;
						if (l) {
							v = (v << l) | (val >> (32 - l));
							rem = val;
							rem_bits = 32 - l;
						}
						*p++ = v;
						if (!(rem_bits & 7)) {
							dout = ((u8 *)dout) - (rem_bits >> 3);
							rem_bits = 0;
							break;
						}
						l = 32;
					} while (1);
				}
				while (len) {
					len -= 32;
					val = *((u8 *)dout++);
					val = (val << 8) | *((u8 *)dout++);
					val = (val << 8) | *((u8 *)dout++);
					val = (val << 8) | *((u8 *)dout++);
					*p++ = val;
				}
				pout = temp_buf;
			}
		}
		spi_reg->ctrl_reg = (spi_reg->ctrl_reg & ~0xFFF00000) | \
				((current_len - 1) << 20);

		writel(spi_reg->ctrl_reg, dev->base + SPI_CON_REG);
		debug("ss%x: ctrl_reg=0x%x, cfg_reg=0x%x\n", dev->ss,
				spi_reg->ctrl_reg, spi_reg->cfg_reg);

		if (flags & SPI_XFER_BEGIN)
			writel(spi_reg->cfg_reg, dev->base + SPI_CFG_REG);

		/* move data to the tx fifo */
		debug("pout=0x%p, current_len=%x\n", pout, current_len);
		if (pout) {
			int len;
			for (len = current_len; len > 0; len -= 32) {
				debug("rem bits = 0x%x, data %x\n", len, *pout);
				writel(*pout++, dev->base + SPI_TX_DATA);
			}
		} else {
			int len;
			for (len = current_len; len > 0; len -= 32) {
				writel(0, dev->base + SPI_TX_DATA);
			}
		}
		if (flags & SPI_XFER_BEGIN) {
			spi_cs_activate(slave);
			flags &= ~SPI_XFER_BEGIN;
		}

		writel(spi_reg->ctrl_reg | (1 << 2), dev->base + SPI_CON_REG);

		/* poll on the TC bit (transfer complete) */
		while ((readl(dev->base + SPI_STAT_REG) & (1 << 7)) == 0) {
			loop_cnt--;
			if (loop_cnt <= 0) {
				printf("Error: re-tried %d times without response. Give up\n",
						SPI_RETRY_TIMES);
				spi_cs_deactivate(slave);
				return -1;
			}
			udelay(100);
		}


		/* move data in the rx buf */
		if (pin) {
			int len;
			for (len = current_len; len > 0; len -= 32) {
				*pin++ = readl(dev->base + SPI_RX_DATA);
				debug("len = 0x%x, read %x\n", len, pin[-1]);
			}
		} else {
			int len;
			for (len = current_len; len > 0; len -= 32)
				readl(dev->base + SPI_RX_DATA);
		}


		/* clear the TC bit */
		writel(3 << 6, dev->base + SPI_STAT_REG);
	
		if (din && (!(flags & SPI_XFER_NOREORDER))) {
			u32 *p = temp_buf;
			u32 v;
			u32 len = current_len;
			u32 l = len & 0x1f;
			u8 *q = (u8 *)din;
			if (rem_in_bits | l) {
				if (!l) l = 32;
				do {
					u32 r = (8 - rem_in_bits) & 7;
					v = *p++;
					v <<= (32 - l);
					len -= l;
					if (r && (l >= r)) {
						*q++ |= (u8)(v >> (32 - r));
						l -= r;
						v <<= r;
					}
					while (l >= 8) {
						*q++ = (u8)(v >> 24);
						l -= 8;
						v <<= 8;
					}
					if (l) {
						*q = (u8)(v >> 24);
					}
					rem_in_bits = l;
					l = 32;
				} while (len);
			} else {
				do {
					v = *p++;
					len -= 32;
					*q++ = (u8)(v >> 24);
					*q++ = (u8)(v >> 16);
					*q++ = (u8)(v >> 8);
					*q++ = (u8)(v >> 0);
				} while (len);
			}
			din = q;
		}
		bitlen -= current_len;
	}
	if (flags & SPI_XFER_END)
		spi_cs_deactivate(slave);
	return ret_val;
}

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return 1;
}

void spi_cs_activate(struct spi_slave *slave)
{
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);

	spi_io_init(dev, 1);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	struct imx_spi_dev_t *dev = to_imx_spi_slave(slave);
	spi_io_init(dev, 0);
	writel(0, dev->base + SPI_CON_REG);
}

