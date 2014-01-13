
#include    "../../armlinux/config.h"
/**
 * 20091115,HSL@RK .
 * enable timer0 , timer2 for check.
 * timer 1 reserved
 */
void RkldTimePowerOnInit( void )
{
    if (ChipType == CONFIG_RK3066)
    {
        g_rk30Time0Reg->TIMER_LOAD_COUNT = 0;
        g_rk30Time0Reg->TIMER_CTRL_REG = 0x01;
     }
    else
    {
        g_rk3188Time0Reg->TIMER_LOAD_COUNT0 = 0xFFFFFFFF;
        g_rk3188Time0Reg->TIMER_CTRL_REG = 0x01;
    }
}
/**
 *  return unit 0.1 ms.
 */
uint32 RkldTimerGetCount( void )
{
    uint32 value;
    if (ChipType == CONFIG_RK3066)
    {
	value = g_rk30Time0Reg->TIMER_CURR_VALUE;
    }
    else
    {
        value = g_rk3188Time0Reg->TIMER_CURR_VALUE0;
    }
    return (~value);
}

uint32 RkldTimerGetTick( void )
{
    return ((RkldTimerGetCount() * 5 ) / 120); // 1us
}

uint32 Timer0Get100ns( void )
{
    return ((RkldTimerGetCount() * 5 ) / 12); // 100ns
}




