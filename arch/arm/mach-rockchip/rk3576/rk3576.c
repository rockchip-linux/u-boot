// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 */
#include <amp.h>
#include <dm.h>
#include <fdt_support.h>
#include <string.h>
#include <scsi.h>
#include <tee/optee.h>
#include <linux/delay.h>
#include <asm/armv8/mmu.h>
#include <asm/arch-rockchip/bootrom.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/hardware.h>
#include <asm/arch-rockchip/smccc.h>
#include <asm/arch-rockchip/vendor.h>

DECLARE_GLOBAL_DATA_PTR;

#define SYS_GRF_BASE		0x2600A000
#define SYS_GRF_SOC_CON2	0x0008
#define SYS_GRF_SOC_CON7	0x001c
#define SYS_GRF_SOC_CON11	0x002c
#define SYS_GRF_SOC_CON12	0x0030

#define GPIO0_IOC_BASE		0x26040000
#define GPIO0A_IOMUX_SEL_H	0x0004
#define GPIO0B_PULL_L		0x0024
#define GPIO0B_IE_L		0x002C
#define TOP_IOC_BASE		0x26044000
#define GPIO1A_IOMUX_SEL_L	0x0020
#define GPIO1A_IOMUX_SEL_H	0x0024
#define GPIO1B_IOMUX_SEL_L	0x0028
#define GPIO1B_IOMUX_SEL_H	0x002c
#define GPIO1C_IOMUX_SEL_L	0x0030
#define GPIO1C_IOMUX_SEL_H	0x0034
#define GPIO1D_IOMUX_SEL_L	0x0038
#define GPIO1D_IOMUX_SEL_H	0x003c
#define GPIO2A_IOMUX_SEL_L	0x0040
#define GPIO2A_IOMUX_SEL_H	0x0044

#define VCCIO_IOC_BASE		0x26046000
#define VCCIO_IOC_GPIO1C_PUL	0x118
#define VCCIO_IOC_GPIO1D_PUL	0x11C
#define VCCIO_IOC_GPIO2A_PUL	0x120

#define VCCIO6_IOC_BASE		0x2604a000
#define VCCIO7_IOC_BASE		0x2604b000
#define VCCIO7_IOC_GPIO4D_IOMUX_SEL_L	0x0398
#define VCCIO7_IOC_XIN_UFS_CON	0x0400

#define PMU1_SGRF_BASE		0x26002000
#define PMU1_SGRF_SOC_CON10	0x0028
#define PMU1_SGRF_SOC_CON11	0x002C

#define SYS_SGRF_BASE		0x26004000
#define SYS_SGRF_SOC_CON14	0x0058
#define SYS_SGRF_SOC_CON15	0x005C
#define SYS_SGRF_SOC_CON20	0x0070

#define FW_SYS_SGRF_BASE	0x26005000
#define SGRF_DOMAIN_CON1	0x4
#define SGRF_DOMAIN_CON2	0x8
#define SGRF_DOMAIN_CON3	0xc
#define SGRF_DOMAIN_CON4	0x10
#define SGRF_DOMAIN_CON5	0x14

#define USBGRF_BASE		0x2601e000
#define USB_GRF_USB3OTG0_CON1	0x0030

#define PMU1_GRF_BASE		0x26026000
#define OS_REG0			0x200
#define USB2PHY0_GRF_BASE	0x2602e000
#define USB2PHY1_GRF_BASE	0x26030000
#define USB2PHY_GRF_CON4	0x0010
#define USB2PHY_GRF_DBG_CON	0x0040
#define USB2PHY_GRF_LS_TIMEOUT	0x0044
#define USB2PHY_GRF_LS_DEB	0x0048
#define USB2PHY_GRF_RX_TIMEOUT	0x004c
#define USB2PHY_GRF_SEQ_LIMT	0x0050

#define TOP_CRU_BASE		    0x27200000
#define TOP_CRU_GATE_CON19	    0x084C
#define TOP_CRU_SOFTRST_CON19	0x0a4C
#define PHPPHYSOFTRST_CON01	    0x8a04
#define TOPCRU_CRU_GLBRST_ST	0x0c04
#define GLB_WDTn_RST_ST			GENMASK(15, 10)

