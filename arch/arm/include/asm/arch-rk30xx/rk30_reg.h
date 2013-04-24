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
#ifndef __RK30_REG_H
#define __RK30_REG_H

/* PMU registers */
typedef volatile struct tagPMU_REG
{
    uint32 PMU_WAKEUP_CFG[2];
    uint32 PMU_PWRDN_CON;
    uint32 PMU_PWRDN_ST;
    uint32 PMU_INT_CON;
    uint32 PMU_INT_ST;
    uint32 PMU_MISC_CON;
    uint32 PMU_OSC_CNT;
    uint32 PMU_PLL_CNT;
    uint32 PMU_PMU_CNT;
    uint32 PMU_DDRIO_PWRON_CNT;
    uint32 PMU_WAKEUP_RST_CLR_CNT;
    uint32 PMU_SCU_PWRDWN_CNT;
    uint32 PMU_SCU_PWRUP_CNT;
    uint32 PMU_MISC_CON1;
    uint32 PMU_GPIO6_CON;
    uint32 PMU_PMU_SYS_REG[4];
} PMU_REG, *pPMU_REG;

#define g_pmuReg ((pPMU_REG)PMU_BASE_ADDR)


/* CRU Registers */
typedef volatile struct tagCRU_REG
{
    uint32 CRU_PLL_CON[4][4];
    uint32 CRU_MODE_CON;
    uint32 CRU_CLKSEL_CON[35];
    uint32 CRU_CLKGATE_CON[10];
    uint32 reserved1[2];
    uint32 CRU_GLB_SRST_FST_VALUE;
    uint32 CRU_GLB_SRST_SND_VALUE;
    uint32 reserved2[2];
    uint32 CRU_SOFTRST_CON[9];
    uint32 CRU_MISC_CON;
    uint32 reserved3[2];
    uint32 CRU_GLB_CNT_TH;
} CRU_REG, *pCRU_REG;

#define g_cruReg ((pCRU_REG)CRU_BASE_ADDR)

typedef struct tagGPIO_LH
{
    uint32 GPIOL;
    uint32 GPIOH;
} GPIO_LH_T;


typedef struct tagGPIO_IOMUX
{
    uint32 GPIOA_IOMUX;
    uint32 GPIOB_IOMUX;
    uint32 GPIOC_IOMUX;
    uint32 GPIOD_IOMUX;
} GPIO_IOMUX_T;


/* REG FILE registers */
typedef volatile struct tagGRF_REG
{
    GPIO_LH_T GRF_GPIO_DIR[7];
    GPIO_LH_T GRF_GPIO_DO[7];
    GPIO_LH_T GRF_GPIO_EN[7];
    GPIO_IOMUX_T GRF_GPIO_IOMUX[7];
    GPIO_LH_T GRF_GPIO_PULL[7];
    uint32 GRF_SOC_CON[3];
    uint32 GRF_SOC_STATUS0;
    uint32 GRF_DMAC1_CON[3];
    uint32 GRF_DMAC2_CON[4];
    uint32 GRF_UOC0_CON[3];
    uint32 GRF_UOC1_CON[4];
    uint32 GRF_DDRC_CON0;
    uint32 GRF_DDRC_STAT;
    uint32 reserved[(0x1c8-0x1a0)/4];
    uint32 GRF_OS_REG[4];
} GRF_REG, *pGRF_REG;


/* TIMER Registers */
typedef volatile struct tagTIMER_REG
{
	uint32 TIMER_LOAD_COUNT;
	uint32 TIMER_CURR_VALUE;
	uint32 TIMER_CTRL_REG;
	uint32 TIMER_EOI;
	uint32 TIMER_INT_STATUS;
} TIMER_REG, *pTIMER_REG;
#define g_grfReg 		((pGRF_REG)RK30_GRF_PHYS)

#endif /* __RK30_REG_H */

