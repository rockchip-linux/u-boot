/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	scu.c
Desc 	:
Author 	:  	yangkai
Date 	:	2008-12-16
Notes 	:

********************************************************************/
#define IN_SCU
#include  <asm/arch/rk29_drivers.h>
#ifdef DRIVERS_SCU

void SCUSetClkInfo(void)
{
	volatile uint32 *apllReg;
	volatile uint32 *clksel0Reg;
	uint32 armfreq;
	uint32 tmphclkdiv;
	uint32 tmppclkdiv;
	
	g_scuReg = (pSCU_REG)SCU_BASE_ADDR;
	apllReg = &(g_scuReg->SCU_APLL_CON);
	clksel0Reg = &(g_scuReg->SCU_CLKSEL0_CON);

	armfreq = 24*(((*apllReg>>4)&0x0fff)+1)/(((*apllReg>>1)&0x07)+1)/(((*apllReg>>16)&0x03f)+1);
	g_chipClk.armFreq = armfreq;

	tmphclkdiv = ((*clksel0Reg>>0)&0x03);
	if (tmphclkdiv == 0x00)
		g_chipClk.ahbDiv = HCLK_DIV1;
	if (tmphclkdiv == 0x01)
		g_chipClk.ahbDiv = HCLK_DIV2;
	if (tmphclkdiv == 0x02)
		g_chipClk.ahbDiv = HCLK_DIV3;
	if (tmphclkdiv == 0x03)
		g_chipClk.ahbDiv = HCLK_DIV4;

	tmppclkdiv = ((*clksel0Reg>>2)&0x03);
	if (tmppclkdiv == 0x00)
		g_chipClk.apbDiv = PCLK_DIV1;
	if (tmppclkdiv == 0x01)
		g_chipClk.apbDiv = PCLK_DIV2;
	if (tmppclkdiv == 0x02)
		g_chipClk.apbDiv = PCLK_DIV4;
}



/*void SCUSetAhpApb(void)
{
	volatile uint32 *apllReg;
	volatile uint32 *clksel0Reg;
	volatile uint32 *modeReg;
	g_scuReg = (pSCU_REG)SCU_BASE_ADDR;
	apllReg = &(g_scuReg->SCU_APLL_CON);
	modeReg = &(g_scuReg->SCU_MODE_CON);
	clksel0Reg = &(g_scuReg->SCU_CLKSEL0_CON);
	//*clksel0Reg = 0x10000734;
	*clksel0Reg = (*clksel0Reg & 0xfffffff0) | 0x05;
	//*apllReg &= ~0x00000001;
	*apllReg = 0x01850842;
	*modeReg |= 0x01<<2;
}
*/
void SCUPrintReg(void)
{
	volatile uint32 *apllReg;
	volatile uint32 *cpllReg;
	volatile uint32 *modeReg;
	volatile uint32 *clksel0Reg;
	g_scuReg = (pSCU_REG)SCU_BASE_ADDR;
	apllReg = &(g_scuReg->SCU_APLL_CON);
	cpllReg = &(g_scuReg->SCU_CPLL_CON);
	modeReg = &(g_scuReg->SCU_MODE_CON);
	clksel0Reg = &(g_scuReg->SCU_CLKSEL0_CON);
	//printf("&apllReg = 0x%x\n",apllReg);
	printf("apllReg = 0x%x\n",*apllReg);
	//printf("&cpllreg = 0x%x\n",cpllReg);
	printf("cpllreg = 0x%x\n",*cpllReg);
	//printf("&modeReg = 0x%x\n",modeReg);
	//printf("modeReg = 0x%x\n",*modeReg);
	printf("SCUGetArmFreq() = %d\n",SCUGetArmFreq());
	printf("SCUGetAHBFreq() = %d\n",SCUGetAHBFreq());
	printf("SCUGetAPBFreq() = %d\n",SCUGetAPBFreq());	
}


/*-------------------------------------------------------------------
Name	: SCUGetFreq
Desc	: 获取PLL频率
Params  :
Return  :
Notes   :
-------------------------------------------------------------------*/
uint32 SCUGetFreq(ePER_CLK_SRC type)
{
    uint32 freq;
    switch(type)
    {
        case CLK_SRC_ARMPLL:
            freq = SCUGetArmFreq();
            break;
        case CLK_SRC_DSPPLL:
            freq = SCUGetDspFreq();
            break;
        case CLK_SRC_CODECPLL:
            freq = SCUGetCodecFreq();
            break;
        default:
            break;
    }
    return freq;
}

