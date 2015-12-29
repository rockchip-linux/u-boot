/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3288_PMU_H
#define __RK3288_PMU_H

#include <asm/io.h>

/*****cru reg offset*****/
#define PMU_WAKEUP_CFG0		0x00
#define PMU_WAKEUP_CFG1		0x04
#define PMU_PWRDN_CON		0x08
#define PMU_PWRDN_ST		0x0C
#define PMU_IDLE_REQ		0x10
#define PMU_IDLE_ST		0x14
#define PMU_PWRMODE_CON		0x18
#define PMU_PWR_STATE		0x1C
#define PMU_OSC_CNT		0x20
#define PMU_PLL_CNT		0x24
#define PMU_STABL_CNT		0x28
#define PMU_DDR0IO_PWRON_CNT	0x2C
#define PMU_DDR1IO_PWRON_CNT	0x30
#define PMU_CORE_PWRDWN_CNT	0x34
#define PMU_CORE_PWRUP_CNT	0x38
#define PMU_GPU_PWRDWN_CNT	0x3C
#define PMU_GPU_PWRUP_CNT	0x40
#define PMU_WAKEUP_RST_CLR_CNT	0x44
#define PMU_SFT_CON		0x48
#define PMU_DDR_SREF_ST		0x4C
#define PMU_INT_CON		0x50
#define PMU_INT_ST		0x54
#define PMU_BOOT_ADDR_SEL	0x58
#define PMU_GRF_CON		0x5C
#define PMU_GPIO_SR		0x60
#define PMU_GPIO0A_PULL		0x64
#define PMU_GPIO0B_PULL		0x68
#define PMU_GPIO0C_PULL		0x6C
#define PMU_GPIO0A_DRV		0x70
#define PMU_GPIO0B_DRV		0x74
#define PMU_GPIO0C_DRV		0x78
#define PMU_GPIO_OP		0x7C
#define PMU_GPIO0_SEL18		0x80
#define PMU_GPIO0A_IOMUX	0x84
#define PMU_GPIO0B_IOMUX	0x88
#define PMU_GPIO0C_IOMUX	0x8C
#define PMU_GPIO0D_IOMUX	0x90
#define PMU_SYS_REG0		0x94
#define PMU_SYS_REG1		0x98
#define PMU_SYS_REG2		0x9C
#define PMU_SYS_REG3		0xA0


enum pmu_power_domain {
	PD_A12_0 = 0,
	PD_A12_1,
	PD_A12_2,
	PD_A12_3,
	PD_BUS = 5,
	PD_PERI,
	PD_VIO,
	PD_VIDEO,
	PD_GPU,
	PD_SCU = 11,
	PD_HEVC = 14,
};

enum pmu_power_domain_st {
	PDST_A12_0 = 0,
	PDST_A12_1,
	PDST_A12_2,
	PDST_A12_3,
	PDST_BUS = 5,
	PDST_PERI,
	PDST_VIO,
	PDST_VIDEO,
	PDST_GPU,
	PDST_HEVC,
	PDST_SCU,
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

enum pmu_idle_req {
	IDLE_REQ_BUS = 0,
	IDLE_REQ_PERI,
	IDLE_REQ_GPU,
	IDLE_REQ_VIDEO,
	IDLE_REQ_VIO,
	IDLE_REQ_CORE,
	IDLE_REQ_ALIVE,
	IDLE_REQ_DMA,
	IDLE_REQ_CPUP,
	IDLE_REQ_HEVC
};

static inline void pmu_set_idle_request(enum pmu_idle_req req, bool idle)
{
	uint32 con;

	con = readl(RKIO_PMU_PHYS + PMU_IDLE_REQ);
	if (idle != 0) {
		con |= (1 << req);
		writel(con, RKIO_PMU_PHYS + PMU_IDLE_REQ);
	} else {
		con &= ~(1 << req);
		writel(con, RKIO_PMU_PHYS + PMU_IDLE_REQ);
	}
}

#endif /* __RK3288_PMU_H */
