/*
 * (C) Copyright 2008-2013 Rockchip Electronics
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

enum rk_plls_id {
	APLL_ID = 0,
	DPLL_ID,
	CPLL_ID,
	GPLL_ID,
	NPLL_ID,
	END_PLL_ID,
};

static void rkclk_pll_wait_lock(enum rk_plls_id pll_id)
{

}

static int rkclk_pll_clk_set_rate(enum rk_plls_id pll_id, uint32 mHz, pll_callback_f cb_f)
{
	return 0;
}


static uint32 rkclk_pll_clk_get_rate(enum rk_plls_id pll_id)
{
	return 0;
}


/*
 * rkplat clock set periph clock from general pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_periph_ahpclk_set(uint32 pll_src, uint32 aclk_div, uint32 hclk_div, uint32 pclk_div)
{

}


/*
 * rkplat clock set cpu clock from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_cpu_coreclk_set(uint32 pll_src, uint32 core_div, uint32 periph_div, uint32 axi_core_div)
{

}


/*
 * rkplat clock set cpu clock from arm pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_cpu_ahpclk_set(uint32 aclk_div, uint32 hclk_div, uint32 pclk_div, uint32 ahb2apb_div)
{

}


/*
 * rkplat clock set ddr clock from ddr pll
 * 	when call this function, make sure pll is in slow mode
 */
static void rkclk_ddr_clk_set(uint32 pll_src, uint32 ddr_div)
{

}


static void rkclk_apll_cb(struct pll_clk_set *clkset)
{
}


static void rkclk_gpll_cb(struct pll_clk_set *clkset)
{
}


static void rkclk_dpll_cb(struct pll_clk_set *clkset)
{
}


static uint32 rkclk_get_cpu_aclk_div(void)
{

}


static uint32 rkclk_get_cpu_hclk_div(void)
{

}


static uint32 rkclk_get_cpu_pclk_div(void)
{

}


static uint32 rkclk_get_periph_aclk_div(void)
{

}


static uint32 rkclk_get_periph_hclk_div(void)
{

}


static uint32 rkclk_get_periph_pclk_div(void)
{

}

