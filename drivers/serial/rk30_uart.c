/*
 * (C) Copyright 2013
 * peter, Software Engineering, <superpeter.cai@gmail.com>.
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
#include  <asm/arch/rk30_drivers.h>

#ifdef DRIVERS_UART

static uint32 uartWorkStatus0 = 0;
static uint32 uartWorkStatus1 = 0;

/*------------------------------------------------------------------
 * UART the serial port
 *-----------------------------------------------------------------*/
int32 UARTSetIOP(pUART_REG phead, uint8 useIrDA)
{
	pUART_REG   phwHead = (pUART_REG)phead;
	if((useIrDA == IRDA_SIR_DISABLED) || (useIrDA == IRDA_SIR_ENSABLED)) {
		phwHead->UART_MCR = useIrDA;
		return (0);
	}

	return (-1); 
}


int32 UARTSetBaudRate(pUART_REG phead, uint32 baudRate)
{
	pUART_REG    phwHead = (pUART_REG)phead;
	uint32  uartTemp;
	if(baudRate > 115200)
		return (-1);        

	phwHead->UART_LCR = phwHead->UART_LCR | LCR_DLA_EN;
	uartTemp = (1000 * 24000) / MODE_X_DIV / baudRate;
	phwHead->UART_DLL = uartTemp & 0xff;
	phwHead->UART_DLH = (uartTemp>>8) & 0xff;
	phwHead->UART_LCR = phwHead->UART_LCR & (~LCR_DLA_EN);

	return (0);
}


int32 UARTSetLcrReg(pUART_REG phead, uint8 byteSize, uint8 parity, uint8 stopBits )
{
	pUART_REG    phwHead = (pUART_REG)phead;
	uint32 lcr;
	int32 bRet = 0;

	lcr = (uint32)(phwHead->UART_LCR);
	lcr &= ~UART_DATABIT_MASK;
	switch ( byteSize )    ///byte set
	{
		case UART_BIT5:
			lcr |= LCR_WLS_5;//SERIAL_5_DATA;
			break;
		case UART_BIT6:
			lcr |= LCR_WLS_6;//SERIAL_6_DATA;
			break;
		case UART_BIT7:
			lcr |= LCR_WLS_7;//SERIAL_7_DATA;
			break;
		case UART_BIT8:
			lcr |= LCR_WLS_8;//SERIAL_8_DATA;
			break;
		default:
			bRet = -1;
			break;
	}

	switch ( parity )  ///Parity set
	{
		case 0:
			lcr |= PARITY_DISABLED;
			break;
		case 1:
			lcr |= PARITY_ENABLED;
			break;
		default:
			bRet = -1;
			break;
	}

	switch ( stopBits )  ///StopBits set
	{
		case 0:
			lcr |= ONE_STOP_BIT;
			break;
		case 1:
			lcr |= ONE_HALF_OR_TWO_BIT;
			break;
		default:
			bRet = -1;
			break;
	}

	if (bRet == 0)
		phwHead->UART_LCR = lcr;

	return(bRet);
}


void UARTSetFifoEnabledNumb(pUART_REG phead)
{
	pUART_REG    phwHead = (pUART_REG)phead;

	phwHead->UART_SFE = SHADOW_FIFI_ENABLED;
	phwHead->UART_SRT = RCVR_TRIGGER_TWO_LESS_FIFO;
	phwHead->UART_STET = TX_TRIGGER_TWO_IN_FIFO;
}


void UARTRest(pUART_REG phead)
{
	pUART_REG    phwHead = (pUART_REG)phead;

	phwHead->UART_SRR = UART_RESET | RCVR_FIFO_REST | XMIT_FIFO_RESET;
	phwHead->UART_IER = 0;
}


