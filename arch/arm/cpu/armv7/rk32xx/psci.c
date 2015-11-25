/*
 * Copyright (C) 2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#include <common.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/arch/rkplat.h>
#include <linux/stringify.h>

#define ___inst_arm(x) ".long " __stringify(x) "\n\t"

#define __inst_arm(x) ___inst_arm(x)

#define __inst_arm_thumb32(arm_opcode, thumb_opcode) __inst_arm(arm_opcode)

#define __SMC(imm4) __inst_arm_thumb32(		\
	   0xE1600070 | (((imm4) & 0xF) << 0),	\
	   0xF7F08000 | (((imm4) & 0xF) << 16)	\
)

static u32 smc_wr_fn(u32 fn_id, u32 arg0, u32 arg1, u32 arg2)
{
	asm volatile(
		__SMC(0) : "+r" (fn_id), "+r" (arg0) : "r" (arg1), "r" (arg2)
	);

	return fn_id;
}

u32 invoke_psci_fn(u32 function_id, u32 arg0, u32 arg1, u32 arg2)
{
	return smc_wr_fn(function_id, arg0, arg1, arg2);
}
