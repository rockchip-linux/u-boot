/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <amp.h>
#include <common.h>
#include <bidram.h>
#include <boot_rkimg.h>
#include <dm.h>
#include <fdt_support.h>
#include <misc.h>
#include <mmc.h>
#include <scsi.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/arch/bootrom.h>
#include <asm/arch/cpu.h>
#include <asm/arch/hardware.h>
#include <asm/arch/boot_mode.h>
#include <asm/arch/ioc_rk3576.h>
#include <asm/arch/mos.h>
#include <asm/arch/rk_atags.h>
#include <asm/arch/rockchip_smccc.h>
#include <asm/system.h>
#include <asm/arch/vendor.h>
#include <optee_include/OpteeClientInterface.h>
#include <configs/rk3576_common.h>

DECLARE_GLOBAL_DATA_PTR;

#define SYS_GRF_BASE		0x2600A000
#define SYS_GRF_SOC_CON2	0x0008
#define SYS_GRF_SOC_CON7	0x001c
#define SYS_GRF_SOC_CON11	0x002c
#define SYS_GRF_SOC_CON12	0x0030

#define GPIO0_IOC_BASE		0x26040000
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
#define PMU1_SGRF_SOC_CON0	0x0000
#define PMU1_SGRF_SOC_CON10	0x0028
#define PMU1_SGRF_SOC_CON11	0x002C
#define PMU1_SGRF_SOC_CON12	0x0030
#define PMU1_SGRF_SOC_CON13	0x0034

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

#define SATA0_BASE_ADDR			0x2a240000
#define SATA1_BASE_ADDR			0x2a250000
#define SATA_PI				0xC
#define SATA_PORT_CMD			0x118
#define SATA_FBS_ENABLE			BIT(22)
#define SYS_SGRF_SOC_CON0	0x20
#define SYS_SGRF_SOC_CON16	0x60
#define SYS_SGRF_SOC_CON17	0x64

#define UART1_BASE		0x27310000
#define UART4_BASE              0x2ad70000
#define UART5_BASE		0x2ad80000
#define UART8_BASE		0x2adb0000
#define CRU_BASE		0x27200000

#ifdef CONFIG_ARM64
#include <asm/armv8/mmu.h>

static struct mm_region rk3576_mem_map[] = {
	{
		.virt = 0x20000000UL,
		.phys = 0x20000000UL,
		.size = 0xb080000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x3fe70000UL,
		.phys = 0x3fe70000UL,
		.size = 0x190000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
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
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_MOS_SECONDARY)
	u32 val = (CONFIG_BAUDRATE == 115200) ? 0x0d : 0x01;

	if (gd->serial.addr == UART1_BASE) {
		writel(0x1 << 16 | 0x1, CRU_BASE + 0x320);
		writel((0x1 << 5) | (0x1 << 6), CRU_BASE + 0x814);
		/* iomux */
		writel((0x9 << 0) | (0x9 << 4) | 0xff0000, TOP_IOC_BASE + 0x48);
		dsb();
		writel(0x83, UART1_BASE + 0xc);
		dsb();
		writel(val, UART1_BASE + 0x0);
		writel(0x03, UART1_BASE + 0xc);
		writel(0x31, UART1_BASE + 0x0);
		dsb();
		writel(0x32, UART1_BASE + 0x0);
		dsb();
		writel(0x33, UART1_BASE + 0x0);
		dsb();
		writel(0x34, UART1_BASE + 0x0);
		dsb();

	} else if (gd->serial.addr == UART5_BASE) {
		writel(0xfff0300, CRU_BASE + 0x400);
		writel(0x1 << 14, CRU_BASE + 0x834);
		writel(0x1 << 15, CRU_BASE + 0x838);
		/* iomux */
		writel(0xff0099, TOP_IOC_BASE + 0x44);
		dsb();

		writel(0x83, UART5_BASE + 0xc);
		dsb();
		writel(val, UART5_BASE + 0x0);
		writel(0x03, UART5_BASE + 0xc);
		writel(0x31, UART5_BASE + 0x0);
		dsb();
		writel(0x32, UART5_BASE + 0x0);
		dsb();
		writel(0x33, UART5_BASE + 0x0);
		dsb();
		writel(0x34, UART5_BASE + 0x0);
		dsb();
	}
#endif
}

