/*
 * (C) Copyright 2016 Fuzhou Rockchip Electronics Co., Ltd
 * William Zhang, SoftWare Engineering, <william.zhang@rock-chips.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RK3228H_GRF_H
#define __RK3228H_GRF_H

#include <asm/io.h>

/* gpio iomux control */
#define GRF_GPIO0A_IOMUX        0x0000
#define GRF_GPIO0B_IOMUX        0x0004
#define GRF_GPIO0C_IOMUX        0x0008
#define GRF_GPIO0D_IOMUX        0x000c
#define GRF_GPIO1A_IOMUX        0x0010
#define GRF_GPIO1B_IOMUX        0x0014
#define GRF_GPIO1C_IOMUX        0x0018
#define GRF_GPIO1D_IOMUX        0x001c
#define GRF_GPIO2A_IOMUX        0x0020
#define GRF_GPIO2BL_IOMUX       0x0024
#define GRF_GPIO2BH_IOMUX       0x0028
#define GRF_GPIO2CL_IOMUX       0x002c
#define GRF_GPIO2CH_IOMUX       0x0030
#define GRF_GPIO2D_IOMUX        0x0034
#define GRF_GPIO3AL_IOMUX       0x0038
#define GRF_GPIO3AH_IOMUX       0x003c
#define GRF_GPIO3BL_IOMUX       0x0040
#define GRF_GPIO3BH_IOMUX       0x0044
#define GRF_GPIO3C_IOMUX        0x0048
#define GRF_GPIO3D_IOMUX        0x004c
#define GRF_COM_IOMUX           0x0050


/* gpio pull down/up control */
#define GRF_GPIO0A_P		0x0100
#define GRF_GPIO0B_P		0x0104
#define GRF_GPIO0C_P		0x0108
#define GRF_GPIO0D_P		0x010c
#define GRF_GPIO1A_P		0x0110
#define GRF_GPIO1B_P		0x0114
#define GRF_GPIO1C_P		0x0118
#define GRF_GPIO1D_P		0x011c
#define GRF_GPIO2A_P		0x0120
#define GRF_GPIO2B_P		0x0124
#define GRF_GPIO2C_P		0x0128
#define GRF_GPIO2D_P		0x012c
#define GRF_GPIO3A_P		0x0130
#define GRF_GPIO3B_P		0x0134
#define GRF_GPIO3C_P		0x0138
#define GRF_GPIO3D_P		0x013c


/* gpio drive strength control */
#define GRF_GPIO0A_E		0x0200
#define GRF_GPIO0B_E		0x0204
#define GRF_GPIO0C_E		0x0208
#define GRF_GPIO0D_E		0x020c
#define GRF_GPIO1A_E		0x0210
#define GRF_GPIO1B_E		0x0214
#define GRF_GPIO1C_E		0x0218
#define GRF_GPIO1D_E		0x021c
#define GRF_GPIO2A_E		0x0220
#define GRF_GPIO2B_E		0x0224
#define GRF_GPIO2C_E		0x0228
#define GRF_GPIO2D_E		0x022c
#define GRF_GPIO3A_E		0x0230
#define GRF_GPIO3B_E		0x0234
#define GRF_GPIO3C_E		0x0238
#define GRF_GPIO3D_E		0x023c


/* gpio sr control */
#define GRF_GPIO0L_SR		0x0300
#define GRF_GPIO0H_SR		0x0304
#define GRF_GPIO1L_SR		0x0308
#define GRF_GPIO1H_SR		0x030c
#define GRF_GPIO2L_SR		0x0310
#define GRF_GPIO2H_SR		0x0314
#define GRF_GPIO3L_SR		0x0318
#define GRF_GPIO3H_SR		0x031c


/* gpio smitter control */
#define GRF_GPIO0L_SMT		0x0380
#define GRF_GPIO0H_SMT		0x0384
#define GRF_GPIO1L_SMT		0x0388
#define GRF_GPIO1H_SMT		0x038c
#define GRF_GPIO2L_SMT		0x0390
#define GRF_GPIO2H_SMT		0x0394
#define GRF_GPIO3L_SMT		0x0398
#define GRF_GPIO3H_SMT		0x039c


/* Soc control part 1 */
#define GRF_SOC_CON0		0x0400
#define GRF_SOC_CON1		0x0404
#define GRF_SOC_CON2		0x0408
#define GRF_SOC_CON3		0x040c
#define GRF_SOC_CON4		0x0410
#define GRF_SOC_CON5		0x0414
#define GRF_SOC_CON6		0x0418
#define GRF_SOC_CON7		0x041c
#define GRF_SOC_CON8		0x0420
#define GRF_SOC_CON9		0x0424
#define GRF_SOC_CON10		0x0428


