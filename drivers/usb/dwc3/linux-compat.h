/**
 * linux-compat.h - DesignWare USB3 Linux Compatibiltiy Adapter  Header
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com
 *
 * Authors: Kishon Vijay Abraham I <kishon@ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 */

#ifndef __DWC3_LINUX_COMPAT__
#define __DWC3_LINUX_COMPAT__

#define pr_debug(format)                debug(format)
#define WARN(val, format, arg...)	debug(format, ##arg)
#define dev_WARN(dev, format, arg...)	debug(format, ##arg)
#define WARN_ON_ONCE(val)		debug("Error %d\n", val)

#define __GFP_ZERO	((__force gfp_t)0x8000u)	/* Return zeroed page on success */


static inline size_t strlcat(char *dest, const char *src, size_t n)
{
	strcat(dest, src);
	return strlen(dest) + strlen(src);
}

static inline void *devm_kzalloc(struct device *dev, unsigned int size,
				 unsigned int flags)
{
	return kzalloc(size, flags);
}
static inline void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	if (size != 0 && n > (~(size_t)0) / size)
		return NULL;
	return kmalloc(n * size, flags | __GFP_ZERO);
}

#endif
