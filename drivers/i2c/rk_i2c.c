/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/

#include <common.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/arch/rk30_drivers.h>
#include <asm/arch/rk_i2c.h>

#define i2c_writel		writel
#define i2c_readl		readl

#define rk_ceil(x, y) \
	({ unsigned long __x = (x), __y = (y); (__x + __y - 1) / __y; })

#define I2C_ADAP_SEL_BIT(nr)        ((nr) + 11)
#define I2C_ADAP_SEL_MASK(nr)        ((nr) + 27)

#define COMPLETE_READ		(1<<STATE_START|1<<STATE_READ|1<<STATE_STOP)
#define COMPLETE_WRITE		(1<<STATE_START|1<<STATE_WRITE|1<<STATE_STOP)

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

struct rk30_i2c {
	void __iomem		*regs;
	unsigned int		speed;
};

#ifdef CONFIG_I2C_MULTI_BUS
 struct rk30_i2c rki2c_base[I2C_BUS_MAX] = {
 #if (CONFIG_RKCHIPTYPE == CONFIG_RK3026)
 	{ .regs = I2C0_BASE_ADDR + SZ_8K, 0 },
	{ .regs = I2C1_BASE_ADDR + SZ_8K, 0 },
	{ .regs = I2C2_BASE_ADDR + SZ_8K, 0 },
	{ .regs = I2C3_BASE_ADDR + SZ_8K, 0 },
 #else
	{ .regs = I2C0_BASE_ADDR + SZ_4K, 0 },
	{ .regs = I2C1_BASE_ADDR + SZ_4K, 0 },
	{ .regs = I2C2_BASE_ADDR + SZ_8K, 0 },
	{ .regs = I2C3_BASE_ADDR + SZ_8K, 0 },
	{ .regs = I2C4_BASE + SZ_8K, 0 }
	#endif
};

static unsigned int gcurrent_bus = I2C_BUS_MAX;
#endif

static void *get_base(void)
{
	if (gcurrent_bus >= I2C_BUS_MAX) {
		printf("I2C bus error!");
		return (void *)NULL;
	}

	if (rki2c_base[gcurrent_bus].regs == 0) {
		printf("I2C base register error!");
		return (void *)NULL;	
	}

	return (void *) &rki2c_base[gcurrent_bus];
}

static void i2c_adap_sel(int nr)
{
    g_grfReg->GRF_SOC_CON[1] = (1 << I2C_ADAP_SEL_BIT(nr)) | (1 << I2C_ADAP_SEL_MASK(nr));
}

static void rk30_show_regs(struct rk30_i2c *i2c)
{
        int i;

        printf("I2C_CON: 0x%08x\n", i2c_readl(i2c->regs + I2C_CON));
        printf("I2C_CLKDIV: 0x%08x\n", i2c_readl(i2c->regs + I2C_CLKDIV));
        printf("I2C_MRXADDR: 0x%08x\n", i2c_readl(i2c->regs + I2C_MRXADDR));
        printf("I2C_MRXRADDR: 0x%08x\n", i2c_readl(i2c->regs + I2C_MRXRADDR));
        printf("I2C_MTXCNT: 0x%08x\n", i2c_readl(i2c->regs + I2C_MTXCNT));
        printf("I2C_MRXCNT: 0x%08x\n", i2c_readl(i2c->regs + I2C_MRXCNT));
        printf("I2C_IEN: 0x%08x\n", i2c_readl(i2c->regs + I2C_IEN));
        printf("I2C_IPD: 0x%08x\n", i2c_readl(i2c->regs + I2C_IPD));
        printf("I2C_FCNT: 0x%08x\n", i2c_readl(i2c->regs + I2C_FCNT));
        for( i = 0; i < 8; i ++) 
                printf("I2C_TXDATA%d: 0x%08x\n", i, i2c_readl(i2c->regs + I2C_TXDATA_BASE + i * 4));
        for( i = 0; i < 8; i ++) 
                printf("I2C_RXDATA%d: 0x%08x\n", i, i2c_readl(i2c->regs + I2C_RXDATA_BASE + i * 4));
}

void rk_i2c_show_regs(void)
{
	rk30_show_regs(get_base());
}


/* returns TRUE if we reached the end of the current message */
static inline int is_msgend(uint cnt, uint r_len, uint b_len)
{
	return cnt >= 1 + r_len + b_len;
}

