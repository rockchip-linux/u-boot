#ifndef COMMON_H
#define COMMON_H

#include <compiler.h>
#include <stdbool.h>
#include <linux/types.h>
#include <blk.h>
#include <linux/errno.h>

struct blk_desc *plat_bootdev(void);

#ifndef CONFIG_XPL_BUILD
int plat_boot_mode(void);
#else
int plat_boot_mode(struct blk_desc *dev_desc, u32 bcb_sector_offset);
#endif

//static inline struct blk_desc *plat_bootdev(void)
//{
//	return NULL;
//}

#endif