const char * const boot_devices[BROM_LAST_BOOTSOURCE + 1] = {
	[BROM_BOOTSOURCE_EMMC] = "/mmc@2a330000",
	[BROM_BOOTSOURCE_SD] = "/mmc@2a310000",
	[BROM_BOOTSOURCE_UFS] = "/ufs@2a2d0000",
};

#ifdef CONFIG_SPL_BUILD
void rockchip_stimer_init(void)
{
	u32 reg;

	/* If Timer already enabled, don't re-init it */
	reg = readl(CONFIG_ROCKCHIP_STIMER_BASE + 0x4);
	if (reg & 0x1)
		return;
#ifdef COUNTER_FREQUENCY
	asm volatile("msr CNTFRQ_EL0, %0" : : "r" (COUNTER_FREQUENCY));
#endif
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE + 0x14);
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE + 0x18);
	writel(0x00010001, CONFIG_ROCKCHIP_STIMER_BASE + 0x04);
}
#endif

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_MOS_SECONDARY)
#define PMU2_CPU4_AUTO_PWR_CON		(0x27380090)
#define PMU1SGRF_SOC_CON(i)		(0x26002000 + ((i) * 4))
#define BITS_WITH_WMASK(bits, msk, shift) \
	((bits) << (shift)) | ((msk) << ((shift) + 16))

enum pmu_cpu_auto_pwr_con {
	pmu_cpu_pm_en = 0,
	pmu_cpu_pm_int_wakeup_en = 1,
	pmu_cpu_pm_dis_int = 2,
	pmu_cpu_pm_sft_wakeup_en = 3,
};

