/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK322X_IRQS_H
#define __RK322X_IRQS_H

#define FIQ_START                       0

#define IRQ_LOCALTIMER                  29

#define IRQ_DMAC_0                      32
#define IRQ_DMAC_1                      33
#define IRQ_DDR_PCTL                    34
#define IRQ_DDR_MON			35
#define IRQ_GPU_PP                      36
#define IRQ_GPU_MMU                     37
#define IRQ_GPU_GP                      38
#define IRQ_VDEC                        39
#define IRQ_VDEC_MMU                    40
#define IRQ_VPU                         41
#define IRQ_VPU_MMU                     42
#define IRQ_VPU_ENC                     43
#define IRQ_SDMMC                       44
#define IRQ_SDIO                        45
#define IRQ_EMMC                        46
#define IRQ_NANDC                       47
#define IRQ_USB_HOST0_EHCI              48
#define IRQ_USB_HOST0_OHCI              49
#define IRQ_USB_HOST0_ARB               50
#define IRQ_USB_HOST1_EHCI              51
#define IRQ_USB_HOST1_OHCI              52
#define IRQ_USB_HOST1_ARB               53
#define IRQ_USB_HOST2_ARB               54
#define IRQ_USB_OTG                     55
#define IRQ_GMAC                        56
#define IRQ_GMAC_PMT                    57
#define IRQ_I2S1_8CH                    58
#define IRQ_I2S0_8CH                    59
#define IRQ_I2S2_2CH                    60
#define IRQ_SPDIF                       61
#define IRQ_CRYPTO                      62
#define IRQ_IEP                         63
#define IRQ_VOP                         64
#define IRQ_RGA                         65
#define IRQ_HDCP                        66
#define IRQ_HDMI                        67
#define IRQ_I2C0                        68
#define IRQ_I2C1                        69
#define IRQ_I2C2                        70
#define IRQ_I2C3                        71
#define IRQ_WDT                         72
#define IRQ_STIMER0                     73
#define IRQ_STIMER1                     74
#define IRQ_TIMER0                      75
#define IRQ_TIMER1                      76
#define IRQ_TIMER2                      77
#define IRQ_TIMER3                      78
#define IRQ_TIMER4                      79
#define IRQ_TIMER5                      80
#define IRQ_SPI                         81
#define IRQ_PWM                         82
#define IRQ_GPIO0                       83
#define IRQ_GPIO1                       84
#define IRQ_GPIO2                       85
#define IRQ_GPIO3                       86
#define IRQ_UART0                       87
#define IRQ_UART1                       88
#define IRQ_UART2                       89
#define IRQ_TSADC                       90
#define IRQ_OTG0_BVALID                 91
#define IRQ_OTG0_ID                     92
#define IRQ_OTG0_LINESTATE              93
#define IRQ_HOST0_LINESTATE             94
#define IRQ_SDIO_DETECT                 95
#define IRQ_SDMMC_DETECT                97
#define IRQ_HOST2_EHCI                  98
#define IRQ_HOST2_OHCI                  99
#define IRQ_HOST1_LINESTATE             100
#define IRQ_HOST2_LINESTATE             101
#define IRQ_MACPHY                      102
#define IRQ_HDMI_WAKEUP                 103
#define IRQ_TSP                         104
#define IRQ_SIM_CARD			105


#define NR_GIC_IRQS                     (4 * 32)
#define NR_GPIO_IRQS                    (4 * 32)
#define NR_IRQS                         (NR_GIC_IRQS + NR_GPIO_IRQS)

#endif /* __RK322X_IRQS_H */
