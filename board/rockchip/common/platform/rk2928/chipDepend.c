
//#include    "config.h"
#include    "../../armlinux/config.h"
#define     DELAY_ARM_FREQ      50
#define     ASM_LOOP_INSTRUCTION_NUM     4
#define     ASM_LOOP_PER_US    (DELAY_ARM_FREQ/ASM_LOOP_INSTRUCTION_NUM) //

/***************************************************************************
函数描述:延时
入口参数:cycle数
出口参数:
调用函数:
***************************************************************************/
void DRVDelayCyc(uint32 count)
{
    count/=4;           //因为每条while循环需要4个CYC, 假设MEM是0等待
    while (count--)
        ;
}

/***************************************************************************
函数描述:延时
入口参数:us数
出口参数:
调用函数:
***************************************************************************/
extern uint32 Timer0Get100ns( void );
void DRVDelayUs(uint32 count)
{
    uint32 tmp;
    uint32 TimerEnd = Timer0Get100ns() + count * 13;
    tmp =  ASM_LOOP_PER_US;//15;
    if(tmp)
        tmp *= count;
    else
        tmp = 1;

    while (--tmp) 
    {
        if(Timer0Get100ns() > TimerEnd)
            break;
    }
}
/***************************************************************************
函数描述:延时
入口参数:
出口参数:
调用函数:
***************************************************************************/
void Delay100cyc(uint16 count)
{
    uint16 i;

    while (count--)
        for (i=0; i<8; i++);
}


/***************************************************************************
函数描述:延时
入口参数:ms数
出口参数:
调用函数:
***************************************************************************/
void DRVDelayMs(uint32 count)
{
    while (count--)
        DRVDelayUs(1000);
}


/***************************************************************************
函数描述:延时
入口参数:s数
出口参数:
调用函数:
***************************************************************************/
void DRVDelayS(uint32 count)
{
    while (count--)
        DRVDelayMs(1000);
}


uint8  ChipType;
uint32 Rk30ChipVerInfo[4];  

void ChipTypeCheck(void)
{ 
	//Rk30ChipVerInfo[0] = 0;
	//ftl_memcpy(Rk30ChipVerInfo, (uint8*)(0x10100000 + 0x27f0 ), 16);   
	//if(Rk30ChipVerInfo[0]== 0x32393243)//&& Rk30ChipVerInfo[3] == 0x56313030) //292C
	//{
	
		ChipType = CONFIG_RK3026;
		Rk30ChipVerInfo[0] = 0x33303241;//"302A"
	//}

	//if(Rk30ChipVerInfo[0]== 0x32393241)//&& Rk30ChipVerInfo[3] == 0x56333030)  // 292A
	//{
	//	ChipType = CONFIG_RK2928;
	//	Rk30ChipVerInfo[0] = 0x32393258; // "292X"
	//}
	
}
#include "../../common/rockusb/USB20.h"
void ModifyUsbVidPid(USB_DEVICE_DESCRIPTOR * pDeviceDescr)
{
	if(ChipType == CONFIG_RK3026)
	{
		pDeviceDescr->idProduct = 0x292c;
		pDeviceDescr->idVendor  = 0x2207;
	}
	else if (ChipType == CONFIG_RK2928)
	{
		pDeviceDescr->idProduct = 0x292A;
		pDeviceDescr->idVendor  = 0x2207;
	}
}

//定义Loader启动异常类型
//系统中设置指定的sdram值为该标志，重启即可进入rockusb
//系统启动失败标志
uint32 IReadLoaderFlag(void)
{
    uint32 reg;
    if (ChipType == CONFIG_RK2928)
    {
        reg = ((*LOADER_FLAG_REG_L) & 0xFFFFuL) | (((*LOADER_FLAG_REG_H) & 0xFFFFuL)<<16);
    }
    else
    {
        reg = ((*LOADER_FLAG_REG_L));
    }

    return (reg);
	
}

