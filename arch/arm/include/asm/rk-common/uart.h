/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKXX_UART_H
#define __RKXX_UART_H


/* uart some key registers offset */
#define UART_RBR	0x00
#define UART_THR	0x00
#define UART_DLL	0x00
#define UART_DLH	0x04
#define UART_IER	0x04
#define UART_LCR	0x0c
#define UART_MCR	0x10
#define UART_USR	0x7c
#define UART_SRR	0x88
#define UART_SFE	0x98
#define UART_SRT	0x9c
#define UART_STET	0xa0


/* UART_IER */
#define THRE_INT_ENABLE                    (1<<7)
#define THRE_INT_DISABLE                   (0)
#define ENABLE_MODEM_STATUS_INT            (1<<3)
#define DISABLE_MODEM_STATUS_INT           (0)
#define ENABLE_RECEIVER_LINE_STATUS_INT    (1<<2)
#define DISABLE_RECEIVER_LINE_STATUS_INT   (0)
#define ENABLE_TRANSMIT_HOLDING_EM_INT     (1<<1) /* Enable Transmit Holding Register Empty Interrupt. */
#define DISABLE_TRANSMIT_HOLDING_EM_INT    (0)
#define ENABLE_RECEIVER_DATA_INT           (1) /* Enable Received Data Available Interrupt. */
#define DISABLE_RECEIVER_DATA_INT          (0)

/* UART_IIR */
#define IR_MODEM_STATUS                    (0)
#define NO_INT_PENDING                     (1)
#define THR_EMPTY                          (2)
#define RECEIVER_DATA_AVAILABLE            (0x04)
#define RECEIVER_LINE_AVAILABLE            (0x06)
#define BUSY_DETECT                        (0x07)
#define CHARACTER_TIMEOUT                  (0x0c)

/* UART_LCR */
#define LCR_DLA_EN                         (1<<7)
#define BREAK_CONTROL_BIT                  (1<<6)
#define PARITY_DISABLED                    (0)
#define PARITY_ENABLED                     (1<<3)
#define ONE_STOP_BIT                       (0)
#define ONE_HALF_OR_TWO_BIT                (1<<2)
#define LCR_WLS_5                          (0x00)
#define LCR_WLS_6                          (0x01)
#define LCR_WLS_7                          (0x02)
#define LCR_WLS_8                          (0x03)
#define UART_DATABIT_MASK                  (0x03)

/* UART_MCR */
#define IRDA_SIR_DISABLED                  (0)
#define IRDA_SIR_ENSABLED                  (1<<6)
#define AUTO_FLOW_DISABLED                 (0)
#define AUTO_FLOW_ENSABLED                 (1<<5)

/* UART_LSR */
#define THRE_BIT_EN                        (1<<5)

/* UART_USR */
#define UART_RECEIVE_FIFO_EMPTY            (0)
#define UART_RECEIVE_FIFO_NOT_EMPTY        (1<<3)
#define UART_TRANSMIT_FIFO_FULL            (0)
#define UART_TRANSMIT_FIFO_NOT_FULL        (1<<1)

/* UART_SFE */
#define SHADOW_FIFI_ENABLED                (1)
#define SHADOW_FIFI_DISABLED               (0)

/* UART_SRT */
#define RCVR_TRIGGER_ONE                   (0)
#define RCVR_TRIGGER_QUARTER_FIFO          (1)
#define RCVR_TRIGGER_HALF_FIFO             (2)
#define RCVR_TRIGGER_TWO_LESS_FIFO         (3)

/* UART_STET */
#define TX_TRIGGER_EMPTY                   (0)
#define TX_TRIGGER_TWO_IN_FIFO             (1)
#define TX_TRIGGER_ONE_FOUR_FIFO           (2)
#define TX_TRIGGER_HALF_FIFO               (3)

/* UART_SRR */
#define UART_RESET                         (1)
#define RCVR_FIFO_REST                     (1<<1)
#define XMIT_FIFO_RESET                    (1<<2)


#define UART_MODE_X_DIV		16

#define UART_BIT5		5
#define UART_BIT6		6
#define UART_BIT7		7
#define UART_BIT8		8


/*		config rk uart input clock
 * rk uart clock source can be 24M crystal input or gating from uart_pll_clk,
 * on uboot system we using 24M crystal input as default.
 */
#define UART_CLOCK_FREQ		CONFIG_SYS_CLK_FREQ_CRYSTAL

#endif /* __RKXX_UART_H */
