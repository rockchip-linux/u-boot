/*
 * DisplayPort driver for rk31xx/rk3368
 *
 * Copyright (C) 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Author:zwl<zwl@rock-chips.com>
         hjc<hjc@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "rk31xx_lvds.h"
#include "rockchip_fb.h"

#if defined(CONFIG_OF)
#include <linux/of.h>
#endif

#if defined(CONFIG_DEBUG_FS)
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif

DECLARE_GLOBAL_DATA_PTR;
struct rk_lvds_device rk31xx_lvds;

static int rk31xx_lvds_pwr_on(vidinfo_t * vid)
{
        struct rk_lvds_device *lvds = &rk31xx_lvds;
	    u32 delay_times = 20;

        if (vid->screen_type == SCREEN_LVDS) {
                /* set VOCM 900 mv and V-DIFF 350 mv */
	        lvds_msk_reg(lvds, MIPIPHY_REGE4, m_VOCM | m_DIFF_V,
			     v_VOCM(0) | v_DIFF_V(2));        
                /* power up lvds pll and ldo */
	        lvds_msk_reg(lvds, MIPIPHY_REG1,
	                     m_SYNC_RST | m_LDO_PWR_DOWN | m_PLL_PWR_DOWN,
	                     v_SYNC_RST(0) | v_LDO_PWR_DOWN(0) | v_PLL_PWR_DOWN(0));
		/* enable lvds lane and power on pll */
		lvds_writel(lvds, MIPIPHY_REGEB,
			    v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) |
			    v_LANE3_EN(1) | v_LANECLK_EN(1) | v_PLL_PWR_OFF(0));

	        /* enable lvds */
	        lvds_msk_reg(lvds, MIPIPHY_REGE3,
	                     m_MIPI_EN | m_LVDS_EN | m_TTL_EN,
	                     v_MIPI_EN(0) | v_LVDS_EN(1) | v_TTL_EN(0));
        } else {
                lvds_msk_reg(lvds, MIPIPHY_REGE3,
	                     m_MIPI_EN | m_LVDS_EN | m_TTL_EN,
	                     v_MIPI_EN(0) | v_LVDS_EN(0) | v_TTL_EN(1));
        }
	/* delay for waitting pll lock on */
	while (delay_times--) {
		if (lvds_phy_lock(lvds)) {
			break;
		}
		udelay(100);
	}

	if (delay_times <= 0)
		printf("wait lvds phy lock failed\n");
		
        return 0;
}

static void rk31xx_output_lvds(vidinfo_t *vid)
{
	struct rk_lvds_device *lvds = &rk31xx_lvds;
	u32 val = 0;

	/* if LVDS transmitter source from VOP, vop_dclk need get invert
	 * set iomux in dts pinctrl
	 */
	if (lvds->soc_type == CONFIG_RK3368) {
		/* enable lvds mode */
		val |= v_RK3368_LVDSMODE_EN(1) | v_RK3368_MIPIPHY_TTL_EN(0);
		/* config data source */
		/*val |= v_LVDS_DATA_SEL(LVDS_DATA_FROM_LCDC); */
		/* config lvds_format */
		val |= v_RK3368_LVDS_OUTPUT_FORMAT(vid->lvds_format);
		/* LSB receive mode */
		val |= v_RK3368_LVDS_MSBSEL(LVDS_MSB_D7);
		val |= v_RK3368_MIPIPHY_LANE0_EN(1) |
		       v_RK3368_MIPIDPI_FORCEX_EN(1);
		/*rk3368  RK3368_GRF_SOC_CON7 = 0X0041C*/
		/*grf_writel(val, 0x0041C);*/
    	grf_writel(val, GRF_SOC_CON7_LVDS);
    } else {
		/* enable lvds mode */
		val |= v_LVDSMODE_EN(1) | v_MIPIPHY_TTL_EN(0);
		/* config data source */
		val |= v_LVDS_DATA_SEL(LVDS_DATA_FROM_LCDC);
		/* config lvds_format */
		val |= v_LVDS_OUTPUT_FORMAT(vid->lvds_format);
		/* LSB receive mode */
		val |= v_LVDS_MSBSEL(LVDS_MSB_D7);
		val |= v_MIPIPHY_LANE0_EN(1) | v_MIPIDPI_FORCEX_EN(1);
		/*rk312x  RK312X_GRF_LVDS_CON0 = 0X00150*/
		grf_writel(val, 0X00150);
    }
	/* digital internal disable */
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(0));

	/* set pll prediv and fbdiv */
	lvds_writel(lvds, MIPIPHY_REG3, v_PREDIV(2) | v_FBDIV_MSB(0));
	lvds_writel(lvds, MIPIPHY_REG4, v_FBDIV_LSB(28));

	lvds_writel(lvds, MIPIPHY_REGE8, 0xfc);

	/* set lvds mode and reset phy config */
	lvds_msk_reg(lvds, MIPIPHY_REGE0,
	             m_MSB_SEL | m_DIG_INTER_RST,
	             v_MSB_SEL(1) | v_DIG_INTER_RST(1));

	rk31xx_lvds_pwr_on(vid);
	lvds_msk_reg(lvds, MIPIPHY_REGE1, m_DIG_INTER_EN, v_DIG_INTER_EN(1));


}

