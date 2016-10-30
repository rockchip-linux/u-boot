/*
 * (C) Copyright 2015 Google, Inc
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <debug_uart.h>
#include <ram.h>
#include <syscon.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/periph.h>
#include <asm/arch/pmu_rk3288.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch/timer.h>
#include <asm/gpio.h>
#include <dm/pinctrl.h>

DECLARE_GLOBAL_DATA_PTR;

#define BOOTROM_CPU_ENTRY_ADDR		(0xff700000 + 0x8)
#define BOOTROM_SPIN_LOCK_ADDR		(0xff700000 + 0x4)
#define BOOTROM_SPIN_UNLOCK		0xdeadbeaf

#define PDST_A12_1	1
#define PDST_A12_2	2
#define PDST_A12_3	3

#define RKIO_PMU_PHYS 0xFF730000
#define PMU_PWRDN_CON 0x08
#define PMU_PWRDN_ST 0x0c


static inline bool pmu_power_domain_is_on(int pd) {
	return !(readl(RKIO_PMU_PHYS + PMU_PWRDN_ST) & (1 << pd));
}

static inline void pmu_set_power_domain(int pd) {
	uint32_t con;

	con = readl(RKIO_PMU_PHYS + PMU_PWRDN_CON);
	con |= (1 << pd);
	writel(con, RKIO_PMU_PHYS + PMU_PWRDN_CON);
}

unsigned long rk_get_cpuid(void)
{
	unsigned long mpidr;

	/* Read MPIDR */
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (mpidr));
	return mpidr & 0xf;
}

static int rk_load_uboot(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	unsigned long cpu_num = simple_strtoul(argv[1], NULL, 10);
	unsigned long load_addr = simple_strtoul(argv[2], NULL, 16);

	printf("cpuid = %d\n", (int) rk_get_cpuid());
	if (rk_get_cpuid() == 0) {
		if (cpu_num == 1) {
			printf("CPU0 Power Off CPU2 and CPU3.\n");
			pmu_set_power_domain(PDST_A12_2);
			do {} while (pmu_power_domain_is_on(PDST_A12_2));
			pmu_set_power_domain(PDST_A12_3);
			do {} while (pmu_power_domain_is_on(PDST_A12_3));
		} else if (cpu_num == 2) {
			printf("CPU0 Power Off CPU1 and CPU3.\n");
			pmu_set_power_domain(PDST_A12_1);
			do {} while (pmu_power_domain_is_on(PDST_A12_1));
			pmu_set_power_domain(PDST_A12_3);
			do {} while (pmu_power_domain_is_on(PDST_A12_3));
		} else if (cpu_num == 3) {
			printf("CPU0 Power Off CPU1 and CPU2.\n");
			pmu_set_power_domain(PDST_A12_1);
			do {} while (pmu_power_domain_is_on(PDST_A12_1));
			pmu_set_power_domain(PDST_A12_2);
			do {} while (pmu_power_domain_is_on(PDST_A12_2));
		}

		printf("PMU pwrdn CON = 0x%08x\n", (int) readl(RKIO_PMU_PHYS + PMU_PWRDN_CON));
		printf("PMU pwrdn State = 0x%08x\n", (int) readl(RKIO_PMU_PHYS + PMU_PWRDN_ST));

		printf("CPU0 Start wakeup CPU%d.\n", (int) cpu_num);
		/* wakeup others cpu from bootrom */
		writel(load_addr, BOOTROM_CPU_ENTRY_ADDR);
		writel(BOOTROM_SPIN_UNLOCK, BOOTROM_SPIN_LOCK_ADDR);
		asm volatile("dsb");
		asm volatile("sev");
	}

	return 0;
}

U_BOOT_CMD(
	rk_load_uboot, 3, 0, rk_load_uboot,
	"load uboot to a single cpu",
	"<cpu_num> 0x<address>"
);
