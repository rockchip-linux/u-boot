/*
 * (C) Copyright 2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <errno.h>
#include <dm.h>
#include <i2c.h>
#include <asm/gpio.h>
#include <errno.h>
#include <common.h>
#include <malloc.h>
#include <fdtdec.h>
#include <power/rockchip_power.h>
#include <asm/arch/rkplat.h>

/* #define I2C_GPIO_DEBUG */

#ifdef I2C_GPIO_DEBUG
#define DBG(args...)   printf(args)
#else
#define DBG(args...)
#endif

DECLARE_GLOBAL_DATA_PTR;

struct gpio_desc {
	uint gpio;		/* GPIO gpio within the device */
};

enum {
	PIN_SDA = 0,
	PIN_SCL,
	PIN_COUNT,
};

enum iomux_mode {
	IOMUX_MODE_GPIO = 0,
	IOMUX_MODE_I2C,
};

struct i2c_gpio_bus {
	/*
	 * udelay - delay [us] between GPIO toggle operations,
	 * which is 1/4 of I2C speed clock period.
	 */
	int udelay;

	int i2c_ch;
	int speed_hz;

	 /* sda, scl */
	struct gpio_desc gpios[PIN_COUNT];
};

struct i2c_data {
	int addr;
	int reg;
	int val;
	int rd;
};

static int ignore_ack;

static int i2c_gpio_sda_get(struct gpio_desc *sda)
{
	return gpio_get_value(sda->gpio);
}

static void i2c_gpio_sda_set(struct gpio_desc *sda, int bit)
{
	gpio_set_value(sda->gpio, bit);
}

static void i2c_gpio_scl_set(struct gpio_desc *scl, int bit)
{
	gpio_set_value(scl->gpio, bit);
}

static void i2c_gpio_write_bit(struct gpio_desc *scl, struct gpio_desc *sda,
			       int delay, uchar bit)
{
	i2c_gpio_scl_set(scl, 0);
	udelay(delay);
	i2c_gpio_sda_set(sda, bit);
	udelay(delay);
	i2c_gpio_scl_set(scl, 1);
	udelay(2 * delay);
}

static int i2c_gpio_read_bit(struct gpio_desc *scl, struct gpio_desc *sda,
			     int delay)
{
	int value;

	i2c_gpio_scl_set(scl, 1);
	udelay(delay);
	value = i2c_gpio_sda_get(sda);
	udelay(delay);
	i2c_gpio_scl_set(scl, 0);
	udelay(2 * delay);

	return value;
}

/* START: High -> Low on SDA while SCL is High */
static void i2c_gpio_send_start(struct gpio_desc *scl, struct gpio_desc *sda,
				int delay)
{
	udelay(delay);
	i2c_gpio_sda_set(sda, 1);
	udelay(delay);
	i2c_gpio_scl_set(scl, 1);
	udelay(delay);
	i2c_gpio_sda_set(sda, 0);
	udelay(delay);
}

/* STOP: Low -> High on SDA while SCL is High */
static void i2c_gpio_send_stop(struct gpio_desc *scl, struct gpio_desc *sda,
			       int delay)
{
	i2c_gpio_scl_set(scl, 0);
	udelay(delay);
	i2c_gpio_sda_set(sda, 0);
	udelay(delay);
	i2c_gpio_scl_set(scl, 1);
	udelay(delay);
	i2c_gpio_sda_set(sda, 1);
	udelay(delay);
}

/* ack should be I2C_ACK or I2C_NOACK */
static void i2c_gpio_send_ack(struct gpio_desc *scl, struct gpio_desc *sda,
			      int delay, int ack)
{
	i2c_gpio_write_bit(scl, sda, delay, ack);
	i2c_gpio_scl_set(scl, 0);
	udelay(delay);
}

/*
 * Send a reset sequence consisting of 9 clocks with the data signal high
 * to clock any confused device back into an idle state.  Also send a
 * <stop> at the end of the sequence for belts & suspenders.
 */
