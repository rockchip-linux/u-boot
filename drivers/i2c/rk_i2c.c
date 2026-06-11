// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2015 Google, Inc
 *
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 */

#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <log.h>
#include <asm/io.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/i2c.h>
#include <asm/arch-rockchip/periph.h>
#include <dm/pinctrl.h>
#include <linux/delay.h>
#include <linux/sizes.h>
#include <dm/pinctrl.h>
#include <asm/gpio.h>

/* i2c timerout */
#define I2C_TIMEOUT_MS		100
#define I2C_RETRY_COUNT		3

/* rk i2c fifo max transfer bytes */
#define RK_I2C_FIFO_SIZE	32

struct rk_i2c;

#if CONFIG_IS_ENABLED(DM_GPIO)
struct recovery_gpio_data {
	struct gpio_desc desc;
	unsigned int bank;
	unsigned int pin;
};

struct i2c_bus_recovery_info {
	int (*get_scl)(struct i2c_bus_recovery_info *ri);
	void (*set_scl)(struct i2c_bus_recovery_info *ri, int val);
	int (*get_sda)(struct i2c_bus_recovery_info *ri);
	void (*set_sda)(struct i2c_bus_recovery_info *ri, int val);

	void (*prepare_recovery)(struct rk_i2c *i2c);
	void (*unprepare_recovery)(struct rk_i2c *i2c);

	/* gpio recovery */
	struct recovery_gpio_data scl;
	struct recovery_gpio_data sda;
	struct udevice *pinctrl_dev;
};
#endif

struct rk_i2c {
	struct udevice *udev;
	struct clk clk;
	struct i2c_regs *regs;
	unsigned int version;
	unsigned int speed;
	unsigned int cfg;

#if CONFIG_IS_ENABLED(DM_GPIO)
	bool bus_recovery;
	struct i2c_bus_recovery_info recovery_info;
#endif
};

enum {
	RK_I2C_LEGACY,
	RK_I2C_NEW,
};

/**
 * @controller_type: i2c controller type
 */
struct rk_i2c_soc_data {
	int controller_type;
};

struct i2c_spec_values {
	unsigned int min_low_ns;
	unsigned int min_high_ns;
	unsigned int max_rise_ns;
	unsigned int max_fall_ns;
};

enum {
	RK_I2C_VERSION0 = 0,
	RK_I2C_VERSION1,
	RK_I2C_VERSION5 = 5,
};

/********************* Private Variable Definition ***************************/

static const struct i2c_spec_values standard_mode_spec = {
	.min_low_ns = 4700,
	.min_high_ns = 4000,
	.max_rise_ns = 1000,
	.max_fall_ns = 300,
};

static const struct i2c_spec_values fast_mode_spec = {
	.min_low_ns = 1300,
	.min_high_ns = 600,
	.max_rise_ns = 300,
	.max_fall_ns = 300,
};

static const struct i2c_spec_values fast_modeplus_spec = {
	.min_low_ns = 500,
	.min_high_ns = 260,
	.max_rise_ns = 120,
	.max_fall_ns = 120,
};

static const struct i2c_spec_values *rk_i2c_get_spec(unsigned int speed)
{
	if (speed == 1000)
		return &fast_modeplus_spec;
	else if (speed == 400)
		return &fast_mode_spec;
	else
		return &standard_mode_spec;
}

static void rk_i2c_show_regs(struct i2c_regs *regs)
{
#ifdef DEBUG
	uint i;

	debug("i2c_con: 0x%08x\n", readl(&regs->con));
	debug("i2c_clkdiv: 0x%08x\n", readl(&regs->clkdiv));
	debug("i2c_mrxaddr: 0x%08x\n", readl(&regs->mrxaddr));
	debug("i2c_mrxraddR: 0x%08x\n", readl(&regs->mrxraddr));
	debug("i2c_mtxcnt: 0x%08x\n", readl(&regs->mtxcnt));
	debug("i2c_mrxcnt: 0x%08x\n", readl(&regs->mrxcnt));
	debug("i2c_ien: 0x%08x\n", readl(&regs->ien));
	debug("i2c_ipd: 0x%08x\n", readl(&regs->ipd));
	debug("i2c_fcnt: 0x%08x\n", readl(&regs->fcnt));
	for (i = 0; i < 8; i++)
		debug("i2c_txdata%d: 0x%08x\n", i, readl(&regs->txdata[i]));
	for (i = 0; i < 8; i++)
		debug("i2c_rxdata%d: 0x%08x\n", i, readl(&regs->rxdata[i]));
#endif
}

