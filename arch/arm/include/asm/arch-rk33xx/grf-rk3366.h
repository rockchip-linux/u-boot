/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3366_GRF_H
#define __RK3366_GRF_H

#include <asm/io.h>


/* gpio iomux control */
#define GRF_GPIO2A_IOMUX        0x0010
#define GRF_GPIO2B_IOMUX        0x0014
#define GRF_GPIO2C_IOMUX        0x0018
#define GRF_GPIO2D_IOMUX        0x001c
#define GRF_GPIO3A_IOMUX        0x0020
#define GRF_GPIO3B_IOMUX        0x0024
#define GRF_GPIO3C_IOMUX        0x0028
#define GRF_GPIO4C_IOMUX        0x0038
#define GRF_GPIO4D_IOMUX        0x003c
#define GRF_GPIO5A_IOMUX        0x0040
#define GRF_GPIO5B_IOMUX        0x0044
#define GRF_GPIO5C_IOMUX        0x0048


/* gpio pull down/up control */
#define GRF_GPIO1A_P		0x0100	/* define for gpio driver */

#define GRF_GPIO2A_P		0x0110
#define GRF_GPIO2B_P		0x0114
#define GRF_GPIO2C_P		0x0118
#define GRF_GPIO2D_P		0x011C
#define GRF_GPIO3A_P		0x0120
#define GRF_GPIO3B_P		0x0124
#define GRF_GPIO3C_P		0x0128
#define GRF_GPIO4C_P		0x0138
#define GRF_GPIO4D_P		0x013C
#define GRF_GPIO5A_P		0x0140
#define GRF_GPIO5B_P		0x0144
#define GRF_GPIO5C_P		0x0148


/* gpio drive strength control */
#define GRF_GPIO1A_E		0x0200	/* define for gpio driver */

#define GRF_GPIO2A_E		0x0210
#define GRF_GPIO2B_E		0x0214
#define GRF_GPIO2C_E		0x0218
#define GRF_GPIO2D_E		0x021C
#define GRF_GPIO3A_E		0x0220
#define GRF_GPIO3B_E		0x0224
#define GRF_GPIO3C_E		0x0228
#define GRF_GPIO4C_E		0x0238
#define GRF_GPIO4D_E		0x023C
#define GRF_GPIO5A_E		0x0240
#define GRF_GPIO5B_E		0x0244
#define GRF_GPIO5C_E		0x0248


/* gpio drive strength control of N/P channel */
#define GRF_GPIO2A_EN		0x0360
#define GRF_GPIO2A_EP		0x0364
#define GRF_GPIO2C_EN		0x0368
#define GRF_GPIO2C_EP		0x036c
#define GRF_GPIO2D_EN		0x0370
#define GRF_GPIO2D_EP		0x0374
#define GRF_GPIO2B3_EN		0x0378
#define GRF_GPIO2A4_EP		0x037c


/* Soc control */
#define GRF_SOC_CON0		0x0400
#define GRF_SOC_CON1		0x0404
#define GRF_SOC_CON2		0x0408
#define GRF_SOC_CON3		0x040C
#define GRF_SOC_CON4		0x0410
#define GRF_SOC_CON5		0x0414
#define GRF_SOC_CON6		0x0418
#define GRF_SOC_CON7		0x041C
#define GRF_SOC_CON8		0x0420
#define GRF_SOC_CON9		0x0424
#define GRF_SOC_CON10		0x0428
#define GRF_SOC_CON11		0x042C
#define GRF_SOC_CON12		0x0430
#define GRF_SOC_CON13		0x0434
#define GRF_SOC_CON14		0x0438
#define GRF_SOC_CON15		0x043C
#define GRF_SOC_CON16		0x0440


/* Soc status */
#define GRF_SOC_STATUS0		0x0480
#define GRF_SOC_STATUS5		0x0494
#define GRF_SOC_STATUS6		0x0498
#define GRF_SOC_STATUS7		0x049C


/* cpu control */
#define GRF_CPU_CON0		0x0500
#define GRF_CPU_CON1		0x0504


/* cpu status */
#define GRF_CPU_STATUS0		0x0520
#define GRF_CPU_STATUS1		0x0524


/* ddrc control */
#define GRF_DDRC0_CON0		0x0600


/* sig detect */
#define GRF_SIG_DETECT_CON	0x0680
#define GRF_SIG_DETECT_STATUS	0x0690
#define GRF_SIG_DETECT_CLR	0x06A0


/* UOC control */
#define GRF_UOC0_CON0		0x0700

#define GRF_UOC1_CON1		0x0718
#define GRF_UOC1_CON2		0x071C
#define GRF_UOC1_CON3		0x0720
#define GRF_UOC1_CON4		0x0724
#define GRF_UOC1_CON5		0x0728


