/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	api_scu.h
Desc 	:	
Author 	:  	yangkai
Date 	:	2008-12-16
Notes 	:

********************************************************************/
#ifndef _API_SCU_H
#define _API_SCU_H

/********************************************************************
**                            宏定义                                *
********************************************************************/
/*SCU CLK SEL0 CON*/
#define CLK_SDMMC1_SHFT     25
#define CLK_SDMMC1_MASK     (0x07u<<25)
#define CLK_SDMMC1_DIV(i)   ((i&0x07u)<<25)

#define CLK_SENSOR_SHFT     23
#define CLK_SENSOR_MASK     (0x03u<<23)
#define CLK_SENSOR_24M      (0x00u<<23)
#define CLK_SENSOR_27M      (0x01u<<23)
#define CLK_SENSOR_48M      (0x02u<<23)

#define CLK_48M_SHFT     20
#define CLK_48M_MASK        (0x07u<<20)
#define CLK_48M_DIV(i)      ((i&0x07u)<<20)


#define CLK_USBPHY_SHFT     18
#define CLK_USBPHY_MASK     (0x03u<<18)
#define CLK_USBPHY_24M      (0x00u<<18)
#define CLK_USBPHY_12M      (0x01u<<18)
#define CLK_USBPHY_48M      (0x01u<<18)

#define CLK_LCDC_ARMPLL     (0x00u<<16)//
#define CLK_LCDC_DSPPLL     (0x01u<<16)//
#define CLK_LCDC_CODPLL     (0x02u<<16)//

#define CLK_LCDC_SHFT     8
#define CLK_LCDC_MASK       (0x0ffu<<8)
#define CLK_LCDC_DIV(i)     ((i&0xffu)<<8)

#define CLK_LCDC_DIVOUT     (0x00<<7)//
#define CLK_LCDC_27M        (0X01<<7)//

#define CLK_SDMMC0_SHFT     4
#define CLK_SDMMC0_MASK     (0x07u<<4)
#define CLK_SDMMC0_DIV(i)   ((i&0x07u)<<4)

#define CLK_PCLK_SHFT     2
#define CLK_PCLK_MASK       (0x03u<<2)
#define CLK_HCLK_PCLK_11    (0x00u<<2)
#define CLK_HCLK_PCLK_21    (0x01u<<2)
#define CLK_HCLK_PCLK_41    (0x02u<<2)

#define CLK_HCLK_SHFT     0
#define CLK_HCLK_MASK       (0x03u<<0)
#define CLK_ARM_HCLK_11     (0x00u<<0)
#define CLK_ARM_HCLK_21     (0x01u<<0)
#define CLK_ARM_HCLK_31     (0x02u<<0)
#define CLK_ARM_HCLK_41     (0x03u<<0)

/*SCU CLK SEL1 CON*/
#define CLK_SHMEM1_SHFT     30
#define CLK_SHMEM1_MASK     (0x01u<<30)
#define CLK_SHMEM1_DEMODCLK (0x00u<<30)
#define CLK_SHMEM1_ARMCLK   (0x01u<<30)

#define CLK_SHMEM0_SHFT     29
#define CLK_SHMEM0_MASK     (0x01u<<29)
#define CLK_SHMEM0_DEMODCLK (0x00u<<29)
#define CLK_SHMEM0_ARMCLK   (0x01u<<29)

#define CLK_HSADCO_SHFT     28
#define CLK_HSADCO_MASK      (0x01u<<28)
#define CLK_HSADCO_NORMAL    (0x00u<<28)
#define CLK_HSADCO_INVERT    (0x01u<<28)

#define CLK_GPS_SHFT     27
#define CLK_GPS_MASK        (0x01u<<27)
#define CLK_GPS_DEMODCLK    (0x00u<<27)
#define CLK_GPS_TUNER_INPUT (0x01u<<27)

#define CLK_DEMOD_INTCLK    (0x00u<<26)//
#define CLK_DEMOD_EXTCLK    (0x01u<<26)//

#define CLK_DEMOD_ARMPLL    (0x00u<<24)//
#define CLK_DEMOD_DSPPLL    (0x01u<<24)//
#define CLK_DEMOD_CODPLL    (0x02u<<24)//

#define CLK_DEMOD_SHFT     16
#define CLK_DEMOD_MASK      (0x0ffu<<16)
#define CLK_DEMOD_DIV(i)    ((i&0x0ffu)<<16)

#define CLK_LSADC_SHFT     8
#define CLK_LSADC_MASK      (0x0ffu<<8)
#define CLK_LSADC_DIV(i)    ((i&0x0ffu)<<8)

