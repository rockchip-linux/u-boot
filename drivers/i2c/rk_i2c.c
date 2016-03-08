/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <i2c.h>
#include <asm/arch/rkplat.h>


#define RKI2C_VERSION		"1.3"

/* i2c debug information config */
//#define RKI2C_DEBUG_INFO

#ifdef RKI2C_DEBUG_INFO
#define i2c_info(format, arg...)	printf(format, ##arg)
#else
#define i2c_info(format, arg...)
#endif


/* register io */
#define i2c_writel(val, addr)		writel(val, addr)
#define i2c_readl(addr)			readl(addr)

/* i2c timerout */
#define I2C_TIMEOUT_US		100000  // 100000us = 100ms
#define I2C_RETRY_COUNT		3

/* i2c error return code */
#define I2C_OK			0
#define I2C_ERROR_TIMEOUT	-1
#define I2C_ERROR_NOACK		-2
#define I2C_ERROR_IO		-3

/* rk i2c fifo max transfer bytes */
#define RK_I2C_FIFO_SIZE	32

/* rk i2c device register size */
#define RK_I2C_REGISTER_SIZE	3

#define RK_CEIL(x, y) \
	({ unsigned long __x = (x), __y = (y); (__x + __y - 1) / __y; })


#define I2C_ADAP_SEL_BIT(nr)	((nr) + 11)
#define I2C_ADAP_SEL_MASK(nr)	((nr) + 27)


/* Control register */
#define I2C_CON			0x000
#define I2C_CON_EN		(1 << 0)
#define I2C_CON_MOD(mod)	((mod) << 1)
#define I2C_MODE_TX		0x00
#define I2C_MODE_TRX		0x01
#define I2C_MODE_RX		0x02
#define I2C_MODE_RRX		0x03
#define I2C_CON_MASK		(3 << 1)

#define I2C_CON_START		(1 << 3)
#define I2C_CON_STOP		(1 << 4)
#define I2C_CON_LASTACK		(1 << 5)
#define I2C_CON_ACTACK		(1 << 6)


/* Clock dividor register */
#define I2C_CLKDIV		0x004
#define I2C_CLKDIV_VAL(divl, divh)	(((divl) & 0xffff) | (((divh) << 16) & 0xffff0000))

/* the slave address accessed  for master rx mode */
#define I2C_MRXADDR		0x008
#define I2C_MRXADDR_SET(vld, addr)	(((vld) << 24) | (addr))


/* the slave register address accessed  for master rx mode */
#define I2C_MRXRADDR		0x00c
#define I2C_MRXRADDR_SET(vld, raddr)	(((vld) << 24) | (raddr))

/* master tx count */
#define I2C_MTXCNT		0x010

/* master rx count */
#define I2C_MRXCNT		0x014

/* interrupt enable register */
#define I2C_IEN			0x018
#define I2C_BTFIEN		(1 << 0)
#define I2C_BRFIEN		(1 << 1)
#define I2C_MBTFIEN		(1 << 2)
#define I2C_MBRFIEN		(1 << 3)
#define I2C_STARTIEN		(1 << 4)
#define I2C_STOPIEN		(1 << 5)
#define I2C_NAKRCVIEN		(1 << 6)

/* interrupt pending register */
#define I2C_IPD                 0x01c
#define I2C_BTFIPD              (1 << 0)
#define I2C_BRFIPD              (1 << 1)
#define I2C_MBTFIPD             (1 << 2)
#define I2C_MBRFIPD             (1 << 3)
#define I2C_STARTIPD            (1 << 4)
#define I2C_STOPIPD             (1 << 5)
#define I2C_NAKRCVIPD           (1 << 6)
#define I2C_IPD_ALL_CLEAN       0x7f

/* finished count */
#define I2C_FCNT                0x020

/* I2C tx data register */
#define I2C_TXDATA_BASE         0X100

/* I2C rx data register */
#define I2C_RXDATA_BASE         0x200


struct rk_i2c {
	uint32_t		regs;
	unsigned int		speed;
};