void ISetLoaderFlag(uint32 flag)
{
    uint32 reg;
    if (ChipType == CONFIG_RK2928)
    {
        reg = ((*LOADER_FLAG_REG_L) & 0xFFFFuL) | (((*LOADER_FLAG_REG_H) & 0xFFFFuL)<<16);
        if(reg == flag)
            return;
        (*LOADER_FLAG_REG_L) = 0xFFFF0000 | (flag & 0xFFFFuL);
        (*LOADER_FLAG_REG_H) = 0xFFFF0000 | ((flag >>16) & 0xFFFFuL);
    }
    else
    {
        if(*LOADER_FLAG_REG_L == flag)
            return;
        *LOADER_FLAG_REG_L = flag;
    }
}
uint32 IReadLoaderMode(void)
{
    return (*LOADER_FLAG_REG_L);
}
uint32 cpuFreq;
void SetFreqFlag(uint32 freq)
{
    cpuFreq = freq;
}


typedef enum PLL_ID_Tag
{
    APLL=0,
    DPLL,
    CPLL,
    GPLL,
    
    PLL_MAX
}PLL_ID;

/*#define PLL_RESET  (((0x1<<5)<<16) | (0x1<<5))
#define PLL_DE_RESET  (((0x1<<5)<<16) | (0x0<<5))
#define NR(n)      ((0x3F<<(8+16)) | ((n-1)<<8))
#define NO(n)      ((0xF<<16) | (n-1))
#define NF(n)      ((0x1FFF<<16) | (n-1))
#define NB(n)      ((0xFFF<<16) | (n-1))
*/
#define PB(n)         ((0x1<<(15+16)) | ((n)<<15))
#define POSTDIV1(n)   ((0x7<<(12+16)) | ((n)<<12))
#define FBDIV(n)      ((0xFFF<<16) | (n))

#define RSTMODE(n)    ((0x1<<(15+16)) | ((n)<<15))
#define RST(n)        ((0x1<<(14+16)) | ((n)<<14))
#define PD(n)         ((0x1<<(13+16)) | ((n)<<13))
#define DSMPD(n)      ((0x1<<(12+16)) | ((n)<<12))
#define LOCK(n)       (((n)>>10)&0x1)
#define POSTDIV2(n)   ((0x7<<(6+16)) | ((n)<<6))
#define REFDIV(n)     ((0x3F<<16) | (n))

static void APLL_cb(void)
{
    g_cruReg->CRU_CLKSEL_CON[0] = (((0x1<<13)|(0x1f<<8)|(0x1<<7)|0x1f)<<16)
                                | (0x0<<13) // aclk cpu select ARM PLL
                                | (0x1<<8)  // APLL:aclk_cpu = 2:1
                                | (0x0<<7)  // core clock select ARM PLL
                                | (0x0);    // APLL:clk_core = 1:1
    g_cruReg->CRU_CLKSEL_CON[1] = (((0x1<<15)|(0x7<<12)|(0x3<<8)|(0x1<<4)|0xf)<<16)
                                | (0x0<<15)     // clk_core:clk_l2c = 1:1
                                | (0x3<<12)     // aclk_cpu:pclk_cpu = 4:1
                                | (0x1<<8)      // aclk_cpu:hclk_cpu = 2:1
                                | (0x1<<4)      // clk_l2c:aclk_core = 2:1
                                | (0x3);        // clk_core:clk_core_periph = 4:1 
}

static void GPLL_cb(void)
{
    g_cruReg->CRU_CLKSEL_CON[10] = (((0x1<<15)|(0x3<<12)|(0x3<<8)|0x1F)<<16) 
                                | (0x0<<15)     //aclk_periph = GPLL/1 = 300MHz
                                | (0x2<<12)     //aclk_periph:pclk_periph = 4:1 = 300MHz : 75MHz
                                | (0x1<<8)      //aclk_periph:hclk_periph = 2:1 = 300MHz : 150MHz
                                | (0x0);       // GPLL:aclk_periph = 1:1  

    g_cruReg->CRU_CLKSEL_CON[25] = ((0x7F)<<16) | (0x1);       // GPLL:spi = 2:1  
                                
}

