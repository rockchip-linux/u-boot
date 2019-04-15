/*
 * (C) Copyright 2019 FrontRow Calypso LLC
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dwc3-uboot.h>
#include <power/regulator.h>
#include <usb.h>

DECLARE_GLOBAL_DATA_PTR;

int rk_board_late_init(void)
{
	printf("Rosetta-RK3328 late init ...\n");

   	if (!env_get("fdtfile"))
   		env_set("fdtfile", "rk3328-rosetta.dtb");
}