int32 UARTInit(eUART_ch_t uartCh, uint32 baudRate)
{
	volatile unsigned int * pGRF_GPIO2L_IOMUX = (volatile unsigned int *)(RK30_GRF_PHYS+0x58);
	pUART_REG puartreg = NULL;
	int32 val = -1;
    
	if(uartCh == UART_CH0) { 
		puartreg = (pUART_REG)UART0_BASE_ADDR;
		if(uartWorkStatus0 == 1)
			return (-1);

		uartWorkStatus0 = 1;
	} else {
		// iomux to uart 1
		*pGRF_GPIO2L_IOMUX |= (1<<8)|(1<<10);
		puartreg = (pUART_REG)UART1_BASE_ADDR;
		if(uartWorkStatus1 == 1)
			return (-1);

		uartWorkStatus1 = 1;
	}

	UARTRest(puartreg);  
	val = UARTSetIOP(puartreg, IRDA_SIR_DISABLED);
	if(val == -1)
		return (-1);

	val = UARTSetLcrReg(puartreg, UART_BIT8, PARITY_DISABLED, ONE_STOP_BIT);
	if(val == -1)
		return (-1);

	val = UARTSetBaudRate(puartreg, baudRate);
	if(val == -1)
		return (-1);

	UARTSetFifoEnabledNumb(puartreg);
	return (0);
}


int32 UARTWriteByte(eUART_ch_t uartCh, uint8 byte)
{
	pUART_REG puartRegStart;  
    
	if(uartCh == UART_CH0) {
		puartRegStart = (pUART_REG)UART0_BASE_ADDR;
	} else {
		puartRegStart = (pUART_REG)UART1_BASE_ADDR;
	}

	while((puartRegStart->UART_USR & UART_TRANSMIT_FIFO_NOT_FULL) == 0);

	puartRegStart->UART_THR = byte;
	return (0);
}


uint8 UARTReadByte(eUART_ch_t uartCh)
{
	pUART_REG puartRegStart;
	uint8 pdata;

	if(uartCh == UART_CH0) {
		puartRegStart = (pUART_REG)UART0_BASE_ADDR;
	} else {
		puartRegStart = (pUART_REG)UART1_BASE_ADDR;
	}

	while((puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == 0);
 
	pdata = (uint8 )puartRegStart->UART_RBR;
    	return (pdata);
}


static int rk30_serial_init(void)
{
	return(UARTInit(CONFIG_UART_NUM, CONFIG_BAUDRATE));
}

static void rk30_serial_setbrg(void)
{
	pUART_REG puartRegStart = NULL;

	if(CONFIG_UART_NUM == UART_CH0) {
		puartRegStart = (pUART_REG)UART0_BASE_ADDR;
	} else {
		puartRegStart = (pUART_REG)UART1_BASE_ADDR;
	}

	UARTSetBaudRate(puartRegStart, CONFIG_BAUDRATE);
}


/*-----------------------------------------------------------------------
 * UART CONSOLE
 *---------------------------------------------------------------------*/
static void rk30_serial_putc(char c)
{
	if (c == '\n')
		serial_putc('\r');
	UARTWriteByte(CONFIG_UART_NUM, c);
}


void rk30_serial_puts(const char *s)
{
	while (*s) {
		serial_putc(*s++);
	}
}


static int rk30_serial_getc(void)
{
	char a = 0;

	a = UARTReadByte(CONFIG_UART_NUM);
	if(0 == a)
		a='\0';

	return(a);
}


static int rk30_serial_tstc(void)
{
	if( CONFIG_UART_NUM == UART_CH0) {
		pUART_REG puartRegStart=(pUART_REG)UART0_BASE_ADDR;
        	return((puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == UART_RECEIVE_FIFO_NOT_EMPTY);
	} else {
		pUART_REG puartRegStart=(pUART_REG)UART1_BASE_ADDR;
        	return((puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == UART_RECEIVE_FIFO_NOT_EMPTY);
	}
}


static struct serial_device rk30_serial_drv = {
	.name	= "rk30_serial",
	.start	= rk30_serial_init,
	.stop	= NULL,
	.setbrg	= rk30_serial_setbrg,
	.putc	= rk30_serial_putc,
	.puts	= default_serial_puts,
	.getc	= rk30_serial_getc,
	.tstc	= rk30_serial_tstc,
};


void rk30_serial_initialize(void)
{
	serial_register(&rk30_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
	return &rk30_serial_drv;
}

#endif /* DRIVERS_UART */

