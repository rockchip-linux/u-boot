/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <serial.h>
#include <asm/arch/rkplat.h>


#define RKUART_VERSION		"1.3"

/* io base config */
static volatile void __iomem *g_rkuart_base[UART_CH_MAX] = {
	(void __iomem *)RKIO_UART0_BASE,
	(void __iomem *)RKIO_UART1_BASE,
	(void __iomem *)RKIO_UART2_BASE,
	(void __iomem *)RKIO_UART3_BASE,
	(void __iomem *)RKIO_UART4_BASE,
	(void __iomem *)RKIO_UART5_BASE,
	(void __iomem *)RKIO_UART6_BASE,
	(void __iomem *)RKIO_UART7_BASE
};


/*------------------------------------------------------------------
 * UART the serial port
 *-----------------------------------------------------------------*/
static volatile void __iomem *rk_uart_get_base(eUART_ch_t ch)
{
	if (ch >= UART_CH_MAX)
		return NULL;

	return g_rkuart_base[ch];
}

static inline void rk_uart_iomux(eUART_ch_t ch)
{
	rk_iomux_config(RK_UART0_IOMUX + ch);
}


static inline int rk_uart_set_iop(volatile void *base, uint32 irda)
{
	if ((irda == IRDA_SIR_DISABLED) || (irda == IRDA_SIR_ENSABLED)) {
		writel(irda, base + UART_MCR);
		return 0;
	}

	return -1;
}


static inline int rk_uart_set_lcr(volatile void *base, uint8 bytesize, uint8 parity, uint8 stopbits)
{
	volatile uint32 lcr;
	int ret = 0;

	lcr = readl(base + UART_LCR);
	lcr &= ~UART_DATABIT_MASK;
	switch (bytesize) { /* byte set */
	case UART_BIT5:
		lcr |= LCR_WLS_5;
		break;
	case UART_BIT6:
		lcr |= LCR_WLS_6;
		break;
	case UART_BIT7:
		lcr |= LCR_WLS_7;
		break;
	case UART_BIT8:
		lcr |= LCR_WLS_8;
		break;
	default:
		ret = -1;
		break;
	}

	switch (parity) { /* Parity set */
	case 0:
		lcr |= PARITY_DISABLED;
		break;
	case 1:
		lcr |= PARITY_ENABLED;
		break;
	default:
		ret = -1;
		break;
	}

	switch (stopbits) { /* stopbits set */
	case 0:
		lcr |= ONE_STOP_BIT;
		break;
	case 1:
		lcr |= ONE_HALF_OR_TWO_BIT;
		break;
	default:
		ret = -1;
		break;
	}

	if (ret == 0)
		writel(lcr, base + UART_LCR);

	return ret;
}


static inline void rk_uart_set_fifo(volatile void *base)
{
	/* shadow FIFO enable */
	writel(SHADOW_FIFI_ENABLED, base + UART_SFE);
	/* fifo 2 less than */
	writel(RCVR_TRIGGER_TWO_LESS_FIFO, base + UART_SRT);
	/* 2 char in tx fifo */
	writel(TX_TRIGGER_TWO_IN_FIFO, base + UART_STET);
}


static inline void rk_uart_reset(volatile void *base)
{
	/* UART reset, rx fifo & tx fifo reset */
	writel(UART_RESET | RCVR_FIFO_REST | XMIT_FIFO_RESET, base + UART_SRR);
	/* interrupt disable */
	writel(0x00, base + UART_IER);
}


static inline int rk_uart_set_baudrate(volatile void *base, uint32 baudrate)
{
	volatile uint32 rate;
	volatile uint32 lcr;
	const unsigned long baudrate_table[] = CONFIG_SYS_BAUDRATE_TABLE;
	int i;

	for (i = 0; i < ARRAY_SIZE(baudrate_table); ++i)
		if (baudrate == baudrate_table[i])
			break;
	if (i == ARRAY_SIZE(baudrate_table))
		return -1;

	/* uart rate is div for 24M input clock */
	rate = UART_CLOCK_FREQ / UART_MODE_X_DIV / baudrate;

	lcr = readl(base + UART_LCR);
	writel(lcr | LCR_DLA_EN, base + UART_LCR);

	writel(rate & 0xFF, base + UART_DLL);
	writel((rate >> 8) & 0xff, base + UART_DLH);

	lcr = readl(base + UART_LCR);
	writel(lcr & (~LCR_DLA_EN), base + UART_LCR);

	return 0;
}


