/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK322X_GRF_H
#define __RK322X_GRF_H

#include <asm/io.h>

/* gpio iomux control */
#define GRF_GPIO0A_IOMUX        0x0000
#define GRF_GPIO0B_IOMUX        0x0004
#define GRF_GPIO0C_IOMUX        0x0008
#define GRF_GPIO0D_IOMUX        0x000C
#define GRF_GPIO1A_IOMUX        0x0010
#define GRF_GPIO1B_IOMUX        0x0014
#define GRF_GPIO1C_IOMUX        0x0018
#define GRF_GPIO1D_IOMUX        0x001C
#define GRF_GPIO2A_IOMUX        0x0020
#define GRF_GPIO2B_IOMUX        0x0024
#define GRF_GPIO2C_IOMUX        0x0028
#define GRF_GPIO2D_IOMUX        0x002C
#define GRF_GPIO3A_IOMUX        0x0030
#define GRF_GPIO3B_IOMUX        0x0034
#define GRF_GPIO3C_IOMUX        0x0038
#define GRF_GPIO3D_IOMUX        0x003C

#define GRF_COM_IOMUX           0x0050


/* gpio pull down/up control */
#define GRF_GPIO0A_P		0x0100
#define GRF_GPIO0B_P		0x0104
#define GRF_GPIO0C_P		0x0108
#define GRF_GPIO0D_P		0x010C
#define GRF_GPIO1A_P		0x0110
#define GRF_GPIO1B_P		0x0114
#define GRF_GPIO1C_P		0x0118
#define GRF_GPIO1D_P		0x011C
#define GRF_GPIO2A_P		0x0120
#define GRF_GPIO2B_P		0x0124
#define GRF_GPIO2C_P		0x0128
#define GRF_GPIO2D_P		0x012C
#define GRF_GPIO3A_P		0x0130
#define GRF_GPIO3B_P		0x0134
#define GRF_GPIO3C_P		0x0138
#define GRF_GPIO3D_P		0x013C


/* gpio drive strength control */
#define GRF_GPIO0A_E		0x0200
#define GRF_GPIO0B_E		0x0204
#define GRF_GPIO0C_E		0x0208
#define GRF_GPIO0D_E		0x020C
#define GRF_GPIO1A_E		0x0210
#define GRF_GPIO1B_E		0x0214
#define GRF_GPIO1C_E		0x0218
#define GRF_GPIO1D_E		0x021C
#define GRF_GPIO2A_E		0x0220
#define GRF_GPIO2B_E		0x0224
#define GRF_GPIO2C_E		0x0228
#define GRF_GPIO2D_E		0x022C
#define GRF_GPIO3A_E		0x0230
#define GRF_GPIO3B_E		0x0234
#define GRF_GPIO3C_E		0x0238
#define GRF_GPIO3D_E		0x023C


/* gpio sr control */
#define GRF_GPIO0L_SR		0x0300
#define GRF_GPIO0H_SR		0x0304
#define GRF_GPIO1L_SR		0x0308
#define GRF_GPIO1H_SR		0x030C
#define GRF_GPIO2L_SR		0x0310
#define GRF_GPIO2H_SR		0x0314
#define GRF_GPIO3L_SR		0x0318
#define GRF_GPIO3H_SR		0x031C

/* gpio smitter control */
#define GRF_GPIO0L_SMT		0x0380
#define GRF_GPIO0H_SMT		0x0384
#define GRF_GPIO1L_SMT		0x0388
#define GRF_GPIO1H_SMT		0x038C
#define GRF_GPIO2L_SMT		0x0390
#define GRF_GPIO2H_SMT		0x0394
#define GRF_GPIO3L_SMT		0x0398
#define GRF_GPIO3H_SMT		0x039C

/* Soc control */
#define GRF_SOC_CON0		0x0400
#define GRF_SOC_CON1		0x0404
#define GRF_SOC_CON2		0x0408
#define GRF_SOC_CON3		0x040C
#define GRF_SOC_CON4		0x0410
#define GRF_SOC_CON5		0x0414
#define GRF_SOC_CON6		0x0418

/* Soc status */
#define GRF_SOC_STATUS0		0x0480
#define GRF_SOC_STATUS1		0x0484
#define GRF_SOC_STATUS2		0x0488


/* cpu control */
#define GRF_CPU_CON0		0x0500
#define GRF_CPU_CON1		0x0504
#define GRF_CPU_CON2		0x0508
#define GRF_CPU_CON3		0x050C


/* cpu status */
#define GRF_CPU_STATUS0		0x0520
#define GRF_CPU_STATUS1		0x0524

/* grf os register */
#define GRF_OS_REG0		0x05C8
#define GRF_OS_REG1		0x05CC
#define GRF_OS_REG2		0x05D0
#define GRF_OS_REG3		0x05D4
#define GRF_OS_REG4		0x05D8
#define GRF_OS_REG5		0x05DC
#define GRF_OS_REG6		0x05E0
#define GRF_OS_REG7		0x05E4


#define GRF_DDRC_STAT           0x0604


#define GRF_SIG_DETECT_CON0	0x0680
#define GRF_SIG_DETECT_CON1	0x0684

#define GRF_SIG_DETECT_STATUS0  0x0690
#define GRF_SIG_DETECT_STATUS1  0x0694

#define GRF_SIG_DETECT_CLR0     0x06A0
#define GRF_SIG_DETECT_CLR1     0x06A4

#define GRF_MMC_DET             0x06B0

#define GRF_HOST0_CON0          0x0700
#define GRF_HOST0_CON1          0x0704
#define GRF_HOST0_CON2          0x0708

