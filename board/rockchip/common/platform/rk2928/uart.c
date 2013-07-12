
/*********************************************************************
 INCLUDE FILES	
*********************************************************************/
#include "config.h"
#include "uart.h"

#ifdef DRIVERS_UART
 
pUART_REG pUartReg = (pUART_REG)UART0_BASE_ADDR;
#define ReadReg32(addr)                     (*(volatile uint32 *)(addr))
#define WriteReg32(addr, data)              (*(volatile uint32 *)(addr) = data)

/*********************************************************************
 *
 *	FUNCTION:		UartInit
 *
 *	DESCRIPTION:	Initializes the serial port.
 *
 *	PARAMETERS: 	None		 
 *
 *	RETURNS:		None
 *
 ********************************************************************/ 
void UartInit(void)
{
    uint32  uartTemp;
    //uint32 lcr;
    volatile uint32 *pRegAddr;
    uint32 chipTag = RKGetChipTag();

    //clk
    //pRegAddr = (volatile uint32*)0x1801801C;
    //*pRegAddr &= ~(0x03<<18);	//open uart 0 and 1 clk
    //关闭usb串口打印
    uart2UsbEn(0);
    g_grfReg->GRF_GPIO_IOMUX[0].GPIOC_IOMUX = (((0x1<<2)|(0x1))<<16)|(0x1<<2)|(0x1);   // sin,sout
    if(chipTag == RK2928G_CHIP_TAG || chipTag == RK2926_CHIP_TAG)
    {
        //g_grfReg->GRF_UOC1_CON[4]  = 0x34003400;   // uart 2 output 2 usb
        //if(!(g_grfReg->GRF_SOC_STATUS0) & (1<<7))) // 如果接usb的话，不能开串口，ddr有开串口，这里不配置
        //    *(uint32*)0x20008190 = 0x34003400; 
        g_grfReg->GRF_GPIO_IOMUX[2].GPIOC_IOMUX = (((0xFul) <<16)|(0xF<<0));   // sin,sout
        pUartReg = (pUART_REG)UART2_BASE_ADDR;
    }
    //Reset
    pUartReg->UART_SRR = UART_RESET | RCVR_FIFO_REST | XMIT_FIFO_RESET;
    pUartReg->UART_IER = 0;
    
    //uart mode
    pUartReg->UART_MCR = IRDA_SIR_DISABLED;

    //BaudRate
    pUartReg->UART_LCR = LCR_DLA_EN | PARITY_DISABLED | ONE_STOP_BIT | LCR_WLS_8;
    uartTemp = (1000 * 24000) / MODE_X_DIV / 115200;

    pUartReg->UART_DLL = uartTemp & 0xff;
    pUartReg->UART_DLH = (uartTemp>>8) & 0xff;
    pUartReg->UART_LCR = PARITY_DISABLED | ONE_STOP_BIT | LCR_WLS_8;

    pUartReg->UART_SFE = SHADOW_FIFI_ENABLED;
    pUartReg->UART_SRT = RCVR_TRIGGER_TWO_LESS_FIFO;
    pUartReg->UART_STET = TX_TRIGGER_TWO_IN_FIFO;
    
}

/*********************************************************************
 *
 *	FUNCTION:		UartWriteByte
 *
 *	DESCRIPTION:	Writes a single byte to the serial port.
 *
 *	PARAMETERS: 	
 *					ch	-	byte of data to send		 
 *
 *	RETURNS:		None
 *
 ********************************************************************/
void UartWriteChar(char c)
{
    uint32 uartTimeOut;
    pUART_REG puartRegStart;  

    if(!pUartReg) return;
    
    puartRegStart = (pUART_REG)pUartReg;
    
    uartTimeOut = 0xFFFF;
    while((puartRegStart->UART_USR & UART_TRANSMIT_FIFO_NOT_FULL) != UART_TRANSMIT_FIFO_NOT_FULL)
    {
        if(uartTimeOut == 0)
        {
            return ;
        }
        uartTimeOut--;
    }
    puartRegStart->UART_THR = c;
}

#endif
