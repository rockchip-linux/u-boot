
#include "config.h"
/**
 * 20091115,HSL@RK .
 * enable timer0 , timer2 for check.
 * timer 1 reserved
 */
void RkldTimePowerOnInit( void )
{
    pTIMER_REG pReg = (pTIMER_REG)TIMER0_BASE_ADDR;
    pCRU_REG ScuReg = (pCRU_REG)CRU_BASE_ADDR;
    pReg->TIMER_LOAD_COUNT = 0;
    pReg->TIMER_CTRL_REG = 0x01;
}
/**
 *  return unit 0.1 ms.
 */
uint32 RkldTimerGetTick( void )
{
     pTIMER_REG pReg = (pTIMER_REG)TIMER0_BASE_ADDR;
     //uint32 Count = (~(pReg->TIMER_CURR_VALUE));
     //Count >>= 4;
     //Count *= 11;
     //Count >>= 4;  // count * 11 / 256 ~~ count / 24
     //return (Count); // 1us
     return ((~(pReg->TIMER_CURR_VALUE)) / 24); // 1us
}

uint32 Timer0Get100ns( void )
{
    pTIMER_REG pReg = (pTIMER_REG)TIMER0_BASE_ADDR;
    //uint32 Count = (~(pReg->TIMER_CURR_VALUE));
    //Count >>= 4;
    //Count *= 11;
    //Count >>= 4;  
    //Count *= 10;  // count * 110 / 256 ~~ count / 2.4
    //return (Count); // 100ns
    return (((~pReg->TIMER_CURR_VALUE) * 10 ) / 24); // 100ns
}


uint32 RkldTimerGetCount( int timer )
{
    pTIMER_REG pReg = (pTIMER_REG)TIMER0_BASE_ADDR;
    return ~pReg[timer].TIMER_CURR_VALUE;
}



