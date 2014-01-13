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
#include <serial.h>
#include  <asm/arch/rk30_drivers.h>

#ifdef DRIVERS_UART

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


inline pUART_REG UARTGetRegBase(eUART_ch_t uartCh)
{    
	if(uartCh == UART_CH0) {
		return (pUART_REG)UART0_BASE_ADDR;
	} else if (uartCh == UART_CH1) {
		return (pUART_REG)UART1_BASE_ADDR;
	} else if (uartCh == UART_CH2) {
		return (pUART_REG)UART2_BASE_ADDR;
	} else {
		return NULL;
	}
}

 
int32 UARTInit(eUART_ch_t uartCh, uint32 baudRate)
{
	pUART_REG pUartReg = NULL;
	int32 val = -1;
    
	if(uartCh == UART_CH0) { 
		// iomux to uart 0
		g_grfReg->GRF_GPIO_IOMUX[1].GPIOA_IOMUX = (((0x1<<2)|(0x1))<<16)|(0x1<<2)|(0x1);   // sin,sout
		pUartReg = (pUART_REG)UART0_BASE_ADDR;
	} else if (uartCh == UART_CH1) {
		// iomux to uart 1
		g_grfReg->GRF_GPIO_IOMUX[1].GPIOA_IOMUX = (((0x1<<10)|(0x1<<8))<<16)|(0x1<<10)|(0x1<<8);   // sin,sout
		pUartReg = (pUART_REG)UART1_BASE_ADDR;
	} else if (uartCh == UART_CH2) {
		// iomux to uart 2
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
		g_grfReg->GRF_GPIO_IOMUX[1].GPIOB_IOMUX = (((0x1<<2)|(0x1))<<16)|(0x1<<2)|(0x1);   // sin,sout
#elif  (CONFIG_RKCHIPTYPE == CONFIG_RK3168)		
		g_grfReg->GRF_GPIO_IOMUX[1].GPIOB_IOMUX = (((0x3<<2)|(0x3))<<16)|(0x1<<2)|(0x1);   // sin,sout
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)
			g_grfReg->GRF_GPIO_IOMUX[1].GPIOB_IOMUX = (((0x3<<2)|(0x3))<<16)|(0x1<<2)|(0x1);   // sin,sout
			g_grfReg->GRF_UOC0_CON[0] = (0x0000 | (0x0300 << 16));
			g_grfReg->GRF_UOC0_CON[2] = (0x0000 | (0x0004 << 16));
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3026)	
			g_grfReg->GRF_GPIO_IOMUX[2].GPIOC_IOMUX = ((0xf<<12)<<16)|(0xf<<12);;   // sin,sout
			#if 1
			if(g_grfReg->GRF_SOC_STATUS0&(1<<10)){
				if(!(g_grfReg->GRF_SOC_STATUS0&(1<<7))){
					g_grfReg->GRF_UOC0_CON0 = 0x007f0055;
					g_grfReg->GRF_UOC1_CON0 = 0x34003000;
				}
				else
					g_grfReg->GRF_UOC1_CON0 = 0x34000000;
			}
			#endif

#endif
		pUartReg = (pUART_REG)UART2_BASE_ADDR;
	}

	UARTRest(pUartReg);

	val = UARTSetIOP(pUartReg, IRDA_SIR_DISABLED);
	if(val == -1)
		return (-1);

	val = UARTSetLcrReg(pUartReg, UART_BIT8, PARITY_DISABLED, ONE_STOP_BIT);
	if(val == -1)
		return (-1);

	val = UARTSetBaudRate(pUartReg, baudRate);
	if(val == -1)
		return (-1);

	UARTSetFifoEnabledNumb(pUartReg);
	return (0);
}


int32 UARTWriteByte(eUART_ch_t uartCh, uint8 byte)
{

#ifdef CONFIG_FASTBOOT_LOG
    fbt_log(&byte, 1, false);
#endif
	pUART_REG puartRegStart = UARTGetRegBase(uartCh);  
    
	while((puartRegStart->UART_USR & UART_TRANSMIT_FIFO_NOT_FULL) == 0);

	puartRegStart->UART_THR = byte;
	return (0);
}

#define  UART_LSR_TEMT                0x40 /* Transmitter empty */

uint32 uart_init()
{
    pUART_REG puartRegStart = UART2_BASE_ADDR; 
     //BaudRate
    puartRegStart->UART_LCR = 0x83;
    puartRegStart->UART_RBR = 0xD;   //²¨ÌØÂÊ:115200
    puartRegStart->UART_LCR = 0x03;
}
uint32 uart_wrtie_byte(uint8 byte)
{
    pUART_REG puartRegStart = UART2_BASE_ADDR; 
    puartRegStart->UART_RBR = byte;
    while(!(puartRegStart->UART_LSR & UART_LSR_TEMT));
    return (0);
}

void print(char *s)
{
	while (*s) 
	{
		if (*s == '\n')
		{
		    uart_wrtie_byte('\r');
		}
	    uart_wrtie_byte(*s);
	    s++;
	}
}
void _print_hex (uint32 hex)
{
    int i = 8;
	uart_wrtie_byte('0');
	uart_wrtie_byte('x');
	while (i--) {
		unsigned char c = (hex & 0xF0000000) >> 28;
		uart_wrtie_byte(c < 0xa ? c + '0' : c - 0xa + 'a');
		hex <<= 4;
	}
}
uint8 UARTReadByte(eUART_ch_t uartCh)
{
	pUART_REG puartRegStart = UARTGetRegBase(uartCh); 
	uint8 pdata;

	while((puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == 0);
 
	pdata = (uint8 )puartRegStart->UART_RBR;
    	return (pdata);
}


 int rk30_serial_init(void)
{
	return (UARTInit(CONFIG_UART_NUM, CONFIG_BAUDRATE));
}

static void rk30_serial_setbrg(void)
{
	pUART_REG puartRegStart = UARTGetRegBase(CONFIG_UART_NUM); 

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
        pUART_REG puartRegStart = UARTGetRegBase(CONFIG_UART_NUM);

        return((puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == UART_RECEIVE_FIFO_NOT_EMPTY);
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