#ifdef CONFIG_I2C_MULTI_BUS
static struct rk_i2c rki2c_base[I2C_BUS_MAX] = {
	{ .regs = (uint32_t)RKIO_I2C0_BASE, 0 },
	{ .regs = (uint32_t)RKIO_I2C1_BASE, 0 },
	{ .regs = (uint32_t)RKIO_I2C2_BASE, 0 },
	{ .regs = (uint32_t)RKIO_I2C3_BASE, 0 },
	{ .regs = (uint32_t)RKIO_I2C4_BASE, 0 },
	{ .regs = (uint32_t)RKIO_I2C5_BASE, 0 },
	{ .regs = (uint32_t)RKIO_I2C6_BASE, 0 },
	{ .regs = (uint32_t)RKIO_I2C7_BASE, 0 }
};
#endif

static uint g_i2c_online_bus = I2C_BUS_MAX;


static inline void *rk_i2c_get_base(void)
{
	if (g_i2c_online_bus >= I2C_BUS_MAX) {
		printf("I2C bus error, PLS set i2c bus first!\n");
		return (void *)NULL;
	}

	if (rki2c_base[g_i2c_online_bus].regs == 0) {
		printf("I2C base register error, PLS check i2c config!\n");
		return (void *)NULL;	
	}

	return (void *)&rki2c_base[g_i2c_online_bus];
}


static inline void rk_i2c_iomux(eI2C_ch_t bus_id)
{
	rk_iomux_config(RK_I2C0_IOMUX + bus_id);
}


static inline void rk_i2c_get_div(int div, int *divh, int *divl)
{
	if (div % 2 == 0) {
		*divh = div / 2;
		*divl = div / 2;
	} else {
		*divh = RK_CEIL(div, 2);
		*divl = div / 2;
	}
}


/* SCL Divisor = 8 * (CLKDIVL+1 + CLKDIVH+1)
 * SCL = PCLK / SCLK Divisor
 * i2c_rate = PCLK
*/
static void rk_i2c_set_clk(struct rk_i2c *i2c, uint32 scl_rate)
{
	uint32 i2c_rate;
	int div, divl, divh;

	if (i2c->speed == scl_rate) {
		return ;
	}
	/* First get i2c rate from pclk */
	i2c_rate = rkclk_get_i2c_clk(g_i2c_online_bus);

	div = RK_CEIL(i2c_rate, scl_rate * 8) - 2;
	if (div < 0) {
		divh = divl = 0;
	} else {
		rk_i2c_get_div(div, &divh, &divl);
	}
	i2c_writel(I2C_CLKDIV_VAL(divl, divh), i2c->regs + I2C_CLKDIV);

	i2c->speed = scl_rate;

	i2c_info("rk_i2c_set_clk: i2c rate = %d, scl rate = %d\n", i2c_rate, scl_rate);
	i2c_info("set i2c clk div = %d, divh = %d, divl = %d\n", div, divh, divl);
	i2c_info("set clk(I2C_CLKDIV: 0x%08x)\n", i2c_readl(i2c->regs + I2C_CLKDIV));
}


static int rk_i2c_init(int speed)
{
	struct rk_i2c *i2c = (struct rk_i2c *)rk_i2c_get_base();

	if (i2c == NULL) {
		return -1;
	}

	i2c_info("rk_i2c_init: I2C bus = %d\n", g_i2c_online_bus);

	rk_i2c_iomux(g_i2c_online_bus);
	rk_i2c_set_clk(i2c, speed);

	return 0;
}


static void rk_i2c_show_regs(struct rk_i2c *i2c)
{
#ifdef RKI2C_DEBUG_INFO
	uint i;

	i2c_info("I2C_CON: 0x%08x\n", i2c_readl(i2c->regs + I2C_CON));
	i2c_info("I2C_CLKDIV: 0x%08x\n", i2c_readl(i2c->regs + I2C_CLKDIV));
	i2c_info("I2C_MRXADDR: 0x%08x\n", i2c_readl(i2c->regs + I2C_MRXADDR));
	i2c_info("I2C_MRXRADDR: 0x%08x\n", i2c_readl(i2c->regs + I2C_MRXRADDR));
	i2c_info("I2C_MTXCNT: 0x%08x\n", i2c_readl(i2c->regs + I2C_MTXCNT));
	i2c_info("I2C_MRXCNT: 0x%08x\n", i2c_readl(i2c->regs + I2C_MRXCNT));
	i2c_info("I2C_IEN: 0x%08x\n", i2c_readl(i2c->regs + I2C_IEN));
	i2c_info("I2C_IPD: 0x%08x\n", i2c_readl(i2c->regs + I2C_IPD));
	i2c_info("I2C_FCNT: 0x%08x\n", i2c_readl(i2c->regs + I2C_FCNT));
	for (i = 0; i < 8; i ++) {
		i2c_info("I2C_TXDATA%d: 0x%08x\n", i, i2c_readl(i2c->regs + I2C_TXDATA_BASE + i * 4));
	}
	for (i = 0; i < 8; i ++) {
		i2c_info("I2C_RXDATA%d: 0x%08x\n", i, i2c_readl(i2c->regs + I2C_RXDATA_BASE + i * 4));
	}
#endif
}


