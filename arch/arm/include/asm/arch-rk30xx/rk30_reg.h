/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
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

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3066)
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

#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3168) || (CONFIG_RKCHIPTYPE == CONFIG_RK3188)

typedef volatile struct tagGRF_REG
{
    GPIO_LH_T GRF_GPIO_DIR[4];
    GPIO_LH_T GRF_GPIO_DO[4];
    GPIO_LH_T GRF_GPIO_EN[4];
    GPIO_IOMUX_T GRF_GPIO_IOMUX[4];
    uint32 GRF_SOC_CON[3];
    uint32 GRF_SOC_STATUS0;
    uint32 GRF_DMAC1_CON[3];
    uint32 GRF_DMAC2_CON[4];
    uint32 GRF_CPU_CON[6]; //  no use
    uint32 GRF_CPU_STAT[2]; //  no use
    uint32 GRF_DDRC_CON0;
    uint32 GRF_DDRC_STAT;
    uint32 GRF_IO_CON[5];
    uint32 reserved0;
    uint32 GRF_UOC0_CON[4];
    uint32 GRF_UOC1_CON[4];
    uint32 GRF_UOC2_CON[3];  //reserved in rk3168
    uint32 GRF_UOC3_CON[2];  //reserved in rk3168
    uint32 GRF_HSIC_STAT;    //reserved in rk3168
    uint32 GRF_OS_REG[8];
} GRF_REG, *pGRF_REG;
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3026)

typedef volatile struct tagGRF_REG
{
    //GPIO_LH_T GRF_GPIO_DIR[4];
   // GPIO_LH_T GRF_GPIO_DO[4];
   // GPIO_LH_T GRF_GPIO_EN[4];
    uint32 reserved0[0xa8/4];
    GPIO_IOMUX_T GRF_GPIO_IOMUX[4]; //0xa8
    uint32 reserved1[(0x100-0xe8)/4];
    uint32 GRF_GPIO_DS;	
    uint32 reserved2[(0x118-0x104)/4];
    GPIO_LH_T GRF_GPIO_PULL[4];     // 0x118
    uint32 reserved3[(0x140-0x138)/4];
    uint32 GRF_SOC_CON[3];   
    uint32 GRF_SOC_STATUS0;
    uint32 GRF_LVDS_CON0;
    uint32 reserved4[(0x15c-0x154)/4];
    uint32 GRF_DMAC_CON[3];
    uint32 reserved5[(0x17c-0x168)/4];
    uint32 GRF_UOC0_CON0;
    uint32 reserved6[(0x190-0x180)/4];
    uint32 GRF_UOC1_CON0;	
    uint32 GRF_UOC1_CON1;
    uint32 reserved7[(0x19c-0x198)/4];
    uint32 GRF_DDRC_STAT;	
    uint32 GRF_UOC_CON; 	
    uint32 reserved8[(0x1a8-0x1a4)/4];
    uint32 GRF_CPU_CON[6]; 
    uint32 GRF_CPU_STAT[2]; //  no use
    uint32 GRF_OS_REG[8];
} GRF_REG, *pGRF_REG;
#endif

/* TIMER Registers */
typedef volatile struct tagTIMER_REG
{
	uint32 TIMER_LOAD_COUNT;
	uint32 TIMER_CURR_VALUE;
	uint32 TIMER_CTRL_REG;
	uint32 TIMER_EOI;
	uint32 TIMER_INT_STATUS;
} TIMER_REG, *pTIMER_REG;

typedef volatile struct tagRK3188TIMER_STRUCT
{
	uint32 TIMER_LOAD_COUNT0;
	uint32 TIMER_LOAD_COUNT1;
	uint32 TIMER_CURR_VALUE0;
	uint32 TIMER_CURR_VALUE1;
	uint32 TIMER_CTRL_REG;
	uint32 TIMER_INT_STATUS;
}RK3188TIMER_REG,*pRK3188TIMER_REG;
#define g_rk30Time0Reg ((pTIMER_REG)TIMER0_BASE_ADDR)
#define g_rk3188Time0Reg ((pRK3188TIMER_REG)TIMER0_BASE_ADDR)

#define g_grfReg 		((pGRF_REG)REG_FILE_BASE_ADDR)

#endif /* __RK30_REG_H */

