/*
** Copyright (c) 2009 ARM Ltd. All rights reserved.
*/

#ifndef PL310_L2CC_H
#define PL310_L2CC_H
#define   L2X0_DYNAMIC_CLK_GATING_EN	(1 << 1)
#define   L2X0_STNDBY_MODE_EN		(1 << 0)

/* Registers shifts and masks */
#define L2X0_CACHE_ID_REV_MASK		(0x3f)
#define L2X0_CACHE_ID_PART_MASK		(0xf << 6)
#define L2X0_CACHE_ID_PART_L210		(1 << 6)
#define L2X0_CACHE_ID_PART_L310		(3 << 6)

#define L2X0_AUX_CTRL_MASK			0xc0000fff
#define L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT	16
#define L2X0_AUX_CTRL_WAY_SIZE_SHIFT		17
#define L2X0_AUX_CTRL_WAY_SIZE_MASK		(0x7 << 17)
#define L2X0_AUX_CTRL_SHARE_OVERRIDE_SHIFT	22
#define L2X0_AUX_CTRL_NS_LOCKDOWN_SHIFT		26
#define L2X0_AUX_CTRL_NS_INT_CTRL_SHIFT		27
#define L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT	28
#define L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT	29
#define L2X0_AUX_CTRL_EARLY_BRESP_SHIFT		30

#define REV_PL310_R2P0				4
void L2x0Init(void);
void L2x0Deinit(void);
void l2x0_flush_range(unsigned long start, unsigned long end);

#endif
