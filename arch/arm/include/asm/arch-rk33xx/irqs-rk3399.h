/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3399_IRQS_H
#define __RK3399_IRQS_H


/* SPI */
#define IRQ_EMMCCORE			43

#define IRQ_GPIO0                       46
#define IRQ_GPIO1                       47
#define IRQ_GPIO2                       48
#define IRQ_GPIO3                       49
#define IRQ_GPIO4                       50

#define IRQ_PWM				93

#define IRQ_SDIO                        96
#define IRQ_SDMMC                       97

#define IRQ_STIMER			101

#define IRQ_TIMER			113

#define IRQ_PMUTIMER			127

#define IRQ_USB3OTG0_BVALID		135
#define IRQ_USB3OTG0_ID			136
#define IRQ_USB3OTG0			137
#define IRQ_USB3OTG0_LINESTATE		138
#define IRQ_USB3OTG0_TXDET		139
#define IRQ_USB3OTG1_BVALID		140
#define IRQ_USB3OTG1_ID			141
#define IRQ_USB3OTG1			142
#define IRQ_USB3OTG1_LINESTATE		143
#define IRQ_USB3OTG1_TXDET		144

#define IRQ_VOPBIG			150
#define IRQ_VOPLIT			151


/* total irq */
#define NR_GIC_IRQS                     (6 * 32)
#define NR_GPIO_IRQS                    (5 * 32)
#define NR_IRQS                         (NR_GIC_IRQS + NR_GPIO_IRQS)


#endif /* __RK3399_IRQS_H */
