/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3368_GRF_H
#define __RK3368_GRF_H

#include <asm/io.h>


/* gpio iomux control */
#define GRF_GPIO1A_IOMUX        0x0000
#define GRF_GPIO1B_IOMUX        0x0004
#define GRF_GPIO1C_IOMUX        0x0008
#define GRF_GPIO1D_IOMUX        0x000c
#define GRF_GPIO2A_IOMUX        0x0010
#define GRF_GPIO2B_IOMUX        0x0014
#define GRF_GPIO2C_IOMUX        0x0018
#define GRF_GPIO2D_IOMUX        0x001c
#define GRF_GPIO3A_IOMUX        0x0020
#define GRF_GPIO3B_IOMUX        0x0024
#define GRF_GPIO3C_IOMUX        0x0028
#define GRF_GPIO3D_IOMUX        0x002c


/* gpio pull down/up control */
#define GRF_GPIO1A_P		0x0100
#define GRF_GPIO1B_P		0x0104
#define GRF_GPIO1C_P		0x0108
#define GRF_GPIO1D_P		0x010C
#define GRF_GPIO2A_P		0x0110
#define GRF_GPIO2B_P		0x0114
#define GRF_GPIO2C_P		0x0118
#define GRF_GPIO2D_P		0x011C
#define GRF_GPIO3A_P		0x0120
#define GRF_GPIO3B_P		0x0124
#define GRF_GPIO3C_P		0x0128
#define GRF_GPIO3D_P		0x012C


/* gpio drive strength control */
#define GRF_GPIO1A_E		0x0200
#define GRF_GPIO1B_E		0x0204
#define GRF_GPIO1C_E		0x0208
#define GRF_GPIO1D_E		0x020C
#define GRF_GPIO2A_E		0x0210
#define GRF_GPIO2B_E		0x0214
#define GRF_GPIO2C_E		0x0218
#define GRF_GPIO2D_E		0x021C
#define GRF_GPIO3A_E		0x0220
#define GRF_GPIO3B_E		0x0224
#define GRF_GPIO3C_E		0x0228
#define GRF_GPIO3D_E		0x022C


/* gpio sr control */
#define GRF_GPIO1L_SR		0x0300
#define GRF_GPIO1H_SR		0x0304
#define GRF_GPIO2L_SR		0x0308
#define GRF_GPIO2H_SR		0x030C
#define GRF_GPIO3L_SR		0x0310
#define GRF_GPIO3H_SR		0x0314


/* gpio smitter control */
#define GRF_GPIO_SMT		0x0380


/* Soc control part 1 */
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
#define GRF_SOC_CON17		0x0444


/* Soc status */
#define GRF_SOC_STATUS0		0x0480
#define GRF_SOC_STATUS1		0x0484
#define GRF_SOC_STATUS2		0x0488
#define GRF_SOC_STATUS3		0x048C
#define GRF_SOC_STATUS4		0x0490
#define GRF_SOC_STATUS5		0x0494
#define GRF_SOC_STATUS6		0x0498
#define GRF_SOC_STATUS7		0x049C
#define GRF_SOC_STATUS8		0x04A0
#define GRF_SOC_STATUS9		0x04A4
#define GRF_SOC_STATUS10	0x04A8
#define GRF_SOC_STATUS11	0x04AC
#define GRF_SOC_STATUS12	0x04B0
#define GRF_SOC_STATUS13	0x04B4
#define GRF_SOC_STATUS14	0x04B8
#define GRF_SOC_STATUS15	0x04BC


/* cpu control */
#define GRF_CPU_CON0		0x0500
#define GRF_CPU_CON1		0x0504
#define GRF_CPU_CON2		0x0508
#define GRF_CPU_CON3		0x050C


/* cpu status */
#define GRF_CPU_STATUS0		0x0520
#define GRF_CPU_STATUS1		0x0524


/* CCI status */
#define GRF_CCI_STATUS0		0x0540
#define GRF_CCI_STATUS1		0x0544
#define GRF_CCI_STATUS2		0x0548
#define GRF_CCI_STATUS3		0x054C
#define GRF_CCI_STATUS4		0x0550
#define GRF_CCI_STATUS5		0x0554
#define GRF_CCI_STATUS6		0x0558
#define GRF_CCI_STATUS7		0x055C
#define GRF_CCI_STATUS8		0x0560
#define GRF_CCI_STATUS9		0x0564
#define GRF_CCI_STATUS10	0x0568
#define GRF_CCI_STATUS11	0x056C
#define GRF_CCI_STATUS12	0x0570
#define GRF_CCI_STATUS13	0x0574
#define GRF_CCI_STATUS14	0x0578
#define GRF_CCI_STATUS15	0x057C


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

#define GRF_UOC3_CON0		0x0738
#define GRF_UOC3_CON1		0x073C

#define GRF_UOC4_CON0		0x0740
#define GRF_UOC4_CON1		0x0744


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


/* PVT monitor control */
#define GRF_PVTM_CON0		0x0800
#define GRF_PVTM_CON1		0x0804
#define GRF_PVTM_CON2		0x0808


/* PVT monitor status */
#define GRF_PVTM_STATUS0	0x080C
#define GRF_PVTM_STATUS1	0x0810
#define GRF_PVTM_STATUS2	0x0814


/* IO Voltage select */
#define GRF_IO_VSEL		0x0900


/* saradc test bit */
#define GRF_SARADC_TESTBIT	0x0904


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
#define SGRF_SOC_CON11		0x002C
#define SGRF_SOC_CON12		0x0030
#define SGRF_SOC_CON13		0x0034
#define SGRF_SOC_CON14		0x0038
#define SGRF_SOC_CON15		0x003C
#define SGRF_SOC_CON16		0x0040
#define SGRF_SOC_CON17		0x0044
#define SGRF_SOC_CON18		0x0048
#define SGRF_SOC_CON19		0x004C
#define SGRF_SOC_CON20		0x0050
#define SGRF_SOC_CON21		0x0054
#define SGRF_SOC_CON22		0x0058
#define SGRF_SOC_CON23		0x005C
#define SGRF_SOC_CON24		0x0060
#define SGRF_SOC_CON25		0x0064
#define SGRF_SOC_CON26		0x0068
#define SGRF_SOC_CON27		0x006C
#define SGRF_SOC_CON28		0x0070
#define SGRF_SOC_CON29		0x0074
#define SGRF_SOC_CON30		0x0078
#define SGRF_SOC_CON31		0x007C
#define SGRF_SOC_CON32		0x0080
#define SGRF_SOC_CON33		0x0084
#define SGRF_SOC_CON34		0x0088
#define SGRF_SOC_CON35		0x008C
#define SGRF_SOC_CON36		0x0090
#define SGRF_SOC_CON37		0x0094
#define SGRF_SOC_CON38		0x0098
#define SGRF_SOC_CON39		0x009C


/* bus dma control */
#define SGRF_BUSDMAC_CON0	0x0100
#define SGRF_BUSDMAC_CON1	0x0104
#define SGRF_BUSDMAC_CON2	0x0108


/* secure grf fast boot address */
#define SGRF_FAST_BOOT_ADDR	0x0180


/* secure efuse control */
#define SGRF_EFUSE_CON		0x0200


#endif /* __RK3368_GRF_H */
