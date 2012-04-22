#include "mx5x_common.h"
#include "mx5x_i2c.h"
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

#if 0
#define DPRINTF(args...)  my_printf(args)
#else
#define DPRINTF(args...)
#endif

static unsigned short div[] = { 30, 32, 36, 42, 48, 52, 60, 72, 80, 88, 104, 128, 144,
	160, 192, 240, 288, 320, 384, 480, 576, 640, 768, 960,
	1152, 1280, 1536, 1920, 2304, 2560, 3072, 3840
};

static inline void i2c_reset(unsigned i2c_base)
{
	IO_WRITE16(i2c_base, I2CR, 0);	/* Reset module */
	IO_WRITE16(i2c_base, I2SR, 0);
	IO_WRITE16(i2c_base, I2CR, I2CR_IEN);
}

void i2c_init(unsigned i2c_base, unsigned speed)
{
	unsigned freq;
	int i;

	iomuxc_setup_i2c1();
	freq = 54000000;
	for (i = 0; i < 0x1f; i++)
		if (freq / div[i] <= speed)
			break;

	DPRINTF("%s: root clock: %d, speed: %d div: %x\n",
		__func__, freq, speed, i);

	IO_WRITE16(i2c_base, IFDR, i);
	i2c_reset(i2c_base);
}

#define ST_BUS_IDLE (0 | (I2SR_IBB << 16))
#define ST_BUS_BUSY (I2SR_IBB | (I2SR_IBB << 16))
#define ST_BYTE_COMPLETE (I2SR_ICF | (I2SR_ICF << 16))
#define ST_BYTE_PENDING (0 | (I2SR_ICF << 16))
#define ST_IIF (I2SR_IIF | (I2SR_IIF << 16))

static unsigned wait_for_sr_state(unsigned i2c_base, unsigned state)
{
	int timeout = 10000;
	unsigned sr;
	for (;;) {
		sr = IO_READ16(i2c_base, I2SR);
		if ((sr & (state >> 16)) == (unsigned short)state)
			break;
		if (!(--timeout)) {
			my_printf("%s: failed sr=%x cr=%x state=%x\n", __func__,
					sr, IO_READ16(i2c_base, I2CR), state);
			return 0;
		}
		delayMicro(1);
	}
	DPRINTF("%s:%x\n", __func__, sr);
	return sr | 0x10000;
}

static int tx_byte(unsigned i2c_base, uint8_t byte)
{
	unsigned sr;
	IO_WRITE16(i2c_base, I2SR, 0);
	IO_WRITE16(i2c_base, I2DR, byte);
	sr = wait_for_sr_state(i2c_base, ST_IIF);
	if ((!sr) || (sr & I2SR_RX_NO_AK)) {
		DPRINTF("%s:%x <= %x\n", __func__, sr, byte);
		return -1;
	}
	DPRINTF("%s:%x\n", __func__, byte);
	return 0;
}

int i2c_probe(unsigned i2c_base, uint8_t chip)
{
	int ret;

	IO_WRITE16(i2c_base, I2CR, 0);	/* Reset module */
	IO_WRITE16(i2c_base, I2CR, I2CR_IEN);
	for (ret = 0; ret < 1000; ret++)
		delayMicro(1);
	IO_WRITE16(i2c_base, I2CR, I2CR_IEN | I2CR_MSTA | I2CR_MTX);
	ret = tx_byte(i2c_base, chip << 1);
	IO_WRITE16(i2c_base, I2CR, I2CR_IEN);

	return ret;
}