/* usb phy control */
#define GRF_USBPHY_CON0		0x0780
#define GRF_USBPHY_CON1		0x0784
#define GRF_USBPHY_CON2		0x0788
#define GRF_USBPHY_CON3		0x078C
#define GRF_USBPHY_CON4		0x0790
#define GRF_USBPHY_CON5		0x0794
#define GRF_USBPHY_CON6		0x0798
#define GRF_USBPHY_CON7		0x079C
#define GRF_USBPHY_CON8		0x07A0
#define GRF_USBPHY_CON9		0x07A4
#define GRF_USBPHY_CON10	0x07A8
#define GRF_USBPHY_CON11	0x07AC
#define GRF_USBPHY_CON12	0x07B0
#define GRF_USBPHY_CON13	0x07B4
#define GRF_USBPHY_CON14	0x07B8
#define GRF_USBPHY_CON15	0x07BC
#define GRF_USBPHY_CON16	0x07C0
#define GRF_USBPHY_CON17	0x07C4
#define GRF_USBPHY_CON18	0x07C8
#define GRF_USBPHY_CON19	0x07CC
#define GRF_USBPHY_CON20	0x07D0
#define GRF_USBPHY_CON21	0x07D4
#define GRF_USBPHY_CON22	0x07D8
#define GRF_USBPHY_CON23	0x07DC


#define GRF_DLL_CON0		0x0800
#define GRF_DLL_CON1		0x0804
#define GRF_DLL_CON2		0x0808

#define GRF_DLL_STATUS0		0x080c
#define GRF_DLL_STATUS1		0x0810
#define GRF_DLL_STATUS2		0x0814

#define GRF_PSDLL_CON0		0x0880
#define GRF_PSDLL_CON1		0x0884
#define GRF_PSDLL_CON2		0x0888
#define GRF_PSDLL_CON3		0x088C

#define GRF_PSDLL_STATUS0 	0x0890
#define GRF_PSDLL_STATUS1 	0x0894
#define GRF_PSDLL_STATUS2 	0x0898
#define GRF_PSDLL_STATUS3 	0x089c
#define GRF_PSDLL_STATUS4 	0x08a0
#define GRF_PSDLL_STATUS5 	0x08a4
#define GRF_PSDLL_STATUS6 	0x08a8

#define GRF_USB3_CON0		0x0a00
#define GRF_USB3_CON1		0x0a04
#define GRF_USB3_STATUS0	0x0a10
#define GRF_USB3_STATUS1	0x0a14
#define GRF_USB3_STATUS2	0x0a18
#define GRF_USB3PHY_CON0	0x0a80
#define GRF_USB3PHY_CON1	0x0a84
#define GRF_USB3PHY_CON2	0x0a88
#define GRF_USB3PHY_CON3	0x0a8c
#define GRF_USB3PHY_CON4	0x0a90
#define GRF_USB3PHY_CON5	0x0a94
#define GRF_USB3PHY_CON6	0x0a98
#define GRF_USB3PHY_CON7	0x0a9c
#define GRF_USB3PHY_CON8	0x0aa0
#define GRF_USB3PHY_STATUS0	0x0ac0


/* IO Voltage select */
#define GRF_IO_VSEL		0x0900


/* Chip ID register */
#define GRF_CHIP_ID		0x0f00


/* secure grf fast boot address */
#define GRF_FAST_BOOT_ADDR	0x0F80




/* secure grf soc control part 1 */
#define SGRF_SOC_CON0		0x0000
#define SGRF_SOC_CON1		0x0004
#define SGRF_SOC_CON2		0x0008
#define SGRF_SOC_CON3		0x000C
#define SGRF_SOC_CON4		0x0010
#define SGRF_SOC_CON5		0x0014
#define SGRF_SOC_CON6		0x0018
#define SGRF_SOC_CON7		0x001C
#define SGRF_SOC_CON8		0x0020
#define SGRF_SOC_CON9		0x0024
#define SGRF_SOC_CON10		0x0028



/* bus dma control */
#define SGRF_BUSDMAC1_CON0	0x0100
#define SGRF_BUSDMAC1_CON1	0x0104
#define SGRF_BUSDMAC1_CON2	0x0108

#define SGRF_BUSDMAC2_CON0	0x0110
#define SGRF_BUSDMAC2_CON1	0x0114
#define SGRF_BUSDMAC2_CON2	0x0118


/* secure grf fast boot address */
#define SGRF_FAST_BOOT_ADDR	0x0180


/* secure efuse control */
#define SGRF_EFUSE_PRG_MASK 	0x0200
#define SGRF_EFUSE_READ_MASK 	0x0204


#endif /* __RK3366_GRF_H */
