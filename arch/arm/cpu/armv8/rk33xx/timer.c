/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <div64.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define RKTIMER_VERSION		"1.5"

/* timer base */
#define TIMER_REG_BASE		RKIO_TIMER_BASE


/* rk timer register offset */
#if defined(CONFIG_RKTIMER_V2)
	#define RK_TIMER_LOADE_COUNT0		0x00
	#define RK_TIMER_LOADE_COUNT1		0x04
	#define RK_TIMER_CURRENT_VALUE0		0x08
	#define RK_TIMER_CURRENT_VALUE1		0x0C
	#define RK_TIMER_CONTROL_REG		0x10
	#define RK_TIMER_EOI			0x14
	#define RK_TIMER_INTSTATUS		0x18

	#define TIMER_LOADE_COUNT		RK_TIMER_LOADE_COUNT0
	#define TIMER_CURR_VALUE		RK_TIMER_CURRENT_VALUE0

	#define TIMER_CTRL_REG			RK_TIMER_CONTROL_REG
	#define TIMER_LOAD_VAL			0xffffffff
#elif defined(CONFIG_RKTIMER_V3)
	/* rk timer register offset */
	#define RK_TIMER_LOADE_COUNT0		0x00
	#define RK_TIMER_LOADE_COUNT1		0x04
	#define RK_TIMER_CURRENT_VALUE0		0x08
	#define RK_TIMER_CURRENT_VALUE1		0x0C
	#define RK_TIMER_LOADE_COUNT2		0x10
	#define RK_TIMER_LOADE_COUNT3		0x14
	#define RK_TIMER_INTSTATUS		0x18
	#define RK_TIMER_CONTROL_REG		0x1C

	#define TIMER_LOADE_COUNT		RK_TIMER_LOADE_COUNT0
	#define TIMER_CURR_VALUE		RK_TIMER_CURRENT_VALUE0

	#define TIMER_CTRL_REG			RK_TIMER_CONTROL_REG
	#define TIMER_LOAD_VAL			0xffffffff
#else
	#error	"PLS define rk timer version."
#endif /* CONFIG_RKTIMER_V2 */

/* rk timer clock source is from 24M crystal input */
#define TIMER_CLOCK		CONFIG_SYS_CLK_FREQ

#define TICKS_PER_HZ		(TIMER_CLOCK / CONFIG_SYS_HZ)
#define TICKS_TO_HZ(x)		((x) / TICKS_PER_HZ)

#define COUNT_TO_USEC(x)	((x) / 24) /* count / ((TIMER_CLOCK / 1000) / 1000) */
#define USEC_TO_COUNT(x)	((x) * 24) /* usec * (TIMER_CLOCK / 1000) / 1000 */

/* rockchip timer is decrementing timer */
static inline uint32 rk_timer_get_curr_count(void)
{
	/*
	 * The hardware timer counts down, therefore we invert to
	 * produce an incrementing timer.
	 */
#ifdef CONFIG_RKTIMER_INCREMENTER
	return readl(TIMER_REG_BASE + TIMER_CURR_VALUE);
#else
	return ~readl(TIMER_REG_BASE + TIMER_CURR_VALUE);
#endif
}

/* init timer register */
int timer_init(void)
{
	writel(TIMER_LOAD_VAL, TIMER_REG_BASE + TIMER_LOADE_COUNT);
	/* auto reload & enable the timer */
	writel(0x01, TIMER_REG_BASE + TIMER_CTRL_REG);

	return 0;
}

/* timer without interrupts */
ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

ulong get_timer_masked(void)
{
	/* current tick value */
	ulong now = TICKS_TO_HZ(rk_timer_get_curr_count());

	if (now >= gd->arch.lastinc)	/* normal (non rollover) */
		gd->arch.tbl += (now - gd->arch.lastinc);
	else {
		/* rollover */
		gd->arch.tbl += (TICKS_TO_HZ(TIMER_LOAD_VAL)
				- gd->arch.lastinc) + now;
	}
	gd->arch.lastinc = now;

	return gd->arch.tbl;
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
	long tmo = USEC_TO_COUNT(usec);
	ulong now, last = rk_timer_get_curr_count();

	while (tmo > 0) {
		now = rk_timer_get_curr_count();
		if (now > last)	/* normal (non rollover) */
			tmo -= now - last;
		else		/* rollover */
			tmo -= TIMER_LOAD_VAL - last + now;
		last = now;
	}
}

/*
 * This function is derived from PowerPC code (read timebase as long).
 * On ARM it just returns the timer value.
 */
uint64_t get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}
