/*
 * (C) Copyright 2013
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
#include <asm/arch/rk30_drivers.h>

DECLARE_GLOBAL_DATA_PTR;

#define TIMER_LOAD_VAL	0xffffffff

#define TIMER_FREQ	CONFIG_SYS_CLK_FREQ


static unsigned long get_current_tick(void);


static inline unsigned long long tick_to_time(unsigned long long tick)
{
	tick *= CONFIG_SYS_HZ;
	do_div(tick, TIMER_FREQ);

	return tick;
}

static inline unsigned long long usec_to_tick(unsigned long long usec)
{
	usec *= TIMER_FREQ;
	do_div(usec, 1000000);

	return usec;
}


int timer_init(void)
{
	pTIMER_REG ptimerReg = (pTIMER_REG)(RK30_TIMER0_PHYS);

	/* set count value */
	ptimerReg->TIMER_LOAD_COUNT = 0;

	/* auto reload & enable the timer */
	ptimerReg->TIMER_CTRL_REG = 1;    

	//reset_timer_masked();
	
	return 0;
}


void reset_timer_masked(void)
{
	pTIMER_REG ptimerReg = (pTIMER_REG)(RK30_TIMER0_PHYS);

	/* reset time */
	gd->arch.lastinc = ptimerReg->TIMER_CURR_VALUE;	/* Monotonic incrementing timer */
	gd->arch.tbl = 0;				/* Last decremneter snapshot */
}


/*
 * timer without interrupts
 */
unsigned long get_timer(unsigned long base)
{
	if (base == 0)
		return 1;
	
	return base - get_timer_masked ();
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
	unsigned long long tmp;
	unsigned long long tmo;

	tmo = usec_to_tick(usec);
	tmp = get_current_tick() - tmo;	/* get current timestamp */

	while (get_current_tick() > tmp);	/* loop till event */
}


unsigned long get_timer_masked(void)
{
	return tick_to_time(get_current_tick());
}


static unsigned long get_current_tick(void)
{
	pTIMER_REG ptimerReg = (pTIMER_REG)(RK30_TIMER0_PHYS);
	unsigned long now = ptimerReg->TIMER_CURR_VALUE;

	if (gd->arch.lastinc >= now)
		gd->arch.tbl -= (gd->arch.lastinc - now);
	else
		gd->arch.tbl -= 0xFFFFFFFF + gd->arch.lastinc - now;

	gd->arch.lastinc = now;

	return gd->arch.tbl;
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
