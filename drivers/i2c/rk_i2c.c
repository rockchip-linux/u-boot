/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
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
#include <asm/io.h>
#include <asm/sizes.h>
#include <i2c.h>
#include <asm/arch/rkplat.h>


#define RKI2C_VERSION		"1.1"

/* i2c debug information config */
//#define RKI2C_DEBUG_INFO

#ifdef RKI2C_DEBUG_INFO
#define rki2c_printf(format, arg...)	printf(format, ##arg)
#else
#define rki2c_printf(format, arg...)
#endif


/* register io */
#define i2c_writel(val, addr)		writel(val, addr)
#define i2c_readl(addr)			readl(addr)

/* rk i2c fifo max transfer bytes */
#define RK_I2C_FIFO_SIZE	32

/* rk i2c device register size */
#define RK_I2C_REGISTER_SIZE	3

#define rk_ceil(x, y) \
	({ unsigned long __x = (x), __y = (y); (__x + __y - 1) / __y; })


#define I2C_ADAP_SEL_BIT(nr)	((nr) + 11)
#define I2C_ADAP_SEL_MASK(nr)	((nr) + 27)


/* Control register */
#define I2C_CON                 0x000
#define I2C_CON_EN              (1 << 0)
#define I2C_CON_MOD(mod)        ((mod) << 1)
#define I2C_CON_MASK            (3 << 1)

#define I2C_CON_START           (1 << 3)
#define I2C_CON_STOP            (1 << 4)
#define I2C_CON_LASTACK         (1 << 5)
#define I2C_CON_ACTACK          (1 << 6)

/* Clock dividor register */
#define I2C_CLKDIV              0x004
#define I2C_CLKDIV_VAL(divl, divh) (((divl) & 0xffff) | (((divh) << 16) & 0xffff0000))    

/* the slave address accessed  for master rx mode */
#define I2C_MRXADDR             0x008
#define I2C_MRXADDR_LOW         (1 << 24)
#define I2C_MRXADDR_MID         (1 << 25)
#define I2C_MRXADDR_HIGH        (1 << 26)

/* the slave register address accessed  for master rx mode */
#define I2C_MRXRADDR            0x00c
#define I2C_MRXRADDR_LOW        (1 << 24)
#define I2C_MRXRADDR_MID        (1 << 25)
#define I2C_MRXRADDR_HIGH       (1 << 26)

/* master tx count */
#define I2C_MTXCNT              0x010

/* master rx count */
#define I2C_MRXCNT              0x014

/* interrupt enable register */
#define I2C_IEN                 0x018
#define I2C_BTFIEN              (1 << 0)
#define I2C_BRFIEN              (1 << 1)
#define I2C_MBTFIEN             (1 << 2)
#define I2C_MBRFIEN             (1 << 3)
#define I2C_STARTIEN            (1 << 4)
#define I2C_STOPIEN             (1 << 5)
#define I2C_NAKRCVIEN           (1 << 6)
#define IRQ_MST_ENABLE          (I2C_MBTFIEN | I2C_MBRFIEN | I2C_NAKRCVIEN | I2C_STARTIEN | I2C_STOPIEN)
#define IRQ_ALL_DISABLE         0

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
	void __iomem		*regs;
	unsigned int		speed;
};

#ifdef CONFIG_I2C_MULTI_BUS
static struct rk_i2c rki2c_base[I2C_BUS_MAX] = {
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
	{ .regs = (void __iomem *)(RKIO_I2C0_PMU_PHYS), 0 },
	{ .regs = (void __iomem *)(RKIO_I2C1_SENSOR_PHYS), 0 },
	{ .regs = (void __iomem *)(RKIO_I2C2_AUDIO_PHYS), 0 },
	{ .regs = (void __iomem *)(RKIO_I2C3_CAM_PHYS), 0 },
	{ .regs = (void __iomem *)(RKIO_I2C4_TP_PHYS), 0 },
	{ .regs = (void __iomem *)(RKIO_I2C5_HDMI_PHYS), 0 }
#else
	#error "PLS config chiptype for i2c base!"
#endif
};
#endif

static uint g_i2c_online_bus = I2C_BUS_MAX;


static inline void *rk_i2c_get_base(void)
{
	if(g_i2c_online_bus >= I2C_BUS_MAX) {
		printf("I2C bus error, PLS set i2c bus first!");
		return (void *)NULL;
	}

	if(rki2c_base[g_i2c_online_bus].regs == 0) {
		printf("I2C base register error, PLS check i2c config!");
		return (void *)NULL;	
	}

	return (void *)&rki2c_base[g_i2c_online_bus];
}


