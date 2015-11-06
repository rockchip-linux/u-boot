/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>

//#define DEBUG

extern bool g_debug;

#define LOGE(fmt, args...) fprintf(stderr, "E/%s(%d): "fmt"\n", __func__, __LINE__, ##args)
#define LOGD(fmt, args...) do {\
    if (g_debug) \
        fprintf(stderr, "D/%s(%d): "fmt"\n", __func__, __LINE__, ##args); \
} while (0)

#endif //COMMON_H