static inline void rk30_i2c_enable(struct rk30_i2c *i2c, unsigned int mode, unsigned int lastnak)
{
        unsigned int con = 0;

        con |= I2C_CON_EN;
        con |= I2C_CON_MOD(mode);
        if(lastnak)
                con |= I2C_CON_LASTACK;
        con |= I2C_CON_START;
	con &= ~(I2C_CON_STOP);
        i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_disable(struct rk30_i2c *i2c)
{
        i2c_writel(0, i2c->regs + I2C_CON);
}

static inline void rk_i2c_clean_start(struct rk30_i2c *i2c)
{
        unsigned int con = i2c_readl(i2c->regs + I2C_CON);

        con &= ~I2C_CON_START;
        i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_send_start(struct rk30_i2c *i2c)
{
        unsigned int con = i2c_readl(i2c->regs + I2C_CON);

        con |= I2C_CON_START;
        if(con & I2C_CON_STOP)
                printf("rk30_i2c_send_start, I2C_CON: stop bit is set.\n");
        
        i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_send_stop(struct rk30_i2c *i2c)
{
        unsigned int con = i2c_readl(i2c->regs + I2C_CON);

        con |= I2C_CON_STOP;
        if(con & I2C_CON_START)
                printf("rk30_i2c_send_stop, I2C_CON: start bit is set.\n");
        
        i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_clean_stop(struct rk30_i2c *i2c)
{
        unsigned int con = i2c_readl(i2c->regs + I2C_CON);

        con &= ~I2C_CON_STOP;
        i2c_writel(con, i2c->regs + I2C_CON);
}

static inline void rk_i2c_get_ipd_event(struct rk30_i2c *i2c, int type)
{
	unsigned int con = i2c_readl(i2c->regs + I2C_IPD);
	int time = 2000;

	while((--time) && (!(con & type))){
		udelay(10);
		con = i2c_readl(i2c->regs + I2C_IPD);
	}
	i2c_writel(type, i2c->regs + I2C_IPD);
	
	if(time <= 0){
		if (type == I2C_STARTIPD){
			printf("I2C_STARTIPD time out.\n");
		} else if (type == I2C_STOPIPD){
			printf("I2C_STOPIPD time out.\n");
		} else if (type == I2C_MBTFIPD){
			printf("I2C_MBTFIPD time out.\n");
		} else if (type == I2C_MBRFIPD){
			printf("I2C_MBRFIPD time out.\n");
		}
	}
}

/* SCL Divisor = 8 * (CLKDIVL + CLKDIVH)
 * SCL = i2c_rate/ SCLK Divisor
*/
static void rk_i2c_set_clk(struct rk30_i2c *i2c, unsigned long scl_rate)
{
        unsigned long i2c_rate;
        unsigned int div, divl, divh;

	if (i2c->speed == scl_rate)
		return ;

	if (gcurrent_bus < I2C_BUS_CH3)
		i2c_rate = 100*1000*1000;
	else
		i2c_rate = 375*100*1000;
        div = rk_ceil(i2c_rate, scl_rate * 8);
        divh = divl = rk_ceil(div, 2);
        i2c_writel(I2C_CLKDIV_VAL(divl, divh), i2c->regs + I2C_CLKDIV);

	i2c->speed = scl_rate;
       printf("i2c->regs_addr = %x,set clk(I2C_CLKDIV: 0x%08x)\n",i2c->regs,i2c_readl(i2c->regs + I2C_CLKDIV));
		
}


static void rk_i2c_write_prepare(struct rk30_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	unsigned int data = 0, cnt = 0, len = 0, i, j;
	unsigned char byte;

	i2c_writel(I2C_MBTFIEN, i2c->regs + I2C_IEN);

	for(i = 0; i < 8; i++) {
		data = 0;
		for(j = 0; j < 4; j++) {
			if (is_msgend(cnt, r_len, b_len))
				break;
			if (cnt == 0) {
				byte = (chip & 0x7f) << 1;
			} else if ((cnt == 1) && (r_len != 0)) {
				byte = (unsigned char)(reg & 0xff);
			} else if ((cnt == 2) && (r_len == 2)) {
				byte = (unsigned char)((reg >> 8) & 0xff);
			} else {
				byte =  buf[len++];
			}

			cnt++;
			data |= (byte << (j * 8));
		}
		//printf("rk_i2c_write_prepare: data = 0x%08x\n", data);
		i2c_writel(data, i2c->regs + I2C_TXDATA_BASE + 4 * i);
		if (is_msgend(cnt, r_len, b_len))
			break;
	}
	i2c_writel(cnt, i2c->regs + I2C_MTXCNT);
}

static void rk_i2c_read_prepare(struct rk30_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	unsigned int addr = (chip & 0x7f) << 1;
	unsigned int p = 0;

	i2c_writel(I2C_MBRFIEN, i2c->regs + I2C_IEN);
	i2c_writel(addr | I2C_MRXADDR_LOW, i2c->regs + I2C_MRXADDR);
	if (r_len == 1) {
		i2c_writel(reg | I2C_MRXADDR_LOW, i2c->regs + I2C_MRXRADDR);
	} else if (r_len == 2) {
		i2c_writel(reg | I2C_MRXRADDR_LOW | I2C_MRXRADDR_MID, i2c->regs + I2C_MRXRADDR);
	} else {
		printf("Not support register len.\n");
	}
	//printf("rk_i2c_read_prepare: reg = 0x%04x, I2C_MRXRADDR = 0x%08x\n", reg, i2c->regs + I2C_MRXRADDR);
	i2c_writel(b_len, i2c->regs + I2C_MRXCNT);
}


static void rk_i2c_get_data(struct rk30_i2c *i2c, uchar *buf, uint b_len)
{
	unsigned int p = 0, i = 0;

	for(i = 0; i < b_len; i++){
		if(i%4 == 0)
			p = i2c_readl(i2c->regs + I2C_RXDATA_BASE +  (i/4) * 4);
		buf[i] = (p >> ((i%4) * 8)) & 0xff;
	}
}

static int rk_i2c_read(struct rk30_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	if (b_len > 32) {
		return -1;
	}
	rk30_i2c_enable(i2c, 1, 1);
	rk_i2c_get_ipd_event(i2c, I2C_STARTIPD);
	rk_i2c_clean_start(i2c);

	rk_i2c_read_prepare(i2c, chip, reg, r_len, buf, b_len);
	rk_i2c_get_ipd_event(i2c, I2C_MBRFIPD);
	rk_i2c_get_data(i2c, buf, b_len);

	rk_i2c_send_stop(i2c);
	rk_i2c_get_ipd_event(i2c, I2C_STOPIPD);
	rk_i2c_clean_stop(i2c);
	rk_i2c_disable(i2c);

	return 0;
}

static int rk_i2c_write(struct rk30_i2c *i2c, uchar chip, uint reg, uint r_len, uchar *buf, uint b_len)
{
	if (r_len + b_len + 1 > 32) {
		return -1;
	}
	rk30_i2c_enable(i2c, 0, 1);
	rk_i2c_get_ipd_event(i2c, I2C_STARTIPD);
	rk_i2c_clean_start(i2c);

	rk_i2c_write_prepare(i2c, chip, reg, r_len, buf, b_len);
	rk_i2c_get_ipd_event(i2c, I2C_MBTFIPD);
	
	rk_i2c_send_stop(i2c);
	rk_i2c_get_ipd_event(i2c, I2C_STOPIPD);
	rk_i2c_clean_stop(i2c);
	rk_i2c_disable(i2c);

	return 0;
}


static void rk_i2c_init(int speed)
{
	struct rk30_i2c *i2c = (struct rk30_i2c *)get_base();
	if (i2c == NULL) {
		printf("rk_i2c_init error: i2c = NULL\n");
		return ;
	}

	//printf("rk_i2c_init: I2C bus = %d\n", gcurrent_bus);
#if 1
	if (gcurrent_bus == I2C_BUS_CH0) {
		i2c_adap_sel(I2C_BUS_CH0);
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
		g_grfReg->GRF_GPIO_IOMUX[2].GPIOD_IOMUX = (((0x1<<10)|(0x1<<8))<<16)|(0x1<<10)|(0x1<<8);
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188) || (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
		g_grfReg->GRF_GPIO_IOMUX[1].GPIOD_IOMUX = (((0x1<<2)|(0x1<<0))<<16)|(0x1<<2)|(0x1<<0);
#elif(CONFIG_RKCHIPTYPE == CONFIG_RK3026)
		g_grfReg->GRF_GPIO_IOMUX[0].GPIOA_IOMUX = (((0x1<<2)|(0x1<<0))<<16)|(0x1<<2)|(0x1<<0);
#endif
	} else if (gcurrent_bus == I2C_BUS_CH1) {
		i2c_adap_sel(I2C_BUS_CH1);
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
		g_grfReg->GRF_GPIO_IOMUX[2].GPIOD_IOMUX = (((0x1<<14)|(0x1<<12))<<16)|(0x1<<14)|(0x1<<12);
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)|| (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
		g_grfReg->GRF_GPIO_IOMUX[1].GPIOD_IOMUX = (((0x1<<6)|(0x1<<4))<<16)|(0x1<<6)|(0x1<<4);
#elif(CONFIG_RKCHIPTYPE == CONFIG_RK3026)
		g_grfReg->GRF_GPIO_IOMUX[0].GPIOA_IOMUX = (((0x1<<6)|(0x1<<4))<<16)|(0x1<<6)|(0x1<<4);
#endif
	} else if (gcurrent_bus == I2C_BUS_CH2) {
		i2c_adap_sel(I2C_BUS_CH2);
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
		g_grfReg->GRF_GPIO_IOMUX[3].GPIOA_IOMUX = (((0x1<<2)|(0x1<<0))<<16)|(0x1<<2)|(0x1<<0);
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)|| (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
		g_grfReg->GRF_GPIO_IOMUX[1].GPIOD_IOMUX = (((0x1<<10)|(0x1<<8))<<16)|(0x1<<10)|(0x1<<8);
#elif(CONFIG_RKCHIPTYPE == CONFIG_RK3026)
		g_grfReg->GRF_GPIO_IOMUX[2].GPIOC_IOMUX = (((0x3<<10)|(0x3<<8))<<16)|(0x3<<10)|(0x3<<8);
#endif
	} else if (gcurrent_bus == I2C_BUS_CH3) {
		i2c_adap_sel(I2C_BUS_CH3);
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
		g_grfReg->GRF_GPIO_IOMUX[3].GPIOA_IOMUX = (((0x1<<6)|(0x1<<4))<<16)|(0x1<<6)|(0x1<<4);
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)|| (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
		g_grfReg->GRF_GPIO_IOMUX[3].GPIOB_IOMUX = (((0x3<<14)|(0x3<<12))<<16)|(0x2<<14)|(0x2<<12);
#elif(CONFIG_RKCHIPTYPE == CONFIG_RK3026)
		g_grfReg->GRF_GPIO_IOMUX[0].GPIOA_IOMUX = (((0x3<<14)|(0x3<<12))<<16)|(0x1<<14)|(0x1<<12);
#endif
	}else if(gcurrent_bus == I2C_BUS_CH4){
		i2c_adap_sel(I2C_BUS_CH4);
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3188)|| (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
		g_grfReg->GRF_GPIO_IOMUX[1].GPIOD_IOMUX = (((0x1<<14)|(0x1<<12))<<16)|(0x1<<14)|(0x1<<12);
#endif
	} 
	else {
		printf("gcurrent_bus is error!\n");
	}
#endif
	rk_i2c_set_clk(i2c, speed);
}

#ifdef CONFIG_I2C_MULTI_BUS
unsigned int i2c_get_bus_num(void)
{
	return gcurrent_bus;
}

int i2c_set_bus_num(unsigned bus_idx)
{
	if (bus_idx >= I2C_BUS_MAX) {
		printf("i2c_set_bus_num: I2C bus error!");
		return -1;
	}

	printf("i2c_set_bus_num: I2C bus = %d\n", bus_idx);

	gcurrent_bus = bus_idx;

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
	struct rk30_i2c *i2c = (struct rk30_i2c *)get_base();

	if (i2c == NULL || buf == NULL) {
		printf("i2c_read error: i2c = 0x%08x, buf = 0x%08x\n", i2c, buf);
		return -1;
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
	struct rk30_i2c *i2c = (struct rk30_i2c *)get_base();

	if (i2c == NULL || buf == NULL) {
		printf("i2c_read error: i2c = 0x%08x, buf = 0x%08x\n", i2c, buf);
		return -1;
	}

	return rk_i2c_write(i2c, chip, addr, alen, buf, len);
}

/*
 * Test if a chip at a given address responds (probe the chip)
 */
int i2c_probe(uchar chip)
{
	struct rk30_i2c *i2c = (struct rk30_i2c *)get_base();

	if (i2c == NULL) {
		printf("i2c_read error: i2c = 0x%08x\n", i2c);
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
	struct rk30_i2c *i2c = (struct rk30_i2c *)get_base();

	if (i2c == NULL) {
		printf("i2c_read error: i2c = 0x%08x\n", i2c);
		return -1;
	}

	if (gcurrent_bus >= I2C_BUS_MAX) {
		printf("I2C bus error, PLS first set bus!");
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

