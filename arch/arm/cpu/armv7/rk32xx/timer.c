/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/io.h>
#include <div64.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define RKTIMER_VERSION		"1.3"


#if defined(CONFIG_RKCHIP_RK3288)
#define TIMER_REG_BASE			RKIO_TIMER_6CH_PHYS
#elif defined(CONFIG_RKCHIP_RK3036) \
	|| defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
#define TIMER_REG_BASE			RKIO_TIMER_PHYS
#else
	#error "Please define timer base for chip type!"
#endif


/* rk timer register offset */
#if defined(CONFIG_RKCHIP_RK3288) || defined(CONFIG_RKCHIP_RK3036) \
	|| defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
#define RK_TIMER_LOADE_COUNT0		0x00
#define RK_TIMER_LOADE_COUNT1		0x04
#define RK_TIMER_CURRENT_VALUE0		0x08
#define RK_TIMER_CURRENT_VALUE1		0x0C
#define RK_TIMER_CONTROL_REG		0x10
#define RK_TIMER_EOI			0x14
#define RK_TIMER_INTSTATUS		0x18

#define TIMER_LOADE_COUNT		RK_TIMER_LOADE_COUNT0
#define TIMER_CURR_VALUE		RK_TIMER_CURRENT_VALUE0

#else
	#error "Please define timer register for chip type!"
#endif


#define TIMER_CTRL_REG		RK_TIMER_CONTROL_REG
#define TIMER_LOAD_VAL		0xffffffff
/* rk timer clock source is from 24M crystal input */
#define TIMER_FREQ		CONFIG_SYS_CLK_FREQ


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
static inline unsigned long long tcount_to_tick(unsigned long long tcount)
{
	tcount *= CONFIG_SYS_HZ;
	do_div(tcount, TIMER_FREQ);

	return tcount;
}

/* calculate the equivalent tick value of the timer count */
static inline unsigned long long tcount_to_usec(unsigned long long tcount)
{
	tcount *= (CONFIG_SYS_HZ * 1000);
	do_div(tcount, TIMER_FREQ);

	return tcount;
}

/* calculate the equivalent timer count of the usec value */
static inline unsigned long long usec_to_tcount(unsigned long long usec)
{
	usec *= TIMER_FREQ;
	do_div(usec, 1000000);

	return usec;
}

static inline unsigned long get_current_timer_value(void)
{
	unsigned long now = rk_timer_get_curr_count();

	if (gd->arch.lastinc >= now) {
		gd->arch.tbl -= (gd->arch.lastinc - now);
	} else {/* count down timer underflow */
		gd->arch.tbl -= (TIMER_LOAD_VAL + gd->arch.lastinc - now);
	}
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
	/* reset time */
	/* init the gd->arch.lastinc and gd->arch.tbl value */
	gd->arch.lastinc = rk_timer_get_curr_count();	/* Monotonic decrementing timer */
	gd->arch.tbl = 0;	/* Last decremneter snapshot, start "advancing" time stamp from 0 */
}


/*
 * timer without interrupts
 */
unsigned long get_timer(unsigned long base)
{
	if (base == 0) {
		return get_timer_masked();
	} else {
		return (base - get_timer_masked());
	}
}


/*
 * get usec timer value
 */
unsigned long get_usec_timer(unsigned long base)
{
	if (base == 0) {
		return tcount_to_usec(get_current_timer_value());
	} else {
		return (base - tcount_to_usec(get_current_timer_value()));
	}
}


//#define UDELAY_TEST

/* delay x useconds */
void __udelay(unsigned long usec)
{
	long tmo = usec_to_tcount(usec); /* delay tick */
	unsigned long now, last = rk_timer_get_curr_count(); /* last tick */

	while (tmo > 0)	{ /* loop till event */
		now = rk_timer_get_curr_count();
		if (last >= now) {
			tmo -= (last - now);
		} else { /* count down timer overflow */
			tmo -= (TIMER_LOAD_VAL + last - now);
		}

		last = now;
	}
}


unsigned long get_timer_masked(void)
{
	return tcount_to_tick(get_current_timer_value());
}


/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
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
