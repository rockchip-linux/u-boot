/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2024 Rockchip Electronics Co., Ltd
 */

#ifndef __RK_MOS_H_
#define __RK_MOS_H_

int mos_secondary_boot(void);
int mos_secondary_late_boot(void);
void mos_secondary_wfe(void);
int mos_fdt_overlay(void *fdt);
int mos_set_boot_stage(u32 stage);
int mos_spl_init(void);
int mos_spl_late_init(void);
int mos_spl_cfg_init(void);
void mos_system_reset(void);
ulong mos_safety_atags_addr(void);

#endif