static int rk_i2c_send_start_bit(struct rk_i2c *i2c)
{
	int TimeOut = I2C_TIMEOUT_US;

	i2c_info("I2c Send Start bit.\n");
	i2c_writel(I2C_IPD_ALL_CLEAN, i2c->regs + I2C_IPD);

	i2c_writel(I2C_CON_EN | I2C_CON_START, i2c->regs + I2C_CON);
	i2c_writel(I2C_STARTIEN, i2c->regs + I2C_IEN);

	while (TimeOut--) {
		if (i2c_readl(i2c->regs + I2C_IPD) & I2C_STARTIPD) {
			i2c_writel(I2C_STARTIPD, i2c->regs + I2C_IPD);
			break;
		}
		udelay(1);
	}
	if (TimeOut <= 0) {
		printf("I2C Send Start Bit Timeout\n");
		rk_i2c_show_regs(i2c);
		return I2C_ERROR_TIMEOUT;
	}

	return I2C_OK;
}


static int rk_i2c_send_stop_bit(struct rk_i2c *i2c)
{
	int TimeOut = I2C_TIMEOUT_US;

	i2c_info("I2c Send Stop bit.\n");
	i2c_writel(I2C_IPD_ALL_CLEAN, i2c->regs + I2C_IPD);

	i2c_writel(I2C_CON_EN | I2C_CON_STOP, i2c->regs + I2C_CON);
	i2c_writel(I2C_CON_STOP, i2c->regs + I2C_IEN);

	while (TimeOut--) {
		if (i2c_readl(i2c->regs + I2C_IPD) & I2C_STOPIPD) {
			i2c_writel(I2C_STOPIPD, i2c->regs + I2C_IPD);
			break;
		}
		udelay(1);
	}
	if (TimeOut <= 0) {
		printf("I2C Send Stop Bit Timeout\n");
		rk_i2c_show_regs(i2c);
		return I2C_ERROR_TIMEOUT;
	}

	return I2C_OK;
}


static inline void rk_i2c_disable(struct rk_i2c *i2c)
{
	i2c_writel(0, i2c->regs + I2C_CON);
}


