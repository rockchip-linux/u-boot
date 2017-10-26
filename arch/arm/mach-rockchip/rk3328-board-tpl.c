/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <fdtdec.h>
#include <led.h>
#include <malloc.h>
#include <mmc.h>
#include <ram.h>
#include <spl.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/bootrom.h>
#include <asm/arch/clock.h>
#include <asm/arch/hardware.h>
#include <asm/arch/periph.h>
#include <asm/arch/sdram.h>
#include <asm/arch/timer.h>
#include <dm/pinctrl.h>
#include <dm/root.h>
#include <dm/test.h>
#include <dm/util.h>
#include <power/regulator.h>
#include <asm/arch/grf_rk3328.h>
#include <asm/arch/uart.h>

#define CRU_BASE		0xFF440000
#define GRF_BASE		0xFF100000
#define UART2_BASE		0xFF130000
#define STIMER_BASE_ADDR		0xFF1d0000
#define CPU_TIMER_BASE			(STIMER_BASE_ADDR + 0x20)

void board_timer_init(void)
{
	/* Initialize CNTFRQ */
	__asm__ volatile ("LDR x0,=24000000");
	__asm__ volatile ("MSR CNTFRQ_EL0, x0");

	/* Enable STimer1 for core */
	writel(0x0, CPU_TIMER_BASE + 0x0010);
	writel(0xffffffff, CPU_TIMER_BASE + 0x0000);
	writel(0xffffffff, CPU_TIMER_BASE + 0x0004);
	writel(0x1, CPU_TIMER_BASE + 0x0010);
}

void board_debug_uart_init(void)
{
	struct rk3328_grf_regs * const grf = (void *)GRF_BASE;
	struct rk_uart * const uart = (void *)UART2_BASE;

	/* uart_sel_clk default select 24MHz */
	writel((3 << (8 + 16)) | (2 << 8), CRU_BASE + 0x148);

	/* init uart baud rate 1500000 */
	writel(0x83, &uart->lcr);
	writel(0x1, &uart->rbr);
	writel(0x3, &uart->lcr);

	/* Enable early UART2 */
	rk_clrsetreg(&grf->com_iomux,
		     IOMUX_SEL_UART2_MASK,
		     IOMUX_SEL_UART2_M1 << IOMUX_SEL_UART2_SHIFT);
	rk_clrsetreg(&grf->gpio2a_iomux,
		     GPIO2A0_SEL_MASK,
		     GPIO2A0_UART2_TX_M1 << GPIO2A0_SEL_SHIFT);
	rk_clrsetreg(&grf->gpio2a_iomux,
		     GPIO2A1_SEL_MASK,
		     GPIO2A1_UART2_RX_M1 << GPIO2A1_SEL_SHIFT);

	/* enable FIFO */
	writel(0x1, &uart->sfe);
}

void board_return_to_bootrom(void)
{
	back_to_bootrom();
}

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_BOOTROM;
}


void board_init_f(ulong dummy)
{
	struct udevice *dev;
	int ret;

#define EARLY_UART
#ifdef EARLY_UART
	debug_uart_init();
	printascii("U-Boot TPL board init\n");
#endif

	board_timer_init();

	ret = spl_early_init();
	if (ret) {
		printf("spl_early_init() failed: %d\n", ret);
		hang();
	}

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		printf("DRAM init failed: %d\n", ret);
		return;
	}
}