static inline void rk_i2c_get_div(int div, int *divh, int *divl)
{
	*divl = div / 2;
	if (div % 2 == 0)
		*divh = div / 2;
	else
		*divh = DIV_ROUND_UP(div, 2);
}

/*
 * SCL Divisor = 8 * (CLKDIVL+1 + CLKDIVH+1)
 * SCL = PCLK / SCLK Divisor
 * i2c_rate = PCLK
 */
static void rk_i2c_set_clk(struct rk_i2c *i2c, unsigned int scl_rate)
{
	unsigned int i2c_rate;
	int div, divl, divh;

	/* First get i2c rate from pclk */
	i2c_rate = clk_get_rate(&i2c->clk);

	div = DIV_ROUND_UP(i2c_rate, scl_rate * 8) - 2;
	divh = 0;
	divl = 0;
	if (div >= 0)
		rk_i2c_get_div(div, &divh, &divl);
	writel(I2C_CLKDIV_VAL(divl, divh), &i2c->regs->clkdiv);

	debug("rk_i2c_set_clk: i2c rate = %d, scl rate = %d\n", i2c_rate,
	      scl_rate);
	debug("set i2c clk div = %d, divh = %d, divl = %d\n", div, divh, divl);
	debug("set clk(I2C_CLKDIV: 0x%08x)\n", readl(&i2c->regs->clkdiv));
}

static int rk_i2c_adapter_clk(struct rk_i2c *i2c, unsigned int scl_rate)
{
	const struct i2c_spec_values *spec;
	unsigned int min_total_div, min_low_div, min_high_div, min_hold_div;
	unsigned int low_div, high_div, extra_div, extra_low_div;
	unsigned int min_low_ns, min_high_ns;
	unsigned int start_setup = 0;
	unsigned int i2c_rate = clk_get_rate(&i2c->clk);
	unsigned int speed;

	debug("rk_i2c_set_clk: i2c rate = %d, scl rate = %d\n", i2c_rate,
	      scl_rate);

	if (scl_rate <= 100000 && scl_rate >= 1000) {
		start_setup = 1;
		speed = 100;
	} else if (scl_rate <= 400000 && scl_rate >= 100000) {
		speed = 400;
	} else if (scl_rate <= 1000000 && scl_rate > 400000) {
		speed = 1000;
	} else {
		debug("invalid i2c speed : %d\n", scl_rate);
		return -EINVAL;
	}

	spec = rk_i2c_get_spec(speed);
	i2c_rate = DIV_ROUND_UP(i2c_rate, 1000);
	speed = DIV_ROUND_UP(scl_rate, 1000);

	min_total_div = DIV_ROUND_UP(i2c_rate, speed * 8);

	min_high_ns = spec->max_rise_ns + spec->min_high_ns;
	min_high_div = DIV_ROUND_UP(i2c_rate * min_high_ns, 8 * 1000000);

	min_low_ns = spec->max_fall_ns + spec->min_low_ns;
	min_low_div = DIV_ROUND_UP(i2c_rate * min_low_ns, 8 * 1000000);

	min_high_div = (min_high_div < 1) ? 2 : min_high_div;
	min_low_div = (min_low_div < 1) ? 2 : min_low_div;

	min_hold_div = min_high_div + min_low_div;

	if (min_hold_div >= min_total_div) {
		high_div = min_high_div;
		low_div = min_low_div;
	} else {
		extra_div = min_total_div - min_hold_div;
		extra_low_div = DIV_ROUND_UP(min_low_div * extra_div,
					     min_hold_div);

		low_div = min_low_div + extra_low_div;
		high_div = min_high_div + (extra_div - extra_low_div);
	}

	high_div--;
	low_div--;

	if (high_div > 0xffff || low_div > 0xffff)
		return -EINVAL;

	/* 1 for data hold/setup time is enough */
	i2c->cfg = I2C_CON_SDA_CFG(1) | I2C_CON_STA_CFG(start_setup);
	writel((high_div << I2C_CLK_DIV_HIGH_SHIFT) | low_div,
	       &i2c->regs->clkdiv);

	debug("set clk(I2C_TIMING: 0x%08x)\n", i2c->cfg);
	debug("set clk(I2C_CLKDIV: 0x%08x)\n", readl(&i2c->regs->clkdiv));

	return 0;
}