static void rk31xx_output_lvttl(vidinfo_t *vid)
{
	u32 val = 0;
	struct rk_lvds_device *lvds = &rk31xx_lvds;
        if (lvds->soc_type == CONFIG_RK3368) {
                /* iomux to lcdc */
                val = 0xf0005000;/*lcdc data 11 10*/
                lvds_pmugrf_writel(LVDS_PMUGRF_GPIO0B_IOMUX, val);

                val = 0xFFFF5555;/*lcdc data 12 13 14 15 16 17 18 19*/
                lvds_pmugrf_writel(LVDS_PMUGRF_GPIO0C_IOMUX, val);

                val = 0xFFFF5555;/*lcdc data 20 21 22 23 HSYNC VSYNC DEN DCLK*/
                lvds_pmugrf_writel(LVDS_PMUGRF_GPIO0D_IOMUX, val);

                lvds_dsi_writel(lvds, 0x0, 0x4);/*set clock lane enable*/
		/* enable lvds mode */
		val = v_RK3368_LVDSMODE_EN(0) | v_RK3368_MIPIPHY_TTL_EN(1) |
			v_RK3368_MIPIPHY_LANE0_EN(1) |
			v_RK3368_MIPIDPI_FORCEX_EN(1);
		grf_writel(val, GRF_SOC_CON7_LVDS);
		val = v_RK3368_FORCE_JETAG(0);
		grf_writel(val, GRF_SOC_CON15_LVDS);
    } else {/*31xx*/
    	/*grf_writel(0xfff35555, GRF_GPIO2B_IOMUX);
    	grf_writel(0x00ff0055, GRF_GPIO2C_IOMUX);
    	grf_writel(0x77771111, GRF_GPIO2C_IOMUX2);
    	grf_writel(0x700c1004, GRF_GPIO2D_IOMUX);*/
    	grf_writel(0xfff35555, 0x00cc);
    	grf_writel(0x00ff0055, 0x00d0);
    	grf_writel(0x77771111, 0x00e8);
    	grf_writel(0x700c1004, 0x00d4);

    	val |= v_LVDSMODE_EN(0) | v_MIPIPHY_TTL_EN(1);  /* enable lvds mode */
    	val |= v_LVDS_DATA_SEL(LVDS_DATA_FROM_LCDC);    /* config data source */
    	grf_writel(0xffff0380, RK312X_GRF_LVDS_CON0);

    	val = v_MIPITTL_CLK_EN(1) | v_MIPITTL_LANE0_EN(1) |
    	        v_MIPITTL_LANE1_EN(1) | v_MIPITTL_LANE2_EN(1) |
    	        v_MIPITTL_LANE3_EN(1);
    	grf_writel(val, GRF_SOC_CON1);
    }
	/* enable lane */
	lvds_writel(lvds, MIPIPHY_REG0, 0x7f);
	val = v_LANE0_EN(1) | v_LANE1_EN(1) | v_LANE2_EN(1) | v_LANE3_EN(1) |
	        v_LANECLK_EN(1) | v_PLL_PWR_OFF(1);
	lvds_writel(lvds, MIPIPHY_REGEB, val);

	/* set ttl mode and reset phy config */
	val = v_LVDS_MODE_EN(0) | v_TTL_MODE_EN(1) | v_MIPI_MODE_EN(0) |
	        v_MSB_SEL(1) | v_DIG_INTER_RST(1);
	lvds_writel(lvds, MIPIPHY_REGE0, val);

	rk31xx_lvds_pwr_on(vid);

}

int rk31xx_lvds_enable(vidinfo_t *vid)
{
        rk31xx_lvds.soc_type = gd->arch.chiptype;
        rk31xx_lvds.regbase = 0xff968000;
        rk31xx_lvds.ctrl_reg = 0xff9600a0;
        rk31xx_lvds.soc_type = CONFIG_RK3368;
	switch (vid->screen_type) {
	case SCREEN_LVDS:
		rk31xx_output_lvds(vid);
	        break;
	case SCREEN_RGB:
		rk31xx_output_lvttl(vid);
	        break;
	default:
	        printf("unsupport screen type\n");
	        break;
	}
	return 0;
}

