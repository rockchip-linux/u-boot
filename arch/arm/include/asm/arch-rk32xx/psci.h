/*
 * Copyright (C) 2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __RKXX_PSCI_H
#define __RKXX_PSCI_H

#define PSCI_SIP_DDR_FREQ	(0x82000008)

u32 invoke_psci_fn(u32 function_id, u32 arg0, u32 arg1, u32 arg2);

#endif /* __RKXX_PSCI_H */