static int i2c_addr(unsigned i2c_base, uint8_t chip, uint addr, int alen)
{
	int retry = 0;
	for (retry = 0; retry < 3; retry++) {
		if (wait_for_sr_state(i2c_base, ST_BUS_IDLE))
			break;
		i2c_reset(i2c_base);
		delayMicro(100000);
	}
	if (retry >= 3) {
		my_printf("%s:bus is busy(%x)\n",
		       __func__, IO_READ16(i2c_base, I2SR));
		return -1;
	}
	IO_WRITE16(i2c_base, I2CR, I2CR_IEN | I2CR_MSTA | I2CR_MTX);
	if (!wait_for_sr_state(i2c_base, ST_BUS_BUSY)) {
		my_printf("%s:trigger start fail(%x)\n",
		       __func__, IO_READ16(i2c_base, I2SR));
		return -1;
	}
	if (tx_byte(i2c_base, chip << 1)) {
		my_printf("%s:chip address cycle fail(%x)\n",
		       __func__, IO_READ16(i2c_base, I2SR));
		return -1;
	}
	while (alen--)
		if (tx_byte(i2c_base, (addr >> (alen * 8)) & 0xff)) {
			my_printf("%s:device address cycle fail(%x)\n",
			       __func__, IO_READ16(i2c_base, I2SR));
			return -1;
		}
	return 0;
}

static int i2c_addr_retry(unsigned i2c_base, uint8_t chip, uint addr, int alen)
{
	int retry;
	int ret;
	for (retry = 0; retry < 5; retry++) {
		ret = i2c_addr(i2c_base, chip, addr, alen);
		if (!ret) {
			if (retry)
				my_printf("%s:recovered\n", __func__);
			break;
		}
		IO_WRITE16(i2c_base, I2CR, I2CR_IEN);
		delayMicro(1000);
		i2c_reset(i2c_base);
		delayMicro(1000);
	}
	return ret;
}

int i2c_read(unsigned i2c_base, uint8_t chip, uint addr, int alen, uint8_t *buf, int len)
{
	uint ret;

	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",
		__func__, chip, addr, alen, len);

	if (i2c_addr_retry(i2c_base, chip, addr, alen)) {
		return -1;
	}

	IO_WRITE16(i2c_base, I2CR, I2CR_IEN | I2CR_MSTA | I2CR_MTX | I2CR_RSTA);

	if (tx_byte(i2c_base, chip << 1 | 1)) {
		my_printf("%s:Send 2th chip address fail(%x)\n",
		       __func__, IO_READ16(i2c_base, I2SR));
		return -1;
	}
	IO_WRITE16(i2c_base, I2CR, I2CR_IEN | I2CR_MSTA |
		((len <= 1) ? I2CR_TX_NO_AK : 0));
	ret = IO_READ16(i2c_base, I2DR);	/* dummy read to clear ICF */
	DPRINTF("CR=%x\n", IO_READ16(i2c_base, I2CR));

	for (;;) {
		if (!wait_for_sr_state(i2c_base, ST_BYTE_COMPLETE)) {
			my_printf("%s: fail sr=%x cr=%x, len=%i\n", __func__, IO_READ16(i2c_base, I2SR), IO_READ16(i2c_base, I2CR), len);
			return -1;
		}
		if (len == 2) {
			IO_WRITE16(i2c_base, I2CR, I2CR_IEN | I2CR_MSTA | I2CR_TX_NO_AK);
		}
		if (len <= 1) {
			IO_WRITE16(i2c_base, I2CR, I2CR_IEN | I2CR_TX_NO_AK);	/* send stop */
		}
		ret = IO_READ16(i2c_base, I2DR);
		DPRINTF("%s:%x\n", __func__, ret);
		if (len <= 0)
			break;
		*buf++ = ret;
		if (!(--len))
			break;

	}
	if (!wait_for_sr_state(i2c_base, ST_BUS_IDLE))
		my_printf("%s:trigger stop fail\n", __func__);
	return 0;
}

int i2c_write(unsigned i2c_base, uint8_t chip, uint addr, int alen, uint8_t *buf, int len)
{
	DPRINTF("%s chip: 0x%02x addr: 0x%04x alen: %d len: %d\n",
		__func__, chip, addr, alen, len);

	if (i2c_addr_retry(i2c_base, chip, addr, alen))
		return -1;

	while (len--)
		if (tx_byte(i2c_base, *buf++)) {
			my_printf("%s: failed\n", __func__);
			return -1;
		}

	IO_WRITE16(i2c_base, I2CR, I2CR_IEN);

	if (!wait_for_sr_state(i2c_base, ST_BUS_IDLE))
		my_printf("%s:trigger stop fail\n", __func__);
	return 0;
}