static int rk_i2c_send_start_bit(struct rk_i2c *i2c, u32 con)
{
	struct i2c_regs *regs = i2c->regs;
	ulong start;

	debug("I2c Send Start bit.\n");
	writel(I2C_IPD_ALL_CLEAN, &regs->ipd);

	writel(I2C_STARTIEN, &regs->ien);
	writel(I2C_CON_EN | I2C_CON_START | i2c->cfg | con, &regs->con);

	start = get_timer(0);
	while (1) {
		if (readl(&regs->ipd) & I2C_STARTIPD) {
			writel(I2C_STARTIPD, &regs->ipd);
			break;
		}
		if (get_timer(start) > I2C_TIMEOUT_MS) {
			debug("I2C Send Start Bit Timeout\n");
			rk_i2c_show_regs(regs);
			return -ETIMEDOUT;
		}
		udelay(1);
	}

	/* clean start bit */
	writel(I2C_CON_EN | i2c->cfg | con, &regs->con);

	return 0;
}

static int rk_i2c_send_stop_bit(struct rk_i2c *i2c)
{
	struct i2c_regs *regs = i2c->regs;
	ulong start;

	debug("I2c Send Stop bit.\n");
	writel(I2C_IPD_ALL_CLEAN, &regs->ipd);

	writel(I2C_CON_EN | i2c->cfg | I2C_CON_STOP, &regs->con);
	writel(I2C_STOPIEN, &regs->ien);

	start = get_timer(0);
	while (1) {
		if (readl(&regs->ipd) & I2C_STOPIPD) {
			writel(I2C_STOPIPD, &regs->ipd);
			break;
		}
		if (get_timer(start) > I2C_TIMEOUT_MS) {
			debug("I2C Send Stop Bit Timeout\n");
			rk_i2c_show_regs(regs);
			return -ETIMEDOUT;
		}
		udelay(1);
	}

	udelay(1);
	return 0;
}

static inline void rk_i2c_disable(struct rk_i2c *i2c)
{
	writel(0, &i2c->regs->ien);
	writel(I2C_IPD_ALL_CLEAN, &i2c->regs->ipd);
	writel(0, &i2c->regs->con);
}

