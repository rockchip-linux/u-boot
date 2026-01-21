/*
 * (C) Copyright 2020 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef RK_EBOOK_H
#define RK_EBOOK_H

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif

enum type_logo {
	EBOOK_LOGO_RESET		= 0,
	EBOOK_LOGO_UBOOT		= BIT(0),
	EBOOK_LOGO_KERNEL		= BIT(1),
	EBOOK_LOGO_CHARGING_0		= BIT(2),
	EBOOK_LOGO_CHARGING_1		= BIT(3),
	EBOOK_LOGO_CHARGING_2		= BIT(4),
	EBOOK_LOGO_CHARGING_3		= BIT(5),
	EBOOK_LOGO_CHARGING_4		= BIT(6),
	EBOOK_LOGO_CHARGING_5		= BIT(7),
	EBOOK_LOGO_CHARGING_LOWPOWER	= BIT(8),
	EBOOK_LOGO_POWEROFF		= BIT(9),
	EBOOK_LOGO_UNMIRROR_TEMP_BUF	= BIT(10),
};

enum update_mode {
	EBOOK_UPDATE_NORMAL = 0,
	EBOOK_UPDATE_DIFF = 1,
};

int rockchip_ebook_show_uboot_logo(void);
int rockchip_ebook_show_charge_logo(int logo_type);

#endif
