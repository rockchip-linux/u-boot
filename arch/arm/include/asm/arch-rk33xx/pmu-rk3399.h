/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3399_PMU_H
#define __RK3399_PMU_H

#include <asm/io.h>


#define pmugrf_readl(offset)		readl(RKIO_PMU_GRF_PHYS + offset)
#define pmugrf_writel(v, offset)	do { writel(v, RKIO_PMU_GRF_PHYS + offset); } while (0)


/* PMU grf */
#define PMU_GRF_GPIO0A_IOMUX	0x0000
#define PMU_GRF_GPIO0B_IOMUX	0x0004

#define PMU_GRF_GPIO1A_IOMUX	0x0010
#define PMU_GRF_GPIO1B_IOMUX	0x0014
#define PMU_GRF_GPIO1C_IOMUX	0x0018
#define PMU_GRF_GPIO1D_IOMUX	0x001C

#define PMU_GRF_GPIO0A_P	0x0040
#define PMU_GRF_GPIO0B_P	0x0044

#define PMU_GRF_GPIO1A_P	0x0050
#define PMU_GRF_GPIO1B_P	0x0054
#define PMU_GRF_GPIO1C_P	0x0058
#define PMU_GRF_GPIO1D_P	0x005C

#define PMU_GRF_SOC_CON0	0x0180
#define PMU_GRF_SOC_CON10	0x01A8
#define PMU_GRF_SOC_CON11	0x01AC

#define PMU_GRF_OS_REG0		0x0300
#define PMU_GRF_OS_REG1		0x0304
#define PMU_GRF_OS_REG2		0x0308
#define PMU_GRF_OS_REG3		0x030C


#endif /* __RK3399_PMU_H */