static int rk_i2c_read(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len,
		       uchar *buf, uint b_len, bool snd)
{
	struct i2c_regs *regs = i2c->regs;
	uchar *pbuf = buf;
	uint bytes_remain_len = b_len;
	uint bytes_xferred = 0;
	uint words_xferred = 0;
	ulong start;
	uint con = 0;
	uint rxdata;
	uint i, j;
	int err = 0;
	bool snd_chunk = false;

	debug("rk_i2c_read: chip = %d, reg = %d, r_len = %d, b_len = %d\n",
	      chip, reg, r_len, b_len);

	/* If the second message for TRX read, resetting internal state. */
	if (snd)
		writel(0, &regs->con);

	writel(I2C_MRXADDR_SET(1, chip << 1 | 1), &regs->mrxaddr);
	if (r_len == 0) {
		writel(0, &regs->mrxraddr);
	} else if (r_len < 4) {
		writel(I2C_MRXRADDR_SET(r_len, reg), &regs->mrxraddr);
	} else {
		debug("I2C Read: addr len %d not supported\n", r_len);
		return -EIO;
	}

	while (bytes_remain_len) {
		if (bytes_remain_len > RK_I2C_FIFO_SIZE) {
			con = I2C_CON_EN;
			bytes_xferred = 32;
		} else {
			/*
			 * The hw can read up to 32 bytes at a time. If we need
			 * more than one chunk, send an ACK after the last byte.
			 */
			con = I2C_CON_EN | I2C_CON_LASTACK;
			bytes_xferred = bytes_remain_len;
		}
		words_xferred = DIV_ROUND_UP(bytes_xferred, 4);

		/*
		 * make sure we are in plain RX mode if we read a second chunk;
		 * and first rx read need to send start bit.
		 */
		if (snd_chunk) {
			con |= I2C_CON_MOD(I2C_MODE_RX);
			writel(con | i2c->cfg, &regs->con);
		} else {
			con |= I2C_CON_MOD(I2C_MODE_TRX);
			err = rk_i2c_send_start_bit(i2c, con);
			if (err)
				return err;
		}

		writel(I2C_MBRFIEN | I2C_NAKRCVIEN, &regs->ien);
		writel(bytes_xferred, &regs->mrxcnt);

		start = get_timer(0);
		while (1) {
			if (readl(&regs->ipd) & I2C_NAKRCVIPD) {
				writel(I2C_NAKRCVIPD, &regs->ipd);
				err = -EREMOTEIO;
				goto i2c_exit;
			}
			if (readl(&regs->ipd) & I2C_MBRFIPD) {
				writel(I2C_MBRFIPD, &regs->ipd);
				break;
			}
			if (get_timer(start) > I2C_TIMEOUT_MS) {
				debug("I2C Read Data Timeout\n");
				err =  -ETIMEDOUT;
				rk_i2c_show_regs(regs);
				goto i2c_exit;
			}
			udelay(1);
		}

		for (i = 0; i < words_xferred; i++) {
			rxdata = readl(&regs->rxdata[i]);
			debug("I2c Read RXDATA[%d] = 0x%x\n", i, rxdata);
			for (j = 0; j < 4; j++) {
				if ((i * 4 + j) == bytes_xferred)
					break;
				*pbuf++ = (rxdata >> (j * 8)) & 0xff;
			}
		}

		bytes_remain_len -= bytes_xferred;
		snd_chunk = true;
		debug("I2C Read bytes_remain_len %d\n", bytes_remain_len);
	}

i2c_exit:
	return err;
}

static int rk_i2c_write(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len,
			uchar *buf, uint b_len)
{
	struct i2c_regs *regs = i2c->regs;
	int err = 0;
	uchar *pbuf = buf;
	uint bytes_remain_len = b_len + r_len + 1;
	uint bytes_xferred = 0;
	uint words_xferred = 0;
	bool next = false;
	ulong start;
	uint txdata;
	uint i, j;

	debug("rk_i2c_write: chip = %d, reg = %d, r_len = %d, b_len = %d\n",
	      chip, reg, r_len, b_len);

	while (bytes_remain_len) {
		if (bytes_remain_len > RK_I2C_FIFO_SIZE)
			bytes_xferred = RK_I2C_FIFO_SIZE;
		else
			bytes_xferred = bytes_remain_len;
		words_xferred = DIV_ROUND_UP(bytes_xferred, 4);

		for (i = 0; i < words_xferred; i++) {
			txdata = 0;
			for (j = 0; j < 4; j++) {
				if ((i * 4 + j) == bytes_xferred)
					break;

				if (i == 0 && j == 0 && pbuf == buf) {
					txdata |= (chip << 1);
				} else if (i == 0 && j <= r_len && pbuf == buf) {
					txdata |= (reg &
						(0xff << ((j - 1) * 8))) << 8;
				} else {
					txdata |= (*pbuf++)<<(j * 8);
				}
			}
			writel(txdata, &regs->txdata[i]);
			debug("I2c Write TXDATA[%d] = 0x%08x\n", i, txdata);
		}

		/* If the write is the first, need to send start bit */
		if (!next) {
			err = rk_i2c_send_start_bit(i2c, I2C_CON_EN |
					   I2C_CON_MOD(I2C_MODE_TX));
			if (err)
				return err;
			next = true;
		} else {
			writel(I2C_CON_EN | I2C_CON_MOD(I2C_MODE_TX) | i2c->cfg,
			       &regs->con);
		}
		writel(I2C_MBTFIEN | I2C_NAKRCVIEN, &regs->ien);
		writel(bytes_xferred, &regs->mtxcnt);

		start = get_timer(0);
		while (1) {
			if (readl(&regs->ipd) & I2C_NAKRCVIPD) {
				writel(I2C_NAKRCVIPD, &regs->ipd);
				err = -EREMOTEIO;
				goto i2c_exit;
			}
			if (readl(&regs->ipd) & I2C_MBTFIPD) {
				writel(I2C_MBTFIPD, &regs->ipd);
				break;
			}
			if (get_timer(start) > I2C_TIMEOUT_MS) {
				debug("I2C Write Data Timeout\n");
				err =  -ETIMEDOUT;
				rk_i2c_show_regs(regs);
				goto i2c_exit;
			}
			udelay(1);
		}

		bytes_remain_len -= bytes_xferred;
		debug("I2C Write bytes_remain_len %d\n", bytes_remain_len);
	}

i2c_exit:
	return err;
}