static int rk_uart_init(eUART_ch_t ch, uint32 baudrate)
{
	volatile void *base = rk_uart_get_base(ch);
	int val = -1;

	if (base == NULL)
		return -1;

	rk_uart_iomux(ch);
	rk_uart_reset(base);

	val = rk_uart_set_iop(base, IRDA_SIR_DISABLED);
	if (val == -1)
		return -1;

	val = rk_uart_set_lcr(base, UART_BIT8, PARITY_DISABLED, ONE_STOP_BIT);
	if (val == -1)
		return -1;

	val = rk_uart_set_baudrate(base, baudrate);
	if (val == -1)
		return -1;

	rk_uart_set_fifo(base);

	return 0;
}


static int rk_uart_sendbyte(eUART_ch_t ch, uint8 byte)
{
	volatile void *base = rk_uart_get_base(ch);

	if (base == NULL)
		return -1;

	do {} while ((readl(base + UART_USR) & UART_TRANSMIT_FIFO_NOT_FULL) == 0);

	writel(byte, base + UART_THR);

	return 0;
}


static uint8 rk_uart_recvbyte(eUART_ch_t ch)
{
	volatile void *base = rk_uart_get_base(ch);
	volatile uint8 data = 0;

	if (base == NULL)
		return 0;

	do {} while ((readl(base + UART_USR) & UART_RECEIVE_FIFO_NOT_EMPTY) == 0);

	data = (uint8)readl(base + UART_RBR);

	return data;
}


static int rk_uart_set_brg(eUART_ch_t ch, uint32 baudrate)
{
	volatile void *base = rk_uart_get_base(ch);

	if (base == NULL)
		return -1;

	return rk_uart_set_baudrate(base, baudrate);
}


static int rk_uart_tstc(eUART_ch_t ch)
{
	volatile void *base = rk_uart_get_base(ch);

	if (base == NULL)
		return -1;

	return (readl(base + UART_USR) & UART_RECEIVE_FIFO_NOT_EMPTY) == UART_RECEIVE_FIFO_NOT_EMPTY;
}


/*-----------------------------------------------------------------------
 * UART CONSOLE
 *---------------------------------------------------------------------*/
static void rk_serial_putc(char c)
{
	if (c == '\n')
		serial_putc('\r');

	rk_uart_sendbyte(CONFIG_UART_NUM, c);
}

static int rk_serial_tstc(void)
{
	return (int)rk_uart_tstc(CONFIG_UART_NUM);
}


static int rk_serial_getc(void)
{
	char a = 0;

	a = rk_uart_recvbyte(CONFIG_UART_NUM);
	if (0 == a)
		a = '\0';

	return a;
}


static void rk_serial_setbrg(void)
{
	rk_uart_set_brg(CONFIG_UART_NUM, CONFIG_BAUDRATE);
}


static int rk_serial_init(void)
{
	int ret = -1;

	ret = rk_uart_init(CONFIG_UART_NUM, CONFIG_BAUDRATE);
#ifdef DEBUG
	if (ret == 0) {
		rk_serial_putc('\n');
		rk_serial_putc('r');
		rk_serial_putc('k');
		rk_serial_putc(' ');
		rk_serial_putc('s');
		rk_serial_putc('e');
		rk_serial_putc('r');
		rk_serial_putc('i');
		rk_serial_putc('a');
		rk_serial_putc('l');
		rk_serial_putc('\n');
	}
#endif

	return ret;
}


static struct serial_device rk_serial_drv = {
	.name	= "rk_serial",
	.start	= rk_serial_init,
	.stop	= NULL,
	.setbrg	= rk_serial_setbrg,
	.getc	= rk_serial_getc,
	.tstc	= rk_serial_tstc,
	.putc	= rk_serial_putc,
	.puts	= default_serial_puts,
};


void rk_serial_initialize(void)
{
	serial_register(&rk_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
	return &rk_serial_drv;
}