static void i2c_gpio_send_reset(struct gpio_desc *scl, struct gpio_desc *sda,
				int delay)
{
	int j;

	for (j = 0; j < 9; j++)
		i2c_gpio_write_bit(scl, sda, delay, 1);

	i2c_gpio_send_stop(scl, sda, delay);
}

/* Set sda high with low clock, before reading slave data */
static void i2c_gpio_sda_high(struct gpio_desc *scl, struct gpio_desc *sda,
			      int delay)
{
	i2c_gpio_scl_set(scl, 0);
	udelay(delay);
	i2c_gpio_sda_set(sda, 1);
	udelay(delay);
}

/* Send 8 bits and look for an acknowledgment */
static int i2c_gpio_write_byte(struct gpio_desc *scl, struct gpio_desc *sda,
			       int delay, uchar data)
{
	int j;
	int nack;

	for (j = 0; j < 8; j++) {
		i2c_gpio_write_bit(scl, sda, delay, data & 0x80);
		data <<= 1;
	}

	udelay(delay);

	/* Look for an <ACK>(negative logic) and return it */
	i2c_gpio_sda_high(scl, sda, delay);
	nack = i2c_gpio_read_bit(scl, sda, delay);

	return ignore_ack ? 0 : nack;
}

/**
 * if ack == I2C_ACK, ACK the byte so can continue reading, else
 * send I2C_NOACK to end the read.
 */
static uchar i2c_gpio_read_byte(struct gpio_desc *scl, struct gpio_desc *sda,
				int delay, int ack)
{
	int  data;
	int  j;

	i2c_gpio_sda_high(scl, sda, delay);
	data = 0;
	for (j = 0; j < 8; j++) {
		data <<= 1;
		data |= i2c_gpio_read_bit(scl, sda, delay);
	}

	i2c_gpio_send_ack(scl, sda, delay, ack);

	return data;
}

static int i2c_gpio_set_bus_speed(struct i2c_gpio_bus *bus,
				  unsigned int speed_hz)
{
	struct gpio_desc *scl = &bus->gpios[PIN_SCL];
	struct gpio_desc *sda = &bus->gpios[PIN_SDA];

	bus->udelay = 1000000 / (speed_hz << 2);

	i2c_gpio_send_reset(scl, sda, bus->udelay);

	return bus->udelay;
}

static int user_i2c_write_byte(struct i2c_gpio_bus *bus,
			       u8 slave, u8 reg, u8 value)
{
	struct gpio_desc *scl = &bus->gpios[PIN_SCL];
	struct gpio_desc *sda = &bus->gpios[PIN_SDA];
	int delay = bus->udelay;
	int ret;

	i2c_gpio_send_start(scl, sda, delay);

	/* slave address */
	ret = i2c_gpio_write_byte(scl, sda, delay, (slave << 1) | 0);
	if (ret) {
		printf("send slave address failed\n");
		return ret;
	}

	/* reg address */
	ret = i2c_gpio_write_byte(scl, sda, delay, reg);
	if (ret) {
		printf("send register address failed\n");
		return ret;
	}

	/* value */
	ret = i2c_gpio_write_byte(scl, sda, delay, value);
	if (ret) {
		printf("send data failed\n");
		return ret;
	}

	i2c_gpio_send_stop(scl, sda, delay);

	return 0;
}

static int user_i2c_read_byte(struct i2c_gpio_bus *bus, u8 slave, u8 reg)
{
	struct gpio_desc *scl = &bus->gpios[PIN_SCL];
	struct gpio_desc *sda = &bus->gpios[PIN_SDA];
	int delay = bus->udelay;
	int ret;

	i2c_gpio_send_start(scl, sda, delay);

	/* slave address */
	ret = i2c_gpio_write_byte(scl, sda, delay, (slave << 1) | 0);
	if (ret) {
		printf("send slave 1st address failed\n");
		return ret;
	}

	/* reg address */
	ret = i2c_gpio_write_byte(scl, sda, delay, reg);
	if (ret) {
		printf("send register address failed\n");
		return ret;
	}

	/* start again and slave address */
	i2c_gpio_send_start(scl, sda, delay);
	ret = i2c_gpio_write_byte(scl, sda, delay, (slave << 1) | 1);
	if (ret) {
		printf("send slave 2nd address failed\n");
		return ret;
	}

	/* read data */
	ret = i2c_gpio_read_byte(scl, sda, delay, 1);
	if (!ret) {
		printf("read data failed\n");
		return ret;
	}

	i2c_gpio_send_stop(scl, sda, delay);

	return ret;
}

