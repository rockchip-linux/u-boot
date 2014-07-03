/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef __RK312X_PMU_H
#define __RK312X_PMU_H

#include <asm/io.h>

/*****pmu reg offset*****/
#define PMU_WAKEUP_CFG		0x00
#define PMU_PWRDN_CON		0x04
#define PMU_PWRDN_ST		0x08
#define PMU_IDLE_REQ		0x0C
#define PMU_IDLE_ST		0x10
#define PMU_PWRMODE_CON		0x14
#define PMU_PWR_STATE		0x18
#define PMU_OSC_CNT		0x1C
#define PMU_CORE_PWRDWN_CNT	0x20
#define PMU_CORE_PWRUP_CNT	0x24
#define PMU_SFT_CON		0x28
#define PMU_DDR_SREF_ST		0x2C
#define PMU_INT_CON		0x30
#define PMU_INT_ST		0x34
#define PMU_SYS_REG0		0x38
#define PMU_SYS_REG1		0x3C
#define PMU_SYS_REG2		0x40
#define PMU_SYS_REG3		0x44


enum pmu_power_domain {
	PD_CORE = 0,
	PD_GPU,
	PD_VIDEO,
	PD_VIO
};

enum pmu_power_domain_st {
	PDST_CORE = 0,
	PDST_GPU,
	PDST_VIDEO,
	PDST_VIO
};

static inline bool pmu_power_domain_is_on(enum pmu_power_domain_st pd)
{
	return !(readl(RKIO_PMU_PHYS + PMU_PWRDN_ST) & (1 << pd));
}

static inline void pmu_set_power_domain(enum pmu_power_domain pd)
{
	writel((1<<pd), RKIO_PMU_PHYS + PMU_PWRDN_CON);
}

enum pmu_idle_req {
	IDLE_REQ_PERI = 0,
	IDLE_REQ_VIDEO,
	IDLE_REQ_VIO,
	IDLE_REQ_GPU,
	IDLE_REQ_CORE,
	IDLE_REQ_SYS,
	IDLE_REQ_MSCH,
	IDLE_REQ_CRYPTO
};

static inline void pmu_set_idle_request(enum pmu_idle_req req, bool idle)
{
	if (idle != 0) {
		writel((1<<req), RKIO_PMU_PHYS + PMU_IDLE_REQ);
	} else {
		writel((0<<req), RKIO_PMU_PHYS + PMU_IDLE_REQ);
	}
}

#endif /* __RK312X_PMU_H */