#if CONFIG_IS_ENABLED(DM_GPIO)
static int rockchip_i2c_get_scl_gpio_value(struct i2c_bus_recovery_info *ri)
{
	return dm_gpio_get_value(&ri->scl.desc);
}

static void rockchip_i2c_set_scl_gpio_value(struct i2c_bus_recovery_info *ri, int val)
{
	dm_gpio_set_value(&ri->scl.desc, val);
}

static int rockchip_i2c_get_sda_gpio_value(struct i2c_bus_recovery_info *ri)
{
	return dm_gpio_get_value(&ri->sda.desc);
}

static void rockchip_i2c_set_sda_gpio_value(struct i2c_bus_recovery_info *ri, int val)
{
	dm_gpio_set_value(&ri->sda.desc, val);
}

void rockchip_i2c_prepare_recovery(struct rk_i2c *i2c)
{
	struct i2c_bus_recovery_info *ri = &i2c->recovery_info;

	dm_gpio_set_dir_flags(&ri->scl.desc, GPIOD_IS_OUT);
	dm_gpio_set_dir_flags(&ri->sda.desc, GPIOD_IS_OUT);
	rockchip_i2c_set_scl_gpio_value(ri, 1);
	rockchip_i2c_set_sda_gpio_value(ri, 1);
	/* set scl & sda to gpio iomux */
	pinctrl_set_gpio_mux(ri->pinctrl_dev, ri->scl.bank,
			     ri->scl.pin, 0);
	pinctrl_set_gpio_mux(ri->pinctrl_dev, ri->sda.bank,
			     ri->sda.pin, 0);
}

void rockchip_i2c_unprepare_recovery(struct rk_i2c *i2c)
{
	struct i2c_bus_recovery_info *ri = &i2c->recovery_info;

	dm_gpio_set_dir_flags(&ri->scl.desc, GPIOD_IS_IN);
	dm_gpio_set_dir_flags(&ri->sda.desc, GPIOD_IS_IN);
	pinctrl_select_state(i2c->udev, "default");
}

static int rockchip_i2c_bus_free(struct rk_i2c *i2c)
{
	struct i2c_bus_recovery_info *bri = &i2c->recovery_info;
	int ret = -EOPNOTSUPP;

	if (bri->get_sda)
		ret = bri->get_sda(bri);

	if (ret < 0)
		return ret;

	return ret ? 0 : -EBUSY;
}

/*
 * We are generating clock pulses. ndelay() determines durating of clk pulses.
 * We will generate clock with rate 100 KHz and so duration of both clock levels
 * is: delay in ns = (10^6 / 100) / 2
 */
#define RECOVERY_NDELAY		5000
#define RECOVERY_CLK_CNT	9

