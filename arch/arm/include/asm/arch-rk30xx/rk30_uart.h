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
#ifdef DRIVERS_UART

#ifndef _RK30_UART_H_
#define _RK30_UART_H_


///UART_IER
#define   THRE_INT_ENABLE                   (1<<7)
#define   THRE_INT_DISABLE                   (0)
#define   ENABLE_MODEM_STATUS_INT           (1<<3)
#define   DISABLE_MODEM_STATUS_INT           (0)
#define   ENABLE_RECEIVER_LINE_STATUS_INT   (1<<2)
#define   DISABLE_RECEIVER_LINE_STATUS_INT   (0)
#define   ENABLE_TRANSMIT_HOLDING_EM_INT    (1<<1) //Enable Transmit Holding Register Empty Interrupt.
#define   DISABLE_TRANSMIT_HOLDING_EM_INT    (0)
#define   ENABLE_RECEIVER_DATA_INT           (1)   //Enable Received Data Available Interrupt.
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
typedef volatile struct tagUART_STRUCT {
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

#define MODE_X_DIV              16


#define UART_BIT5 5
#define UART_BIT6 6
#define UART_BIT7 7
#define UART_BIT8 8

#define UART_BYTE_TIME_OUT_CNT  0xff

///uartWorkStatusFlag
#define   UART0_TX_WORK                     (1)
#define   UART0_RX_WORK                     (1<<1)
#define   UART1_TX_WORK                     (1<<2)
#define   UART1_RX_WORK                     (1<<3)
#define   UART_ERR_RX                       (1<<4)


typedef enum UART_ch {
    UART_CH0,
    UART_CH1
}eUART_ch_t;


#define UART0_BASE_ADDR		RK30_UART0_PHYS
#define UART1_BASE_ADDR		RK30_UART1_PHYS

#endif /* _RK30_UART_H_ */

#endif /* DRIVERS_UART */












