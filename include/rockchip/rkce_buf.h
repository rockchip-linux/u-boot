/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright (c) 2025 Rockchip Electronics Co., Ltd. */

#ifndef __RKCE_BUF_H__
#define __RKCE_BUF_H__

#include <malloc.h>
#include <linux/types.h>

#define RKCE_BUF_ALIGN_SIZE		16

#define rkce_cma_init(device)
#define rkce_cma_deinit(device)

#define rkce_cma_alloc(size) memalign(RKCE_BUF_ALIGN_SIZE, size)
#define rkce_cma_free(buf) free(buf)

#define rkce_cma_virt2phys(buf) (((unsigned long)buf) & 0xffffffff)
#define rkce_cma_phys2virt(phys) ((unsigned long)phys)

#endif
