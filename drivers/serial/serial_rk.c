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
#include <serial.h>
#include <asm/arch/rkplat.h>


#define RKUART_VERSION		"1.1"


static volatile void __iomem * g_rkuart_base[UART_CH_MAX] = {
#if defined(CONFIG_RKCHIP_RK3288)
	(void __iomem *)RKIO_UART0_BT_PHYS,
	(void __iomem *)RKIO_UART1_BB_PHYS,
	(void __iomem *)RKIO_UART2_DBG_PHYS,
	(void __iomem *)RKIO_UART3_GPS_PHYS,
	(void __iomem *)RKIO_UART4_EXP_PHYS
#elif defined(CONFIG_RKCHIP_RK3036) || defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	(void __iomem *)RKIO_UART0_PHYS,
	(void __iomem *)RKIO_UART1_PHYS,
	(void __iomem *)RKIO_UART2_PHYS
#else
	#error "PLS config chip type for uart base!"
#endif
};


/*------------------------------------------------------------------
 * UART the serial port
 *-----------------------------------------------------------------*/
static volatile void __iomem* rk_uart_get_base(eUART_ch_t ch)
{
	if (ch >= UART_CH_MAX) {
		return NULL;
	}

	return g_rkuart_base[ch];
}

static void rk_uart_iomux(eUART_ch_t ch)
{
	rk_iomux_config(RK_UART0_IOMUX + ch);
}


static int rk_uart_set_iop(pUART_REG pbase, uint32 irda)
{
	if ((irda == IRDA_SIR_DISABLED) || (irda == IRDA_SIR_ENSABLED)) {
		pbase->UART_MCR = irda;
		return 0;
	}

	return -1;
}


static int rk_uart_set_lcr(pUART_REG pbase, uint8 bytesize, uint8 parity, uint8 stopbits)
{
	uint32 lcr;
	int ret = 0;

	lcr = pbase->UART_LCR;
	lcr &= ~UART_DATABIT_MASK;
	switch ( bytesize )    // byte set
	{
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

	switch ( parity )  // Parity set
	{
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

	switch ( stopbits )  // stopbits set
	{
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

	if (ret == 0) {
		pbase->UART_LCR = lcr;
	}

	return ret;
}


static void rk_uart_set_fifo(pUART_REG pbase)
{
	pbase->UART_SFE = SHADOW_FIFI_ENABLED;
	pbase->UART_SRT = RCVR_TRIGGER_TWO_LESS_FIFO;
	pbase->UART_STET = TX_TRIGGER_TWO_IN_FIFO;
}


static void rk_uart_reset(pUART_REG pbase)
{
	pbase->UART_SRR = UART_RESET | RCVR_FIFO_REST | XMIT_FIFO_RESET;
	pbase->UART_IER = 0;
}


/* Notice: if rk_uart_set_baudrate set to static function, system will be error. */
static int rk_uart_set_baudrate(pUART_REG pbase, uint32 baudrate)
{
	volatile uint32 rate;

	if ((baudrate < 9600) || (baudrate > 115200)) {
		return -1;
	}

	/* uart rate is div for 24M input clock */
	rate = RK_UART_CLOCK_FREQ / MODE_X_DIV / baudrate;

	pbase->UART_LCR = (pbase->UART_LCR | LCR_DLA_EN);
	pbase->UART_DLL = rate & 0xff;
	pbase->UART_DLH = (rate >> 8) & 0xff;
	pbase->UART_LCR = (pbase->UART_LCR & (~LCR_DLA_EN));

	return 0;
}


static int rk_uart_init(eUART_ch_t ch, uint32 baudrate)
{
	pUART_REG pbase = (pUART_REG)rk_uart_get_base(ch);
	int val = -1;
    
	if (pbase == NULL) {
		return (-1);
	}

	rk_uart_iomux(ch);
	rk_uart_reset(pbase);

	val = rk_uart_set_iop(pbase, IRDA_SIR_DISABLED);
	if (val == -1) {
		return (-1);
	}

	val = rk_uart_set_lcr(pbase, UART_BIT8, PARITY_DISABLED, ONE_STOP_BIT);
	if(val == -1) {
		return (-1);
	}

	val = rk_uart_set_baudrate(pbase, baudrate);
	if(val == -1) {
		return (-1);
	}

	rk_uart_set_fifo(pbase);

	return (0);
}


static int rk_uart_sendbyte(eUART_ch_t ch, uint8 byte)
{
	pUART_REG pbase = (pUART_REG)rk_uart_get_base(ch);

	if (pbase == NULL) {
		return (-1);
	}

	while((pbase->UART_USR & UART_TRANSMIT_FIFO_NOT_FULL) == 0);

	pbase->UART_THR = byte;

	return (0);
}


static uint8 rk_uart_recvbyte(eUART_ch_t ch)
{
	pUART_REG pbase = (pUART_REG)rk_uart_get_base(ch);
	volatile uint8 data = 0;

	if (pbase == NULL) {
		return 0;
	}

	while((pbase->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == 0);
 
	data = (uint8)pbase->UART_RBR;

	return data;
}


static int rk_uart_set_brg(eUART_ch_t ch, uint32 baudrate)
{
	pUART_REG pbase = (pUART_REG)rk_uart_get_base(ch);

	if (pbase == NULL) {
		return -1;
	}

	return rk_uart_set_baudrate(pbase, baudrate);
}


static int rk_uart_tstc(eUART_ch_t ch)
{
	pUART_REG pbase = (pUART_REG)rk_uart_get_base(ch);

	if (pbase == NULL) {
		return -1;
	}

	return ((pbase->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == UART_RECEIVE_FIFO_NOT_EMPTY);
}


/*-----------------------------------------------------------------------
 * UART CONSOLE
 *---------------------------------------------------------------------*/
static void rk_serial_putc(char c)
{
	if (c == '\n') {
		serial_putc('\r');
	}

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
	if (0 == a) {
		a = '\0';
	}

	return a;
}


static void rk_serial_setbrg(void)
{
	rk_uart_set_brg(CONFIG_UART_NUM, CONFIG_BAUDRATE);
}


static int rk_serial_init(void)
{
	return (int)rk_uart_init(CONFIG_UART_NUM, CONFIG_BAUDRATE);
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