/*static void DPLL_cb(void)
{ 
    g_cruReg->CRU_CLKSEL_CON[26] = ((0x3 | (0x1<<8))<<16)
                                                  | (0x0<<8)     //clk_ddr_src = DDR PLL
                                                  | 0;           //clk_ddr_src:clk_ddrphy = 1:1 
}*/

/*****************************************
REFDIV   FBDIV     POSTDIV1/POSTDIV2      FOUTPOSTDIV           freq Step     finally use
1        17 - 66   1-49                   8MHz  - 1600MHz                 
=========================================================================================
1        17 - 66   8                      50MHz  - 200MHz          3MHz          50MHz   <= 150MHz
1        17 - 66   7                      57MHz  - 228MHz          3.42MHz
1        17 - 66   6                      66MHz  - 266MHz          4MHz          150MHz  <= 200MHz
1        17 - 66   5                      80MHz  - 320MHz          4.8MHz
1        17 - 66   4                      100MHz - 400MHz          6MHz          200MHz  <= 300MHz
1        17 - 66   3                      133MHz - 533MHz          8MHz          
1        17 - 66   2                      200MHz - 800MHz          12MHz         300MHz  <= 600MHz
1        17 - 66   1                      400MHz - 1600MHz         24MHz         600MHz  <= 1200MHz
******************************************/
static void Set_PLL(PLL_ID pll_id, uint32 MHz, pFunc cb)
{
    uint32 refdiv,postdiv1,fbdiv;  //FREF越大，VCO越大，jitter就会小
    int delay = 1000;
    
    if(MHz <= 300)
    {
        postdiv1 = 2;
    }
    else if(MHz <= 600)
    {
        postdiv1 = 2;
    }
    else
    {
        postdiv1 = 1;
    }
    refdiv = 6;
    fbdiv=(MHz*refdiv*postdiv1)/24;
    
    g_cruReg->CRU_MODE_CON = (0x1<<((pll_id*4) +  16)) | (0x0<<(pll_id*4));            //PLL slow-mode
    
    g_cruReg->CRU_PLL_CON[pll_id][0] = POSTDIV1(postdiv1) | FBDIV(fbdiv);
    g_cruReg->CRU_PLL_CON[pll_id][1] = DSMPD(1) | POSTDIV2(1) | REFDIV(refdiv);

    while (delay > 0) 
    {
        DRVDelayUs(1);
		if (LOCK(g_cruReg->CRU_PLL_CON[pll_id][1]))
			break;
		delay--;
	}
	
    if(cb)
    {
        cb();
    }
    g_cruReg->CRU_MODE_CON = (0x1<<((pll_id*4) +  16))  | (0x1<<(pll_id*4));            //PLL normal
    return;
}

void SetARMPLL(uint16 nMhz)
{
    //Set_PLL(APLL, 600, APLL_cb);
    //Set_PLL(GPLL, 300, GPLL_cb);
}

uint32 GetPLLCLK(uint8 pll_id)
{
    uint32 refdiv,postdiv1,fbdiv,postdiv2; 
    uint32 ArmPll;
    ArmPll =  g_cruReg->CRU_PLL_CON[pll_id][0];
    fbdiv = (ArmPll&0xFFFul) ;
    postdiv1 = ((ArmPll >> 12)&0x7ul) ;
    
    ArmPll =  g_cruReg->CRU_PLL_CON[pll_id][1];
    refdiv =  (ArmPll&0x3Ful) ;
    postdiv2 = ((ArmPll >> 6)&0x7ul) ; 
    ArmPll = 24*fbdiv/(refdiv*postdiv1*postdiv2);
    //printf("GetPLLCLK  = %d\n",ArmPll);
    return ArmPll;
}