int rockchip_i2c_scl_recovery(struct rk_i2c *i2c)
{
	struct i2c_bus_recovery_info *bri = &i2c->recovery_info;
	int i = 0, scl = 1, ret = 0;

	if (bri->prepare_recovery)
		bri->prepare_recovery(i2c);

	/*
	 * If we can set SDA, we will always create a STOP to ensure additional
	 * pulses will do no harm. This is achieved by letting SDA follow SCL
	 * half a cycle later. Check the 'incomplete_write_byte' fault injector
	 * for details. Note that we must honour tsu:sto, 4us, but lets use 5us
	 * here for simplicity.
	 */
	bri->set_scl(bri, scl);
	ndelay(RECOVERY_NDELAY);
	if (bri->set_sda)
		bri->set_sda(bri, scl);
	ndelay(RECOVERY_NDELAY / 2);

	/*
	 * By this time SCL is high, as we need to give 9 falling-rising edges
	 */
	while (i++ < RECOVERY_CLK_CNT * 2) {
		if (scl) {
			/* SCL shouldn't be low here */
			if (!bri->get_scl(bri)) {
				printf("SCL is stuck low, exit recovery\n");
				ret = -EBUSY;
				break;
			}
		}

		scl = !scl;
		bri->set_scl(bri, scl);
		/* Creating STOP again, see above */
		if (scl)  {
			/* Honour minimum tsu:sto */
			ndelay(RECOVERY_NDELAY);
		} else {
			/* Honour minimum tf and thd:dat */
			ndelay(RECOVERY_NDELAY / 2);
		}
		if (bri->set_sda)
			bri->set_sda(bri, scl);
		ndelay(RECOVERY_NDELAY / 2);

		if (scl) {
			ret = rockchip_i2c_bus_free(i2c);
			if (ret == 0)
				break;
		}
	}

	/* If we can't check bus status, assume recovery worked */
	if (ret == -EOPNOTSUPP)
		ret = 0;

	if (bri->unprepare_recovery)
		bri->unprepare_recovery(i2c);

	/* give a tbuf time */
	ndelay(RECOVERY_NDELAY);

	return ret;
}
#endif

static int rockchip_i2c_xfer(struct udevice *bus, struct i2c_msg *msg,
			     int nmsgs)
{
	struct rk_i2c *i2c = dev_get_priv(bus);
	bool snd = false; /* second message for TRX read */
	int ret = 0;
#ifdef CONFIG_IRQ
	ulong flags;
#endif

#if CONFIG_IS_ENABLED(DM_GPIO)
	if (i2c->bus_recovery) {
		struct i2c_regs *regs = i2c->regs;
		unsigned int line_status;

		/* check sda line state */
		line_status = readl(&regs->st) & 0x3;
		if (line_status == 0x2) {
			printf("rockchip i2c line status(SCL=HIGH, SDA=LOW), recovery it!\n");
			ret = rockchip_i2c_scl_recovery(i2c);
			if (ret)
				printf("rockchip_i2c_scl_recovery failed ret: %d\n", ret);
		}
		i2c->bus_recovery = false;
	}
#endif

	debug("i2c_xfer: %d messages\n", nmsgs);
	if (nmsgs > 2 || ((nmsgs == 2) && (msg->flags & I2C_M_RD))) {
		debug("Not support more messages now, split them\n");
		return -EINVAL;
	}

#ifdef CONFIG_IRQ
	local_irq_save(flags);
#endif
	/* Nack enabled */
	i2c->cfg |= I2C_CON_ACTACK;
	for (; nmsgs > 0; nmsgs--, msg++) {
		debug("i2c_xfer: chip=0x%x, len=0x%x\n", msg->addr, msg->len);

		if (msg->flags & I2C_M_RD) {
			/* If snd is true, it is TRX mode. */
			ret = rk_i2c_read(i2c, msg->addr, 0, 0, msg->buf,
					  msg->len, snd);
		} else {
			snd = true;
			ret = rk_i2c_write(i2c, msg->addr, 0, 0, msg->buf,
					   msg->len);
		}

		if (ret) {
			debug("i2c_write: error sending\n");
			break;
		}
	}

	rk_i2c_send_stop_bit(i2c);
	rk_i2c_disable(i2c);
#ifdef CONFIG_IRQ
	local_irq_restore(flags);
#endif
	return ret;
}

static unsigned int rk3x_i2c_get_version(struct rk_i2c *i2c)
{
	struct i2c_regs *regs = i2c->regs;
	uint version;

	version = readl(&regs->con) & I2C_CON_VERSION;

	return version >>= I2C_CON_VERSION_SHIFT;
}

int rockchip_i2c_set_bus_speed(struct udevice *bus, unsigned int speed)
{
	struct rk_i2c *i2c = dev_get_priv(bus);

	if (i2c->version >= RK_I2C_VERSION1)
		rk_i2c_adapter_clk(i2c, speed);
	else
		rk_i2c_set_clk(i2c, speed);

	return 0;
}