/*----------------------------------------------------------------------
Name	: SCUSelectClk
Desc	: 模块时钟源选择
Params  : module:模块名称
          param: 时钟选择参数
Return  :
Notes   : 每个时钟模块的选择参数都是不一样的
----------------------------------------------------------------------*/
void SCUSelectClk(eCLK_SEL module, uint32 param)
{
    switch (module)
    {
        case CLK_SEL_HCLK:
            g_scuReg->SCU_CLKSEL0_CON &= ~CLK_HCLK_MASK;
            g_scuReg->SCU_CLKSEL0_CON |= (param&CLK_HCLK_MASK);
            break;
        case CLK_SEL_PCLK:
            g_scuReg->SCU_CLKSEL0_CON &= ~CLK_PCLK_MASK;
	    param = param<<2;
            g_scuReg->SCU_CLKSEL0_CON |= (param&CLK_PCLK_MASK);
            break;
        case CLK_SEL_SDMMC0:
            g_scuReg->SCU_CLKSEL0_CON &= ~CLK_SDMMC0_MASK;
            g_scuReg->SCU_CLKSEL0_CON |= (param&CLK_SDMMC0_MASK);
            break;
        case CLK_SEL_LCDC:
            g_scuReg->SCU_CLKSEL0_CON &= ~CLK_LCDC_MASK;
            g_scuReg->SCU_CLKSEL0_CON |= (param&CLK_LCDC_MASK);
            break;
        case CLK_SEL_USBPHY:
            g_scuReg->SCU_CLKSEL0_CON &= ~CLK_USBPHY_MASK;
            g_scuReg->SCU_CLKSEL0_CON |= (param&CLK_USBPHY_MASK);
            break;
        case CLK_SEL_48M:
            g_scuReg->SCU_CLKSEL0_CON &= ~CLK_48M_MASK;
            g_scuReg->SCU_CLKSEL0_CON |= (param&CLK_48M_MASK);
            break;
        case CLK_SEL_SENSOR:
            g_scuReg->SCU_CLKSEL0_CON &= ~CLK_SENSOR_MASK;
            g_scuReg->SCU_CLKSEL0_CON |= (param&CLK_SENSOR_MASK);
            break;
        case CLK_SEL_SDMMC1:
            g_scuReg->SCU_CLKSEL0_CON &= ~CLK_SDMMC1_MASK;
            g_scuReg->SCU_CLKSEL0_CON |= (param&CLK_SDMMC1_MASK);
            break;
        case CLK_SEL_CODEC:
            g_scuReg->SCU_CLKSEL1_CON &= ~CLK_CODEC_MASK;
            g_scuReg->SCU_CLKSEL1_CON |= (param&CLK_CODEC_MASK);
            break;
        case CLK_SEL_LSADC:
            g_scuReg->SCU_CLKSEL1_CON &= ~CLK_LSADC_MASK;
            g_scuReg->SCU_CLKSEL1_CON |= (param&CLK_LSADC_MASK);
            break;
        case CLK_SEL_DEMOD:
            g_scuReg->SCU_CLKSEL1_CON &= ~CLK_DEMOD_MASK;
            g_scuReg->SCU_CLKSEL1_CON |= (param&CLK_DEMOD_MASK);
            break;
        case CLK_SEL_GPS:
            g_scuReg->SCU_CLKSEL1_CON &= ~CLK_GPS_MASK;
            g_scuReg->SCU_CLKSEL1_CON |= (param&CLK_GPS_MASK);
            break;
        case CLK_SEL_HSADCO:
            g_scuReg->SCU_CLKSEL1_CON &= ~CLK_HSADCO_MASK;
            g_scuReg->SCU_CLKSEL1_CON |= (param&CLK_HSADCO_MASK);
            break;
        case CLK_SEL_SHMEM0:
            g_scuReg->SCU_CLKSEL1_CON &= ~CLK_SHMEM0_MASK;
            g_scuReg->SCU_CLKSEL1_CON |= (param&CLK_SHMEM0_MASK);
            break;
        case CLK_SEL_SHMEM1:
            g_scuReg->SCU_CLKSEL1_CON &= ~CLK_SHMEM1_MASK;
            g_scuReg->SCU_CLKSEL1_CON |= (param&CLK_SHMEM1_MASK);
            break;
        default:
            break;
    }

}

