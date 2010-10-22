/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	scu.h
Desc 	:	
Author 	:  	yangkai
Date 	:	2008-12-16
Notes 	:

********************************************************************/
#ifdef DRIVERS_SCU
#ifndef _SCU_H
#define _SCU_H

/********************************************************************
**                            宏定义                                *
********************************************************************/
/*SCU PLL CON*/
#define PLL_TEST        (0x01u<<25)
#define PLL_SAT         (0x01u<<24)
#define PLL_FAST        (0x01u<<23)
#define PLL_PD          (0x01u<<22)
#define PLL_CLKR(i)     ((i&0x3f)<<16)
#define PLL_CLKF(i)     ((i&0x0fff)<<4)
#define PLL_CLKOD(i)    ((i&0x07)<<1)
#define PLL_BYPASS      (0X01)

/*SCU MODE CON*/
#define SCU_INT_CLR         (0x01u<<8)
#define SCU_WAKEUP_POS      (0x00u<<7)
#define SCU_WAKEUP_NEG      (0x01u<<7)
#define SCU_ALARM_WAKEUP_DIS (0x01u<<6)
#define SCU_EXT_WAKEUP_DIS   (0x01u<<5)
#define SCU_STOPMODE_EN     (0x01u<<4)

#define SCU_CPUMODE_MASK    (0x11u<<2)
#define SCU_CPUMODE_SLOW    (0x00u<<2)
#define SCU_CPUMODE_NORMAL  (0x01u<<2)
#define SCU_CPUMODE_DSLOW   (0x02u<<2)

#define SCU_DSPMODE_MASK    (0x11u<<0)
#define SCU_DSPMODE_SLOW    (0x00u<<0)
#define SCU_DSPMODE_NORMAL  (0x01u<<0)
#define SCU_DSPMODE_DSLOW   (0x02u<<0)

/*SCU PMU MODE*/
#define PMU_SHMEM_PWR_STAT  (0x01u<<8)
#define PMU_DEMOD_PWR_STAT  (0x01u<<7)
#define PMU_CPU_PWR_STAT    (0x01u<<6)
#define PMU_DSP_PWR_STAT    (0x01u<<5)

#define PMU_EXT_SWITCH_PWR  (0x01u<<4)

#define PMU_SHMEM_PD        (0x01u<<3)
#define PMU_DEMOD_PD        (0x01u<<2)
#define PMU_CPU_PD          (0x01u<<1)
#define PMU_DSP_PD          (0x01u<<0)

/*SCU SOFTWARE RESET CON*/
#define CLK_RST_SDRAM       (1<<28)
#define CLK_RST_SHMEM1      (1<<27)
#define CLK_RST_SHMEM0      (1<<26)
#define CLK_RST_DSPA2A      (1<<25)
#define CLK_RST_SDMMC1      (1<<24)
#define CLK_RST_ARM         (1<<23)
#define CLK_RST_DEMODGEN    (1<<22)
#define CLK_RST_PREFFT      (1<<21)
#define CLK_RST_RS          (1<<20)
#define CLK_RST_BITDITL     (1<<19)
#define CLK_RST_VITERBI     (1<<18)
#define CLK_RST_FFT         (1<<17)
#define CLK_RST_FRAMEDET    (1<<16)
#define CLK_RST_IQIMBALANCE (1<<15)
#define CLK_RST_DOWNMIXER   (1<<14)
#define CLK_RST_AGC         (1<<13)
#define CLK_RST_USBPHY      (1<<12)
#define CLK_RST_USBC        (1<<11)
#define CLK_RST_DEMOD       (1<<10)
#define CLK_RST_SDMMC0      (1<<9)
#define CLK_RST_DEBLK       (1<<8)
#define CLK_RST_LSADC       (1<<7)
#define CLK_RST_I2S         (1<<6)
#define CLK_RST_DSPPER      (1<<5)
#define CLK_RST_DSP         (1<<4)
#define CLK_RST_NANDC       (1<<3)
#define CLK_RST_VIP         (1<<2)
#define CLK_RST_LCDC        (1<<1)
#define CLK_RST_USBOTG      (1<<0)

#define FREQ_ARM_MAX    300
#define FREQ_ARM_MIN    24
#define FREQ_ARM_IDLE    24
#define FREQ_HCLK_MAX   133
#define FREQ_HCLK_MIN   12
#define FREQ_PCLK_MAX   66
#define FREQ_PCLK_MIN   12
#define FREQ_DSP_MAX    350
#define FREQ_DSP_MIN    24

/********************************************************************
**                          结构定义                                *
********************************************************************/
typedef volatile struct tagSCU_REG
{
    uint32 SCU_APLL_CON;//[3];//0:arm 1:dsp 2:codec
    uint32 SCU_DPLL_CON;
    uint32 SCU_CPLL_CON;
    uint32 SCU_MODE_CON;
    uint32 SCU_PMU_CON;
    uint32 SCU_CLKSEL0_CON;
    uint32 SCU_CLKSEL1_CON;
    uint32 SCU_CLKGATE0_CON;
    uint32 SCU_CLKGATE1_CON;
    uint32 SCU_CLKGATE2_CON;
    uint32 SCU_SOFTRST_CON;
    uint32 SCU_CHIPCFG_CON;
    uint32 SCU_CPUPD;
}SCU_REG,*pSCU_REG;

typedef struct tagSCU_CLK_INFO
{
    uint32 armFreq;     //ARM PLL FREQ
    uint32 dspFreq;     //DSP PLL FREQ
    uint32 codecFreq;   //CODEC PLL FREQ
    uint32 ahbDiv;
    uint32 apbDiv;
}SCU_CLK_INFO,*pSCU_CLK_INFO;


/********************************************************************
**                          变量定义                                *
********************************************************************/
#undef EXT
#ifdef IN_SCU
    #define EXT
#else    
    #define EXT extern
#endif    
    
EXT pSCU_REG g_scuReg;
EXT SCU_CLK_INFO g_chipClk;

EXT uint64 g_APPList;
EXT uint32 g_moduleClkList[3];
/********************************************************************
**                          函数声明                                *
********************************************************************/

/********************************************************************
**                          表格定义                                *
********************************************************************/

#endif
#endif