/* Soc status */
#define GRF_SOC_STATUS0		0x0480
#define GRF_SOC_STATUS1		0x0484
#define GRF_SOC_STATUS2		0x0488
#define GRF_SOC_STATUS3		0x048c
#define GRF_SOC_STATUS4		0x0490


/* USB3.0 OTG control */
#define GRF_USB3OTG_CON0	0x04c0
#define GRF_USB3OTG_CON1	0x04c4


/* cpu control */
#define GRF_CPU_CON0		0x0500
#define GRF_CPU_CON1		0x0504


/* cpu status */
#define GRF_CPU_STATUS0		0x0520
#define GRF_CPU_STATUS1		0x0524


/* os reg */
#define GRF_OS_REG0		0x05c8
#define GRF_OS_REG1		0x05cc
#define GRF_OS_REG2		0x05d0
#define GRF_OS_REG3		0x05d4
#define GRF_OS_REG4		0x05d8
#define GRF_OS_REG5		0x05dc
#define GRF_OS_REG6		0x05e0
#define GRF_OS_REG7		0x05e4


/* sig detect */
#define GRF_SIG_DETECT_CON	0x0680
#define GRF_SIG_DETECT_STATUS	0x0690
#define GRF_SIG_DETECT_CLR	0x06a0
#define GRF_SDMMC_DET_COUNTER	0x06b0


/* usb host0 control */
#define GRF_HOST0_CON0		0x0700
#define GRF_HOST0_CON1		0x0704
#define GRF_HOST0_CON2		0x0708


/* usb2.0 otg control */
#define GRF_OTG_CON0		0x0880


/* usb host0 status */
#define GRF_HOST0_STATUS	0x0890


/* mac control */
#define GRF_MAC_CON0		0x0900
#define GRF_MAC_CON1		0x0904
#define GRF_MAC_CON2		0x0908


/* mac phy control */
#define GRF_MACPHY_CON0		0x0b00
#define GRF_MACPHY_CON1		0x0b04
#define GRF_MACPHY_CON2		0x0b08
#define GRF_MACPHY_CON3		0x0b0c
#define GRF_MACPHY_STATUS	0x0b10




/* secure grf soc control part 1 */
#define SGRF_SOC_CON0		0x0000
#define SGRF_SOC_CON1		0x0004
#define SGRF_SOC_CON2		0x0008
#define SGRF_SOC_CON3		0x000C
#define SGRF_SOC_CON4		0x0010
#define SGRF_SOC_CON5		0x0014


/* bus dma control */
#define SGRF_BUSDMAC_CON0	0x0100
#define SGRF_BUSDMAC_CON1	0x0104
#define SGRF_BUSDMAC_CON2	0x0108
#define SGRF_BUSDMAC_CON3	0x010c
#define SGRF_BUSDMAC_CON4	0x0110
#define SGRF_BUSDMAC_CON5	0x0114


/* secure grf fast boot address */
#define SGRF_FAST_BOOT_ADDR	0x0180


/* secure efuse control */
#define SGRF_EFUSE_CON		0x0200


/* hdcp key reg */
#define SGRF_HDCP_KEY_REG0	0x0280
#define SGRF_HDCP_KEY_REG1	0x0284
#define SGRF_HDCP_KEY_REG2	0x0288
#define SGRF_HDCP_KEY_REG3	0x028c
#define SGRF_HDCP_KEY_REG4	0x0290
#define SGRF_HDCP_KEY_REG5	0x0294
#define SGRF_HDCP_KEY_REG6	0x0298
#define SGRF_HDCP_KEY_REG7	0x029c
#define SGRF_HDCP_KEY_ACCESS_MASK	0x02a0




/* ddr grf soc control part */
#define	DDR_GRF_CON(i)		(0x4 * (i))
#define	DDR_GRF_STATUS(i)	(0x0100 + 0x4 * (i))




/* usb2phy grf soc control part */
#define	USB2PHY_GRF_REG(i)		(0x4 * (i))
#define	USB2PHY_GRF_CON(i)		(0x0100 + 0x4 * (i))
#define	USB2PHY_GRF_SIG_DETECT_CON0	0x0110




/* usb3phy grf soc control part */
#define	USB3PHY_GRF_CON(i)		(0x4 * (i))
#define	USB3PHY_GRF_SIG_DETECT_CON0	0x28
#define	USB3PHY_GRF_STATUS1		0x34
#define	USB3PHY_GRF_WAKEUP_CON0		0x40


#endif /* __RK3228H_GRF_H */
