/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright (c) 2025 Rockchip Electronics Co., Ltd. */

#ifndef __RKCE_DEBUG_H__
#define __RKCE_DEBUG_H__

#include <stdio.h>

#define rk_err(fmt, args...) printf("RKCE: E [%s %d]: " fmt "\n", \
				    __func__, __LINE__, ##args)

#define rk_warn(fmt, args...) printf("RKCE: W [%s %d]: " fmt "\n", \
				    __func__, __LINE__, ##args)

#define rk_info(fmt, args...) printf(fmt, ##args)

#if defined(DEBUG)
#define rk_debug(fmt, args...) printf("RKCE: D [%s %d]: " fmt "\n", \
				      __func__, __LINE__, ##args)

#define rk_trace(fmt, args...) printf("RKCE: T [%s %d]: " fmt "\n", \
				      __func__, __LINE__, ##args)
#else
#define rk_debug(fmt, args...)
#define rk_trace(fmt, args...)
#endif

#define rkce_dump_td(td)

#if defined(DEBUG)
#define rkce_dumphex(var_name, data, len) print_hex_dump(KERN_CONT, (var_name), \
							 DUMP_PREFIX_OFFSET, \
							 16, 1, (data), (len), false)
#else
#define rkce_dumphex(var_name, data, len)
#endif

#endif
