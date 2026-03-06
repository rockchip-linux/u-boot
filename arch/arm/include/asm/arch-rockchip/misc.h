/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * RK3399: Architecture common definitions
 *
 * Copyright (C) 2019 Collabora Inc - https://www.collabora.com/
 *      Rohan Garg <rohan.garg@collabora.com>
 */

int rockchip_cpuid_from_efuse(const u32 cpuid_offset,
			      const u32 cpuid_length,
			      u8 *cpuid);
int rockchip_setup_macaddr(void);
int rockchip_setup_serial_number(void);
void rockchip_capsule_update_board_setup(void);
void board_run_recovery_wipe_data(void);