static inline void rk_i2c_iomux(rk_i2c_bus_ch_t bus_id)
{
	rk_iomux_config(RK_I2C0_IOMUX + bus_id);
}


static void rk_i2c_show_regs(struct rk_i2c *i2c)
{
#ifdef RKI2C_DEBUG_INFO
	uint i;

	rki2c_printf("I2C_CON: 0x%08x\n", i2c_readl(i2c->regs + I2C_CON));
	rki2c_printf("I2C_CLKDIV: 0x%08x\n", i2c_readl(i2c->regs + I2C_CLKDIV));
	rki2c_printf("I2C_MRXADDR: 0x%08x\n", i2c_readl(i2c->regs + I2C_MRXADDR));
	rki2c_printf("I2C_MRXRADDR: 0x%08x\n", i2c_readl(i2c->regs + I2C_MRXRADDR));
	rki2c_printf("I2C_MTXCNT: 0x%08x\n", i2c_readl(i2c->regs + I2C_MTXCNT));
	rki2c_printf("I2C_MRXCNT: 0x%08x\n", i2c_readl(i2c->regs + I2C_MRXCNT));
	rki2c_printf("I2C_IEN: 0x%08x\n", i2c_readl(i2c->regs + I2C_IEN));
	rki2c_printf("I2C_IPD: 0x%08x\n", i2c_readl(i2c->regs + I2C_IPD));
	rki2c_printf("I2C_FCNT: 0x%08x\n", i2c_readl(i2c->regs + I2C_FCNT));
	for(i = 0; i < 8; i ++) {
		rki2c_printf("I2C_TXDATA%d: 0x%08x\n", i, i2c_readl(i2c->regs + I2C_TXDATA_BASE + i * 4));
	}
	for(i = 0; i < 8; i ++) {
		rki2c_printf("I2C_RXDATA%d: 0x%08x\n", i, i2c_readl(i2c->regs + I2C_RXDATA_BASE + i * 4));
	}
#endif
}

/* returns TRUE if we reached the end of the current message */
static inline int is_msgend(uint cnt, uint r_len, uint b_len)
{
	return (cnt >= 1 + r_len + b_len);
}

