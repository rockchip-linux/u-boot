/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3368_PMU_H
#define __RK3368_PMU_H

#include <asm/io.h>


#define pmugrf_readl(offset)		readl(RKIO_PMU_GRF_PHYS + offset)
#define pmugrf_writel(v, offset)	do { writel(v, RKIO_PMU_GRF_PHYS + offset); } while (0)


/* PMU pmu */
#define PMU_WAKEUP_CFG0		0x0000
#define PMU_WAKEUP_CFG1		0x0004
#define PMU_WAKEUP_CFG2		0x0008
#define PMU_PWRDN_CON		0x000C
#define PMU_PWRDN_ST		0x0010
#define PMU_PWRMODE_CORE_CON	0x0014
#define PMU_PWRMODE_COMMON_CON	0x0018
#define PMU_SFT_CON		0x001C
#define PMU_INT_CON		0x0020
#define PMU_INT_ST		0x0024
#define PMU_GPIO_POS_INT_CON	0x0028
#define PMU_GPIO_NEG_INT_CON	0x002C
#define PMU_GPIO_POS_INT_ST	0x0030
#define PMU_GPIO_NEG_INT_ST	0x0034
#define PMU_CORE_PWR_ST		0x0038
#define PMU_BUS_IDLE_REQ	0x003C
#define PMU_BUS_IDLE_ST		0x0040
#define PMU_PWOER_ST		0x0044
#define PMU_OSC_CNT		0x0048
#define PMU_PLLLOCK_CNT		0x004C
#define PMU_PLLRST_CNT		0x0050
#define PMU_STABLE_CNT		0x0054
#define PMU_DDRIO_PWRON_CNT	0x0058
#define PMU_WAKEUP_RST_CLR_CNT	0x005C
#define PMU_DDR_SREF_ST		0x0060
#define PMU_SYS_REG0		0x0064
#define PMU_SYS_REG1		0x0068
#define PMU_SYS_REG2		0x006C
#define PMU_SYS_REG3		0x0070
#define PMU_GPU_APM_CON		0x0074
#define PMU_GPU_APM_CNT		0x0078
#define PMU_TIMEOUT_CNT		0x007C


/* PMU grf */
#define PMU_GRF_GPIO0A_IOMUX	0x0000
#define PMU_GRF_GPIO0B_IOMUX	0x0004
#define PMU_GRF_GPIO0C_IOMUX	0x0008
#define PMU_GRF_GPIO0D_IOMUX	0x000C
#define PMU_GRF_GPIO0A_P	0x0010
#define PMU_GRF_GPIO0B_P	0x0014
#define PMU_GRF_GPIO0C_P	0x0018
#define PMU_GRF_GPIO0D_P	0x001C
#define PMU_GRF_GPIO0A_E	0x0020
#define PMU_GRF_GPIO0B_E	0x0024
#define PMU_GRF_GPIO0C_E	0x0028
#define PMU_GRF_GPIO0D_E	0x002C
#define PMU_GRF_GPIO0L_SR	0x0030
#define PMU_GRF_GPIO0H_SR	0x0034

#define PMU_GRF_SOC_CON0	0x0100

#define PMU_GRF_PVTM_CON0	0x0180
#define PMU_GRF_PVTM_CON1	0x0184
#define PMU_GRF_PVTM_STATUS0	0x0190
#define PMU_GRF_PVTM_STATUS1	0x0194

#define PMU_GRF_OS_REG0		0x0200
#define PMU_GRF_OS_REG1		0x0204
#define PMU_GRF_OS_REG2		0x0208
#define PMU_GRF_OS_REG3		0x020C


enum pmu_power_domain {
	PD_A53_L0 = 0,
	PD_A53_L1,
	PD_A53_L2,
	PD_A53_L3,
	PD_SCU_L,
	PD_A53_B0,
	PD_A53_B1,
	PD_A53_B2,
	PD_A53_B3,
	PD_SCU_B,
	PD_CORE,
	PD_BUS = 12,
	PD_PERI,
	PD_VIDEO,
	PD_VIO,
	PD_GPU0,
	PD_GPU1,
};

enum pmu_power_domain_st {
	PDST_A53_L0 = 0,
	PDST_A53_L1,
	PDST_A53_L2,
	PDST_A53_L3,
	PDST_SCU_L,
	PDST_A53_B0,
	PDST_A53_B1,
	PDST_A53_B2,
	PDST_A53_B3,
	PDST_SCU_B,
	PDST_CORE,
	PDST_BUS,
	PDST_PERI,
	PDST_VIDEO,
	PDST_VIO,
	PDST_GPU0,
	PDST_GPU1,
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


#endif /* __RK3368_PMU_H */