#define CLK_CODEC_SHFT     3
#define CLK_CODEC_MASK      (0x1fu<<3)
#define CLK_CODEC_DIV(i)    ((i&0x1fu)<<3)

#define CLK_CODEC_CPLLCLK   (0x00u<<2)//
#define CLK_CODEC_12M       (0x01u<<2)//

#define CLK_CPLL_SLOW       (0x00u<<0)//
#define CLK_CPLL_NORMAL     (0x01u<<0)//
#define CLK_CPLL_DSLOW      (0x02u<<0)//


#define DEMOD_CLK_TYPE CLK_SRC_CODECPLL
#define LCDC_CLK_TYPE CLK_SRC_CODECPLL

/********************************************************************
**                          结构定义                                *
********************************************************************/
typedef enum _SCU_RST
{
    SCU_RST_USBOTG = 0,
    SCU_RST_LCDC,
    SCU_RST_VIP,
    SCU_RST_NANDC,
    SCU_RST_DSP,
    SCU_RST_DSPPER,
    SCU_RST_I2S,
    SCU_RST_LSADC,
    SCU_RST_DEBLK,
    SCU_RST_SDMMC0,
    SCU_RST_DEMOD,
    SCU_RST_USBC,
    SCU_RST_USBPHY,
    SCU_RST_AGC,
    SCU_RST_DOWNMIXER,
    SCU_RST_IQIMBALANCE,
    SCU_RST_FRAMEDET,
    SCU_RST_FFT,
    SCU_RST_VITERBI,
    SCU_RST_BITDITL,
    SCU_RST_RS,
    SCU_RST_PREFFT,
    SCU_RST_DEMODGEN,
    SCU_RST_ARM,
    SCU_RST_SDMMC1,
    SCU_RST_DSPA2A,
    SCU_RST_SHMEM0,
    SCU_RST_SHMEM1,
    SCU_RST_SDRAM,
    SCU_RST_MAX
}eSCU_RST;

typedef enum _CLK_GATE
{
    /*SCU CLK GATE 0 CON*/
    CLK_GATE_ARM = 0,
    CLK_GATE_DSP,
    CLK_GATE_DMA,
    CLK_GATE_SRAMARM,
    CLK_GATE_SRAMDSP,
    CLK_GATE_HIF,
    CLK_GATE_OTGBUS,
    CLK_GATE_OTGPHY,
    CLK_GATE_NANDC,
    CLK_GATE_INTC,
    CLK_GATE_DEBLK,
    CLK_GATE_LCDC,
    CLK_GATE_VIP,
    CLK_GATE_I2S,
    CLK_GATE_SDMMC0,
    CLK_GATE_EBROM,
    CLK_GATE_GPIO0,
    CLK_GATE_GPIO1,
    CLK_GATE_UART0,
    CLK_GATE_UART1,
    CLK_GATE_I2C0,
    CLK_GATE_I2C1,
    CLK_GATE_SPI0,
    CLK_GATE_SPI1,
    CLK_GATE_PWM,
    CLK_GATE_TIMER,
    CLK_GATE_WDT,
    CLK_GATE_RTC,
    CLK_GATE_LSADC,
    CLK_GATE_SHMEM0,
    CLK_GATE_SHMEM1,
    CLK_GATE_SDMMC1,
    
    /*SCU CLK GATE 1 CON*/
    CLK_GATE_HSADC = 32,
    CLK_GATE_DEMODFIFO,
    CLK_GATE_DEMODBUS,
    CLK_GATE_DEMODOTHER,
    CLK_GATE_AGC,
    CLK_GATE_DOWNMIXER,
    CLK_GATE_PREFFT,
    CLK_GATE_IQIMBALANCE,
    CLK_GATE_FRAMEDET,
    CLK_GATE_FFTMEM,
    CLK_GATE_BITDITL,
    CLK_GATE_VITERBIMEM,
    CLK_GATE_PREFFTMEM,
    CLK_GATE_VITERBI,
    CLK_GATE_RS,
    CLK_GATE_EXTMEM,
    CLK_GATE_SDRMEM,
    CLK_GATE_MSDRMEM,
    CLK_GATE_DEMOD,
    CLK_GATE_LCDCh,
    
    /*SCU CLK GATE 2 CON*/
    CLK_GATE_ARMIBUS = 64,
    CLK_GATE_ARMDBUS,
    CLK_GATE_DSPBUS,
    CLK_GATE_EXPBUS,
    CLK_GATE_APBBUS,
    CLK_GATE_EFUSE,
    CLK_GATE_DTCM1,
    CLK_GATE_DTCM0,
    CLK_GATE_ITCM,
    CLK_GATE_MAX
}eCLK_GATE;

