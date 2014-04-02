
#include    "config.h"
#include   <asm/io.h>

#define     DELAY_ARM_FREQ      50
#define     ASM_LOOP_INSTRUCTION_NUM     4
#define     ASM_LOOP_PER_US    (DELAY_ARM_FREQ/ASM_LOOP_INSTRUCTION_NUM) //
 
/***************************************************************************
函数描述:延时
入口参数:us数
出口参数:
调用函数:
***************************************************************************/
extern uint32 Timer0Get100ns( void );
void DRVDelayUs(uint32 count)
{
#if 0
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
#else
	__udelay(count);
#endif
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
    DRVDelayUs(1000*count);
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
    Rk30ChipVerInfo[0] = 0;
    ftl_memcpy(Rk30ChipVerInfo, (uint8*)(BOOT_ROM_CHIP_VER_ADDR), 16);
    
#if(CONFIG_RKCHIPTYPE == CONFIG_RK3288)
    ChipType = CONFIG_RK3288;
    Rk30ChipVerInfo[0] =  0x33323041; // "320A"
#endif 
}

#include "rockusb/USB20.h"
void ModifyUsbVidPid(USB_DEVICE_DESCRIPTOR * pDeviceDescr)
{
    if(ChipType == CONFIG_RK3066B) 
    {
        pDeviceDescr->idProduct = 0x310A;
        pDeviceDescr->idVendor  = 0x2207;
    }
    else if (ChipType == CONFIG_RK3168)
    {
        pDeviceDescr->idProduct = 0x300B;
        pDeviceDescr->idVendor  = 0x2207;
    }
    else if (ChipType == CONFIG_RK3188 || ChipType == CONFIG_RK3188B)
    {
        pDeviceDescr->idProduct = 0x310B;
        pDeviceDescr->idVendor  = 0x2207;
    }
    else if (ChipType == CONFIG_RK3288)
    {
        pDeviceDescr->idProduct = 0x320A;
        pDeviceDescr->idVendor  = 0x2207;
    }
}

//定义Loader启动异常类型
//系统中设置指定的sdram值为该标志，重启即可进入rockusb
//系统启动失败标志
uint32 IReadLoaderFlag(void)
{
    return (*((REG32*)PMU_SYS_REG0));
}

void ISetLoaderFlag(uint32 flag)
{
    if(*((REG32*)PMU_SYS_REG0) == flag)
        return;
    *((REG32*)PMU_SYS_REG0) = flag;
}

uint32 IReadLoaderMode(void)
{
    return (*((REG32*)PMU_SYS_REG1));
}

typedef enum PLL_ID_Tag
{
    APLL=0,
    DPLL,
    CPLL,
    GPLL,
    
    PLL_MAX
}PLL_ID;

#define PLL_RESET  (((0x1<<5)<<16) | (0x1<<5))
#define PLL_DE_RESET  (((0x1<<5)<<16) | (0x0<<5))
#define NR(n)      ((0x3F<<(8+16)) | ((n-1)<<8))
#define NO(n)      ((0x3F<<16) | (n-1))
#define NF(n)      ((0xFFFF<<16) | (n-1))
#define NB(n)      ((0xFFF<<16) | (n-1))





/*****************************************
NR   NO     NF             Fout(range)
3    8      37.5 - 187.5
4    6      50   - 250    100 - 150
6    4      75   - 375    150 - 250
12   2      150  - 750    250 - 500
24   1      300  - 1500   500 - 1000
******************************************/
//rk 3066b 不能小于100Mhz



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
    pCRU_REG ScuReg=(pCRU_REG)CRU_BASE_ADDR;
    
    ArmPll = GetGPLLCLK();
    AhbClk = ScuReg->CRU_CLKSEL_CON[10];
    Div1 = (AhbClk&0x1F) + 1;
    Div2 = 1<<((AhbClk>>8)&0x3);
    AhbClk = ArmPll/(Div1*Div2);
    //printf("AhbClk = %d\n",AhbClk);
    return AhbClk*1000;
}

uint32 GetMmcCLK(void)
{
    //return (GetAHBCLK());
     return rk_get_general_pll()/1000;
}


