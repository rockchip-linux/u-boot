/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
#include <common.h>
#include <asm/io.h>
#include <div64.h>
#include <asm/arch/rk30_drivers.h>


DECLARE_GLOBAL_DATA_PTR;

#define TIMER_LOAD_VAL	0xffffffff

#define TIMER_FREQ	CONFIG_SYS_CLK_FREQ


static unsigned long get_current_tick(void);


inline unsigned long long tick_to_time(unsigned long long tick)
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
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066) || (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
	/* set count value */
	g_rk30Time0Reg->TIMER_LOAD_COUNT = TIMER_LOAD_VAL;
	/* auto reload & enable the timer */
	g_rk30Time0Reg->TIMER_CTRL_REG = 0x01;
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188 ||CONFIG_RKCHIPTYPE == CONFIG_RK3026)
	g_rk3188Time0Reg->TIMER_LOAD_COUNT0 = TIMER_LOAD_VAL;
	g_rk3188Time0Reg->TIMER_CTRL_REG = 0x01;
#endif 

	reset_timer_masked();
	return 0;
}

inline unsigned long get_rk_current_tick()
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066) || (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
    return g_rk30Time0Reg->TIMER_CURR_VALUE;
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188 ||CONFIG_RKCHIPTYPE == CONFIG_RK3026)
    return g_rk3188Time0Reg->TIMER_CURR_VALUE0;
#endif
}

void reset_timer_masked(void)
{
	gd->arch.lastinc = get_rk_current_tick();	/* Monotonic incrementing timer */
	gd->arch.tbl = 0;				/* Last decremneter snapshot */
}


/*
 * timer without interrupts
 */
unsigned long get_timer(unsigned long base)
{
	if (base == 0)
		return get_timer_masked();
	else
		return (base - get_timer_masked());
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
    long long tmo;
    long long now, last;

    tmo = usec_to_tick(usec);

    /* get current timestamp */
    last = get_rk_current_tick();
    while (tmo > 0)	/* loop till event */
    {
        now = get_rk_current_tick();
        if (last >= now) {
            tmo -= last - now;
        } else {
            tmo -= 0xFFFFFFFF + last - now;
        }
        last = now;
    }
}


unsigned long get_timer_masked(void)
{
	return tick_to_time(get_current_tick());
}


static unsigned long get_current_tick(void)
{
	unsigned long now;

    now = get_rk_current_tick();

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