void mos_board_reset(void)
{
	u32 core_pm_value;

	writel(CONFIG_SPL_TEXT_BASE >> 16, PMU1SGRF_SOC_CON(14));
	dsb();

	memcpy((char *)CONFIG_SPL_TEXT_BASE,
	       (char *)CONFIG_MOS_SECONDARY_SPL_BACKUP_ADDR, SZ_256K);
	flush_dcache_range(CONFIG_SPL_TEXT_BASE, CONFIG_SPL_TEXT_BASE + SZ_256K);
	dsb();

	core_pm_value = BIT(pmu_cpu_pm_en) |
			BIT(pmu_cpu_pm_dis_int) | BIT(pmu_cpu_pm_sft_wakeup_en);
	writel(BITS_WITH_WMASK(core_pm_value, 0xf, 0), PMU2_CPU4_AUTO_PWR_CON);
	dsb();

	isb();
	mdelay(5);

	while (1)
		wfi();
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

void board_set_iomux(enum if_type if_type, int devnum, int routing)
{
	switch (if_type) {
	case IF_TYPE_MMC:
		if (devnum == 0) {
			writel(0xffff1111, TOP_IOC_BASE + GPIO1A_IOMUX_SEL_L);
			writel(0xffff1111, TOP_IOC_BASE + GPIO1A_IOMUX_SEL_H);
			writel(0xffff1111, TOP_IOC_BASE + GPIO1B_IOMUX_SEL_L);
		} else if (devnum == 1) {
			writel(0xffff1111, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_L);
			writel(0x00ff0011, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_H);
			/* Pull up */
			writel(0x0FFF0FFF, VCCIO_IOC_BASE + VCCIO_IOC_GPIO2A_PUL);
		}
		break;
	case IF_TYPE_MTD:
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
		printf("Bootdev 0x%x is not support\n", if_type);
	}
}

void board_unset_iomux(enum if_type if_type, int devnum, int routing)
{
	switch (if_type) {
	case IF_TYPE_MTD:
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

#ifdef CONFIG_SPL_BUILD
#ifndef CONFIG_MOS_SECONDARY
#if 0
static void uart4_m1_init(void)
{
	/* uart4_tx_m1/uart4_rx_m1 */
	writel(0x00ff0099, TOP_IOC_BASE + 0x34);
	/* uart4 xin_osc0_func_mux, sclk uart4 div 0 */
	writel(0x0fff0300, CRU_BASE + 0x3fc);
	/* reset uart : UART_SRR = UART_RESET | RCVR_FIFO_REST | XMIT_FIFO_RESET */
	writel(0x07, UART4_BASE + 0x88);
	udelay(10);
	/* disable uart : UART_IER = 0 */
	writel(0x0, UART4_BASE + 0x4);
	/*
	 * set bit8/no parity/1 stop bit :
	 * UART_LCR = UART_BIT8|PARITY_DISABLED|ONE_STOP_BIT|LCR_DLA_EN
	 */
	writel(0x83, UART4_BASE + 0xc);
	/* set bautrate 1500000 : UART_DLL = 1 / UART_DLH = 0 */
	writel(0x01, UART4_BASE + 0x0);
	writel(0x00, UART4_BASE + 0x4);
	/* LCR_DLA_DISABLE */
	writel(0x03, UART4_BASE + 0xc);
	/* fifo enable */
	writel(0x07, UART4_BASE + 0x8);
}

static void uart8_m1_init(void)
{
	/* uart8_tx_m1/uart8_rx_m1 */
	writel(0xff009900, TOP_IOC_BASE + 0x44);
	/* uart8 xin_osc0_func_mux, sclk uart8 div 0 */
	writel(0x0fff0300, CRU_BASE + 0x40c);
	/* reset uart : UART_SRR = UART_RESET | RCVR_FIFO_REST | XMIT_FIFO_RESET */
	writel(0x07, UART8_BASE + 0x88);
	udelay(10);
	/* disable uart : UART_IER = 0 */
	writel(0x0, UART8_BASE + 0x4);
	/*
	 * set bit8/no parity/1 stop bit :
	 * UART_LCR = UART_BIT8|PARITY_DISABLED|ONE_STOP_BIT|LCR_DLA_EN
	 */
	writel(0x83, UART8_BASE + 0xc);
	/* set bautrate 1500000 : UART_DLL = 1 / UART_DLH = 0 */
	writel(0x01, UART8_BASE + 0x0);
	writel(0x00, UART8_BASE + 0x4);
	/* LCR_DLA_DISABLE */
	writel(0x03, UART8_BASE + 0xc);
	/* fifo enable */
	writel(0x07, UART8_BASE + 0x8);
}
#endif

int spl_fit_standalone_release(char *id, uintptr_t entry_point)
{
	/* pmu mcu */
	if (!strcmp(id, "mcu0")) {
#if 0
		uart4_m1_init();
		uart8_m1_init();
#endif
		/* open pmu m0 rtc / core / biu / root */
		writel(0x59020000, PMU1_CRU_BASE + PMU1_CRU_GATE_CON03);
		/* jtag select to pmumcu */
		// writel(0x003f0008, SYS_GRF_BASE + SYS_GRF_SOC_CON7);
		/* switch iomux to jtag, gpio2_a2 gpio2_a3 */
		writel(0xff009900, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_L);
		/*
		 * CODE: 0x00000000 remap to entry_point
		 *
		 * ROM
		 * MCU Access Addr : 0x00000000 ~ 0x1FFFFFFF
		 * Remap Rule      : MCU Addr - 0x00000000 + PMU1_SGRF_SOC_CON12
		 */
		writel(entry_point, PMU1_SGRF_BASE + PMU1_SGRF_SOC_CON12);
		/*
		 * SCMI SHMEM: 1M + 60K(0x4010f000)
		 *
		 * RAM
		 * MCU Access Addr : 0x20000000 ~ 0x3FFFFFFF
		 * DDR Addr        : 0x40000000 + 0x5FFFFFFF
		 * Remap Rule      : MCU Addr - 0x20000000 + PMU1_SGRF_SOC_CON13(0x40000000)
		 */
		writel(0x40000000, PMU1_SGRF_BASE + PMU1_SGRF_SOC_CON13);
	} else if (!strcmp(id, "mcu1")) { /* BUS-MCU */
#if defined(CONFIG_MOS_SUPPORT) && !defined(CONFIG_MOS_SECONDARY)
		/* Set the BUS-MCU to access ddr memory */
		writel(0x7 << 16, FW_SYS_SGRF_BASE + SGRF_DOMAIN_CON4);

		/*
		 * set busmcu_hprot_secure_ctrl bit
		 * Set BUS-MCU as secure master
		 */
		writel(0x100 << 16 | 0x000, SYS_SGRF_BASE + SYS_SGRF_SOC_CON0);

		/* set MCU_CODE_START_ADDR */
		writel(entry_point, SYS_SGRF_BASE + SYS_SGRF_SOC_CON16);

		/* set BUS_MCU_SRAM_START_ADDRESS */
		writel(mos_safety_atags_addr(), SYS_SGRF_BASE + SYS_SGRF_SOC_CON17);

		/*
		 * bus m0 configuration:
		 * open bus m0 rtc / core / biu / root
		 */
		writel(0x5c000000, TOP_CRU_BASE + TOP_CRU_GATE_CON19);

		/* select bus m0 jtag GPIO2A2 GPIO2A3 */
		// writel(0x003f0010, SYS_GRF_BASE + SYS_GRF_SOC_CON7);
		// writel(0xff009900, TOP_IOC_BASE + GPIO2A_IOMUX_SEL_L);

		udelay(10);

		/* release bus m0 jtag / core / biu */
		/* writel(0x38000000, TOP_CRU_BASE + TOP_CRU_SOFTRST_CON19); */
#endif
	} else if (!strcmp(id, "init0")) {
#ifdef CONFIG_MOS_SUPPORT
		mos_spl_cfg_init();
#endif
	}

	return 0;
}
#endif
#endif

#ifndef CONFIG_TPL_BUILD
int arch_cpu_init(void)
{
#ifdef CONFIG_MOS_SECONDARY
	return 0;
#endif

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

	/* Set the sdmmc0 iomux */
	board_set_iomux(IF_TYPE_MMC, 1, 0);

	/* UFS PHY select 26M from ppll */
	writel(0x00030002, PMU1_CRU_BASE + PMU1_CRU_CLKSEL_CON03);

	/* set iomux UFS_REFCLK, UFS_RSTN */
	writel(0x00FF0011, VCCIO7_IOC_BASE + VCCIO7_IOC_GPIO4D_IOMUX_SEL_L);
	/* set UFS_RSTN to high */
	udelay(20);
	writel(0x00100010, VCCIO7_IOC_BASE + VCCIO7_IOC_XIN_UFS_CON);

	/*
	 * Set the GPIO0B0~B3 pull up and input enable.
	 * Keep consistent with other IO.
	 */
	writel(0x00ff00ff, GPIO0_IOC_BASE + GPIO0B_PULL_L);
	writel(0x000f000f, GPIO0_IOC_BASE + GPIO0B_IE_L);

#ifndef CONFIG_MOS_SUPPORT
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
#else
	/*
	 * bus mcu_cache_peripheral_addr
	 * The uncache area ranges from 0x20000000 to 0x48200000
	 * and contains rpmsg shared memory
	 */
	writel(0x20000000, SYS_SGRF_BASE + SYS_SGRF_SOC_CON14);
	writel(0x41300000, SYS_SGRF_BASE + SYS_SGRF_SOC_CON15);

	/*
	 * pmu mcu_cache_peripheral_addr
	 * The uncache area ranges from 0x20000000 to 0x40200000
	 * and contains scmi scp shared memory and atags shared memory.
	 */
	writel(0x20000000, PMU1_SGRF_BASE + PMU1_SGRF_SOC_CON10);
	writel(0x41600000, PMU1_SGRF_BASE + PMU1_SGRF_SOC_CON11);
#endif

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
		board_set_iomux(IF_TYPE_MMC, 1, 0);

#if defined(CONFIG_ROCKCHIP_EMMC_IOMUX)
	board_set_iomux(IF_TYPE_MMC, 0, 0);
#elif defined(CONFIG_ROCKCHIP_SFC_IOMUX)
	/*
	 * (IF_TYPE_MTD, 0, 0) FSPI0
	 * (IF_TYPE_MTD, 0, 1) FSPI1 M0
	 * (IF_TYPE_MTD, 0, 2) FSPI1 M1
	 */
	board_set_iomux(IF_TYPE_MTD, 0, 0);
#endif /* #if defined(CONFIG_ROCKCHIP_EMMC_IOMUX) */

#ifndef CONFIG_SPL_BUILD//xxxxxxx
	/* assert pmumcu core */
	/* writel(0x00010001, PMU1_CRU_BASE + PMU1_CRU_SOFTRST_CON03); */
	/* udelay(5); */
	/* release pmumcu jtag/core/biu */
	/* writel(0x38000000, PMU1_CRU_BASE + PMU1_CRU_SOFTRST_CON03); */
#endif
	return 0;
}
#endif

#ifdef CONFIG_MOS_SUPPORT
#include <dm.h>
#include <asm/io.h>
#include <asm/arch/cru_rk3576.h>

#ifdef CONFIG_MOS_SECONDARY
static int rk3576_cpub_pvtpll_set_rate(ulong rate)
{
	u32 length, length_frac;

	if (rate >= 1608000000) {
		length = 5;
		length_frac = 1;
	} else if (rate >= 1416000000) {
		length = 7;
		length_frac = 1;
	} else if (rate >= 1200000000) {
		length = 11;
		length_frac = 0;
	} else {
		length = 17;
		length_frac = 0;
	}

	writel(0x1dff0000 | (length << 2) | length_frac, PVTPLL_BIGCORE_BASE + RK3576_PVTPLL_GCK_LEN);
	/* set cal cnt = 24, T = 1us */
	writel(0x18, PVTPLL_BIGCORE_BASE + RK3576_PVTPLL_GCK_CAL_CNT);
	/* enable pvtpll */
	writel(0x00220022, PVTPLL_BIGCORE_BASE + RK3576_PVTPLL_GCK_CFG);
	/* start pvtpll */
	writel(0x00230023, PVTPLL_BIGCORE_BASE + RK3576_PVTPLL_GCK_CFG);

	/* set pvtpll_src parent from 24MHz/32KHz to pvtpll */
	writel(0x00200020, RK3576_CRU_BASE + RK3576_BIGCORE_CLKSEL_CON(2));

	/* set litcore unclean_src parent to pvtpll_src */
	writel(0x30002000, RK3576_CRU_BASE + RK3576_BIGCORE_CLKSEL_CON(1));
	/*
	* set litcore parent from pvtpll_src to unclean_src,
	* because autocs is on litcore unclean_src.
	*/
	writel(0xc0000000, RK3576_CRU_BASE + RK3576_BIGCORE_CLKSEL_CON(1));
	/* set litcore unclean_src div to 0 */
	writel(0x0f800000, RK3576_CRU_BASE + RK3576_BIGCORE_CLKSEL_CON(1));
	return 0;
}
#else
static int rk3576_cpul_pvtpll_set_rate(ulong rate)
{
	u32 length, length_frac;

	if (rate >= 1608000000) {
		length = 6;
		length_frac = 2;
	} else if (rate >= 1416000000) {
		length = 8;
		length_frac = 0;
	} else if (rate >= 1200000000) {
		length = 11;
		length_frac = 0;
	} else {
		length = 17;
		length_frac = 0;
	}

	writel(0x1dff0000 | (length << 2) | length_frac, PVTPLL_LITCORE_BASE + RK3576_PVTPLL_GCK_LEN);
	/* set cal cnt = 24, T = 1us */
	writel(0x18, PVTPLL_LITCORE_BASE + RK3576_PVTPLL_GCK_CAL_CNT);
	/* enable pvtpll */
	writel(0x00220022, PVTPLL_LITCORE_BASE + RK3576_PVTPLL_GCK_CFG);
	/* start pvtpll */
	writel(0x00230023, PVTPLL_LITCORE_BASE + RK3576_PVTPLL_GCK_CFG);

	/* set pvtpll_src parent from 24MHz/32KHz to pvtpll */
	writel(0x20002000, RK3576_CRU_BASE + RK3576_LITCORE_CLKSEL_CON(1));

	/* set litcore unclean_src parent to pvtpll_src */
	writel(0x30002000, RK3576_CRU_BASE + RK3576_LITCORE_CLKSEL_CON(0));
	/*
	* set litcore parent from pvtpll_src to unclean_src,
	* because autocs is on litcore unclean_src.
	*/
	writel(0x00c00000, RK3576_CRU_BASE + RK3576_LITCORE_CLKSEL_CON(1));
	/* set litcore unclean_src div to 0 */
	writel(0x0f800000, RK3576_CRU_BASE + RK3576_LITCORE_CLKSEL_CON(0));
	return 0;
}

static int rk3576_cci_pvtpll_set_rate(ulong rate)
{
	u32 length, pvtpll_en = 0;

	if (rate >= 900000000)
		length = 30;
	else if (rate >= 800000000)
		length = 31;
	else
		length = 34;

	writel(0x1dff0000 | (length << 2), PVTPLL_CCI_BASE + RK3576_PVTPLL_GCK_LEN);
	/* set cal cnt = 24, T = 1us */
	writel(0x18, PVTPLL_CCI_BASE + RK3576_PVTPLL_GCK_CAL_CNT);
	/* enable pvtpll */
	pvtpll_en = readl(PVTPLL_CCI_BASE + RK3576_PVTPLL_GCK_CFG);
	if (pvtpll_en && 0x22 != 0x22)
		writel(0x00220022, PVTPLL_CCI_BASE + RK3576_PVTPLL_GCK_CFG);
	/* start pvtpll */
	writel(0x00230023, PVTPLL_CCI_BASE + RK3576_PVTPLL_GCK_CFG);

	/* set cci mux pvtpll */
	writel(0x40004000, RK3576_CRU_BASE + RK3576_CCI_CLKSEL_CON(4));
	writel(0x30001000, RK3576_CRU_BASE + RK3576_CCI_CLKSEL_CON(4));
	writel(0x0f800000, RK3576_CRU_BASE + RK3576_CCI_CLKSEL_CON(4));
	return 0;
}
#endif

int set_armclk_rate(void)
{
	u32 is_pvtpll;
#ifdef CONFIG_MOS_SECONDARY
	is_pvtpll = (readl(RK3576_CRU_BASE + RK3576_BIGCORE_CLKSEL_CON(1)) &
		     CLK_LITCORE_SEL_MASK) >> CLK_LITCORE_SEL_SHIFT;
	if (is_pvtpll == 2)
		return 0;
	rk3576_cpub_pvtpll_set_rate(1608000000);
#else
	is_pvtpll = (readl(RK3576_CRU_BASE + RK3576_LITCORE_CLKSEL_CON(0)) &
		     CLK_LITCORE_SEL_MASK) >> CLK_LITCORE_SEL_SHIFT;
	if (is_pvtpll == 2)
		return 0;
	rk3576_cpul_pvtpll_set_rate(1608000000);
	rk3576_cci_pvtpll_set_rate(1608000000 / 2);
#endif

	return 0;
}
#endif

#if defined(CONFIG_SCSI) && defined(CONFIG_CMD_SCSI) && defined(CONFIG_UFS)
int rk_board_dm_fdt_fixup(const void *blob)
{
	struct blk_desc *desc = rockchip_get_bootdev();
	const char *status = NULL;
	int node = -1;

#ifdef CONFIG_MOS_SUPPORT
	return 0;
#endif
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
	if (desc->if_type != IF_TYPE_SCSI) {
		node = fdt_path_offset(blob, "/ufs@2a2d0000");
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

	node = fdt_path_offset(blob, "/sata@2a240000");
	if (node >= 0) {
		/*
		* Set SATA FBSCP and PORTS_IMPL for kernel drivers
		*/
		writel(SATA_FBS_ENABLE, SATA0_BASE_ADDR + SATA_PORT_CMD);
		writel(1, SATA0_BASE_ADDR + SATA_PI);
		writel(SATA_FBS_ENABLE, SATA1_BASE_ADDR + SATA_PORT_CMD);
		writel(1, SATA1_BASE_ADDR + SATA_PI);
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
	char licence_str[1024] = {0};
	int ret, size, node;

	size = vendor_storage_read(MULTI_MODULE_KEY_ID, licence_str, 1024);
	if (size > 0) {
		ret = trusty_verify_config_ip(licence_str);
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

	return 0;
}
#endif
