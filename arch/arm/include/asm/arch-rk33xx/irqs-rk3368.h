/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3368_IRQS_H
#define __RK3368_IRQS_H


#define FIQ_START                       0

#define IRQ_HYPERVISOR_TIMER            26
#define IRQ_VIRTUAL_TIMER               27
#define IRQ_SECURE_TIMER                28
#define IRQ_NOSECURE_TIMER              29

#define RKXX_IRQ(x)                     (x + 32)

#define IRQ_DMAC_BUS0                   RKXX_IRQ(0)
#define IRQ_DMAC_BUS1                   RKXX_IRQ(1)
#define IRQ_DMAC_PERI0                  RKXX_IRQ(2)
#define IRQ_DMAC_PERI1                  RKXX_IRQ(3)
#define IRQ_DDR_UPCTL                   RKXX_IRQ(4)
#define IRQ_HSADC                       RKXX_IRQ(5)

#define IRQ_GPU                         RKXX_IRQ(8)
#define IRQ_VEPU                        RKXX_IRQ(9)
#define IRQ_VDPU                        RKXX_IRQ(10)

#define IRQ_HEVC                        RKXX_IRQ(12)
#define IRQ_VIP                         RKXX_IRQ(13)
#define IRQ_ISP                         RKXX_IRQ(14)
#define IRQ_VOP                         RKXX_IRQ(15)
#define IRQ_HDCP                        RKXX_IRQ(16)
#define IRQ_IEP                         RKXX_IRQ(17)
#define IRQ_RGA                         RKXX_IRQ(18)
#define IRQ_DSI_HOST                    RKXX_IRQ(19)

#define IRQ_USB_OTG                     RKXX_IRQ(23)
#define IRQ_USB_HOST_EHCI               RKXX_IRQ(24)
#define IRQ_USB_HOST_OHCI               RKXX_IRQ(25)

#define IRQ_GMAC                        RKXX_IRQ(27)
#define IRQ_GMAC_PMT                    RKXX_IRQ(28)
#define IRQ_SFC                         RKXX_IRQ(29)

#define IRQ_SDMMC                       RKXX_IRQ(32)
#define IRQ_SDIO                        RKXX_IRQ(33)

#define IRQ_EMMC                        RKXX_IRQ(35)
#define IRQ_SARADC                      RKXX_IRQ(36)
#define IRQ_TSADC                       RKXX_IRQ(37)
#define IRQ_NANDC                       RKXX_IRQ(38)
#define IRQ_PERI_MMU                    RKXX_IRQ(39)
#define IRQ_I2S_2CH                     RKXX_IRQ(40)
#define IRQ_SPI2                        RKXX_IRQ(41)
#define IRQ_TPS                         RKXX_IRQ(42)
#define IRQ_SCR                         RKXX_IRQ(43)
#define IRQ_SPI0                        RKXX_IRQ(44)
#define IRQ_SPI1                        RKXX_IRQ(45)
#define IRQ_TIMER1_6CH_0                RKXX_IRQ(46)
#define IRQ_TIMER1_6CH_1                RKXX_IRQ(47)
#define IRQ_CRYPTO                      RKXX_IRQ(48)
#define IRQ_TIMER1_6CH_2                RKXX_IRQ(49)
#define IRQ_TIMER1_6CH_3                RKXX_IRQ(50)
#define IRQ_TIMER1_6CH_4                RKXX_IRQ(51)
#define IRQ_TIMER1_6CH_5                RKXX_IRQ(52)
#define IRQ_I2S_8CH                     RKXX_IRQ(53)
#define IRQ_SPDIF_8CH                   RKXX_IRQ(54)
#define IRQ_UART_BT                     RKXX_IRQ(55)
#define IRQ_UART_BB                     RKXX_IRQ(56)
#define IRQ_UART_DBG                    RKXX_IRQ(57)
#define IRQ_UART_GPS                    RKXX_IRQ(58)
#define IRQ_UART_EXP                    RKXX_IRQ(59)
#define IRQ_I2C_PMU                     RKXX_IRQ(60)
#define IRQ_I2C_AUDIO                   RKXX_IRQ(61)
#define IRQ_I2C_SENSOR                  RKXX_IRQ(62)
#define IRQ_I2C_CAM                     RKXX_IRQ(63)
#define IRQ_I2C_TP                      RKXX_IRQ(64)
#define IRQ_I2C_HDMI                    RKXX_IRQ(65)
#define IRQ_TIMER0_6CH_0                RKXX_IRQ(66)
#define IRQ_TIMER0_6CH_1                RKXX_IRQ(67)
#define IRQ_TIMER0_6CH_2                RKXX_IRQ(68)
#define IRQ_TIMER0_6CH_3                RKXX_IRQ(69)
#define IRQ_TIMER0_6CH_4                RKXX_IRQ(70)
#define IRQ_TIMER0_6CH_5                RKXX_IRQ(71)
#define IRQ_SECURE_TIMER_2CH_0          RKXX_IRQ(72)
#define IRQ_SECURE_TIMER_2CH_1          RKXX_IRQ(73)
#define IRQ_PWM0                        RKXX_IRQ(74)
#define IRQ_PWM1                        RKXX_IRQ(75)
#define IRQ_PWM2                        RKXX_IRQ(76)
#define IRQ_PWM3                        RKXX_IRQ(77)
#define IRQ_RK_PWM                      RKXX_IRQ(78)
#define IRQ_WDT_CPU                     RKXX_IRQ(79)
#define IRQ_PMU                         RKXX_IRQ(80)
#define IRQ_GPIO0                       RKXX_IRQ(81)
#define IRQ_GPIO1                       RKXX_IRQ(82)
#define IRQ_GPIO2                       RKXX_IRQ(83)
#define IRQ_GPIO3                       RKXX_IRQ(84)

#define IRQ_WDT_MCU                     RKXX_IRQ(86)

#define IRQ_PERI_AHB_ARBITER0_USB       RKXX_IRQ(90)
#define IRQ_PERI_AHB_ARBITER1_EMEM      RKXX_IRQ(91)
#define IRQ_PERI_AHB_ARBITER2_MMU       RKXX_IRQ(92)
#define IRQ_OTG_ID                      RKXX_IRQ(93)
#define IRQ_OTG_BVALID                  RKXX_IRQ(94)
#define IRQ_OTG_LINESTATE               RKXX_IRQ(95)
#define IRQ_USBHOST_LINESTATE           RKXX_IRQ(96)

#define IRQ_SDMMC_DETECT_N              RKXX_IRQ(99)
#define IRQ_SDIO_DETECT_N               RKXX_IRQ(100)
#define IRQ_MIPI_CSI_HOST               RKXX_IRQ(101)
#define IRQ_HDMI_WAKEUP                 RKXX_IRQ(102)
#define IRQ_HDMI                        RKXX_IRQ(103)
#define IRQ_EDP_HDMI                    RKXX_IRQ(104)
#define IRQ_EDP_DP                      RKXX_IRQ(105)
#define IRQ_SDMMC_DETECT_DUAL_EDGE      RKXX_IRQ(106)

#define IRQ_CCI_NEVNTCNTVOERFLOW        RKXX_IRQ(109)
#define IRQ_CCI400                      RKXX_IRQ(110)


#define NR_GIC_IRQS                     (5 * 32)
#define NR_GPIO_IRQS                    (4 * 32)
#define NR_IRQS                         (NR_GIC_IRQS + NR_GPIO_IRQS)


#endif /* __RK3368_IRQS_H */
