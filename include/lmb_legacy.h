#ifndef _LINUX_LMB_LEGACY_H
#define _LINUX_LMB_LEGACY_H
#ifdef __KERNEL__

#include <asm/types.h>
/*
 * Logical memory blocks.
 *
 * Copyright (C) 2001 Peter Bergner, IBM Corp.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define MAX_LMB_REGIONS 16

struct lmb_lg_property {
	phys_addr_t base;
	phys_size_t size;
};

struct lmb_lg_region {
	unsigned long cnt;
	phys_size_t size;
	struct lmb_lg_property region[MAX_LMB_REGIONS+1];
};

struct lmb_lg {
	struct lmb_lg_region memory;
	struct lmb_lg_region reserved;
};

extern struct lmb_lg lmb;

extern void lmb_lg_init(struct lmb_lg *lmb);
extern long lmb_lg_add(struct lmb_lg *lmb, phys_addr_t base, phys_size_t size);
extern long lmb_lg_reserve(struct lmb_lg *lmb, phys_addr_t base, phys_size_t size);
extern phys_addr_t lmb_lg_alloc(struct lmb_lg *lmb, phys_size_t size, ulong align);
extern phys_addr_t lmb_lg_alloc_base(struct lmb_lg *lmb, phys_size_t size, ulong align,
			    phys_addr_t max_addr);
extern phys_addr_t __lmb_lg_alloc_base(struct lmb_lg *lmb, phys_size_t size, ulong align,
			      phys_addr_t max_addr);
extern int lmb_lg_is_reserved(struct lmb_lg *lmb, phys_addr_t addr);
extern long lmb_lg_free(struct lmb_lg *lmb, phys_addr_t base, phys_size_t size);

extern void lmb_lg_dump_all(struct lmb_lg *lmb);

static inline phys_size_t
lmb_size_bytes(struct lmb_lg_region *type, unsigned long region_nr)
{
	return type->region[region_nr].size;
}

void board_lmb_reserve(struct lmb_lg *lmb);
void arch_lmb_reserve(struct lmb_lg *lmb);

#endif /* __KERNEL__ */

#endif /* _LINUX_LMB_LEGACY_H */
