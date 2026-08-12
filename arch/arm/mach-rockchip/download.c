/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <button.h>
#include <hotkey.h>
#include <image.h>
#include <part.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <asm/arch-rockchip/param.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/common.h>
#include <linux/input.h>
#include <linux/usb/phy-rockchip-usb2.h>

DECLARE_GLOBAL_DATA_PTR;

void rbrom_download(void)
{
	if (!is_hotkey(HK_BROM_DNL))
		return;

	printf("Enter bootrom download...");
	writel(BOOT_BROM_DOWNLOAD, (void *)CONFIG_ROCKCHIP_BOOT_MODE_REG);
	do_reset(NULL, 0, 0, NULL);
	printf("failed!\n");
}

static int download_key_pressed(void)
{
#if defined(CONFIG_BUTTON)
	return button_is_on(KEY_VOLUMEUP);
#else
	return 0;
#endif
}

void rockusb_download(void)
{
	int vbus = 1; /* Assumed 1 in case of no rockusb */

	if (download_key_pressed() || is_hotkey(HK_ROCKUSB_DNL)) {
		printf("download %skey pressed... ",
		       is_hotkey(HK_ROCKUSB_DNL) ? "hot" : "");
#ifdef CONFIG_CMD_ROCKUSB
		vbus = rockchip_u2phy_vbus_detect();
#endif
		if (vbus > 0) {
			printf("%sentering download mode...\n",
			       IS_ENABLED(CONFIG_CMD_ROCKUSB) ?
			       "" : "no rockusb, ");

			/* try rockusb download and brom download */
			run_command("download", 0);
		} else {
			printf("entering recovery mode!\n");
			env_set("reboot_mode", "recovery-key");
		}
	} else if (is_hotkey(HK_FASTBOOT)) {
		env_set("reboot_mode", "fastboot");
	}
}
