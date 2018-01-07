/*
 * (C) Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <adc.h>
#include <asm/io.h>
#include <asm/arch/boot_mode.h>
#include <cli.h>
#include <dm.h>
#include <fdtdec.h>

DECLARE_GLOBAL_DATA_PTR;

void set_back_to_bootrom_dnl_flag(void)
{
	writel(BOOT_BROM_DOWNLOAD, CONFIG_ROCKCHIP_BOOT_MODE_REG);
}

/*
 * detect download key status by adc, most rockchip
 * based boards use adc sample the download key status,
 * but there are also some use gpio. So it's better to
 * make this a weak function that can be override by
 * some special boards.
 */
#define KEY_DOWN_MIN_VAL	0
#define KEY_DOWN_MAX_VAL	30

__weak int rockchip_dnl_key_pressed(void)
{
	const void *blob = gd->fdt_blob;
	unsigned int val;
	int channel = 1;
	int node;
	u32 chns[2];

	node = fdt_node_offset_by_compatible(blob, 0, "adc-keys");
	if (node >= 0) {
	       if (!fdtdec_get_int_array(blob, node, "io-channels", chns, 2))
		       channel = chns[1];
	}

	if (adc_channel_single_shot("saradc", channel, &val)) {
		printf("%s adc_channel_single_shot fail!\n", __func__);
		return false;
	}

	if ((val >= KEY_DOWN_MIN_VAL) && (val <= KEY_DOWN_MAX_VAL))
		return true;
	else
		return false;
}

void rockchip_blink_power(int times)
{
		for (int i = 0; i < times; ++i) {
			cli_simple_run_command("led power off", 0);
			mdelay(100);
			cli_simple_run_command("led power on", 0);
			mdelay(100);
		}
}

int rockchip_dnl_mode(int num_modes)
{
	int mode = 0;

	while(mode < num_modes) {
		++mode;

		printf("rockchip_dnl_mode = %d mode\n", mode);
		rockchip_blink_power(mode);

		// return early
		if (mode == num_modes) {
			goto end;
		}

		// wait 2 seconds
		for (int i = 0; i < 100; ++i) {
			if (!rockchip_dnl_key_pressed()) {
				goto end;
			}
			mdelay(20);
		}
	}

end:
	cli_simple_run_command("led power off", 0);
	cli_simple_run_command("led standby on", 0);
	return mode;
}

void rockchip_dnl_mode_check(void)
{
	if (!rockchip_dnl_key_pressed()) {
		return 0;
	}

	switch(rockchip_dnl_mode(4)) {
	case 0:
		return;

	case 1:
		printf("entering ums mode...\n");
		cli_simple_run_command("ums 0 mmc 0", 0);
		cli_simple_run_command("ums 0 mmc 1", 0);
		break;

	case 2:
		printf("entering fastboot mode...\n");
		cli_simple_run_command("mmc dev 0; fastboot usb 0", 0);
		cli_simple_run_command("mmc dev 1; fastboot usb 0", 0);
		break;

	case 3:
		printf("entering download mode...\n");
		cli_simple_run_command("rockusb 0 mmc 0", 0);
		cli_simple_run_command("rockusb 0 mmc 1", 0);
		break;

	case 4:
		printf("entering maskrom mode...\n");
		break;
	}

	set_back_to_bootrom_dnl_flag();
	do_reset(NULL, 0, 0, NULL);
}

int setup_boot_mode(void)
{
	void *reg;
	int boot_mode;
	char env_preboot[256] = {0};

	rockchip_dnl_mode_check();

	reg = (void *)CONFIG_ROCKCHIP_BOOT_MODE_REG;

	boot_mode = readl(reg);

	debug("boot mode %x.\n", boot_mode);

	/* Clear boot mode */
	writel(BOOT_NORMAL, reg);

	switch (boot_mode) {
	case BOOT_NORMAL:
		printf("normal boot\n");
		env_set("boot_mode", "normal");
		break;
	case BOOT_LOADER:
		printf("enter Rockusb!\n");
		env_set("boot_mode", "loader");
		env_set("preboot", "setenv preboot; rockusb 0 mmc 0");
		break;
	case BOOT_RECOVERY:
		printf("enter recovery!\n");
		env_set("boot_mode", "recovery");
		env_set("reboot_mode", "recovery");
		break;
	case BOOT_FASTBOOT:
		printf("enter fastboot!\n");
		env_set("boot_mode", "fastboot");
#if defined(CONFIG_FASTBOOT_FLASH_MMC_DEV)
		snprintf(env_preboot, 256,
				"setenv preboot; mmc dev %x; fastboot usb 0; ",
				CONFIG_FASTBOOT_FLASH_MMC_DEV);
#elif defined(CONFIG_FASTBOOT_FLASH_NAND_DEV)
		snprintf(env_preboot, 256,
				"setenv preboot; fastboot usb 0; ");
#endif
		env_set("preboot", env_preboot);
		break;
	case BOOT_CHARGING:
		printf("enter charging!\n");
		env_set("boot_mode", "charging");
		env_set("preboot", "setenv preboot; charge");
		break;
	case BOOT_UMS:
		printf("enter UMS!\n");
		env_set("boot_mode", "ums");
		env_set("preboot", "setenv preboot; ums mmc 0");
		break;
	}

	return 0;
}