uint32 GetGPLLCLK(void)
{
    uint32 NR,NF,NO;
    uint32 ArmPll;
    pCRU_REG ScuReg=(pCRU_REG)CRU_BASE_ADDR;

    ArmPll =  ScuReg->CRU_PLL_CON[3][0];
    //printf("GetGPLLCLK0 = %x\n",ArmPll);
    NO = (ArmPll&0x3Ful) + 1;
    NR = ((ArmPll >> 8)&0x3Ful) + 1;
    ArmPll =  ScuReg->CRU_PLL_CON[3][1];
    //printf("GetGPLLCLK1 = %x\n",ArmPll);
    NF = (ArmPll&0x1FFFul) + 1;
    //printf("GetGPLLCLK3 = %x\n",ArmPll);
    //ArmPll =  ScuReg->CRU_PLL_CON[3][2];
    ArmPll = 24*NF/(NR*NO);
    //printf("ArmPll = %d\n",ArmPll);
    return ArmPll;
}

uint32 GetAHBCLK(void)
{
    uint32 Div1,Div2;
    uint32 ArmPll;
    uint32 AhbClk;
    if (ChipType == CONFIG_RK2928 || ChipType == CONFIG_RK3026)
    	 ArmPll = GetPLLCLK(3);
    else
        ArmPll = GetGPLLCLK();
	
    AhbClk = g_cruReg->CRU_CLKSEL_CON[10];
    Div1 = (AhbClk&0x1F) + 1;
    Div2 = 1<<((AhbClk>>8)&0x3);
    AhbClk = ArmPll/(Div1*Div2);
    //printf("AhbClk = %d\n",AhbClk);
    return AhbClk*1000;
}

uint32 GetMmcCLK(void)
{
    uint32 ArmPll;
    if (ChipType == CONFIG_RK2928 || ChipType == CONFIG_RK3026)
    	ArmPll = GetPLLCLK(3)  * 1000;
	else
        ArmPll = GetAHBCLK();
    return (ArmPll );
}

void uart2UsbEn(uint8 en)
{    
    if(en)
    {
        g_grfReg->GRF_UOC1_CON0 = 0x34000000;	
        if((!(g_grfReg->GRF_SOC_STATUS0) & (1<<7)) && (g_BootRockusb == 0))
        {
            DRVDelayUs(1);
            if(!(g_grfReg->GRF_SOC_STATUS0) & (1<<7))
            {
            	  #if 0
                g_grfReg->GRF_UOC0_CON[0] = 0x10001000;
                //g_grfReg->GRF_UOC0_CON[4] = 0x007f0055;
                g_grfReg->GRF_UOC1_CON[4] = 0x34003000;
		  #endif
		g_grfReg->GRF_UOC0_CON0 = 0x007f0055;
		g_grfReg->GRF_UOC1_CON0 = 0x34003000;		
            }
		
        }
    }
    else
    {
    	#if 0
        g_grfReg->GRF_UOC1_CON[4] = 0x34000000; 
        //g_grfReg->GRF_UOC0_CON[4] = 0x00010000;
        g_grfReg->GRF_UOC0_CON[0] = 0x10000000;
	#endif
	g_grfReg->GRF_UOC1_CON0 = 0x34000000;	
    }
}


/**************************************************************************
USB PHY RESET
***************************************************************************/
bool UsbPhyReset(void)
{
    if (ChipType == CONFIG_RK2928 || ChipType == CONFIG_RK3026) 
    {
        *(uint32*)0x20008190 = 0x34000000; 
    }
    else if(ChipType == CONFIG_RK3026)
    {
        *(uint32*)0x20008190 = 0x34000000; 
    }
    DRVDelayUs(1100); //1.1ms
    g_cruReg->CRU_SOFTRST_CON[4] = ((7ul<<5)<<16)|(7<<5);
    DRVDelayUs(10*100);    //delay 10ms
    g_cruReg->CRU_SOFTRST_CON[4] = (uint32)((7ul<<5)<<16)|(0<<5);
    DRVDelayUs(1*100);     //delay 1ms
    return (TRUE);
}

