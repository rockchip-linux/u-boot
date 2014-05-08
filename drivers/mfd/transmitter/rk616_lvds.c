#include <common.h>
#include <lcd.h>
#include <asm/arch/rkplat.h>
#include <i2c.h>
#include <asm/io.h>
#include "../rk616.h"
#include "rk616_lvds.h"


static int rk616_lvds_cfg(int enable,int lvds_mode,int lvds_ch_nr,int lvds_format)
{
	int val = 0;
	int odd = (panel_info.vl_hbpd & 0x01)?0:1;


	if(enable == 0) //lvds port is not used ,power down lvds
        {
                val &= ~(LVDS_CH1TTL_EN | LVDS_CH0TTL_EN | LVDS_CH1_PWR_EN |
                        LVDS_CH0_PWR_EN | LVDS_CBG_PWR_EN);
                val |= LVDS_PLL_PWR_DN | (LVDS_CH1TTL_EN << 16) | (LVDS_CH0TTL_EN << 16) |
                        (LVDS_CH1_PWR_EN << 16) | (LVDS_CH0_PWR_EN << 16) |
                        (LVDS_CBG_PWR_EN << 16) | (LVDS_PLL_PWR_DN << 16);

                I2C_WRITE(CRU_LVDS_CON0,&val);
        }
        else
        {
                if(lvds_mode == SCREEN_LVDS)  //lvds mode
                {

                        if(lvds_ch_nr == 2) //dual lvds channel
                        {
                                val = 0;
                                val &= ~(LVDS_CH0TTL_EN | LVDS_CH1TTL_EN | LVDS_PLL_PWR_DN);
                                val = (LVDS_DCLK_INV)|(LVDS_CH1_PWR_EN) |(LVDS_CH0_PWR_EN) | LVDS_HBP_ODD(odd) |
                                        (LVDS_CBG_PWR_EN) | (LVDS_CH_SEL) | (LVDS_OUT_FORMAT(lvds_format)) |
                                        (LVDS_CH0TTL_EN << 16) | (LVDS_CH1TTL_EN << 16) |(LVDS_CH1_PWR_EN << 16) |
                                        (LVDS_CH0_PWR_EN << 16) | (LVDS_CBG_PWR_EN << 16) | (LVDS_CH_SEL << 16) |
                                        (LVDS_OUT_FORMAT_MASK) | (LVDS_DCLK_INV << 16) | (LVDS_PLL_PWR_DN << 16) |
                                        (LVDS_HBP_ODD_MASK);

                		I2C_WRITE(CRU_LVDS_CON0,&val);
                        }
                        else //single lvds channel
                        {
                                val = 0;
                                val &= ~(LVDS_CH0TTL_EN | LVDS_CH1TTL_EN | LVDS_CH1_PWR_EN | LVDS_PLL_PWR_DN | LVDS_CH_SEL); //use hannel 0
                                val |= (LVDS_CH0_PWR_EN) |(LVDS_CBG_PWR_EN) | (LVDS_OUT_FORMAT(lvds_format)) |
                                      (LVDS_CH0TTL_EN << 16) | (LVDS_CH1TTL_EN << 16) |(LVDS_CH0_PWR_EN << 16) |
                                       (LVDS_DCLK_INV ) | (LVDS_CH0TTL_EN << 16) | (LVDS_CH1TTL_EN << 16) |(LVDS_CH0_PWR_EN << 16) |

                                        (LVDS_CBG_PWR_EN << 16)|(LVDS_CH_SEL << 16) | (LVDS_PLL_PWR_DN << 16)|
                                       (LVDS_OUT_FORMAT_MASK) | (LVDS_DCLK_INV << 16);
                                I2C_WRITE(CRU_LVDS_CON0,&val);

                        }

                }
                else //mux lvds port to RGB mode
                {
                        val &= ~(LVDS_CBG_PWR_EN| LVDS_CH1_PWR_EN | LVDS_CH0_PWR_EN);
                        val |= (LVDS_CH0TTL_EN)|(LVDS_CH1TTL_EN )|(LVDS_PLL_PWR_DN)|
                                (LVDS_CH0TTL_EN<< 16)|(LVDS_CH1TTL_EN<< 16)|(LVDS_CH1_PWR_EN << 16) |
                                (LVDS_CH0_PWR_EN << 16)|(LVDS_CBG_PWR_EN << 16)|(LVDS_PLL_PWR_DN << 16);
                        I2C_WRITE(CRU_LVDS_CON0,&val);

                        val &= ~(LVDS_OUT_EN);
                        val |= (LVDS_OUT_EN << 16);
                        I2C_WRITE(CRU_IO_CON0,&val);
                }
        }
        return 0;
}




int set_lvds_reg(int enable)
{	
	int lvds_mode = 0,lvds_ch_nr =0,lvds_format =0;

	lvds_mode = panel_info.screen_type;
	lvds_ch_nr = panel_info.lvds_ch_nr;
	lvds_format = panel_info.lvds_format;
	
	rk616_lvds_cfg(enable,lvds_mode,lvds_ch_nr,lvds_format);		

	return 0;
}


