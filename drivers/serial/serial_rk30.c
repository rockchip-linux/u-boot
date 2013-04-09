/*******************************************************************/
/*    Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.             */
/*******************************************************************
File        :      uart.c
Desc        :     UART 的驱动相关程序
Author      :     lhh
Date        :      2008-11-11
Notes       :
$Log        :      uart.c,v $

Revision 0.00  2008/10/30 09:46:27    rockchipuser

moved from drivers/serial/rk30_uart.c

 ********************************************************************/
#define  IN_DRIVER_UART
#include <common.h>
#include <serial.h>
#include <asm/arch/rk30_drivers.h>
#include "serial_rk30.h"

/*----------------------------------------------------------------------
Name      : UARTSetIOP(void      *pHead)
Desc      : 设置UART口的作用为串口或为IRDA口
Params    : 
pHead：@parm points to device head
UseIrDA:IRDA和串口的设置
Return    :成功还回0，失败还回-1。
----------------------------------------------------------------------*/
int32 UARTSetIOP(pUART_REG phead, uint8 useIrDA)
{
    pUART_REG   phwHead = (pUART_REG)phead;
    if((useIrDA == IRDA_SIR_DISABLED) || (useIrDA == IRDA_SIR_ENSABLED))
    {
        phwHead->UART_MCR = useIrDA;
        return (0);
    }
    return (-1); 
}

/*----------------------------------------------------------------------
Name      : UARTSetBaudRate(pUART_REG phead, uint32 baudRate)
Desc      : 串口初始化
Params    : 
phead：@parm points to device head
baudRate:串口初始化的速度，用查表得到
Return    :成功还回0，失败还回-1。
note      :IOMUX控制先不做或在APB里设置
----------------------------------------------------------------------*/
int32 UARTSetBaudRate(pUART_REG phead, uint32 baudRate)
{
    uint32  uartTemp;
    pUART_REG    phwHead = (pUART_REG)phead;

    if(baudRate>115200)
    {
        return (-1);        
    }
    phwHead->UART_LCR = phwHead->UART_LCR | LCR_DLA_EN;////| PARITY_DISABLED | ONE_STOP_BIT | LCR_WLS_8;  ///enable DL ,set 8 bit character
    //#if (BOARDTYPE == RK2800_FPGA)
    //uartTemp = (2000000 *SCUGetAPBFreq()) / MODE_X_DIV / baudRate;
    // #else
    uartTemp = (1000 * 24000) / MODE_X_DIV / baudRate;
    //#endif
    phwHead->UART_DLL = uartTemp & 0xff;
    phwHead->UART_DLH = (uartTemp>>8) & 0xff;
    phwHead->UART_LCR = phwHead->UART_LCR & (~LCR_DLA_EN);  ///PARITY_DISABLED | ONE_STOP_BIT | LCR_WLS_8;

    return (0);
}

/*----------------------------------------------------------------------
Name      : UARTSetLcrReg(pUART_REG phead, uint8 byteSize, uint8 parity, uint8 stopBits )
Desc      : This routine sets the WordSize of the device.
Params    :
phead：void * returned by HWInit
byteSize:ByteSize field
parity:parity field
stopBits:stop bit number
Return    :  成功还回0，失败还回-1。
----------------------------------------------------------------------*/
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
    {
        phwHead->UART_LCR = lcr;
    }
    return(bRet);
}

/*----------------------------------------------------------------------
Name      : UARTSetFifoEnabledNumb()
Desc      : 设置UART口的FIFO多少时发生中断
Params    : 
phead：@parm points to device head
Return    :成功还回0，失败还回-1。
----------------------------------------------------------------------*/
void UARTSetFifoEnabledNumb(pUART_REG phead)
{
    pUART_REG    phwHead = (pUART_REG)phead;
    phwHead->UART_SFE = SHADOW_FIFI_ENABLED;
    phwHead->UART_SRT = RCVR_TRIGGER_TWO_LESS_FIFO;
    phwHead->UART_STET = TX_TRIGGER_TWO_IN_FIFO;
}

/*----------------------------------------------------------------------
Name      : UARTRest(pUART_REG phead)
Desc      : 对UART串复位
Params    : 

Return    :
----------------------------------------------------------------------*/
void UARTRest(pUART_REG phead)
{
    pUART_REG    phwHead = (pUART_REG)phead;
    phwHead->UART_SRR = UART_RESET | RCVR_FIFO_REST | XMIT_FIFO_RESET;
    phwHead->UART_IER = 0;
    /// phwHead->UART_SRR = 0;
}

