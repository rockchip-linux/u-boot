#include <common.h>
#include <lcd.h>
#include <asm/arch/rkplat.h>
#include <i2c.h>
#include <asm/io.h>
#include "rk616.h"
#include "transmitter/rk616_lvds.h"

#if  (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
#define GRF_IOMUX_CONFIG_I2S (1<<0 | 1<<2 | 1<<4 | 1<<6 | 1<<8 | 1<<10 | 1<<12 | 1<<14|(0xffff0000))
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)
#define GRF_IOMUX_CONFIG_I2S (1<<0 | 1<<2 | 1<<4 | 1<<6 | 1<<8 | 1<<10 |(0xffff0000))
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
#define GRF_IOMUX_CONFIG_I2S (1<<0 | 1<<2 | 1<<4 | 1<<6 | 1<<8 | 1<<10 |(0xffff0000))
#endif

#define CRU_I2S_CLK_24M ((0x2<<8)|(0x2<<8)<<16)	
//#define RK616_TEST


static int rk616_use_lcd0_cfg(rk616_route *route)
{
	route->vif0_bypass = VIF0_CLK_BYPASS;
	route->vif0_en     = 0;
 	route->vif0_clk_sel = VIF0_CLKIN_SEL(VIF_CLKIN_SEL_PLL0);
	route->pll0_clk_sel = PLL0_CLK_SEL(LCD0_DCLK);
	route->dither_sel  = DITHER_IN_SEL(DITHER_SEL_VIF0); //dither from vif0

	route->pll1_clk_sel = 0;//PLL1_CLK_SEL(LCD1_DCLK);
	route->vif1_clk_sel = VIF1_CLKIN_SEL(VIF_CLKIN_SEL_PLL1);
	route->hdmi_sel     = HDMI_IN_SEL(HDMI_CLK_SEL_VIF1);

	route->vif1_bypass = VIF1_CLK_BYPASS;
	route->vif1_en     = 0;
	route->scl_en      = 0;            
	route->sclin_sel   = SCL_IN_SEL(SCL_SEL_VIF0); 
	route->lcd1_input  = 0; 
	

	return 0;
}


static void rk616_router_cfg(rk616_route * route)
{
	u32 val = 0;
	int ret = 0;

	val = (route->pll0_clk_sel) | (route->pll1_clk_sel) |
		PLL1_CLK_SEL_MASK | PLL0_CLK_SEL_MASK; //pll1 clk from lcdc1_dclk,pll0 clk from lcdc0_dclk,mux_lcdx = lcdx_clk
	I2C_WRITE(CRU_CLKSEL0_CON,&val);
	
	val = (route->sclk_sel) | SCLK_SEL_MASK;
	I2C_WRITE(CRU_CLKSEL1_CON,&val);
	
	val = (SCL_IN_SEL_MASK) | (DITHER_IN_SEL_MASK) | (HDMI_IN_SEL_MASK) | 
		(VIF1_CLKIN_SEL_MASK) | (VIF0_CLKIN_SEL_MASK) | (VIF1_CLK_BYPASS << 16) | 
		(VIF0_CLK_BYPASS << 16) |(route->sclin_sel) | (route->dither_sel) | 
		(route->hdmi_sel) | (route->vif1_bypass) | (route->vif0_bypass) |
		(route->vif1_clk_sel)| (route->vif0_clk_sel); 
	I2C_WRITE(CRU_CLKSEL2_CON,&val);

	return ;
}


static void rk616_dither_cfg(rk616_route * route)
{
	int val = 0;

        if(panel_info.screen_type != SCREEN_RGB) //if RGB screen , not invert D_CLK
                val = FRC_DCLK_INV | (FRC_DCLK_INV << 16);

	I2C_WRITE(FRC_REG,&val);	

	return;
}

 
static int rk616_set_reg(int channel)
{
	struct rk616_route * route;
	
	i2c_set_bus_num(I2C_CH);	

	route = calloc(sizeof(route),1);

	if(channel == 0){	
		rk616_use_lcd0_cfg(route);
	}
	else if(channel == 1){

	}
	else{
	}

	rk616_router_cfg(route);
	rk616_dither_cfg(route);

	return 0;
}


static int rk616_common_config()
{
	//12m i2s clk input -- grf io mux
#if  (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
	g_grfReg->GRF_GPIO_IOMUX[0].GPIOB_IOMUX = GRF_IOMUX_CONFIG_I2S;
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3188)
	g_grfReg->GRF_GPIO_IOMUX[1].GPIOC_IOMUX = GRF_IOMUX_CONFIG_I2S;
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3168)
	g_grfReg->GRF_GPIO_IOMUX[1].GPIOC_IOMUX = GRF_IOMUX_CONFIG_I2S;
#endif
    //rk_iomux_config(RK_I2S_IOMUX);

	//12m i2s clk input -- cru config
	g_cruReg->cru_clksel_con[3] = CRU_I2S_CLK_24M;	
	return 0;
}

#ifdef RK616_TEST
static int rk616_read_all_reg()
{
        int addr = 0,write_value = 0,i=0,read_value=0;

	
        while(addr<=0x9c)
        {
                i2c_read(0x50,addr,2,&read_value,4);
                printf("read rk616:%02x -- %08x\r\n",addr,read_value);
                i++;
                addr+=0x04;
        }
}
#endif 

int rk616_init(int lcdc_chn)
{
	//set grf and cru...
	rk616_common_config();

	//power on in rk30xx.c
	rk616_power_on();
	
	//init rk616 reg
	rk616_set_reg(lcdc_chn);

	//lvds  or mipi
	if(panel_info.screen_type == SCREEN_LVDS){
#ifdef CONFIG_RK616_LVDS
		set_lvds_reg(1);
#endif
	}
	else if(panel_info.screen_type == SCREEN_MIPI){
	}
	else{
	}	

#ifdef RK616_TEST
	rk616_read_all_reg();
#endif
	return 0;
}