/**************************************************************************
USB PHY RESET
***************************************************************************/
bool UsbPhyReset(void)
{
    if(ChipType == CONFIG_RK3188 || ChipType == CONFIG_RK3188B)
    {
        //uart2UsbEn(0);
        //g_3066B_grfReg->GRF_UOC0_CON[0] = (0x0000 | (0x0300 << 16));
        //g_3066B_grfReg->GRF_UOC0_CON[2] = (0x0000 | (0x0004 << 16));
        //3188 配置为software control usb phy，usb没有接的时候访问DiEpDma和DopDma会死机
       // ddr 初始化时会配置下面三行代码
       // g_3066B_grfReg->GRF_UOC0_CON[2] = ((0x01 << 2) | ((0x01 << 2) << 16));  //software control usb phy enable
       // g_3066B_grfReg->GRF_UOC0_CON[3] = (0x2A | (0x3F << 16));  //usb phy enter suspend
       // g_3066B_grfReg->GRF_UOC0_CON[0] = (0x0300 | (0x0300 << 16)); // uart enable
    }
    
    if(ChipType == CONFIG_RK3066)
    {
        g_grfReg->GRF_UOC0_CON[2] = (0x0000 | (0x0004 << 16)); //software control usb phy disable
    }
    else
    {
        g_grfReg->GRF_UOC0_CON[2] = (0x0000 | (0x0004 << 16)); //software control usb phy disable
    }

    DRVDelayUs(1100); //1.1ms
    g_cruReg->CRU_SOFTRST_CON[8] = ((7ul<<4)<<16)|(7<<4);
    DRVDelayUs(10*100);    //delay 10ms
    g_cruReg->CRU_SOFTRST_CON[8] = (uint32)((7ul<<4)<<16)|(0<<4);
    DRVDelayUs(1*100);     //delay 1ms
    return (TRUE);
}


void sdmmcGpioInit(uint32 ChipSel)
{
	writel(0xffffaaaa,0xff770020);
	writel(0x000c0008,0xff770024);
	writel(0x003f002a,0xff770028);
}


void FW_NandDeInit(void)
{
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
extern void ResetCpu(unsigned long remap_addr);

void SoftReset(void)
{
    pFunc fp;
    pCRU_REG cruReg=(pCRU_REG)CRU_BASE_ADDR;
    pUSB_OTG_REG OtgReg=(pUSB_OTG_REG)USB_OTG_BASE_ADDR;

    disable_interrupts();
    UsbPhyReset();
    OtgReg->Device.dctl |= 0x02;          //soft disconnect
    FW_NandDeInit();

    MMUDeinit();              /*关闭MMU*/
    //cruReg->CRU_MODE_CON = 0x33030000;    //cpu enter slow mode
    //Delay100cyc(10);
    g_giccReg->ICCEOIR=INT_USB_OTG;
    //DisableRemap();
    if(ChipType == CONFIG_RK3066)
    {
        ResetCpu((GRF_BASE + 0x150));
    }
    else if(ChipType == CONFIG_RK3288)
    {
        ResetCpu((0xff740000));
    }
    else
    {
        ResetCpu((GRF_BASE + 0xA0));
    }
    //cruReg->CRU_GLB_SRST_FST_VALUE = 0xfdb9; //kernel 使用 fst reset时，loader会死机，问题还没有查，所有loader还是用snd reset
    cruReg->CRU_GLB_SRST_SND_VALUE = 0xeca8; //soft reset
    Delay100cyc(10);
    while(1);
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
    uint32 data = g_cruReg->CRU_SOFTRST_CON[8];
    data = ((1<<16)|(1))<<(sdmmcId + 1);
    g_cruReg->CRU_SOFTRST_CON[8] = data;
    DRVDelayUs(100);
    data = ((1<<16)|(0))<<(sdmmcId + 1);
    g_cruReg->CRU_SOFTRST_CON[8] = data;
    DRVDelayUs(200);
    EmmcPowerEn(1);
}

int32 SCUSelSDClk(uint32 sdmmcId, uint32 div)
{
    if((div == 0))//||(sdmmcId > 1))
    {
        return (-1);
    }
#if 1
    if(0 == sdmmcId)
    {
        g_cruReg->CRU_CLKSEL_CON[11] = (0x3Ful<<16)|(div-1)<<0;
    }
    else if(1 == sdmmcId)
    {
        g_cruReg->CRU_CLKSEL_CON[12] = (0x3Ful<<16)|(div-1)<<0;
    }
    else    //emmc
    {
        //RkPrintf("SCUSelSDClk 2 %d\n",div);
        g_cruReg->CRU_CLKSEL_CON[12] = (0xFFul<<24)|(div-1)<<8 |(1<<14);//emmc use gerenall pll
    }
#endif
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




