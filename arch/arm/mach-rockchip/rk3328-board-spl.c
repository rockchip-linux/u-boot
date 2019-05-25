/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <ram.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/bootrom.h>
#include <asm/arch/cru_rk3328.h>
#include <asm/arch/grf_rk3328.h>
#include <asm/arch/hardware.h>
#include <asm/arch/periph.h>
#include <asm/arch/timer.h>
#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/pinctrl/rockchip.h>

#define GRF_BASE		0xFF100000
#define GPIO0_BASE  0xFF210000

DECLARE_GLOBAL_DATA_PTR;

void board_debug_uart_init(void)
{
}

void board_init_sdmmc_pwr_en(void)
{
	struct rk3328_grf_regs * const grf = (void *)GRF_BASE;
	struct rockchip_gpio_regs * const gpio0 = (void *)GPIO0_BASE;

	printf("board_init_sdmmc_pwr_en\n");

	rk_clrsetreg(&grf->com_iomux,
		IOMUX_SEL_SDMMC_MASK,
		IOMUX_SEL_SDMMC_M1 << IOMUX_SEL_SDMMC_SHIFT);

	rk_clrsetreg(&grf->gpio0d_iomux,
		GPIO0D6_SEL_MASK,
		GPIO0D6_GPIO << GPIO0D6_SEL_SHIFT);

	rk_clrsetreg(&grf->gpio1a_iomux,
		GPIO1A0_SEL_MASK,
		GPIO1A0_CARD_DATA_CLK_CMD_DETN
		<< GPIO1A0_SEL_SHIFT);

	// set GPIO0_D6 to GPIO_ACTIVE_LOW
	rk_clrreg(&gpio0->swport_dr,
		1 << RK_PD6);

	// set GPIO0_D6 to OUTPUT
	rk_setreg(&gpio0->swport_ddr, 
		1 << RK_PD6);
}

void board_init_f(ulong dummy)
{
	struct udevice *dev;
	int ret;

	ret = spl_early_init();
	if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
		hang();
	}

	preloader_console_init();
	board_init_sdmmc_pwr_en();

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		debug("DRAM init failed: %d\n", ret);
		return;
	}
}

u32 spl_boot_mode(const u32 boot_device)
{
	return MMCSD_MODE_RAW;
}

u32 spl_boot_device(void)
{
	u32 bootdevice_brom_id = readl(RK3328_BROM_BOOTSOURCE_ID_ADDR);
	switch (bootdevice_brom_id) {
		case BROM_BOOTSOURCE_EMMC:
			printf("booted from eMMC\n");
			return BOOT_DEVICE_MMC1;

		case BROM_BOOTSOURCE_SD:
			printf("booted from SD\n");
			return BOOT_DEVICE_MMC2;

		case BROM_BOOTSOURCE_SPINOR:
			printf("booted from SPI flash\n");
			return BOOT_DEVICE_SPI;

		case BROM_BOOTSOURCE_USB:
			printf("booted from USB\n");
			return BOOT_DEVICE_MMC1;
	}

	return BOOT_DEVICE_BOOTROM;
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* Just empty function now - can't decide what to choose */
	debug("%s: %s\n", __func__, name);

	return 0;
}
#endif
