/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <div64.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define RKTIMER_VERSION		"1.4"

/* timer base */
#define TIMER_REG_BASE			RKIO_TIMER_BASE


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
#define TIMER_FREQ			CONFIG_SYS_CLK_FREQ


/* rockchip timer is decrementing timer */
static inline uint32 rk_timer_get_curr_count(void)
{
	return readl(TIMER_REG_BASE + TIMER_CURR_VALUE);
}

static inline void rk_timer_init(void)
{
	writel(TIMER_LOAD_VAL, TIMER_REG_BASE + TIMER_LOADE_COUNT);
	/* auto reload & enable the timer */
	writel(0x01, TIMER_REG_BASE + TIMER_CTRL_REG);
}


/* calculate the equivalent tick value of the timer count */
static inline uint64_t tcount_to_tick(uint64_t tcount)
{
	tcount *= CONFIG_SYS_HZ;
	do_div(tcount, TIMER_FREQ);

	return tcount;
}

/* calculate the equivalent tick value of the timer count */
static inline uint64_t tcount_to_usec(uint64_t tcount)
{
	tcount *= (CONFIG_SYS_HZ * 1000);
	do_div(tcount, TIMER_FREQ);

	return tcount;
}

/* calculate the equivalent timer count of the usec value */
static inline uint64_t usec_to_tcount(uint64_t usec)
{
	usec *= TIMER_FREQ;
	do_div(usec, 1000000);

	return usec;
}

static inline unsigned long get_current_timer_value(void)
{
	unsigned long now = rk_timer_get_curr_count();

#ifdef CONFIG_RKTIMER_INCREMENTER
	if (now >= gd->arch.lastinc)
		gd->arch.tbl += (now - gd->arch.lastinc);
	else /* count up timer underflow */
		gd->arch.tbl += (TIMER_LOAD_VAL - gd->arch.lastinc + now + 1);
#else
	if (gd->arch.lastinc >= now)
		gd->arch.tbl -= (gd->arch.lastinc - now);
	else /* count down timer underflow */
		gd->arch.tbl -= (TIMER_LOAD_VAL + gd->arch.lastinc - now);
#endif
	gd->arch.lastinc = now;

	return gd->arch.tbl;
}

/* nothing really to do with interrupts, just starts up a counter. */
int timer_init(void)
{
	rk_timer_init();
	reset_timer_masked();

	return 0;
}


void reset_timer_masked(void)
{
	/* reset time, capture current incrementer value time */
	gd->arch.lastinc = rk_timer_get_curr_count();	/* Monotonic incrementing timer */
	gd->arch.tbl = 0;	/* start "advancing" time stamp from 0 */
}


/*
 * timer without interrupts
 */
ulong get_timer(ulong base)
{
#ifdef CONFIG_RKTIMER_INCREMENTER
	return get_timer_masked() - base;
#else
	if (base == 0)
		return get_timer_masked();
	else
		return (base - get_timer_masked());
#endif
}


/*
 * get usec timer value
 */
uint64_t get_usec_timer(uint64_t base)
{
#ifdef CONFIG_RKTIMER_INCREMENTER
	return (tcount_to_usec(get_current_timer_value()) - base);
#else
	if (base == 0)
		return tcount_to_usec(get_current_timer_value());
	else
		return (base - tcount_to_usec(get_current_timer_value()));
#endif
}


/* delay x useconds */
void __udelay(unsigned long usec)
{
	long tmo = (long)usec_to_tcount(usec); /* delay tick */
	unsigned long now, last = rk_timer_get_curr_count(); /* last tick */

	while (tmo > 0)	{ /* loop till event */
		now = rk_timer_get_curr_count();
#ifdef CONFIG_RKTIMER_INCREMENTER
		if (now > last)
			tmo -= (now - last);
		else /* count up timer overflow */
			tmo -= (TIMER_LOAD_VAL - last + now + 1);
#else
		if (last >= now)
			tmo -= (last - now);
		else /* count down timer overflow */
			tmo -= (TIMER_LOAD_VAL + last - now);
#endif
		last = now;
	}
}


ulong get_timer_masked(void)
{
	return tcount_to_tick(get_current_timer_value());
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
