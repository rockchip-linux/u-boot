/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	timer.c
Desc 	:	TIMER驱动程序
Author 	:  	yangkai
Date 	:	2008-11-11
Notes 	:

********************************************************************/
#define IN_TIMER
#include  <asm/arch/rk28_drivers.h>
#ifdef DRIVERS_TIMER
#include <common.h>
#include <div64.h>

#define TIMER_LOAD_VAL	0xffffffff
#define READ_TIMER_RK (g_timerReg->Timer2CurrentValue)
static ulong timestamp;
static ulong lastinc;
static ulong timer_freq;

/*----------------------------------------------------------------------
Name	: TimerSetCount
Desc	: 根据APB频率设置TIMER计数初值等参数
Params  : timerNum:TIMER编号
          usTick:TIMER定时微秒数
Return  : 0:成功
Notes   :
----------------------------------------------------------------------*/
uint32 TimerSetCount(eTIMER_NUM timerNum, uint32 usTick)
{
    uint32 loadCount;
//    loadCount = SCUGetAPBFreq();
    loadCount = usTick*25;  //25M clk
//    loadCount = msTick;

    switch (timerNum)
    {
        case TIMER1:
            g_timerReg->Timer1ControlReg = 0;            //disable the timer
            g_timerReg->Timer1LoadCount = loadCount;     //load the init count value
            g_timerReg->Timer1ControlReg |= 0x03;        //enable the timer
            *(volatile uint32 *)GPIO0_BASE_ADDR |= 1; 
            break;
        case TIMER2:
            g_timerReg->Timer2ControlReg = 0;            //disable the timer
            g_timerReg->Timer2LoadCount = loadCount;     //load the init count value
            g_timerReg->Timer2ControlReg |= 0x03;        //enable the timer
            *(volatile uint32 *)GPIO0_BASE_ADDR |= 2; 
            break;
        case TIMER3:
            g_timerReg->Timer3ControlReg = 0;            //disable the timer
            g_timerReg->Timer3LoadCount = loadCount;     //load the init count value
            g_timerReg->Timer3ControlReg |= 0x03;        //enable the timer
            *(volatile uint32 *)GPIO0_BASE_ADDR |= 4; 
            break;
        default:
            break;
    }

    return(0);
}
/*----------------------------------------------------------------------
Name	: TimerInit
Desc	: TIMER初始化
Params  : timerNum:TIMER编号
Return  : 0:成功
Notes   : 
----------------------------------------------------------------------*/
uint32 TimerInit(eTIMER_NUM timerNum)
{
    g_timerReg = (pTIMER_REG)TIMER_BASE_ADDR;
    g_timerIRQ[timerNum] = 0;

    switch (timerNum)
    {
        case TIMER1:
            g_timerReg->Timer1ControlReg = 0;//disable timer1
//            IRQRegISR(IRQ_TIMER1, Timer1ISR);
//            IRQEnable(IRQ_TIMER1);
            break;
        case TIMER2:
            g_timerReg->Timer2ControlReg = 0;//disable timer2
//            IRQRegISR(IRQ_TIMER2, Timer2ISR);
//            IRQEnable(IRQ_TIMER2);
            break;
        case TIMER3:
            g_timerReg->Timer3ControlReg = 0;//disable timer1
//            IRQRegISR(IRQ_TIMER3, Timer3ISR);
//            IRQEnable(IRQ_TIMER3);
            break;
        default:
            break;
    }

    return(0);
}

static inline unsigned long long tick_to_time(unsigned long long tick)
{
	tick *= CONFIG_SYS_HZ;
	do_div(tick, timer_freq);
	return tick;
}

static inline unsigned long long usec_to_tick(unsigned long long usec)
{
	usec *= timer_freq;
	do_div(usec, 1000000);
	return usec;
}

/* nothing really to do with interrupts, just starts up a counter. */
int timer_init(void)
{
//	at91_pmc_t *pmc = (at91_pmc_t *) AT91_PMC_BASE;
//	at91_pit_t *pit = (at91_pit_t *) AT91_PIT_BASE;
	/*
	 * Enable PITC Clock
	 * The clock is already enabled for system controller in boot
	 */
//	writel(1 << AT91_ID_SYS, &pmc->pcer);

	/* Enable PITC */
//	writel(TIMER_LOAD_VAL | AT91_PIT_MR_EN , &pit->mr);

//	reset_timer_masked();

//	timer_freq = get_mck_clk_rate() >> 4;
       TimerInit(TIMER2);
 TimerSetCount(TIMER2, TIMER_LOAD_VAL);

//	SCUSetClkInfo();
      timer_freq = SCUGetAPBFreq()*1000000; 

reset_timer_masked();

	return 0;
}

/*
 * timer without interrupts
 */
unsigned long long get_ticks(void)
{
//	at91_pit_t *pit = (at91_pit_t *) AT91_PIT_BASE;

//	ulong now = readl(&pit->piir);
        ulong now = READ_TIMER_RK;

//	if (now >= lastinc)	/* normal mode (non roll) */
		/* move stamp forward with absolut diff ticks */
//		timestamp += (now - lastinc);
//	else			/* we have rollover of incrementer */
//		timestamp += (0xFFFFFFFF - lastinc) + now;
//	lastinc = now;

	if (now <= lastinc)
		timestamp -= (lastinc - now);
	else
		timestamp -= (0xFFFFFFFF + lastinc) - now;
		lastinc = now;

	return timestamp;
}

void reset_timer_masked(void)
{
	/* reset time */
//	at91_pit_t *pit = (at91_pit_t *) AT91_PIT_BASE;

	/* capture current incrementer value time */
//	lastinc = readl(&pit->piir);
	lastinc = READ_TIMER_RK;
	timestamp = 0; /* start "advancing" time stamp from 0 */
}

ulong get_timer_masked(void)
{
	return tick_to_time(get_ticks());
}

void __udelay(unsigned long usec)
{
	unsigned long long tmp;
	ulong tmo;

	tmo = usec_to_tick(usec);
	tmp = get_ticks() - tmo;	/* get current timestamp */

	while (get_ticks() > tmp)	/* loop till event */
		 /*NOP*/;
}

void reset_timer(void)
{
	reset_timer_masked();
}

ulong get_timer(ulong base)
{
	if (base == 0)
		return 1;
	else
		return base - get_timer_masked ();
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	ulong tbclk;

	tbclk = CONFIG_SYS_HZ;
	return tbclk;
}

#endif