/**************************************************************************
USB PHY RESET
***************************************************************************/
void FlashCsInit(void)
{ 
    if (ChipType == CONFIG_RK2928 || ChipType == CONFIG_RK3026)
    {
        g_grfReg->GRF_GPIO_IOMUX[1].GPIOD_IOMUX = (0xFFFFuL<<16)|0x5555;   // nand d0-d7
        g_grfReg->GRF_GPIO_PULL[1].GPIOH = 0xFF00FF00;                     //disable pull up d0~d7
        g_grfReg->GRF_GPIO_IOMUX[2].GPIOA_IOMUX = (0xFFFFuL<<16)|0x5555;   // nand dqs,cs0,wp,rdy,rdn,wrn,cle,ale 
    }
}

/**************************************************************************
USB PHY RESET
***************************************************************************/
void SpiGpioInit(void)
{

}

void sdmmcGpioInit(uint32 ChipSel)
{
    if(ChipSel == 2)
    {
        if (ChipType == CONFIG_RK2928 || ChipType == CONFIG_RK3026)
        {
            g_grfReg->GRF_GPIO_IOMUX[1].GPIOC_IOMUX = ((0xFuL<<12)<<16)|(0xA<<12);        // emmc rstn,cmd
            g_grfReg->GRF_GPIO_IOMUX[1].GPIOD_IOMUX = (0xFFFFuL<<16)|0xAAAA;              // emmc d0-d7
            g_grfReg->GRF_GPIO_IOMUX[2].GPIOA_IOMUX = (((0x3uL<<14)|(0x3<<10))<<16) 
                                                      |(0x2uL<<14)|(0x2<<10);             // emmc_clk,pwren
            g_grfReg->GRF_GPIO_PULL[1].GPIOH = 0xFF000000;                                // pull up d0~d7
        }
    }
#ifdef RK_SDCARD_BOOT_EN
    else if(ChipSel == 0)
    {
        if (ChipType == CONFIG_RK2928 || ChipType == CONFIG_RK3026)
        {
            g_grfReg->GRF_GPIO_IOMUX[1].GPIOB_IOMUX = (((0x1<<14)|(0x1<<12))<<16)|(0x1<<14)|(0x1<<12);  // mmc0_cmd mmc0_pwren
            g_grfReg->GRF_GPIO_IOMUX[1].GPIOC_IOMUX = (((0x1<<10)|(0x1<<8)|(0x1<<6)|(0x1<<4)|(0x1<<0))<<16)
                                                    |(0x1<<10)|(0x1<<8)|(0x1<<6)|(0x1<<4)|(0x1<<0); //mmc0_clkout d0-d3
        }
    }
#endif    
}

/***************************************************************************
函数描述:关闭TCM
入口参数:无
出口参数:无
调用函数:无
***************************************************************************/
void DisableRemap(void)
{
// TODO: Disable Remap
    //clean remap bit in grf enabled, remap 0x0000 to rom, 
    //*(unsigned long volatile *)(GRF_BASE + GRF_SOC_CON0) 0x20008140  = 0x10001000; 
    g_grfReg->GRF_SOC_CON[0] = ((0x1uL<<12)<<16)|(0x0<<12); 
}


void FW_NandDeInit(void)
{
#if 0
#ifdef RK_FLASH_BOOT_EN 
    FlashDeInit();
    FlashTimingCfg(150*1000);
#endif
#endif
#ifdef RK_FLASH_BOOT_EN
    if(gpMemFun->flag == BOOT_FROM_FLASH)
    {
        FtlDeInit();
        FlashDeInit();
    }
#endif

#ifdef RK_SDMMC_BOOT_EN
    SdmmcDeInit();
#endif
}


