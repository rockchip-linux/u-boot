/*
 * (C) Copyright 2020 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef RK_EBOOK_H
#define RK_EBOOK_H

enum type_logo {
	EBOOK_LOGO_RESET = 0,
	EBOOK_LOGO_UBOOT = 1 << 0,
	EBOOK_LOGO_KERNEL = 1 << 1,
	EBOOK_LOGO_CHARGING_0 = 1 << 2,
	EBOOK_LOGO_CHARGING_1 = 1 << 3,
	EBOOK_LOGO_CHARGING_2 = 1 << 4,
	EBOOK_LOGO_CHARGING_3 = 1 << 5,
	EBOOK_LOGO_CHARGING_4 = 1 << 6,
	EBOOK_LOGO_CHARGING_5 = 1 << 7,
	EBOOK_LOGO_CHARGING_LOWPOWER = 1 << 8,
	EBOOK_LOGO_POWEROFF = 1 << 9,
	EBOOK_LOGO_UNMIRROR_TEMP_BUF = 1 << 10,
};

enum update_mode {
	EBOOK_UPDATE_NORMAL = 0,
	EBOOK_UPDATE_DIFF = 1,
};

int rockchip_ebook_show_uboot_logo(void);
int rockchip_ebook_show_charge_logo(int logo_type);

#endif