static inline void rk_i2c_enable(struct rk_i2c *i2c, uint mode, uint lastnak)
{
	uint con = 0;

	con |= I2C_CON_EN;
	con |= I2C_CON_MOD(mode);
	if(lastnak) {
		con |= I2C_CON_LASTACK;
	}
	con |= I2C_CON_START;
	con &= ~(I2C_CON_STOP);
	i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_disable(struct rk_i2c *i2c)
{
	i2c_writel(0, i2c->regs + I2C_CON);
}

static inline void rk_i2c_clean_start(struct rk_i2c *i2c)
{
	uint con = i2c_readl(i2c->regs + I2C_CON);

	con &= ~I2C_CON_START;
	i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_send_start(struct rk_i2c *i2c)
{
	uint con = i2c_readl(i2c->regs + I2C_CON);

	con |= I2C_CON_START;
	if(con & I2C_CON_STOP) {
		printf("rk_i2c_send_start, I2C_CON: stop bit is set.\n");
	}
	i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_send_stop(struct rk_i2c *i2c)
{
	uint con = i2c_readl(i2c->regs + I2C_CON);

	con |= I2C_CON_STOP;
	if(con & I2C_CON_START) {
		printf("rk_i2c_send_stop, I2C_CON: start bit is set.\n");
	}

	i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_clean_stop(struct rk_i2c *i2c)
{
	uint con = i2c_readl(i2c->regs + I2C_CON);

	con &= ~I2C_CON_STOP;
	i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_clean_all_ipd(struct rk_i2c *i2c)
{
	i2c_writel(I2C_IPD_ALL_CLEAN, i2c->regs + I2C_IPD);
}

static inline int rk_i2c_get_ipd_event(struct rk_i2c *i2c, int type)
{
	uint con = i2c_readl(i2c->regs + I2C_IPD);
	int time = 2000;

	while((--time) && (!(con & type))) {
		udelay(10);
		con = i2c_readl(i2c->regs + I2C_IPD);
	}
	rki2c_printf("i2c ipd register = 0x%x, type = 0x%x\n", i2c_readl(i2c->regs + I2C_IPD), type);
	i2c_writel(type, i2c->regs + I2C_IPD);

	if (con & I2C_NAKRCVIPD) {
		printf("No ACK Received when get ipd event = 0x%x!\n", type);
		i2c_writel(I2C_NAKRCVIPD, i2c->regs + I2C_IPD);
		return -1;
	}

	if(time <= 0) {
		printf("Get ipd event = 0x%x time out.\n", type);
		return -2;
	}
	
	return 0;
}

/* SCL Divisor = 8 * (CLKDIVL + CLKDIVH)
 * SCL = PCLK / SCLK Divisor
 * i2c_rate = PCLK
*/
static void rk_i2c_set_clk(struct rk_i2c *i2c, uint32 scl_rate)
{
	uint32 i2c_rate;
	uint div, divl, divh;

	if(i2c->speed == scl_rate) {
		return ;
	}
	/* First get i2c rate from pclk */
	i2c_rate = rkclk_get_i2c_clk(g_i2c_online_bus);

	div = rk_ceil(i2c_rate, scl_rate * 8);
	divh = divl = rk_ceil(div, 2);
	i2c_writel(I2C_CLKDIV_VAL(divl, divh), i2c->regs + I2C_CLKDIV);

	i2c->speed = scl_rate;

	rki2c_printf("rk_i2c_set_clk: i2c rate = %d, scl rate = %d\n", i2c_rate, scl_rate);
	rki2c_printf("set i2c clk div = %d, divh = %d, divl = %d\n", div, divh, divl);
	rki2c_printf("set clk(I2C_CLKDIV: 0x%08x)\n", i2c_readl(i2c->regs + I2C_CLKDIV));
}

static int rk_i2c_write_prepare(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	uint data = 0, cnt = 0, len = 0;
	uint i, j;
	uchar byte;

	if (r_len > RK_I2C_REGISTER_SIZE) {
		printf("Not support register len.\n");
		return -1;
	}

	i2c_writel(I2C_MBTFIEN, i2c->regs + I2C_IEN);

	for(i = 0; i < 8; i++) {
		data = 0;
		for(j = 0; j < 4; j++) {
			if(is_msgend(cnt, r_len, b_len))
				break;
			if(cnt == 0) {
				byte = (chip & 0x7f) << 1;
			} else if((cnt == 1) && (r_len != 0)) {
				byte = (uchar)(reg & 0xff);
			} else if((cnt == 2) && (r_len == 2)) {
				byte = (uchar)((reg >> 8) & 0xff);
			} else if((cnt == 3) && (r_len == 3)) {
				byte = (uchar)((reg >> 16) & 0xff);
			} else {
				byte =  buf[len++];
			}

			cnt++;
			data |= (byte << (j * 8));
		}
		//rki2c_printf("rk_i2c_write_prepare: data = 0x%08x\n", data);
		i2c_writel(data, i2c->regs + I2C_TXDATA_BASE + 4 * i);
		if(is_msgend(cnt, r_len, b_len)) {
			break;
		}
	}

	i2c_writel(cnt, i2c->regs + I2C_MTXCNT);
	return 0;
}

static int rk_i2c_read_prepare(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	uint addr = (chip & 0x7f) << 1;

	if (r_len > RK_I2C_REGISTER_SIZE) {
		printf("Not support register len.\n");
		return -1;
	}

	i2c_writel(I2C_MBRFIEN, i2c->regs + I2C_IEN);
	i2c_writel(addr | I2C_MRXADDR_LOW, i2c->regs + I2C_MRXADDR);

	/* if r_len == 0, device has no register addr, detect read/write mode */
	if(r_len == 1) {
		i2c_writel(reg | I2C_MRXADDR_LOW, i2c->regs + I2C_MRXRADDR);
	} else if(r_len == 2) {
		i2c_writel(reg | I2C_MRXRADDR_LOW | I2C_MRXRADDR_MID, i2c->regs + I2C_MRXRADDR);
	} else if(r_len == 3) {
		i2c_writel(reg | I2C_MRXRADDR_LOW | I2C_MRXRADDR_MID | I2C_MRXRADDR_HIGH, i2c->regs + I2C_MRXRADDR);
	}

	//rki2c_printf("rk_i2c_read_prepare: reg = 0x%04x, I2C_MRXRADDR = 0x%08x\n", reg, i2c->regs + I2C_MRXRADDR);
	i2c_writel(b_len, i2c->regs + I2C_MRXCNT);

	return 0;
}

static void rk_i2c_get_data(struct rk_i2c *i2c, uchar *buf, uint b_len)
{
	uint p = 0, i = 0;

	for(i = 0; i < b_len; i++) {
		if(i%4 == 0) {
			p = i2c_readl(i2c->regs + I2C_RXDATA_BASE +  (i/4) * 4);
		}
		buf[i] = (p >> ((i%4) * 8)) & 0xff;
	}
}

static int rk_i2c_read(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	int err;

	if(b_len > RK_I2C_FIFO_SIZE) {
		printf("rk_i2c_read: recv len > fifo buffer.\n");
		return -1;
	}

	//rki2c_printf("rk_i2c_read: chip = %d, reg = %d, r_len = %d, b_len = %d\n", chip, reg, r_len, b_len);
	rk_i2c_enable(i2c, 1, 1);
	rk_i2c_clean_all_ipd(i2c);
	err = rk_i2c_get_ipd_event(i2c, I2C_STARTIPD);
	if(err < 0) {
		goto read_fail;
	}
	rk_i2c_clean_start(i2c);

	err = rk_i2c_read_prepare(i2c, chip, reg, r_len, buf, b_len);
	if(err < 0) {
		goto read_fail;
	}
	err = rk_i2c_get_ipd_event(i2c, I2C_MBRFIPD);
	if(err < 0) {
		goto read_fail;
	}
	rk_i2c_get_data(i2c, buf, b_len);

	rk_i2c_send_stop(i2c);
	err = rk_i2c_get_ipd_event(i2c, I2C_STOPIPD);
	if(err < 0) {
		goto read_fail;
	}
	rk_i2c_clean_stop(i2c);
	rk_i2c_clean_all_ipd(i2c);
	rk_i2c_disable(i2c);

	return 0;

read_fail:
	rk_i2c_send_stop(i2c);
	rk_i2c_clean_all_ipd(i2c);
	rk_i2c_disable(i2c);
	return err;
}

static int rk_i2c_write(struct rk_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	int err;

	if(r_len + b_len + 1 > RK_I2C_FIFO_SIZE) {
		printf("rk_i2c_write: send len > fifo buffer.\n");
		return -1;
	}

	//rki2c_printf("rk_i2c_write: chip = %d, reg = %d, r_len = %d, b_len = %d\n", chip, reg, r_len, b_len);
	rk_i2c_enable(i2c, 0, 1);
	rk_i2c_clean_all_ipd(i2c);
	err = rk_i2c_get_ipd_event(i2c, I2C_STARTIPD);
	if(err < 0) {
		goto write_fail;
	}
	rk_i2c_clean_start(i2c);

	err = rk_i2c_write_prepare(i2c, chip, reg, r_len, buf, b_len);
	if(err < 0) {
		goto write_fail;
	}
	err = rk_i2c_get_ipd_event(i2c, I2C_MBTFIPD);
	if(err < 0) {
		goto write_fail;
	}

	rk_i2c_send_stop(i2c);
	err = rk_i2c_get_ipd_event(i2c, I2C_STOPIPD);
	if(err < 0) {
		goto write_fail;
	}
	rk_i2c_clean_stop(i2c);
	rk_i2c_clean_all_ipd(i2c);
	rk_i2c_disable(i2c);

	return 0;

write_fail:
	rk_i2c_send_stop(i2c);
	rk_i2c_clean_all_ipd(i2c);
	rk_i2c_disable(i2c);
	return err;
}


static int rk_i2c_init(int speed)
{
	struct rk_i2c *i2c = (struct rk_i2c *)rk_i2c_get_base();

	if(i2c == NULL) {
		return -1;
	}

	rki2c_printf("rk_i2c_init: I2C bus = %d\n", g_i2c_online_bus);

	rk_i2c_iomux(g_i2c_online_bus);
	rk_i2c_set_clk(i2c, speed);

	return 0;
}


void rki2c_show_regs(void)
{
	debug("rk i2c version: %s\n", RKI2C_VERSION);
	rk_i2c_show_regs(rk_i2c_get_base());
}


#ifdef CONFIG_I2C_MULTI_BUS
unsigned int i2c_get_bus_num(void)
{
	return g_i2c_online_bus;
}


int i2c_set_bus_num(unsigned bus_idx)
{
	if(bus_idx >= I2C_BUS_MAX) {
		printf("i2c_set_bus_num: I2C bus error!");
		return -1;
	}

	rki2c_printf("i2c_set_bus_num: I2C bus = %d\n", bus_idx);

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

	if(i2c == NULL) {
		return -1;
	}
	if ((buf == NULL) && (len != 0)) {
		printf("i2c_read: buf == NULL");
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

	if(i2c == NULL) {
		return -1;
	}
	if ((buf == NULL) && (len != 0)) {
		printf("i2c_write: buf == NULL");
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

	if(i2c == NULL) {
		return -1;
	}

	return rk_i2c_write(i2c, chip, 0, 0, NULL, 0);
}


/*
 * Init I2C Bus
 */
void i2c_init(int speed, int unused)
{
	rk_i2c_init(speed);
}

/*
 * Set I2C Speed
 */
int i2c_set_bus_speed(unsigned int speed)
{
	struct rk_i2c *i2c = (struct rk_i2c *)rk_i2c_get_base();

	if(i2c == NULL) {
		return -1;
	}

	if(g_i2c_online_bus >= I2C_BUS_MAX) {
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