#define PMU1_CRU_BASE		0x27220000
#define PMU1_CRU_CLKSEL_CON03	0x030c
#define PMU1_CRU_GATE_CON03	0x080C
#define PMU1_CRU_SOFTRST_CON03	0x0a0C

#define SATA0_BASE_ADDR		0x2a240000
#define SATA1_BASE_ADDR		0x2a250000
#define SATA_PI			0xC
#define SATA_PORT_CMD		0x118
#define SATA_FBS_ENABLE		BIT(22)

const char * const boot_devices[BROM_LAST_BOOTSOURCE + 1] = {
	[BROM_BOOTSOURCE_EMMC] = "/soc/mmc@2a330000",
	[BROM_BOOTSOURCE_SPINOR] = "/soc/spi@2a340000",
	[BROM_BOOTSOURCE_SPINAND] = "/soc/spi@2a340000",
	[BROM_BOOTSOURCE_SD] = "/soc/mmc@2a310000",
	[BROM_BOOTSOURCE_UFS] = "/soc/ufs@2a2d0000",
};

#ifdef CONFIG_ARM64
#include <asm/armv8/mmu.h>
static struct mm_region rk3576_mem_map[] = {
	{
		/* I/O area */
		.virt = 0x20000000UL,
		.phys = 0x20000000UL,
		.size = 0xb080000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* PMU_SRAM, CBUF, SYSTEM_SRAM */
		.virt = 0x3fe70000UL,
		.phys = 0x3fe70000UL,
		.size = 0x190000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* MSCH_DDR_PORT */
		.virt = 0x40000000UL,
		.phys = 0x40000000UL,
		.size = 0x100000000UL - 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x100000000UL,
		.phys = 0x100000000UL,
		.size = 0x400000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		/* PCIe 0+1 */
		.virt = 0x900000000UL,
		.phys = 0x900000000UL,
		.size = 0x100800000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = rk3576_mem_map;
#endif

void board_debug_uart_init(void)
{
}

#define HP_TIMER_BASE			CONFIG_ROCKCHIP_STIMER_BASE
#define HP_CTRL_REG			0x04
#define TIMER_EN			BIT(0)
#define HP_LOAD_COUNT0_REG		0x14
#define HP_LOAD_COUNT1_REG		0x18

#ifdef CONFIG_SPL_BUILD
void rockchip_stimer_init(void)
{
	u32 reg;

	if (!IS_ENABLED(CONFIG_XPL_BUILD))
		return;

	reg = readl(HP_TIMER_BASE + HP_CTRL_REG);
	if (reg & TIMER_EN)
		return;
#if (CONFIG_COUNTER_FREQUENCY > 0)
	asm volatile("msr cntfrq_el0, %0" : : "r" (CONFIG_COUNTER_FREQUENCY));
#endif
	writel(0xffffffff, HP_TIMER_BASE + HP_LOAD_COUNT0_REG);
	writel(0xffffffff, HP_TIMER_BASE + HP_LOAD_COUNT1_REG);
	writel((TIMER_EN << 16) | TIMER_EN, HP_TIMER_BASE + HP_CTRL_REG);
}
#endif

void reset_misc(void)
{
#ifdef CONFIG_SPL_BUILD
	/* For RK3576 SPL, should extraly write os_reg0 and reset to maskrom. */
	if (readl(CONFIG_ROCKCHIP_BOOT_MODE_REG) == BOOT_BROM_DOWNLOAD)
		writel(BOOT_BROM_DOWNLOAD, PMU1_GRF_BASE + OS_REG0);
#elif CONFIG_SUPPORT_USBPLUG
	/*
	 * For RK3576 USBPLUG, should clear maskrom flag both in os_reg0 and os_reg16.
	 * It already clear os_reg16 under ./drivers/usb/gadget/f_rockusb.c
	 */
	if (readl(CONFIG_ROCKCHIP_BOOT_MODE_REG) != BOOT_BROM_DOWNLOAD)
		writel(0, (void *)PMU1_GRF_BASE + OS_REG0);
#endif
}
void board_set_iomux(enum uclass_id uclass, int devnum, int routing)
{
	switch (uclass) {
	case UCLASS_MMC:
		if (devnum == 0) {
			writel(0xffff1111, TOP_IOC_BASE + GPIO1A_IOMUX_SEL_L);
			writel(0xffff1111, TOP_IOC_BASE + GPIO1A_IOMUX_SEL_H);
			writel(0xffff1111, TOP_IOC_BASE + GPIO1B_IOMUX_SEL_L);
		} else if (devnum == 1) {
			writel(0xffff1111, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_L);
			writel(0x00ff0011, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_H);
			writel(0xf0001000, GPIO0_IOC_BASE + GPIO0A_IOMUX_SEL_H);
			/* Pull up */
			writel(0x0FFF0FFF, VCCIO_IOC_BASE + VCCIO_IOC_GPIO2A_PUL);
		}
		break;
	case UCLASS_MTD:
		if (routing == 0) {
			/* FSPI0 M0 */
			writel(0xffff2222, TOP_IOC_BASE + GPIO1A_IOMUX_SEL_L);
			writel(0xffff2020, TOP_IOC_BASE + GPIO1B_IOMUX_SEL_L);
		} else if (routing == 1) {
			/* FSPI1 M0 */
			writel(0xffff2222, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_L);
			writel(0x00ff0022, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_H);
			/* Pull up */
			writel(0x03ff03ff, VCCIO_IOC_BASE + VCCIO_IOC_GPIO2A_PUL);
		} else if (routing == 2) {
			/* FSPI1 M1 */
			writel(0xf0003000, TOP_IOC_BASE + GPIO1C_IOMUX_SEL_L);
			writel(0xffff3333, TOP_IOC_BASE + GPIO1C_IOMUX_SEL_H);
			writel(0x00f00030, TOP_IOC_BASE + GPIO1D_IOMUX_SEL_H);
			/* Pull up */
			writel(0xffc0ffc0, VCCIO_IOC_BASE + VCCIO_IOC_GPIO1C_PUL);
		}
		break;
	default:
		printf("Bootdev 0x%x is not support\n", uclass);
	}
}

void board_unset_iomux(enum uclass_id uclass, int devnum, int routing)
{
	switch (uclass) {
	case UCLASS_MTD:
		if (routing == 0) {
			/* FSPI0 M0 -> GPIO */
			writel(0xffff0000, TOP_IOC_BASE + GPIO1A_IOMUX_SEL_L);
			writel(0xffff0000, TOP_IOC_BASE + GPIO1B_IOMUX_SEL_L);
		} else if (routing == 1) {
			/* FSPI1 M0 -> GPIO */
			writel(0xffff0000, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_L);
			writel(0x00ff0000, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_H);
		} else if (routing == 2) {
			/* FSPI1 M1 -> GPIO */
			writel(0xf0000000, TOP_IOC_BASE + GPIO1C_IOMUX_SEL_L);
			writel(0xffff0000, TOP_IOC_BASE + GPIO1C_IOMUX_SEL_H);
			writel(0x00f00000, TOP_IOC_BASE + GPIO1D_IOMUX_SEL_H);
		}
		break;
	default:
		break;
	}
}

/* @brief: release reset for MCU
 * @param id: id of MCU, like: bus_mcu, pmu_mcu
 * @param entry_point: entry of firmware, use for address map
 * */
int fit_standalone_ext_release(char *id, standalone_args_t *args)
{
	if (!strcmp(id, "bus_mcu")) {
		uintptr_t load = args->load;
		uintptr_t sram_start = args->sram_start;
		uintptr_t uc_start = args->uc_start;
		uintptr_t uc_end = args->uc_end;

		/* relative bus m0 jtag / core / biu */
		writel(0x38003800, TOP_CRU_BASE + TOP_CRU_SOFTRST_CON19);

		/* address map: map 0 to entry_point, 0x20000000 to sram */
		sip_smc_mcu_config(ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
				   ROCKCHIP_SIP_CONFIG_MCU_CODE_START_ADDR,
				   load);
		if (sram_start) {
			sip_smc_mcu_config(ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
					   ROCKCHIP_SIP_CONFIG_MCU_SRAM_START_ADDR,
					   sram_start);
		}
		if (uc_start && uc_end) {
			sip_smc_mcu_config(ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
					   ROCKCHIP_SIP_CONFIG_MCU_UNCACHE_START_ADDR,
					   uc_start);
			sip_smc_mcu_config(ROCKCHIP_SIP_CONFIG_BUSMCU_0_ID,
					   ROCKCHIP_SIP_CONFIG_MCU_UNCACHE_END_ADDR,
					   uc_end);
		}

		/*
		* bus m0 configuration:
		* open bus m0 rtc / core / biu / root
		*/
		writel(0x5c000000, TOP_CRU_BASE + TOP_CRU_GATE_CON19);

		/* select bus m0 jtag GPIO2A2 GPIO2A3 */
		//writel(0x003f0010, SYS_GRF_BASE + SYS_GRF_SOC_CON7);
		//writel(0xff009900, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_L);

		/* release bus m0 jtag / core / biu */
		writel(0x38000000, TOP_CRU_BASE + TOP_CRU_SOFTRST_CON19);
	}
	else if (!strcmp(id, "pmu_mcu")) {

		/* pmu m0 configuration: */
		/* open pmu m0 rtc / core / biu / root */
		/* writel(0x59020000, PMU1_CRU_BASE + PMU1_CRU_GATE_CON03); */

		/* select pmu m0 jtag */
		/* writel(0x003f0008, SYS_GRF_BASE + SYS_GRF_SOC_CON7); */
		/* writel(0xff009900, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_L); */

		/* release pmu m0 jtag / core / biu */
		/* writel(0x38000000, PMU1_CRU_BASE + PMU1_CRU_SOFTRST_CON03); */
	}

	return 0;
}

#ifndef CONFIG_TPL_BUILD
int arch_cpu_init(void)
{
#if defined(CONFIG_SPL_BUILD) || defined(CONFIG_SUPPORT_USBPLUG)
	u32 cru_glbrst_st = readl(TOP_CRU_BASE + TOPCRU_CRU_GLBRST_ST);
	/* write BOOT_WATCHDOG to boot mode register, if reset by WDT */
	if (cru_glbrst_st & GLB_WDTn_RST_ST) {
		/*
		 * Keep boot mode as BOOT_PANIC instead of switching to BOOT_WATCHDOG
		 * if the WDT reset was triggered by a kernel panic.
		 * This ensures the real cause (panic) is not obscured by watchdog reset.
		 */
		if (readl(CONFIG_ROCKCHIP_BOOT_MODE_REG) != BOOT_PANIC)
			writel(BOOT_WATCHDOG, CONFIG_ROCKCHIP_BOOT_MODE_REG);
		/* clear flag if reset by WDT trigger */
		writel((cru_glbrst_st & ~GLB_WDTn_RST_ST), TOP_CRU_BASE + TOPCRU_CRU_GLBRST_ST);
	}

	u32 val;

	/* Set the emmc to access ddr memory */
	val = readl(FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON2);
	writel(val | 0x7, FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON2);

	/* Set the sdmmc0 to access ddr memory */
	val = readl(FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON5);
	writel(val | 0x700, FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON5);

	/* Set the UFS to access ddr memory */
	val = readl(FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON3);
	writel(val | 0x70000, FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON3);

	/* Set the fspi0 and fspi1 to access ddr memory */
	val = readl(FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON4);
	writel(val | 0x7700, FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON4);

	/* Set the decom to access ddr memory */
	val = readl(FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON1);
	writel(val | 0x700, FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON1);

	/*
	 * Set the GPIO0B0~B3 pull up and input enable.
	 * Keep consistent with other IO.
	 */
	writel(0x00ff00ff, GPIO0_IOC_BASE + GPIO0B_PULL_L);
	writel(0x000f000f, GPIO0_IOC_BASE + GPIO0B_IE_L);

	/*
	 * bus mcu_cache_peripheral_addr
	 * The uncache area ranges from 0x20000000 to 0x48200000
	 * and contains rpmsg shared memory
	 */
	writel(0x20000000, SYS_SGRF_BASE + SYS_SGRF_SOC_CON14);
	writel(0x48200000, SYS_SGRF_BASE + SYS_SGRF_SOC_CON15);

	/*
	 * pmu mcu_cache_peripheral_addr
	 * The uncache area ranges from 0x20000000 to 0x48200000
	 * and contains rpmsg shared memory
	 */
	/* writel(0x20000000, PMU1_SGRF_BASE + PMU1_SGRF_SOC_CON10); */
	/* writel(0x48200000, PMU1_SGRF_BASE + PMU1_SGRF_SOC_CON11); */

	/* TODO: pmu mcu code addr need bl31 support */
	/* writel(0x48200000, 0x26002030); */

	/*
	 * Set SYS_GRF_SOC_CON2[12](input of pwm2_ch0) as 0,
	 * keep consistent with other pwm.
	 */
	writel(0x10000000, SYS_GRF_BASE + SYS_GRF_SOC_CON2);

	/*
	 * Assert reset the combophy0 and combophy1,
	 * de-assert reset in Kernel combophy driver.
	 */
	writel(0x01200120, TOP_CRU_BASE + PHPPHYSOFTRST_CON01);

	/*
	 * Assert SIDDQ for USB 2.0 PHY1 to power down
	 * PHY1 analog block to save power. And let the
	 * PHY0 for OTG0 interface still in normal mode.
	 */
	writel(0x20002000, USB2PHY1_GRF_BASE + USB2PHY_GRF_CON4);

	/*
	 * Enable USB to DEBUG
	 * 1. Set linestate timeout 8ms
	 * 2. Set linestate fiter time 500us
	 * 3. Set Rx timeout counter for RX pulldown 2s
	 * 4. Set handshake counter number for SE0 and
	 *    SE1 sequence at least 5.
	 */
	writel(0xff, USB2PHY0_GRF_BASE + USB2PHY_GRF_LS_TIMEOUT);
	writel(0x10, USB2PHY0_GRF_BASE + USB2PHY_GRF_LS_DEB);
	writel(0xffff, USB2PHY0_GRF_BASE + USB2PHY_GRF_RX_TIMEOUT);
	writel(0x05, USB2PHY0_GRF_BASE + USB2PHY_GRF_SEQ_LIMT);
	writel(0x00010001, USB2PHY0_GRF_BASE + USB2PHY_GRF_DBG_CON);

	/* Enable noc slave response timeout */
	writel(0x80008000, SYS_GRF_BASE + SYS_GRF_SOC_CON11);
	writel(0xffffffe0, SYS_GRF_BASE + SYS_GRF_SOC_CON12);

	/*
	 * Select usb otg0 pipe phy status to 0 that
	 * ensure rockusb can work at high-speed even
	 * if usb3 phy isn't ready.
	 */
	writel(0x000c0008, USBGRF_BASE + USB_GRF_USB3OTG0_CON1);

	/*
	 * Enable cci channels for below module AXI R/W
	 * Module: GMAC0/1, MMU0/1(PCIe, SATA, USB3)
	 */
	writel(0xffffff00, SYS_SGRF_BASE + SYS_SGRF_SOC_CON20);
#endif

	/* Enabled SDMMC iomux in default except FSPI1_M0 boot */
	if (readl(TOP_IOC_BASE + GPIO2A_IOMUX_SEL_L) != 0x2222)
		board_set_iomux(UCLASS_MMC, 1, 0);

#if defined(CONFIG_ROCKCHIP_EMMC_IOMUX)
	board_set_iomux(UCLASS_MMC, 0, 0);
#elif defined(CONFIG_ROCKCHIP_SFC_IOMUX)
	/*
	 * (UCLASS_MTD, 0, 0) FSPI0
	 * (UCLASS_MTD, 1, 0) FSPI1 M0
	 * (UCLASS_MTD, 2, 0) FSPI1 M1
	 */
	board_set_iomux(UCLASS_MTD, 0, 0);
#endif /* #if defined(CONFIG_ROCKCHIP_EMMC_IOMUX) */

	/* UFS PHY select 26M from ppll */
	writel(0x00030002, PMU1_CRU_BASE + PMU1_CRU_CLKSEL_CON03);

	return 0;
}
#endif

void rk_board_ufs_reset_device(void)
{
	/* set UFS_RSTN to low */
	writel(0x00100000, VCCIO7_IOC_BASE + VCCIO7_IOC_XIN_UFS_CON);
	/* set iomux UFS_REFCLK, UFS_RSTN */
	writel(0x00FF0011, VCCIO7_IOC_BASE + VCCIO7_IOC_GPIO4D_IOMUX_SEL_L);
	udelay(10);
	/* set UFS_RSTN to high */
	writel(0x00100010, VCCIO7_IOC_BASE + VCCIO7_IOC_XIN_UFS_CON);
}

#if defined(CONFIG_SCSI) && defined(CONFIG_CMD_SCSI) && defined(CONFIG_UFS)
int rk_board_dm_fdt_fixup(const void *blob)
{
	struct blk_desc *desc = plat_bootdev();
	const char *status = NULL;
	int node = -1;

	/*
	 * 1. Kernel DTS will enable UFS by default.
	 *
	 * 2. It hangs if Kernel UFS driver tries to access UFS registers when there
	 * is no power supply for UFS.
	 *
	 * So generally, disable UFS when detect fail.
	 *
	 * To save time spent on detecting UFS, you can disable UFS in kernel dts or
	 * U-Boot defconfig.
	 *
	 */
	if (desc->uclass_id != UCLASS_SCSI) {
		node = fdt_node_offset_by_compatible(blob, 0, "rockchip,rk3576-ufs");
		if (node >= 0) {
			status = fdt_getprop(blob, node, "status", NULL);
			if (status && strcmp(status, "disabled")) {
				if (scsi_scan(true)) {
					fdt_setprop((void *)blob, node, "status", "disabled", 9);
					printf("FDT: UFS was not detected, disabling UFS.\n");
				}
			}
		}
	}

	node = fdt_node_offset_by_compatible(blob, 0, "rockchip,rk-ahci");
	if (node >= 0) {
		status = fdt_getprop(blob, node, "status", NULL);
		if (status && strcmp(status, "disabled")) {
			/*
			* Set SATA FBSCP and PORTS_IMPL for kernel drivers
			*/
			writel(SATA_FBS_ENABLE, SATA0_BASE_ADDR + SATA_PORT_CMD);
			writel(1, SATA0_BASE_ADDR + SATA_PI);
			writel(SATA_FBS_ENABLE, SATA1_BASE_ADDR + SATA_PORT_CMD);
			writel(1, SATA1_BASE_ADDR + SATA_PI);
		}
	}

	return 0;
}
#endif

/* @brief: Fix up the device tree of gmac0
 *
 * This function enables GMAC0 after verifying the license.
 *
 * @param blob Pointer to the device tree blob
 * @return 0 on success
 **/
#if defined(CONFIG_ROCKCHIP_VENDOR_PARTITION)
int rk_board_fdt_fixup(const void *blob)
{
#ifndef CONFIG_SUPPORT_USBPLUG
	char licence_str[1024] = {0};
	int ret, size, node;

	size = vendor_storage_read(MULTI_MODULE_KEY_ID, licence_str, 1024);
	if (size > 0) {
		ret = optee_verify_config_ip(licence_str, "gmac0");
		if (!ret)
			printf("gmac0 can be enabled safely\n");
		else
			return 0;

		node = fdt_path_offset(blob, "/ethernet@2a220000");
		if (node < 0) {
			printf("Error: /ethernet@2a220000 cannot find node\n");
		} else {
			ret = fdt_setprop_string((void *)blob, node, "status", "okay");
			if (ret < 0)
				printf("Error: /ethernet@2a220000 cannot set status property\n");
		}
	}
#endif
	return 0;
}
#endif
