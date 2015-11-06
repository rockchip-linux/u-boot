/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "common.h"

bool g_debug = 
#ifdef DEBUG
    true;
#else
    false;
#endif //DEBUG
