/******************************************************************/
/*   Copyright (C) 2008 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File    :  uart.h
Desc    :  定义UART的寄存器结构体\寄存器位的宏定义\接口函数

Author  : lhh
Date    : 2008-10-29
Modified:
Revision:           1.00
$Log: uart.h,v $
Revision 1.1  2010/12/06 02:43:52  Administrator
*** empty log message ***

Revision 1.1.2.1  2010/05/19 09:37:27  Administrator
*** empty log message ***

Revision 1.1  2009/08/07 09:52:54  Administrator
*** empty log message ***

Revision 1.1  2009/03/16 07:37:15  Administrator
*** empty log message ***

*********************************************************************/

#ifndef _DRIVER_UART_H_
#define _DRIVER_UART_H_


///UART_IER
#define   THRE_INT_ENABLE                   (1<<7)
#define   THRE_INT_DISABLE                   (0)
#define   ENABLE_MODEM_STATUS_INT           (1<<3)
#define   DISABLE_MODEM_STATUS_INT           (0)
#define   ENABLE_RECEIVER_LINE_STATUS_INT   (1<<2)
#define   DISABLE_RECEIVER_LINE_STATUS_INT   (0)
#define   ENABLE_TRANSMIT_HOLDING_EM_INT    (1<<1) ///Enable Transmit Holding Register Empty Interrupt.
#define   DISABLE_TRANSMIT_HOLDING_EM_INT    (0)
#define   ENABLE_RECEIVER_DATA_INT           (1)   ////Enable Received Data Available Interrupt.
#define   DISABLE_RECEIVER_DATA_INT          (0)

///UART_IIR
#define   IR_MODEM_STATUS                    (0)
#define   NO_INT_PENDING                     (1)
#define   THR_EMPTY                          (2)
#define   RECEIVER_DATA_AVAILABLE            (0x04)
#define   RECEIVER_LINE_AVAILABLE            (0x06)
#define   BUSY_DETECT                        (0x07)
#define   CHARACTER_TIMEOUT                  (0x0c)

///UART_LCR
#define  LCR_DLA_EN                         (1<<7)
#define  BREAK_CONTROL_BIT                  (1<<6)
#define  PARITY_DISABLED                     (0)
#define  PARITY_ENABLED                     (1<<3)
#define  ONE_STOP_BIT                        (0)
#define  ONE_HALF_OR_TWO_BIT                (1<<2)
#define  LCR_WLS_5                           (0x00)
#define  LCR_WLS_6                           (0x01)
#define  LCR_WLS_7                           (0x02)
#define  LCR_WLS_8                           (0x03)
#define  UART_DATABIT_MASK                   (0x03)


///UART_MCR
#define  IRDA_SIR_DISABLED                   (0)
#define  IRDA_SIR_ENSABLED                  (1<<6)
#define  AUTO_FLOW_DISABLED                  (0)
#define  AUTO_FLOW_ENSABLED                 (1<<5)

///UART_LSR
#define  THRE_BIT_EN                        (1<<5)

///UART_USR
#define  UART_RECEIVE_FIFO_EMPTY             (0)
#define  UART_RECEIVE_FIFO_NOT_EMPTY         (1<<3)
#define  UART_TRANSMIT_FIFO_FULL             (0)
#define  UART_TRANSMIT_FIFO_NOT_FULL         (1<<1)

///UART_SFE
#define  SHADOW_FIFI_ENABLED                 (1)
#define  SHADOW_FIFI_DISABLED                (0)

///UART_SRT
#define  RCVR_TRIGGER_ONE                    (0)
#define  RCVR_TRIGGER_QUARTER_FIFO           (1)
#define  RCVR_TRIGGER_HALF_FIFO              (2)
#define  RCVR_TRIGGER_TWO_LESS_FIFO          (3)

//UART_STET
#define  TX_TRIGGER_EMPTY                    (0)
#define  TX_TRIGGER_TWO_IN_FIFO              (1)
#define  TX_TRIGGER_ONE_FOUR_FIFO            (2)
#define  TX_TRIGGER_HALF_FIFO                (3)

///UART_SRR
#define  UART_RESET                          (1)
#define  RCVR_FIFO_REST                     (1<<1)
#define  XMIT_FIFO_RESET                    (1<<2)





//UART Registers
typedef volatile struct tagUART_STRUCT
{
    uint32 UART_RBR;
    uint32 UART_DLH;
    uint32 UART_IIR;
    uint32 UART_LCR;
    uint32 UART_MCR;
    uint32 UART_LSR;
    uint32 UART_MSR;
    uint32 UART_SCR;
    uint32 RESERVED1[(0x30-0x20)/4];
    uint32 UART_SRBR[(0x70-0x30)/4];
    uint32 UART_FAR;
    uint32 UART_TFR;
    uint32 UART_RFW;
    uint32 UART_USR;
    uint32 UART_TFL;
    uint32 UART_RFL;
    uint32 UART_SRR;
    uint32 UART_SRTS;
    uint32 UART_SBCR;
    uint32 UART_SDMAM;
    uint32 UART_SFE;
    uint32 UART_SRT;
    uint32 UART_STET;
    uint32 UART_HTX;
    uint32 UART_DMASA;
    uint32 RESERVED2[(0xf4-0xac)/4];
    uint32 UART_CPR;
    uint32 UART_UCV;
    uint32 UART_CTR;
} UART_REG, *pUART_REG;

#define UART_THR UART_RBR
#define UART_DLL UART_RBR
#define UART_IER UART_DLH
#define UART_FCR UART_IIR
#define UART_STHR[(0x6c-0x30)/4]  UART_SRBR[(0x6c-0x30)/4]

#define MODE_X_DIV            16


#undef  EXT
#ifdef  IN_DRIVER_UART
#define EXT
uint32 uartWorkStatus0=0;
uint32 uartWorkStatus1=0;
#else
#define EXT     extern
extern uint32 uartWorkStatus0;
extern uint32 uartWorkStatus1;
#endif

EXT uint8 receBuff0[32];
EXT uint8 receBuff1[32];
EXT uint8 receBuffCnt0;
EXT uint8 receBuffCnt1;
EXT volatile uint8 *puart0ReadCnt;
EXT volatile uint8 *puart0WriteCnt;
EXT volatile uint8 *puart1ReadCnt;
EXT volatile uint8 *puart1WriteCnt;
EXT volatile uint32 g_uart0ReadLength;
EXT volatile uint32 g_uart0WriteLength;
EXT volatile uint32 g_uart1ReadLength;
EXT volatile uint32 g_uart1WriteLength;


/********************************************************************/

void UartInit(void);
void UartWriteChar(char c);

#endif /* _UART_H */

/*********************************************************************
 END OF FILE
*********************************************************************/