/*----------------------------------------------------------------------
Name	: SCUPwrDwnPLL
Desc	: 关闭一个PLL
Params  : pllId:PLL编号
Return  :
Notes   :
----------------------------------------------------------------------*/
void SCUPwrDwnPLL(eSCU_PLL_ID pllId)
{
    switch (pllId)
    {
        case PLL_ARM:
            g_scuReg->SCU_APLL_CON |= PLL_PD;
            break;
        case PLL_DSP:
            g_scuReg->SCU_DPLL_CON |= PLL_PD;
            break;
        case PLL_CODEC:
            g_scuReg->SCU_CPLL_CON |= PLL_PD;
            break;
        default:
            break;
    }
}

/*----------------------------------------------------------------------
Name	: SCUSetPll
Desc	: 设置PLL
Params  : pllId:PLL编号
          freq:要设置的频率
Return  :
Notes   : PLL分频方式见下图，需要设置三个分频因子，并需要满足以下条件:
          1. Fref/NR value range requirement : 97.7KHz - 800MHz
          2. Fref/NR * NF value range requirement: 160MHz - 800MHz
           ,------------------------.
           |   ___     ___     ___  |  ___
           '->|/NF|-->| P |-->|VCO|-->|/OD|--->|\
              `==='   | F |   `---'   `---'    | \
           ,->|/NR|-->| D |                    |  \
           |  `---'   `---'                    |  |-->CLKOUT
           |                                   |  /
          Fref(24M)--------------------------->| /
                                               |/
----------------------------------------------------------------------*/
void SCUSetPll(eSCU_PLL_ID pllId, uint32 freq)
{
    uint32 count;
    uint32 pllLock = (0x40u << pllId);
    volatile uint32 *pllReg;
//    pAPB_REG  apbReg = (pAPB_REG)REG_FILE_BASE_ADDR;

    switch (pllId)
    {
        case PLL_ARM:
            pllReg = &(g_scuReg->SCU_APLL_CON);
            break;
        case PLL_DSP:
            pllReg = &(g_scuReg->SCU_DPLL_CON);
            break;
        case PLL_CODEC:
            pllReg = &(g_scuReg->SCU_CPLL_CON);
            break;
        default:
            break;
    }

    if (*pllReg & PLL_PD)
    {
        *pllReg &= ~PLL_PD;
    }

    if (freq < 40 )			    //  25M ~ 39	vco= 200 ~ 320
    {
        *pllReg = PLL_SAT|PLL_FAST|(PLL_CLKR(3-1))|(PLL_CLKF(freq-1))|(PLL_CLKOD(8-1));
    }
    else if (freq < 80)			//  40 ~ 79,  vco = 160 ~ 320
    {
        *pllReg = PLL_SAT|PLL_FAST|(PLL_CLKR(4-1))|(PLL_CLKF(freq-1))|(PLL_CLKOD(6-1));
    }
    else if (freq < 160)		// 80 ~ 159,  vco = 160 ~ 320
    {
        *pllReg = PLL_SAT|PLL_FAST|(PLL_CLKR(12-1))|(PLL_CLKF(freq-1))|(PLL_CLKOD(2-1));
    }
    else                        // 160 ~ 300 , VCO = 160 ~ 300
    {
        *pllReg = PLL_SAT|PLL_FAST|(PLL_CLKR(24-1))|(PLL_CLKF(freq-1))|(PLL_CLKOD(1-1));
    }

    //等待PLL进入稳定状态，约0.3ms.
    DRVDelayCyc(5000);
    count = 2000;
    while (count --)
    {
        if (g_grfReg->CPU_APB_REG0 & pllLock)
        {
            break;
        }
        DRVDelayCyc(100);
    }
}