static int i2c_gpio_set_iomux(struct i2c_gpio_bus *bus, int mode)
{
#if defined(CONFIG_RKCHIP_RK3126)
	if (mode == IOMUX_MODE_GPIO)
		mdelay(10);

	switch (bus->i2c_ch) {
	case 0:
		if (mode == IOMUX_MODE_GPIO)
			grf_writel((1 << 18) | (1 << 16) | (0 << 2) | (0 << 0),
				   GRF_GPIO0A_IOMUX);
		else
			grf_writel((1 << 18) | (1 << 16) | (1 << 2) | (1 << 0),
				   GRF_GPIO0A_IOMUX);
		break;

	case 1:
		if (mode == IOMUX_MODE_GPIO)
			grf_writel((3 << 22) | (1 << 20) | (0 << 6) | (0 << 4),
				   GRF_GPIO0A_IOMUX);
		else
			grf_writel((3 << 22) | (1 << 20) | (1 << 6) | (1 << 4),
				   GRF_GPIO0A_IOMUX);
		break;

	case 2:
		if (mode == IOMUX_MODE_GPIO)
			grf_writel((7 << 20) | (7 << 16) | (0 << 4) | (0 << 0),
				   GRF_GPIO2C_IOMUX2);
		else
			grf_writel((7 << 20) | (7 << 16) | (3 << 4) | (3 << 0),
				   GRF_GPIO2C_IOMUX2);
		break;

	case 3:
		if (mode == IOMUX_MODE_GPIO)
			grf_writel((3 << 30) | (3 << 28) | (0 << 14) | (0 << 12),
				   GRF_GPIO0A_IOMUX);
		else
			grf_writel((3 << 30) | (3 << 28) | (1 << 14) | (1 << 12),
				   GRF_GPIO0A_IOMUX);
		break;

	default:
		printf("invalid i2c channel: %d\n", bus->i2c_ch);
		return -EINVAL;
	}

	if (mode == IOMUX_MODE_I2C)
		mdelay(5);
#else
	return -EINVAL;
#endif

	return 0;
}

/* To enable this, you should add dts node like this:
 *
 * gpio_simulate_i2c {
 *     compatible = "gpio-simulate-i2c";
 *     gpio-scl = <&gpio2 GPIO_C5 GPIO_ACTIVE_HIGH>;
 *     gpio-sda = <&gpio2 GPIO_C4 GPIO_ACTIVE_HIGH>;
 *     i2c-channel = <2>;
 *     i2c-speed-hz = <100000>;
 *     i2c-ignore-ack = <0>;
 *     i2c-data = <0x1a 0x17 0 1>, <0x1a 0x18 0 1>,
 * 	          <0x1a 0x3b 0x35 0>, <0x1a 0x27 0x11 0>;
 *     status = "okay";
 * };
 *
 * 'i2c-data' format: <slave  reg  value  dir>
 *     @slave: device i2c address;
 *     @reg: register address of slave;
 *     @value: write value, ignored when read;
 *     @dir: read=1, write=0;
 *
 * The driver will set 'i2c-data' one by one.
 */