typedef enum _CLK_SEL
{
    CLK_SEL_HCLK = 0,
    CLK_SEL_PCLK,
    CLK_SEL_SDMMC0,
    CLK_SEL_LCDC,
    CLK_SEL_USBPHY,
    CLK_SEL_48M,
    CLK_SEL_SENSOR,
    CLK_SEL_SDMMC1,

    CLK_SEL_CODEC = 8,
    CLK_SEL_LSADC,
    CLK_SEL_DEMOD,
    CLK_SEL_GPS,
    CLK_SEL_HSADCO,
    CLK_SEL_SHMEM0,
    CLK_SEL_SHMEM1,

    CLK_SEL_MAX
}eCLK_SEL;

typedef enum _SCU_APP
{
    SCU_DUMMY = 0,
    SCU_IDLE ,
    SCU_INIT,
    SCU_MEDIALIBUPDATE,

    SCU_MAINMENU,
    SCU_BROWER,
    SCU_MP3,
    SCU_MP3H,
    SCU_WMA,

    SCU_WAV,
    SCU_APE,
    SCU_FLAC,
    SCU_RA,

    SCU_AAC,
    SCU_OGG,
    SCU_EQ,
    SCU_RECORDADPCM,
    SCU_RECORDMP3,

    SCU_VIDEOLOWLL,
    SCU_VIDEOLOWL,
    SCU_VIDEOLOW,
    SCU_VIDEOMEDLOW,
    SCU_VIDEOMED,
    SCU_VIDEOMEDHIGH,
    SCU_VIDEOHIGH,
    SCU_VIDEOTVOUT,
    SCU_RVLOW,

    SCU_RVMED,
    SCU_RVHIGH,
    //SCU_PICTURE,
    SCU_BMP,
    SCU_JPEG,

    SCU_GIF,
    SCU_TXT,
    SCU_FM,
    SCU_STOPWATCH,
    SCU_GAME,
    SCU_USB,
    SCU_BLON,
    SCU_LCD_UPDATE,

    SCU_APP_MAX
}eSCU_APP;

typedef struct tagSCU_APP_TABLE
{
    uint8  scuAppId;
    uint8  counter;
    uint16 armFreq;
    uint16 dspFreq;
    uint16 hclkDiv;
}SCU_APP_TABLE,*pSCU_APP_TABLE;

typedef enum _SCU_PCLK_DIV
{
    PCLK_DIV1 = CLK_HCLK_PCLK_11>>2,       // AHB clk : APB clk =  1:1
    PCLK_DIV2 = CLK_HCLK_PCLK_21>>2,       // AHB clk : APB clk =  2:1
    PCLK_DIV4 = CLK_HCLK_PCLK_41>>2,       // AHB clk : APB clk =  4:1
    PCLK_DIV_MAX
}eSCU_PCLK_DIV;

typedef enum _SCU_HCLK_DIV
{
    HCLK_DIV1 = CLK_ARM_HCLK_11,           // arm clk : hclk  =  1:1
    HCLK_DIV2 = CLK_ARM_HCLK_21,           // arm clk : hclk  =  2:1
    HCLK_DIV3 = CLK_ARM_HCLK_31,           // arm clk : hclk  =  3:1
    HCLK_DIV4 = CLK_ARM_HCLK_41,           // arm clk : hclk  =  4:1
    HCLK_DIV_MAX
}eSCU_HCLK_DIV;

typedef enum _SCU_PLL_ID
{
    PLL_ARM = 0,
    PLL_DSP = 1,
    PLL_CODEC = 2,
    PLL_MAX
}eSCU_PLL_ID;
typedef enum _PER_CLK_SRC
{
    CLK_SRC_CODECPLL,
    CLK_SRC_ARMPLL,
    CLK_SRC_DSPPLL,
    CLK_SRC_EXT,
    CLK_SRC_MAX
}ePER_CLK_SRC;

/********************************************************************
**                          变量定义                                *
********************************************************************/

/********************************************************************
**                      对外函数接口声明                            *
********************************************************************/
//#ifdef FPGA_BOARD
//extern void SetArmPll(int ndiv, int bypass);
//extern void DisableTestBLK(void);
//extern void SCUInit(uint32 nMHz);
//#else
//extern void SCUInit(void);
//#endif
#define SCUGetArmFreq() ((uint32)g_chipClk.armFreq)
#define SCUGetDspFreq() ((uint32)g_chipClk.dspFreq)
#define SCUGetCodecFreq() ((uint32)g_chipClk.codecFreq)
#define SCUGetAHBFreq() (((uint32)g_chipClk.armFreq)>>g_chipClk.ahbDiv)
#define SCUGetAPBFreq() (SCUGetAHBFreq()>>g_chipClk.apbDiv)