/***************************************************************************
函数描述:系统复位
入口参数:无
出口参数:无
调用函数:无
***************************************************************************/
void SoftReset(void)
{
#if 1
    pFunc fp;
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG0_BASE_ADDR ;//USB_OTG_BASE_ADDR
    
     DisableIRQ();
    UsbPhyReset();
    OtgReg->Device.dctl |= 0x02;          //soft disconnect  
    FW_NandDeInit();
    MMUDeinit();              /*关闭MMU*/
    //g_cruReg->CRU_MODE_CON = (0x1uL << 16) | (0); //arm enter slow mode 
    //Delay100cyc(10);
   // DisableRemap();
    g_giccReg->ICCEOIR=USB_OTG_INT_CH;
     if (ChipType == CONFIG_RK2928)
    {
        ResetCpu_3026(0x20008140);
    }
    else if(ChipType == CONFIG_RK3026)
    {
    	ResetCpu_3026(0x20008140);
    	#if 0
        g_cruReg->CRU_MODE_CON = 0x33030000;
        DRVDelayUs(10);
        WriteReg32(0x20008140,0x10000000);
        DRVDelayUs(10);
        fp=0x00;
        fp();
	#endif
    }	
    g_cruReg->CRU_GLB_SRST_SND_VALUE = 0xeca8; //soft reset
     Delay100cyc(10);
    while(1);
#else
    pFunc fp;
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG0_BASE_ADDR ;//USB_OTG_BASE_ADDR

    OtgReg->Device.dctl |= 0x02;          //soft disconnect
    DisableIRQ();
    UsbPhyReset();
    FW_NandDeInit();

    MMUDeinit();              /*关闭MMU*/

    g_cruReg->CRU_MODE_CON = (0x1uL << 16) | (0); //arm enter slow mode 
    Delay100cyc(10);
    DisableRemap();
    g_giccReg->ICCEOIR=USB_OTG_INT_CH;
    //g_cruReg->CRU_GLB_SRST_FST_VALUE = 0xfdb9;
    g_cruReg->CRU_GLB_SRST_SND_VALUE = 0xeca8; //soft reset grf gpio no reset
    Delay100cyc(10);
    while(1);
#endif
}

void EmmcPowerEn(uint8 En)
{
// TODO: EMMC 电源控制
    if(En)
    {
        g_EMMCReg->SDMMC_PWREN = 1;  // power enable
        g_EMMCReg->SDMMC_RST_n = 1;  // reset off
    }
    else
    {
        g_EMMCReg->SDMMC_PWREN = 0;  // power disable
        g_EMMCReg->SDMMC_RST_n = 0;  //reset on
    }
}

void SDCReset(uint32 sdmmcId)
{
#ifndef FPGA_EMU 
    uint32 data = g_cruReg->CRU_SOFTRST_CON[5];
    data = ((1<<16)|(1))<<(sdmmcId + 1);
    g_cruReg->CRU_SOFTRST_CON[5] = data;
    DRVDelayUs(100);
    data = ((1<<16)|(0))<<(sdmmcId + 1);
    g_cruReg->CRU_SOFTRST_CON[5] = data;
    DRVDelayUs(200);
    EmmcPowerEn(1);
#endif
}

int32 SCUSelSDClk(uint32 sdmmcId, uint32 div)
{
    if((div == 0))//||(sdmmcId > 1))
    {
        return (-1);
    }
    if(0 == sdmmcId)
    {
        g_cruReg->CRU_CLKSEL_CON[11] =  (((0x3Ful<<0) | (1<<6))<<16)|(((div-1)<<0) | (1<<6)); //general pll
    }
    else if(1 == sdmmcId)
    {
        g_cruReg->CRU_CLKSEL_CON[12] =  (((0x3Ful<<0) | (1<<6))<<16)|(((div-1)<<0) | (1<<6)); //general pll
    }
    else    //emmc
    {
        g_cruReg->CRU_CLKSEL_CON[12] =  (((0x3Ful<<8) | (1<<7))<<16)|(((div-1)<<8) | (1<<7)); //general pll
    }
    return(0);
}

//mode=1  changemode to normal mode;
//mode=0  changemode to boot mode
int32 eMMC_changemode(uint8 mode)
{ 
#ifdef RK_SDMMC_BOOT_EN    
    eMMC_SetDataHigh();
#endif
}

/*----------------------------------------------------------------------
Name	: IOMUXSetSDMMC1
Desc	: 设置SDMMC1相关管脚
Params  : type: IOMUX_SDMMC1 设置成SDMMC1信号线
                IOMUX_SDMMC1_OTHER设置成非SDMMC1信号线
Return  : 
Notes   : 默认使用4线，不使用pwr_en, write_prt, detect_n信号
----------------------------------------------------------------------*/
void IOMUXSetSDMMC(uint32 sdmmcId,uint32 Bits)
{
// TODO:SDMMC IOMUX 
}