/*----------------------------------------------------------------------
Name	: SCUSetARMPll
Desc	: 设置ARM PLL频率
Params  : freq:ARM PLL频率
Return  :
Notes   :
----------------------------------------------------------------------*/
void SCUSetARMPll(uint32 freq)
{
    g_scuReg->SCU_MODE_CON &= ~SCU_CPUMODE_MASK;//SLOW MODE
    if (freq == FREQ_ARM_MIN)
    {
        SCUPwrDwnPLL(PLL_ARM);
    }
    else
    {
        SCUSetPll(PLL_ARM, freq);
        g_scuReg->SCU_MODE_CON |= SCU_CPUMODE_NORMAL;
    }
    g_chipClk.armFreq = freq;
}
/*----------------------------------------------------------------------
Name	: SCUSetARMFreq
Desc	: 设置ARM主频、HCLK、PCLK分频
Params  : freq: ARM 频率
          hclkDiv: HCLK 分频因子
          pclkDiv: PCLK 分频因子
Return  :
Notes   : ARM, HCLK, PCLK的频率应在有效范围内
----------------------------------------------------------------------*/
void SCUSetARMFreq(uint32 freq, eSCU_HCLK_DIV hclkDiv, eSCU_PCLK_DIV pclkDiv)
{
    uint32  hclk, pclk;

    if (freq > FREQ_ARM_MAX)
    {
        freq = FREQ_ARM_MAX;
    }
    if (freq < FREQ_ARM_MIN)
    {
        freq = FREQ_ARM_MIN;
    }
    hclk = freq >> hclkDiv;
    pclk = hclk >> pclkDiv;

    if (hclk <= FREQ_HCLK_MIN)
    {
        hclkDiv = HCLK_DIV1;
        hclk = freq;
    }

    if (hclk > FREQ_HCLK_MAX)
    {
        hclkDiv = HCLK_DIV4;
    }

    if (pclk <= FREQ_PCLK_MIN)
    {
        pclkDiv = PCLK_DIV1;
        pclk = hclk;
    }

    if (pclk > FREQ_PCLK_MAX)
    {
        pclkDiv = PCLK_DIV4;
    }

    if ((g_chipClk.armFreq == freq)
            && (g_chipClk.ahbDiv == hclkDiv)
            && (g_chipClk.apbDiv == pclkDiv))
    {
        return;
    }
    //必需先设置好总线分频，然后设置CPU频率
    SCUSelectClk(CLK_SEL_HCLK, hclkDiv);
    SCUSelectClk(CLK_SEL_PCLK, pclkDiv);
    SCUSetARMPll(freq);
    g_chipClk.ahbDiv = hclkDiv;
    g_chipClk.apbDiv = pclkDiv;

}
/*-------------------------------------------------------------------
Name	: SCUDisableClk
Desc	: 关闭IP时钟
Params  :
Return  :
Notes   :
-------------------------------------------------------------------*/
void SCUDisableClk(eCLK_GATE clkId)
{
    *(volatile uint32*)(g_scuReg->SCU_CLKGATE0_CON + (clkId>>5))|= (0x01<<(clkId&0x1f));
}