static int rk_i2c_read(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	int err = I2C_OK;
	int TimeOut = I2C_TIMEOUT_US;
	uchar *pbuf = buf;
	uint bytes_remain_len = b_len;
	uint bytes_tranfered_len = 0;
	uint words_tranfered_len = 0;
	uint con = 0;
	uint rxdata;
	uint i, j;

	i2c_info("rk_i2c_read: chip = %d, reg = %d, r_len = %d, b_len = %d\n", chip, reg, r_len, b_len);

	err = rk_i2c_send_start_bit(i2c);
	if (err != I2C_OK) {
		return err;
	}

	i2c_writel(I2C_MRXADDR_SET(1, chip << 1 | 1), i2c->regs + I2C_MRXADDR);
	if (r_len == 0) {
		i2c_writel(0, i2c->regs + I2C_MRXRADDR);
	} else if (r_len < 4) {
		i2c_writel(I2C_MRXRADDR_SET(r_len, reg), i2c->regs + I2C_MRXRADDR);
	} else {
		printf("I2C Read: addr len %d not supported\n", r_len);
		return I2C_ERROR_IO;
	}

	while (bytes_remain_len) {
		if (bytes_remain_len > RK_I2C_FIFO_SIZE) {
			con = I2C_CON_EN | I2C_CON_MOD(I2C_MODE_TRX);
			bytes_tranfered_len = 32;
		} else {
			con = I2C_CON_EN | I2C_CON_MOD(I2C_MODE_TRX) | I2C_CON_LASTACK;
			bytes_tranfered_len = bytes_remain_len;
		}
		words_tranfered_len = RK_CEIL(bytes_tranfered_len, 4);

		i2c_writel(con, i2c->regs + I2C_CON);
		i2c_writel(bytes_tranfered_len, i2c->regs + I2C_MRXCNT);
		i2c_writel(I2C_MBRFIEN | I2C_NAKRCVIEN, i2c->regs + I2C_IEN);

		TimeOut = I2C_TIMEOUT_US;
		while (TimeOut--) {
			if (i2c_readl(i2c->regs + I2C_IPD) & I2C_NAKRCVIPD) {
				i2c_writel(I2C_NAKRCVIPD, i2c->regs + I2C_IPD);
				err = I2C_ERROR_NOACK;
			}
			if (i2c_readl(i2c->regs + I2C_IPD) & I2C_MBRFIPD) {
				i2c_writel(I2C_MBRFIPD, i2c->regs + I2C_IPD);
				break;
			}
			udelay(1);
		}

		if (TimeOut <= 0) {
			printf("I2C Read Data Timeout\n");
			err =  I2C_ERROR_TIMEOUT;
			rk_i2c_show_regs(i2c);
			goto i2c_exit;
		}

		for (i = 0; i < words_tranfered_len; i++) {
			rxdata = i2c_readl(i2c->regs + I2C_RXDATA_BASE + i * 4);
			i2c_info("I2c Read RXDATA[%d] = 0x%x\n", i, rxdata);
			for (j = 0; j < 4; j++) {
				if ((i * 4 + j) == bytes_tranfered_len) {
					break;
				}
				*pbuf++ = (rxdata >> (j * 8)) & 0xff;
			}
		}
		bytes_remain_len -= bytes_tranfered_len;
		i2c_info("I2C Read bytes_remain_len %d\n", bytes_remain_len);
	}

i2c_exit:
	// Send stop bit
	rk_i2c_send_stop_bit(i2c);
	// Disable Controller
	rk_i2c_disable(i2c);

	return err;
}

static int rk_i2c_write(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	int err = I2C_OK;
	int TimeOut = I2C_TIMEOUT_US;
	uchar *pbuf = buf;
	uint bytes_remain_len = b_len + r_len + 1;
	uint bytes_tranfered_len = 0;
	uint words_tranfered_len = 0;
	uint txdata;
	uint i, j;

	i2c_info("rk_i2c_write: chip = %d, reg = %d, r_len = %d, b_len = %d\n", chip, reg, r_len, b_len);

	err = rk_i2c_send_start_bit(i2c);
	if (err != I2C_OK) {
		return err;
	}

	while (bytes_remain_len) {
		if (bytes_remain_len > RK_I2C_FIFO_SIZE) {
			bytes_tranfered_len = 32;
		} else {
			bytes_tranfered_len = bytes_remain_len;
		}
		words_tranfered_len = RK_CEIL(bytes_tranfered_len, 4);

		for (i = 0; i < words_tranfered_len; i++) {
			txdata = 0;
			for (j = 0; j < 4; j++) {
				if ((i * 4 + j) == bytes_tranfered_len) {
					break;
				}

				if (i == 0 && j == 0) {
					txdata |= (chip << 1);
				} else if (i == 0 && j <= r_len) {
					txdata |= (reg & (0xff << ((j - 1) * 8))) << 8;
				} else {
					txdata |= (*pbuf++)<<(j * 8);
				}
				i2c_writel(txdata, i2c->regs + I2C_TXDATA_BASE + i * 4);
			}
			i2c_info("I2c Write TXDATA[%d] = 0x%x\n", i, txdata);
		}

		i2c_writel(I2C_CON_EN | I2C_CON_MOD(I2C_MODE_TX), i2c->regs + I2C_CON);
		i2c_writel(bytes_tranfered_len, i2c->regs + I2C_MTXCNT);
		i2c_writel(I2C_MBTFIEN | I2C_NAKRCVIEN, i2c->regs + I2C_IEN);

		TimeOut = I2C_TIMEOUT_US;
		while (TimeOut--) {
			if (i2c_readl(i2c->regs + I2C_IPD) & I2C_NAKRCVIPD) {
				i2c_writel(I2C_NAKRCVIPD, i2c->regs + I2C_IPD);
				err = I2C_ERROR_NOACK;
			}
			if (i2c_readl(i2c->regs + I2C_IPD) & I2C_MBTFIPD) {
				i2c_writel(I2C_MBTFIPD, i2c->regs + I2C_IPD);
				break;
			}
			udelay(1);
		}

		if (TimeOut <= 0) {
			printf("I2C Write Data Timeout\n");
			err =  I2C_ERROR_TIMEOUT;
			rk_i2c_show_regs(i2c);
			goto i2c_exit;
		}

		bytes_remain_len -= bytes_tranfered_len;
		i2c_info("I2C Write bytes_remain_len %d\n", bytes_remain_len);
	}

i2c_exit:
	// Send stop bit
	rk_i2c_send_stop_bit(i2c);
	// Disable Controller
	rk_i2c_disable(i2c);

	return err;
}