static int rockchip_i2c_of_to_plat(struct udevice *bus)
{
	struct rk_i2c *priv = dev_get_priv(bus);
	int ret;

	ret = clk_get_by_index(bus, 0, &priv->clk);
	if (ret < 0) {
		debug("%s: Could not get clock for %s: %d\n", __func__,
		      bus->name, ret);
		return ret;
	}

	return 0;
}

#if defined(CONFIG_MOS_SUPPORT) && !defined(CONFIG_SPL_BUILD)
static int rockchip_i2c_clk_init(struct udevice *dev)
{
	struct clk_bulk clks = { 0 };
	int ret = 0;

	ret = clk_get_bulk(dev, &clks);
	if (ret == -ENOSYS || ret == -ENOENT)
		return 0;
	if (ret) {
		dev_err(dev, "failed to get clk: %d\n", ret);
		return ret;
	}

	ret = clk_enable_bulk(&clks);
	if (ret) {
		dev_err(dev, "failed to enable clk: %d\n", ret);
		clk_release_bulk(&clks);
		return ret;
	}

	return 0;
}
#endif

#if CONFIG_IS_ENABLED(DM_GPIO)
static int rk_i2c_parse_pinctrl_and_request_gpio(struct udevice *dev,
						 struct rk_i2c *i2c)
{
	struct recovery_gpio_data *data;
	struct ofnode_phandle_args args;
	struct gpio_desc *desc;
	int ret, i, bus_num;
	char gpio_name[16];
	u32 pins[8];

	ret = uclass_get_device_by_seq(UCLASS_PINCTRL, 0, &i2c->recovery_info.pinctrl_dev);
	if (ret) {
		ret = uclass_first_device_err(UCLASS_PINCTRL, &i2c->recovery_info.pinctrl_dev);
		if (ret) {
			printf("failed to get pinctrl device %d\n", ret);
			return ret;
		}
	}

	ret = dev_read_phandle_with_args(dev, "pinctrl-0", NULL, 0, 0, &args);
	if (ret) {
		printf("No pinctrl-0: %d\n", ret);
		return ret;
	}
	if (ofnode_read_u32_array(args.node, "rockchip,pins", pins, 8))
		return -EINVAL;

	bus_num = dev->seq_;
	for (i = 0; i < 2; i++) {
		struct udevice *gpio_dev;
		u32 bank = pins[i * 4 + 0];
		u32 pin  = pins[i * 4 + 1];

		data = (i == 0) ? &i2c->recovery_info.scl : &i2c->recovery_info.sda;
		desc = &data->desc;

		ret = uclass_get_device_by_seq(UCLASS_GPIO, bank, &gpio_dev);
		if (ret) {
			printf("GPIO bank %u not found\n", bank);
			return ret;
		}

		data->bank = bank;
		desc->dev = gpio_dev;
		desc->offset = data->pin = pin;
		desc->flags = 0;

		snprintf(gpio_name, sizeof(gpio_name), "i2c%d-%s-gpio",
			 bus_num, (i == 0) ? "scl" : "sda");

		ret = dm_gpio_request(desc, gpio_name);
		if (ret) {
			printf("Request GPIO%u_%u failed: %d\n", bank, pin, ret);
			return ret;
		}

		ret = dm_gpio_set_dir_flags(desc, GPIOD_IS_IN);
		if (ret)
			return ret;

		debug("recovery %s: bank=%u pin=%u (GPIO%u_%c%d)\n",
		      (i == 0) ? "SCL" : "SDA",
		      bank, pin, bank, 'A' + pin / 8, pin % 8);
	}

	return 0;
}
#endif

