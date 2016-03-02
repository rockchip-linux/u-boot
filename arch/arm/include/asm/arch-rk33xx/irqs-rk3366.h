/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3366_IRQS_H
#define __RK3366_IRQS_H


#define FIQ_START                       0

#define IRQ_HYPERVISOR_TIMER            26
#define IRQ_VIRTUAL_TIMER               27
#define IRQ_SECURE_TIMER                28
#define IRQ_NOSECURE_TIMER              29


#define IRQ_DMAC_BUS0                   32
#define IRQ_DMAC_BUS1                   33
#define IRQ_DMAC_PERI0                  34
#define IRQ_DMAC_PERI1                  35
#define IRQ_DDR_UPCTL                   36
#define IRQ_HSADC                       37

#define IRQ_GPU0                        38
#define IRQ_GPU1                        39
#define IRQ_GPU2                        40
#define IRQ_VEPU                        41
#define IRQ_VEPU_MMU                    42
#define IRQ_VDPU                        43
#define IRQ_RKVDEC_MMU                  44
#define IRQ_RKVDEC                      45
#define IRQ_ISP                         46
#define IRQ_VOP0                        47
#define IRQ_HDCP                        48
#define IRQ_IEP                         49
#define IRQ_RGA                         50
#define IRQ_DSI0_HOST                   51
#define IRQ_VOP1                        52
#define IRQ_HOST_ARBI                   53
#define IRQ_USB3                        54
#define IRQ_USB_OTG                     55
#define IRQ_USB_HOST_EHCI               56
#define IRQ_USB_HOST_OHCI               57

#define IRQ_GMAC                        59
#define IRQ_GMAC_PMT                    60
#define IRQ_SFC                         61

#define IRQ_USB3_HOST_ERROR             63

#define IRQ_SDMMC                       64
#define IRQ_SDIO                        65

#define IRQ_EMMC                        67
#define IRQ_SARADC                      68
#define IRQ_TSADC                       69
#define IRQ_NANDC                       70
#define IRQ_USB3_HOST_SMI               71
#define IRQ_I2S_2CH                     72

#define IRQ_SCR                         75
#define IRQ_SPI0                        76
#define IRQ_SPI1                        77

#define IRQ_CRYPTO                      80

#define IRQ_DCF_ERROR                   83
#define IRQ_DCF_DONE                    84
#define IRQ_I2S_8CH                     85
#define IRQ_SPDIF_8CH                   86
#define IRQ_UART_BT                     87

#define IRQ_UART_DBG                    88
#define IRQ_UART_GPS                    89

#define IRQ_I2C_PMU                     92
#define IRQ_I2C_AUDIO                   93
#define IRQ_I2C_SENSOR                  94
#define IRQ_I2C_CAM                     95
#define IRQ_I2C_TP                      96
#define IRQ_I2C_HDMI                    97
#define IRQ_TIMER_6CH_0                 98
#define IRQ_TIMER_6CH_1                 99
#define IRQ_TIMER_6CH_2                 100
#define IRQ_TIMER_6CH_3                 101
#define IRQ_TIMER_6CH_4                 102
#define IRQ_TIMER_6CH_5                 103
#define IRQ_SECURE_TIMER_2CH_0          104
#define IRQ_SECURE_TIMER_2CH_1          105
#define IRQ_USB3_RXDET			106
#define IRQ_USB3_LINESTATE		107
#define IRQ_USB3_ID			108
#define IRQ_USB3_BVALID                 109
#define IRQ_RK_PWM                      110
#define IRQ_WDT_CPU                     111
#define IRQ_PMU                         112
#define IRQ_GPIO0                       113
#define IRQ_GPIO1                       114
#define IRQ_GPIO2                       115
#define IRQ_GPIO3                       116
#define IRQ_GPIO4                       117
#define IRQ_WDT_MCU                     118
#define IRQ_GPIO5                       119
#define IRQ_WIFI2HOST                   120
#define IRQ_BT                          121
#define IRQ_BT2HOST                     122
#define IRQ_WIFI                        123
#define IRQ_OTG_ID                      125
#define IRQ_OTG_BVALID                  126
#define IRQ_OTG_LINESTATE               127
#define IRQ_USBHOST_LINESTATE           128

#define IRQ_SDMMC_DETECT_N              131
#define IRQ_SDIO_DETECT_N               132

#define IRQ_HDMI_WAKEUP                 134
#define IRQ_HDMI                        135

#define IRQ_SDMMC_DETECT_DUAL_EDGE      138
#define IRQ_DFI_MONITOR			139

#define IRQ_EXT_ERROR                   143


#define NR_GIC_IRQS                     (6 * 32)
#define NR_GPIO_IRQS                    (6 * 32)
#define NR_IRQS                         (NR_GIC_IRQS + NR_GPIO_IRQS)


#endif /* __RK3366_IRQS_H */