/*-------------------------------------------------------------------
Name	: SCUEnableClk
Desc	: 打开IP时钟
Params  :
Return  :
Notes   :
-------------------------------------------------------------------*/
void SCUEnableClk(eCLK_GATE clkId)
{
    *(volatile uint32*)(&(g_scuReg->SCU_CLKGATE0_CON) + (clkId>>5))&= ~(0x01<<(clkId&0x1f));
}
/*----------------------------------------------------------------------
Name	: SCUDefaultSet
Desc	: SCU默认设置
Params  :
Return  :
Notes   :
----------------------------------------------------------------------*/
void SCUDefaultSet(void)
{
    g_scuReg->SCU_MODE_CON = SCU_INT_CLR|SCU_WAKEUP_POS|SCU_ALARM_WAKEUP_DIS\
                             |SCU_EXT_WAKEUP_DIS|SCU_CPUMODE_NORMAL|SCU_DSPMODE_NORMAL;
    g_scuReg->SCU_PMU_CON = PMU_SHMEM_PD | PMU_DEMOD_PD | PMU_DSP_PD;
    g_scuReg->SCU_CLKSEL0_CON = CLK_SDMMC1_DIV(3) | CLK_SENSOR_24M | CLK_48M_DIV(3)
                                |CLK_USBPHY_24M|CLK_LCDC_CODPLL|CLK_LCDC_DIV(7)
                                |CLK_LCDC_DIVOUT|CLK_SDMMC0_DIV(3)|CLK_ARM_HCLK_21
                                |CLK_HCLK_PCLK_11;
    g_scuReg->SCU_CLKSEL1_CON = CLK_SHMEM1_DEMODCLK|CLK_SHMEM0_DEMODCLK|CLK_HSADCO_NORMAL\
                                |CLK_GPS_DEMODCLK|CLK_DEMOD_INTCLK|CLK_DEMOD_CODPLL\
                                |CLK_DEMOD_DIV(3)|CLK_LSADC_DIV(0)|CLK_CODEC_DIV(1)\
                                |CLK_CODEC_12M|CLK_CPLL_NORMAL;
    g_scuReg->SCU_CHIPCFG_CON = 0x0bb80000;
    
    g_APPList = 0;
    g_moduleClkList[0] = ~((0x01u<<CLK_GATE_SHMEM1) | (0x01u<<CLK_GATE_SHMEM0)//0x71ff743
                         |(0x01u<<CLK_GATE_LSADC)   | (0x01u<<CLK_GATE_PWM)
                         |(0x01u<<CLK_GATE_SPI1)    | (0x01u<<CLK_GATE_SPI0)
                         |(0x01u<<CLK_GATE_I2C1)    | (0x01u<<CLK_GATE_I2C0)
                         |(0x01u<<CLK_GATE_UART1)   | (0x01u<<CLK_GATE_UART0)
                         |(0x01u<<CLK_GATE_GPIO1)   | (0x01u<<CLK_GATE_GPIO1)
                         |(0x01u<<CLK_GATE_SDMMC0)  | (0x01u<<CLK_GATE_I2S)
                         |(0x01u<<CLK_GATE_VIP)     | (0x01u<<CLK_GATE_DEBLK)
                         |(0x01u<<CLK_GATE_HIF)     | (0x01u<<CLK_GATE_SRAMDSP)
                         |(0x01u<<CLK_GATE_SRAMARM) | (0x01u<<CLK_GATE_DMA)
                         |(0x01u<<CLK_GATE_DSP));
    
    g_moduleClkList[1] = ~((0x01u<<(CLK_GATE_HSADC&0x1f))  | (0x01u<<(CLK_GATE_DEMODFIFO&0x1f))
                         |(0x01u<<(CLK_GATE_DEMODBUS&0x1f))| (0x01u<<(CLK_GATE_DEMODOTHER&0x1f))
                         |(0x01u<<(CLK_GATE_AGC&0x1f))     | (0x01u<<(CLK_GATE_DOWNMIXER&0x1f))
                         |(0x01u<<(CLK_GATE_PREFFT&0x1f))  | (0x01u<<(CLK_GATE_IQIMBALANCE&0x1f))
                         |(0x01u<<(CLK_GATE_FRAMEDET&0x1f))| (0x01u<<(CLK_GATE_FFTMEM&0x1f))
                         |(0x01u<<(CLK_GATE_BITDITL&0x1f)) | (0x01u<<(CLK_GATE_VITERBIMEM&0x1f))
                         |(0x01u<<(CLK_GATE_PREFFTMEM&0x1f))|(0x01u<<(CLK_GATE_VITERBI&0x1f))
                         |(0x01u<<(CLK_GATE_RS&0x1f))       | (0x01u<<(CLK_GATE_EXTMEM&0x1f))
                         |(0x01u<<(CLK_GATE_MSDRMEM&0x1f))  | (0x01u<<(CLK_GATE_DEMOD&0x1f)));
    
    g_moduleClkList[2] = ~((0x01u<<(CLK_GATE_DSPBUS&0x1f)) | (0x01u<<(CLK_GATE_EXPBUS&0x1f))
                         |(0x01u<<(CLK_GATE_APBBUS&0x1f))   | (0x01u<<(CLK_GATE_EFUSE&0x1f)));
    
    g_scuReg->SCU_CLKGATE0_CON = ~g_moduleClkList[0];
    //g_scuReg->SCU_CLKGATE1_CON = ~g_moduleClkList[1];
    //g_scuReg->SCU_CLKGATE2_CON = ~g_moduleClkList[2];
}


/*----------------------------------------------------------------------
Name	: SCUInit
Desc	: SCU初始化
Params  :
Return  :
Notes   : 初始化本模块使用的全局变量并进行默认设置
------------------------------------------------------------------------*/
/*-------------通过rk2800sdk.h中设置相关SCU宏设置主频/ahb分频/apb分频----------------*/
void SCUInit(uint32 SCUARMFreq, eSCU_HCLK_DIV SCUhclkDiv, eSCU_PCLK_DIV SCUpclkDiv)
{
      g_scuReg = (pSCU_REG)SCU_BASE_ADDR;
      SCUDefaultSet();
      SCUSetARMFreq(SCUARMFreq, SCUhclkDiv, SCUpclkDiv);
}
void SCUInit2818DDR(uint32 SCUARMFreq, eSCU_HCLK_DIV SCUhclkDiv, eSCU_PCLK_DIV SCUpclkDiv)
{
	  g_scuReg = (pSCU_REG)SCU_BASE_ADDR;
	  SCUSetARMFreq(SCUARMFreq, SCUhclkDiv, SCUpclkDiv);
}

#endif