#define GRF_HOST1_CON0          0x0710
#define GRF_HOST1_CON1          0x0714
#define GRF_HOST1_CON2          0x0718

#define GRF_HOST2_CON0          0x0720
#define GRF_HOST2_CON1          0x0724
#define GRF_HOST2_CON2          0x0728


#define GRF_USBPHY0_CON0        0x0760
#define GRF_USBPHY0_CON1        0x0764
#define GRF_USBPHY0_CON2        0x0768
#define GRF_USBPHY0_CON3        0x076C
#define GRF_USBPHY0_CON4        0x0770
#define GRF_USBPHY0_CON5        0x0774
#define GRF_USBPHY0_CON6        0x0778
#define GRF_USBPHY0_CON7        0x077C
#define GRF_USBPHY0_CON8        0x0780
#define GRF_USBPHY0_CON9        0x0784
#define GRF_USBPHY0_CON10       0x0788
#define GRF_USBPHY0_CON11       0x078C
#define GRF_USBPHY0_CON12       0x0790
#define GRF_USBPHY0_CON13       0x0794
#define GRF_USBPHY0_CON14       0x0798
#define GRF_USBPHY0_CON15       0x079C
#define GRF_USBPHY0_CON16       0x07A0
#define GRF_USBPHY0_CON17       0x07A4
#define GRF_USBPHY0_CON18       0x07A8
#define GRF_USBPHY0_CON19       0x07AC
#define GRF_USBPHY0_CON20       0x07B0
#define GRF_USBPHY0_CON21       0x07B4
#define GRF_USBPHY0_CON22       0x07B8
#define GRF_USBPHY0_CON23       0x07BC
#define GRF_USBPHY0_CON24       0x07C0
#define GRF_USBPHY0_CON25       0x07C4
#define GRF_USBPHY0_CON26       0x07C8


#define GRF_USBPHY1_CON0        0x0800
#define GRF_USBPHY1_CON1        0x0804
#define GRF_USBPHY1_CON2        0x0808
#define GRF_USBPHY1_CON3        0x080C
#define GRF_USBPHY1_CON4        0x0810
#define GRF_USBPHY1_CON5        0x0814
#define GRF_USBPHY1_CON6        0x0818
#define GRF_USBPHY1_CON7        0x081C
#define GRF_USBPHY1_CON8        0x0820
#define GRF_USBPHY1_CON9        0x0824
#define GRF_USBPHY1_CON10       0x0828
#define GRF_USBPHY1_CON11       0x082C
#define GRF_USBPHY1_CON12       0x0830
#define GRF_USBPHY1_CON13       0x0834
#define GRF_USBPHY1_CON14       0x0838
#define GRF_USBPHY1_CON15       0x083C
#define GRF_USBPHY1_CON16       0x0840
#define GRF_USBPHY1_CON17       0x0844
#define GRF_USBPHY1_CON18       0x0848
#define GRF_USBPHY1_CON19       0x084C
#define GRF_USBPHY1_CON20       0x0850
#define GRF_USBPHY1_CON21       0x0854
#define GRF_USBPHY1_CON22       0x0858
#define GRF_USBPHY1_CON23       0x085C
#define GRF_USBPHY1_CON24       0x0860
#define GRF_USBPHY1_CON25       0x0864
#define GRF_USBPHY1_CON26       0x0868

#define GRF_UOC_CON             0x0880
#define GRF_UOC_STATUS0         0x0884


#define GRF_MAC_CON0		0x0900
#define GRF_MAC_CON1		0x0904


/* dmac control */
#define GRF_DMAC_CON0           0x0B00
#define GRF_DMAC_CON1           0x0B04
#define GRF_DMAC_CON2           0x0B08
#define GRF_DMAC_CON3           0x0B0C


#define GRF_MACPHY_STATUS       0x0B10


#define SGRF_SOC_CON0                   0x0000
#define SGRF_SOC_CON1                   0x0004
#define SGRF_SOC_CON2                   0x0008
#define SGRF_SOC_CON3                   0x000C
#define SGRF_SOC_CON4                   0x0010
#define SGRF_SOC_CON5                   0x0014
#define SGRF_SOC_CON6                   0x0018
#define SGRF_SOC_CON7                   0x001C
#define SGRF_SOC_CON8                   0x0020
#define SGRF_SOC_CON9                   0x0024
#define SGRF_SOC_CON10                  0x0028

#define SGRF_BUSDMAC_CON0               0x0100
#define SGRF_BUSDMAC_CON1               0x0104
#define SGRF_BUSDMAC_CON2               0x0108
#define SGRF_BUSDMAC_CON3               0x010C

#define SGRF_FAST_BOOT_ADDR             0x0180

#define SGRF_EFUSE_PRG_MASK             0x0200

#define SGRF_HDCP_KEY_REG0              0x0280
#define SGRF_HDCP_KEY_REG1              0x0284
#define SGRF_HDCP_KEY_REG2              0x0288
#define SGRF_HDCP_KEY_REG3              0x028c
#define SGRF_HDCP_KEY_REG4              0x0290
#define SGRF_HDCP_KEY_REG5              0x0294
#define SGRF_HDCP_KEY_REG6              0x0298
#define SGRF_HDCP_KEY_REG7              0x029c

#define SGRF_HDCP_KEY_ACCESS_MASK       0x02a0


#define grf_readl(offset)	readl(RKIO_GRF_PHYS + offset)
#define grf_writel(v, offset)	do { writel(v, RKIO_GRF_PHYS + offset); } while (0)


#endif /* __RK322X_GRF_H */