int gpio_simulate_i2c(void)
{
	struct i2c_gpio_bus gbus;
	struct i2c_gpio_bus *bus = &gbus;
	struct fdt_gpio_state gpio_scl, gpio_sda;
	const char *prop;
	const void *blob = gd->fdt_blob;
	struct i2c_data *i2c_datas;
	int node, i, len, groups, count, err;
	u8 addr, reg, val, rd;

	/* find node */
	node = fdt_node_offset_by_compatible(blob, 0, "gpio-simulate-i2c");
	if (node < 0) {
		debug("can't find gpio-simulate-i2c node\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, node)) {
		debug("gpio-simulate-i2c is disabled\n");
		return -EACCES;
	}

	/* gpio-scl */
	if (fdtdec_decode_gpio(blob, node, "gpio-scl", &gpio_scl)) {
		printf("can't find prop: gpio_scl\n");
		return -EINVAL;
	}
	bus->gpios[PIN_SCL].gpio = gpio_scl.gpio;
	gpio_direction_output(bus->gpios[PIN_SCL].gpio, 1);

	/* gpio-sda */
	if (fdtdec_decode_gpio(blob, node, "gpio-sda", &gpio_sda)) {
		printf("can't find prop: gpio_sda\n");
		return -EINVAL;
	}
	bus->gpios[PIN_SDA].gpio = gpio_sda.gpio;
	gpio_direction_output(bus->gpios[PIN_SDA].gpio, 1);

	/* others */
	bus->i2c_ch = fdtdec_get_int(blob, node, "i2c-channel", -1);
	if (bus->i2c_ch < 0) {
		printf("can't find prop: i2c-ch\n");
		return -EINVAL;
	}
	bus->speed_hz = fdtdec_get_int(blob, node, "i2c-speed-hz", -1);
	if (bus->speed_hz < 0) {
		printf("can't find prop: i2c-speed-hz\n");
		return -EINVAL;
	}

	ignore_ack = fdtdec_get_int(blob, node, "i2c-ignore-ack", -1);
	if (ignore_ack < 0) {
		printf("can't find prop: i2c-ignore-ack\n");
		return -EINVAL;
	}

	DBG("channel=%d, scl=%d, sda=%d, speed=%d, ignore-ack=%d\n",
	    bus->i2c_ch, bus->gpios[PIN_SCL].gpio, bus->gpios[PIN_SDA].gpio,
	    bus->speed_hz, ignore_ack);

	/* setting values */
	prop = fdt_getprop(blob, node, "i2c-data", &len);
	if (!prop) {
		printf("can't find prop: i2c-data\n");
		return -EINVAL;
	}

	count = len / sizeof(u32);	/* totoal int array members */
	if (!count || count % 4) {
		printf("not 4 members per group\n");
		return -EINVAL;
	}

	groups = count / 4;		/* 4 members in a group */
	i2c_datas = (struct i2c_data *)malloc(sizeof(struct i2c_data) * groups);
	if (!i2c_datas) {
		printf("malloc i2c_datas failed\n");
		return -ENOMEM;
	}
	memset(i2c_datas, 0, sizeof(sizeof(struct i2c_data) * groups));

	err = fdtdec_get_int_array(blob, node, "i2c-data",
				   (u32 *)i2c_datas, count);
	if (err) {
		printf("get i2c-data failed\n");
		return err;
	}

	for (i = 0; i < groups; i++) {
		DBG("0x%x, 0x%x, 0x%x, 0x%x\n",
		    i2c_datas[i].addr, i2c_datas[i].reg,
		    i2c_datas[i].val, i2c_datas[i].rd);
	}

	/* switch to gpio mode */
	i2c_gpio_set_iomux(bus, IOMUX_MODE_GPIO);

	/* i2c transfer init */
	i2c_gpio_set_bus_speed(bus, bus->speed_hz);

	/* read or write data */
	for (i = 0; i < groups; i++) {
		addr = i2c_datas[i].addr;
		reg = i2c_datas[i].reg;
		val = i2c_datas[i].val;
		rd = i2c_datas[i].rd;
		if (rd) {
			printf("%s: slave[0x%x], reg[0x%x] = 0x%x\n", __func__,
			       addr, reg, user_i2c_read_byte(bus, addr, reg));
		} else {
			DBG("i2c write: addr=0x%x, reg=0x%x, val=0x%x\n",
			    addr, reg, val);
			user_i2c_write_byte(bus, addr, reg, val);
		}
	}

	/* switch back to i2c mode */
	i2c_gpio_set_iomux(bus, IOMUX_MODE_I2C);

	return 0;
}