#ifdef CONFIG_I2C_MULTI_BUS
unsigned int i2c_get_bus_num(void)
{
	return g_i2c_online_bus;
}


int i2c_set_bus_num(unsigned bus_idx)
{
	i2c_info("i2c_set_bus_num: I2C bus = %d\n", bus_idx);

	if (bus_idx >= I2C_BUS_MAX) {
		printf("i2c_set_bus_num: I2C bus error!\n");
		return -1;
	}

	g_i2c_online_bus = bus_idx;

	return 0;
}
#endif


/*
 * i2c_read - Read from i2c memory
 * @chip:	target i2c address
 * @addr:	address to read from
 * @alen:
 * @buffer:	buffer for read data
 * @len:	no of bytes to be read
 *
 * Read from i2c memory.
 */
int i2c_read(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	struct rk_i2c *i2c = (struct rk_i2c *)rk_i2c_get_base();

	if (i2c == NULL) {
		return -1;
	}
	if ((buf == NULL) && (len != 0)) {
		printf("i2c_read: buf == NULL\n");
		return -2;
	}

	return rk_i2c_read(i2c, chip, addr, alen, buf, len);
}


/*
 * i2c_write - Write to i2c memory
 * @chip:	target i2c address
 * @addr:	address to read from
 * @alen:
 * @buffer:	buffer for read data
 * @len:	num of bytes to be read
 *
 * Write to i2c memory.
 */
int i2c_write(uchar chip, uint addr, int alen, uchar *buf, int len)
{
	struct rk_i2c *i2c = (struct rk_i2c *)rk_i2c_get_base();

	if (i2c == NULL) {
		return -1;
	}
	if ((buf == NULL) && (len != 0)) {
		printf("i2c_write: buf == NULL\n");
		return -2;
	}

	return rk_i2c_write(i2c, chip, addr, alen, buf, len);
}


/*
 * Test if a chip at a given address responds (probe the chip)
 */
int i2c_probe(uchar chip)
{
	struct rk_i2c *i2c = (struct rk_i2c *)rk_i2c_get_base();

	i2c_info("i2c_probe\n");

	if (i2c == NULL) {
		return -1;
	}

	return rk_i2c_write(i2c, chip, 0, 0, NULL, 0);
}


/*
 * Init I2C Bus
 */
void i2c_init(int speed, int unused)
{
	i2c_info("i2c_init\n");
	rk_i2c_init(speed);
}

/*
 * Set I2C Speed
 */
int i2c_set_bus_speed(unsigned int speed)
{
	struct rk_i2c *i2c = (struct rk_i2c *)rk_i2c_get_base();

	i2c_info("i2c_set_bus_speed\n");

	if (i2c == NULL) {
		return -1;
	}

	if (g_i2c_online_bus >= I2C_BUS_MAX) {
		return -1;
	}

	rk_i2c_set_clk(i2c, speed);

	return 0;
}

/*
 * Get I2C Speed
 */
unsigned int i2c_get_bus_speed(void)
{
	return 0;
}

int i2c_get_bus_num_fdt(int bus_addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rki2c_base); i++) {
		if ((uint32_t)bus_addr == rki2c_base[i].regs)
			return i;
	}

	printf("%s: Can't find any matched I2C bus\n", __func__);
	return -1;
}