/*----------------------------------------------------------------------
Name      : UARTInit(pRK_UART_INFO puartInitHwHead)
Desc      : 设置UART口的初始化,可以做一些初始参数设置
Params    : puartInitHwHead：结构体参数初始化，非法值会被设置成默认值
Return    :成功还回0，失败还回-1。
----------------------------------------------------------------------*/
int32 UARTInit(eUART_ch_t uartCh, uint32 baudRate)
{
    int8  returnTemp;
    pUART_REG puartreg ;

    if(uartCh == UART_CH0)
    {
        puartreg = (pUART_REG)UART0_BASE_ADDR;
        if(uartWorkStatus0 == 1)
        {
            return (-1);
        }
        uartWorkStatus0 = 1;
        //        SCUEnableClk(CLK_GATE_UART0);

    }
    else
    {
        puartreg = (pUART_REG)UART1_BASE_ADDR;
        if(uartWorkStatus1 == 1)
        {
            return (-1);
        }
        uartWorkStatus1 = 1;
        //        SCUEnableClk(CLK_GATE_UART1);
    }
    UARTRest(puartreg);  
    returnTemp = UARTSetIOP(puartreg,IRDA_SIR_DISABLED);
    if(returnTemp == -1)
    {
        return (-1);
    }
    returnTemp = UARTSetLcrReg(puartreg,UART_BIT8,PARITY_DISABLED,ONE_STOP_BIT);
    if(returnTemp == -1)
    {
        return (-1);
    }
    returnTemp = UARTSetBaudRate(puartreg,baudRate);
    if(returnTemp == -1)
    {
        return (-1);
    }
    UARTSetFifoEnabledNumb(puartreg);
    /// UARTSetIntEnabled(puartreg,uartIntNumb);
    return (0);
}
/*----------------------------------------------------------------------
Name      : UARTWriteByte(uint8 ch)
Desc      : 串口写一个字节
Params    : byte:输入的字节值
Return  :无
----------------------------------------------------------------------*/
int32 UARTWriteByte(eUART_ch_t uartCh, uint8 byte)
{
    uint32 uartTimeOut;
    pUART_REG puartRegStart;  

    // while (!(puartRegStart->UART_LSR & THRE_BIT_EN))
    //{
    // }
    if(uartCh == UART_CH0)
    {
        puartRegStart = (pUART_REG)UART0_BASE_ADDR;
    }
    else
    {
        puartRegStart = (pUART_REG)UART1_BASE_ADDR;
    }
    uartTimeOut = UART_BYTE_TIME_OUT_CNT;
    //    while((puartRegStart->UART_USR & UART_TRANSMIT_FIFO_NOT_FULL) != UART_TRANSMIT_FIFO_NOT_FULL)
    //   {
    //       if(uartTimeOut == 0)
    //        {
    //            return (-1);
    //        }
    //        uartTimeOut--;
    //    }

    while((puartRegStart->UART_USR & UART_TRANSMIT_FIFO_NOT_FULL) == 0);

    puartRegStart->UART_THR = byte;
    return (0);
}

/*----------------------------------------------------------------------
Name      : UARTReadByte(eUART_ch_t uartCh, uint8 data)
Desc      : 串口读一个字节
Params    : byte:输入的字节值
Return  :无
----------------------------------------------------------------------*/
uint8 UARTReadByte(eUART_ch_t uartCh)
{
    uint32 uartTimeOut;
    uint8 pdata;
    pUART_REG puartRegStart;

    if(uartCh == UART_CH0)
    {
        puartRegStart = (pUART_REG)UART0_BASE_ADDR;
    }
    else
    {
        puartRegStart = (pUART_REG)UART1_BASE_ADDR;
    }
    uartTimeOut = UART_BYTE_TIME_OUT_CNT;
    //    while((puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) != UART_RECEIVE_FIFO_NOT_EMPTY)
    //    {
    //        if(uartTimeOut == 0)
    //        {
    //              return (0);          
    //        }
    //       uartTimeOut--;                
    //   }  
    while((puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == 0);

    pdata = (uint8 )puartRegStart->UART_RBR;
    return (pdata);
}

void rk30_serial_putc(const char c)

{
    if (c == '\n')
        serial_putc('\r');
    UARTWriteByte(CONFIG_UART_NUM, c);
}

int rk30_serial_init(void)
{
    return(UARTInit(CONFIG_UART_NUM,115200));
}

void rk30_serial_setbrg(void)
{
    UARTSetBaudRate(CONFIG_UART_NUM,115200);
}

int rk30_serial_getc(void)
{
    char a;
    a=UARTReadByte(CONFIG_UART_NUM);
    if(0==a)
        a='\0';
    return(a);
}

int rk30_serial_tstc(void)
{
    if( CONFIG_UART_NUM == UART_CH0)
    {
        pUART_REG puartRegStart=(pUART_REG)UART0_BASE_ADDR;
        return((puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == UART_RECEIVE_FIFO_NOT_EMPTY);
    }
    else
    {
        pUART_REG puartRegStart=(pUART_REG)UART1_BASE_ADDR;
        return((puartRegStart->UART_USR & UART_RECEIVE_FIFO_NOT_EMPTY) == UART_RECEIVE_FIFO_NOT_EMPTY);
    }
}

static struct serial_device rk30_serial_drv = {
    .name   = "rk30_serial",
    .start  = rk30_serial_init,
    .stop   = NULL,
    .setbrg = rk30_serial_setbrg,
    .putc   = rk30_serial_putc,
    .puts   = default_serial_puts,
    .getc   = rk30_serial_getc,
    .tstc   = rk30_serial_tstc,
};

void rk30_serial_initialize(void)
{
    serial_register(&rk30_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
    return &rk30_serial_drv;
}