extern void SCUSelectClk(eCLK_SEL module, uint32 param);
extern void SCUSetCodecPll(uint32 freq);
extern void SCUSetCodecFreq(uint32 freq);
extern void SCUSetDemodFreq(uint32 freq);
extern void SCUSetLCDCFreq(uint32 freq);
extern void SCUEnableClk(eCLK_GATE clkId);
extern void SCUDisableClk(eCLK_GATE clkId);
extern void SCURstModule(eSCU_RST moduleId);
extern void SCUUnrstModule(eSCU_RST moduleId);

extern uint32 SCUStartAPP(eSCU_APP appId);
extern uint32 SCUStopAPP(eSCU_APP appId);

/********************************************************************
**                          表格定义                                *
********************************************************************/
#ifdef IN_SCU
SCU_APP_TABLE g_pmuAPPTabel[SCU_APP_MAX] =
{
    {SCU_DUMMY,                 0,  0,  0,  HCLK_DIV1},
    {SCU_IDLE,                  0,  FREQ_ARM_IDLE,  0,  HCLK_DIV1},
    {SCU_INIT,                  0,  192,  0,  HCLK_DIV1 },
    {SCU_MEDIALIBUPDATE,        0,  192,  0,  HCLK_DIV1 },
    {SCU_MAINMENU,              0,  12,  0,  HCLK_DIV1},
    {SCU_BROWER,                0,  40,  0,  HCLK_DIV1},
    {SCU_MP3,                   0,  54,  0,  HCLK_DIV1},
    {SCU_MP3H,                  0,  58,  0,  HCLK_DIV1},
    {SCU_WMA,                   0,  80,  0,  HCLK_DIV1},
    {SCU_WAV,                   0,  24,  0,  HCLK_DIV1},
    {SCU_APE,                   0,  FREQ_ARM_MAX,  0,  HCLK_DIV1},
    {SCU_FLAC,                  0,  64,  0,  HCLK_DIV1},
    {SCU_RA,                    0,  60,  0,  HCLK_DIV1},
    {SCU_AAC,                   0,  80,  0,  HCLK_DIV1},
    {SCU_OGG,                   0,  80,  0,  HCLK_DIV1},
    {SCU_EQ,                    0,  70,  0,  HCLK_DIV1},
    {SCU_RECORDADPCM,           0,  96,  0,  HCLK_DIV1},
    {SCU_RECORDMP3,             0,  48,  0,  HCLK_DIV1},

    {SCU_VIDEOLOWLL,            0,  10, 70,  HCLK_DIV1},
    {SCU_VIDEOLOWL,             0,  20, 80,  HCLK_DIV1},
    {SCU_VIDEOLOW,              0,  120,  0,  HCLK_DIV1},
    {SCU_VIDEOMEDLOW,           0,  120,  100,  HCLK_DIV1},
    {SCU_VIDEOMED,              0,  120,  140,  HCLK_DIV1},
    {SCU_VIDEOMEDHIGH,          0,  132,  140,  HCLK_DIV1},
    {SCU_VIDEOHIGH,             0,  140,  176,  HCLK_DIV1},
    {SCU_VIDEOTVOUT,            0,  FREQ_ARM_MAX,  0,  HCLK_DIV1},

    {SCU_RVLOW,                 0,  100,  80,  HCLK_DIV1},
    {SCU_RVMED,                 0,  120,  120,  HCLK_DIV1},
    {SCU_RVHIGH,                0,  200,  0,  HCLK_DIV1},
    {SCU_BMP,                   0,  100,  0,  HCLK_DIV1},
    {SCU_JPEG,                  0,  100,  150,  HCLK_DIV1},
    {SCU_GIF,                   0,  100,  0,  HCLK_DIV1},
    {SCU_TXT,                   0,  24,  0,  HCLK_DIV1},
    {SCU_FM,                    0,  0,  0,  HCLK_DIV1},
    {SCU_STOPWATCH,             0,  40,  0,  HCLK_DIV1},
    {SCU_GAME,                  0,  60,  0,  HCLK_DIV1},
    {SCU_USB,                   0,  96,  0,  HCLK_DIV1},

#ifdef RGB_PANEL
    {SCU_BLON,                  0,  100,  0,  HCLK_DIV1},  //根据屏大小再做调节
#else
    {SCU_BLON,                  0,  60,  0,  HCLK_DIV1},
#endif

    {SCU_LCD_UPDATE,            0,  20,  0,  HCLK_DIV1}
};
#endif

#endif