static int rockchip_i2c_probe(struct udevice *bus)
{
	struct rk_i2c *priv = dev_get_priv(bus);
	struct rk_i2c_soc_data *soc_data;
	struct udevice *pinctrl;
	int bus_nr;
	int ret;

	priv->regs = dev_read_addr_ptr(bus);

	soc_data = (struct rk_i2c_soc_data*)dev_get_driver_data(bus);

	if (soc_data->controller_type == RK_I2C_LEGACY) {
		ret = dev_read_alias_seq(bus, &bus_nr);
		if (ret < 0) {
			debug("%s: Could not get alias for %s: %d\n",
			 __func__, bus->name, ret);
			return ret;
		}

		ret = uclass_get_device(UCLASS_PINCTRL, 0, &pinctrl);
		if (ret) {
			debug("%s: Cannot find pinctrl device\n", __func__);
			return ret;
		}

		/* pinctrl will switch I2C to new type */
		ret = pinctrl_request_noflags(pinctrl, PERIPH_ID_I2C0 + bus_nr);
		if (ret) {
			debug("%s: Failed to switch I2C to new type %s: %d\n",
				__func__, bus->name, ret);
			return ret;
		}
	}

	/* disable autostop */
	writel(0, &priv->regs->con1);

	priv->udev = bus;
	priv->version = rk3x_i2c_get_version(priv);
#if CONFIG_IS_ENABLED(DM_GPIO)
	if (priv->version >= RK_I2C_VERSION1) {
		if (!rk_i2c_parse_pinctrl_and_request_gpio(bus, priv))
			priv->bus_recovery = true;
	}

	if (priv->bus_recovery) {
		priv->recovery_info.get_scl = rockchip_i2c_get_scl_gpio_value;
		priv->recovery_info.set_scl = rockchip_i2c_set_scl_gpio_value;
		priv->recovery_info.get_sda = rockchip_i2c_get_sda_gpio_value;
		priv->recovery_info.set_sda = rockchip_i2c_set_sda_gpio_value;
		priv->recovery_info.prepare_recovery = rockchip_i2c_prepare_recovery;
		priv->recovery_info.unprepare_recovery = rockchip_i2c_unprepare_recovery;
		pinctrl_select_state(priv->udev, "default");
	}
#endif

	return 0;
}

static const struct dm_i2c_ops rockchip_i2c_ops = {
	.xfer		= rockchip_i2c_xfer,
	.set_bus_speed	= rockchip_i2c_set_bus_speed,
};

static const struct rk_i2c_soc_data rk3066_soc_data = {
	.controller_type = RK_I2C_LEGACY,
};

static const struct rk_i2c_soc_data rk3188_soc_data = {
	.controller_type = RK_I2C_LEGACY,
};

static const struct rk_i2c_soc_data rk3228_soc_data = {
	.controller_type = RK_I2C_NEW,
};

static const struct rk_i2c_soc_data rk3288_soc_data = {
	.controller_type = RK_I2C_NEW,
};

static const struct rk_i2c_soc_data rk3328_soc_data = {
	.controller_type = RK_I2C_NEW,
};

static const struct rk_i2c_soc_data rk3399_soc_data = {
	.controller_type = RK_I2C_NEW,
};

static const struct rk_i2c_soc_data rv1126b_soc_data = {
	.controller_type = RK_I2C_NEW,
};

static const struct udevice_id rockchip_i2c_ids[] = {
	{
		.compatible = "rockchip,rk3066-i2c",
		.data = (ulong)&rk3066_soc_data,
	},
	{
		.compatible = "rockchip,rk3188-i2c",
		.data = (ulong)&rk3188_soc_data,
	},
	{
		.compatible = "rockchip,rk3228-i2c",
		.data = (ulong)&rk3228_soc_data,
	},
	{
		.compatible = "rockchip,rk3288-i2c",
		.data = (ulong)&rk3288_soc_data,
	},
	{
		.compatible = "rockchip,rk3328-i2c",
		.data = (ulong)&rk3328_soc_data,
	},
	{
		.compatible = "rockchip,rk3399-i2c",
		.data = (ulong)&rk3399_soc_data,
	},
	{
		.compatible = "rockchip,rv1126b-i2c",
		.data = (ulong)&rv1126b_soc_data,
	},
	{ }
};

U_BOOT_DRIVER(rockchip_rk3066_i2c) = {
	.name	= "rockchip_rk3066_i2c",
	.id	= UCLASS_I2C,
	.of_match = rockchip_i2c_ids,
	.of_to_plat = rockchip_i2c_of_to_plat,
	.probe	= rockchip_i2c_probe,
	.priv_auto	= sizeof(struct rk_i2c),
	.ops	= &rockchip_i2c_ops,
};

DM_DRIVER_ALIAS(rockchip_rk3066_i2c, rockchip_rk3288_i2c)
