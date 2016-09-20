/*
 * (C) Copyright 2016 Fuzhou Rockchip Electronics Co., Ltd
 * William Zhang, SoftWare Engineering, <william.zhang@rock-chips.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RK322XH_PMU_H
#define __RK322XH_PMU_H

#include <asm/io.h>


#define pmugrf_readl(offset)		readl(RKIO_PMU_GRF_PHYS + offset)
#define pmugrf_writel(v, offset)	do { writel(v, RKIO_PMU_GRF_PHYS + offset); } while (0)


/* PMU pmu */
#define PMU_WAKEUP_CFG0		0x0000
#define PMU_PWRDN_CON		0x000C
#define PMU_PWRDN_ST		0x0010
#define PMU_PWRMODE_COMMON_CON	0x0018
#define PMU_SFT_CON		0x001C
#define PMU_INT_CON		0x0020
#define PMU_INT_ST		0x0024
#define PMU_PWOER_ST		0x0044
#define PMU_CPU0_APM_CON	0x0080
#define PMU_CPU1_APM_CON	0x0084
#define PMU_CPU2_APM_CON	0x0088
#define PMU_CPU3_APM_CON	0x008c
#define PMU_SYS_REG0		0x00a0
#define PMU_SYS_REG1		0x00a4
#define PMU_SYS_REG2		0x00a8
#define PMU_SYS_REG3		0x00ac


enum pmu_power_domain {
	PD_A53_0 = 0,
	PD_A53_1,
	PD_A53_2,
	PD_A53_3,
};

enum pmu_power_domain_st {
	PDST_A53_0 = 0,
	PDST_A53_1,
	PDST_A53_2,
	PDST_A53_3,
};

static inline bool pmu_power_domain_is_on(enum pmu_power_domain_st pd)
{
	return !(readl(RKIO_PMU_PHYS + PMU_PWRDN_ST) & (1 << pd));
}

static inline void pmu_set_power_domain(enum pmu_power_domain pd)
{
	uint32 con;

	con = readl(RKIO_PMU_PHYS + PMU_PWRDN_CON);
	con |= (1 << pd);
	writel(con, RKIO_PMU_PHYS + PMU_PWRDN_CON);
}


#endif /* __RK322XH_PMU_H */