// pwr key gpio3 c5
/*void powerOn(void)
{
    uint32 chipTag = RKGetChipTag();
    if(chipTag == RK2928G_CHIP_TAG)
    {
        write_XDATA32((GPIO1_BASE_ADDR+0), ReadReg32(GPIO1_BASE_ADDR)|((1ul<<(1)))); //out put high
        write_XDATA32((GPIO1_BASE_ADDR+0x4), (read_XDATA32(GPIO1_BASE_ADDR+0x4)|((0x1ul<<(1))))); // port1 A1 out
    }
    else if(chipTag == RK2926_CHIP_TAG)
    {
        write_XDATA32((GPIO1_BASE_ADDR+0), ReadReg32(GPIO1_BASE_ADDR)|((1ul<<(2)))); //out put high
        write_XDATA32((GPIO1_BASE_ADDR+0x4), (read_XDATA32(GPIO1_BASE_ADDR+0x4)|((0x1ul<<(2))))); // port1 A2 out
    }
}

void powerOff(void)
{
    uint32 chipTag = RKGetChipTag();
    if(chipTag == RK2928G_CHIP_TAG)
    {
        write_XDATA32((GPIO1_BASE_ADDR+0), ReadReg32(GPIO1_BASE_ADDR)&(~(1ul<<(1)))); //out put low
        write_XDATA32((GPIO1_BASE_ADDR+0x4), (read_XDATA32(GPIO1_BASE_ADDR+0x4)|((0x1ul<<(1))))); // port1 A1 out
    }
    else if(chipTag == RK2926_CHIP_TAG)
    {
        write_XDATA32((GPIO1_BASE_ADDR+0), ReadReg32(GPIO1_BASE_ADDR)&(~(1ul<<(2)))); //out put low
        write_XDATA32((GPIO1_BASE_ADDR+0x4), (read_XDATA32(GPIO1_BASE_ADDR+0x4)|((0x1ul<<(2))))); // port1 A2 out
    }
}*/

void power_io_ctrl(uint8 mode)
{
    gpio_conf *key_gpio = &pin_powerHold.key.gpio;
    if(mode)         // 输出高电平
    {
        write_XDATA32((key_gpio->io_write), ReadReg32((key_gpio->io_write))|((1ul<<key_gpio->index))); //out put high
        write_XDATA32(key_gpio->io_dir_conf, (read_XDATA32(key_gpio->io_dir_conf)|((1ul<<key_gpio->index)))); // port6 B0 out
    }
    else                //其他情况输出低电平
    {
        write_XDATA32((key_gpio->io_write), ReadReg32((key_gpio->io_write))&(~(1ul<<key_gpio->index))); //out put high
        write_XDATA32(key_gpio->io_dir_conf, (read_XDATA32(key_gpio->io_dir_conf)|((1ul<<key_gpio->index)))); // port6 B0 out
    }
}

void powerOn(void)
{
    gpio_conf *key_gpio = &pin_powerHold.key.gpio;
    power_io_ctrl((key_gpio->valid)&0x01);
}

void powerOff(void)
{
    gpio_conf *key_gpio = &pin_powerHold.key.gpio;
    power_io_ctrl((key_gpio->valid&0x01)==0);
}

int RKGetChipTag(void)
{
    uint32 i;
    uint32 hCnt = 0;
    uint32 valueL;
    uint32 valueH;
    uint32 value;
    WriteReg32((GPIO3_BASE_ADDR+0x4),  (ReadReg32(GPIO3_BASE_ADDR+0x4)&(~(0x7ul<<0)))); //gpio3   portA 0:2 input
    value = (ReadReg32(GPIO3_BASE_ADDR+0x50))&0x07;
    return value;
}

uint32 RKGetDDRTag(void)
{
    return 0x30334B52; // "RK30"
}

