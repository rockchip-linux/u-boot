/******************************************************************/
/*   Copyright (C) 2008 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File    :  grf.h
Desc    :  定义通用寄存器结构体\寄存器位的宏定义\接口函数

Author  : yangkai
Date    : 2009-01-07
Modified:
Revision:           1.00
*********************************************************************/
#ifdef DRIVERS_GRF
#ifndef _DRIVER_GRF_H_
#define _DRIVER_GRF_H_

/********************************************************************
**                            宏定义                                *
********************************************************************/
//cpu apb reg0
#define APBREG0_UTMI_LINESTATE0             (1<<0)
#define APBREG0_UTMI_LINESTATE1             (1<<1)
#define APBREG0_SPIM_SLEEP_STAT             (1<<2)
#define APBREG0_SPIS_SLEEP_STAT             (1<<3)
#define APBREG0_TIMER0_EN_STAT              (1<<4)
#define APBREG0_TIMER1_EN_STAT              (1<<5)
#define APBREG0_TIMER2_EN_STAT              (1<<6)
#define APBREG0_APLL_LOCK_STAT              (1<<7)
#define APBREG0_DPLL_LOCK_STAT              (1<<8)
#define APBREG0_CPLL_LOCK_STAT              (1<<9)
#define APBREG0_CX_OCM_CORE_RST_STAT        (1<<7)

//CPU APB REG4
#define APBREG4_SDRAMC_PRIO(pri)            (pri)
#define APBREG4_SDRAMC_SDR                  (0<<15)
#define APBREG4_SDRAMC_MOBILE               (0<<15)
#define APBREG4_NOR_DATA_WID_8              (4<<16)
#define APBREG4_NOR_DATA_WID_16             (0<<16)
#define APBREG4_SDRAM_PD                    (1<<19)
#define APBREG4_SMEM_PD                     (1<<20)
#define APBREG4_EXIT_SELFREF                (1<<21)
#define APBREG4_SDRAM_READ_PIPE             (1<<22)
#define APBREG4_DEMOD_LDPC_OD_REV           (1<<23)
#define APBREG4_SDRAM_IOVOL18_EN            (1<<24)
#define APBREG4_LCDC_BYPASS                 (1<<25)
#define APBREG4_HOST_IF_DATA_16BIT          (1<<26)
#define APBREG4_HOST_IF_EN                  (1<<27)
#define APBREG4_ARBITER_ARM_PAUSE           (1<<28)
#define APBREG4_ARBITER_EXP_PAUSE           (1<<29)
#define APBREG4_SPIM_IF_TYPE                (1<<30)
#define APBREG4_DMAREQ4_LCDC                (0x01u<<31)
#define APBREG4_DMAREQ4_SDMMC1              (0x00u<<31)

//CPU APB REG5
#define APBREG5_REMAP                       (1<<0)
#define APBREG5_FIQ_SWI                     (1<<1)
#define APBREG5_DSP_WAKE_UP                 (1<<2)
#define APBREG5_DSP_EXT_WAIT                (1<<3)
#define APBREG5_DSP_BOOT                    (1<<4)
#define APBREG5_DSP_NMI                     (1<<5)
#define APBREG5_LCDC_IO_TRI                 (1<<6)
#define APBREG5_VIP_VSYNC_HIGH              (1<<7)
#define APBREG5_VIP_VSYNC_LOW               (0<<7)
#define APBREG5_SHMEM0_DEMOD                (0<<8)
#define APBREG5_SHMEM0_DSPL2                (1<<8)
#define APBREG5_SHMEM0_CPUL2                (2<<8)
#define APBREG5_SHMEM1_DEMOD                (0<<10)
#define APBREG5_SHMEM1_DSPL2                (1<<10)
#define APBREG5_SHMEM1_CPUL2                (2<<10)
#define APBREG5_ARMTCM_WAIT                 (1<<12)

//IOMUX_A_CON
#define  IOMUXA_I2C0                        (0<<30)
#define  IOMUXA_GPIO1_A45                   (1<<30)
#define  IOMUXA_GPIO1_A67                   (0<<28)
#define  IOMUXA_UART1_SIR                   (1<<28)
#define  IOMUXA_I2C1                        (2<<28)
#define  IOMUXA_GPIO1_B1                    (0<<26)
#define  IOMUXA_UART1_SOUT                  (1<<26)
#define  IOMUXA_CX_TIMER1_PMW               (2<<26)
#define  IOMUXA_GPIO1_B0                    (0<<24)
#define  IOMUXA_UART1_SIN                   (1<<24)
#define  IOMUXA_CX_TIMER0_PWM               (2<<24)
#define  IOMUXA_GPIO1_C237                  (0<<23)
#define  IOMUXA_SDMMC1_CMD_DATA0_CLKOUT     (1<<23)
#define  IOMUXA_GPIO1_C456                  (0<<22)
#define  IOMUXA_SDMMC1_DATA123              (1<<22)
#define  IOMUXA_GPIO1_A1237                 (0<<21)
#define  IOMUXA_SPI1                        (1<<21)
#define  IOMUXA_GPIO0_B0                    (0<<16)
#define  IOMUXA_SPI0_CSN1                   (1<<16)
#define  IOMUXA_SDMMC1_PWR_EN               (2<<16)
#define  IOMUXA_GPIO1_C1                    (0<<14)
#define  IOMUXA_UART0_SOUT                  (1<<14)
#define  IOMUXA_SDMMC1_WRITE_PRT            (2<<14)
#define  IOMUXA_GPIO1_C0                    (0<<12)
#define  IOMUXA_UART0_SIN                   (1<<12)
#define  IOMUXA_SDMMC1_DETECT_N             (2<<12)
#define  IOMUXA_GPIO1_B4                    (0<<10)
#define  IOMUXA_PWM2                        (1<<10)
#define  IOMUXA_SDMMC0_WRITE_PRT            (2<<10)
#define  IOMUXA_GPIO1_B3                    (0<<8)
#define  IOMUXA_PWM1                        (1<<8)
#define  IOMUXA_SDMMC0_DETECT_N             (2<<8)
#define  IOMUXA_GPIO0_B1                    (0<<6)
#define  IOMUXA_SM_CS1_N                    (1<<6)
#define  IOMUXA_SDMMC0_PWR_EN               (2<<6)
#define  IOMUXA_GPIO1_D234                  (0<<5)
#define  IOMUXA_SDMM0_DATA123               (1<<5)
#define  IOMUXA_GPIO1_D015                  (0<<4)
#define  IOMUXA_SDMMC0_CMD_DATA0_CLKOUT     (1<<4)
#define  IOMUXA_GPIO0_B567                  (0<<2)
#define  IOMUXA_SPI0                        (1<<2)
#define  IOMUXA_SDMMC0_DATA567              (2<<2)
#define  IOMUXA_GPIO0_B4                    (0<<0)
#define  IOMUXA_SPI0_CSN0                   (1<<0)
#define  IOMUXA_SDMMC0_DATA4                (2<<0)

///IOMUX_B_CON
#define  IOMUXB_GPIO2_24                    (0<<20)
#define  IOMUXB_GPS_CLK                     (1<<20)
#define  IOMUXB_HSADC_CLKOUT                (2<<20)
#define  IOMUXB_HSADC_DATA_I98              (0<<19)
#define  IOMUXB_TS_FAIL_TS_VALID            (1<<19)
#define  IOMUXB_GPIO0_A7                    (0<<18)
#define  IOMUXB_FLASH_CS3                   (1<<18)
#define  IOMUXB_GPIO0_A6                    (0<<17)
#define  IOMUXB_FLASH_CS2                   (1<<17)
#define  IOMUXB_GPIO0_A5                    (0<<16)
#define  IOMUXB_FLASH_CS1                   (1<<16)
#define  IOMUXB_GPIO1_B5                    (0<<14)
#define  IOMUXB_PWM3                        (1<<14)
#define  IOMUXB_DEMOD_PWM_OUT               (2<<14)
#define  IOMUXB_GPIO0_B3                    (0<<13)
#define  IOMUXB_UART0_RTS_N                 (1<<13)
#define  IOMUXB_GPIO0_B2                    (0<<12)
#define  IOMUXB_UART0_CTS_N                 (1<<12)
#define  IOMUXB_GPIO1_B2                    (0<<11)
#define  IOMUXB_PWM0                        (1<<11)
#define  IOMUXB_GPIO0_D0_7                  (0<<10)
#define  IOMUXB_LCDC_DATA8_15               (1<<10)
#define  IOMUXB_GPIO0_C2_7                  (0<<9)
#define  IOMUXB_LCDC_DATA18_23              (1<<9)
#define  IOMUXB_GPIO0_C01                   (0<<8)
#define  IOMUXB_LCDC_DATA16_17              (1<<8)
#define  IOMUXB_GPIO2_26                    (0<<7)
#define  IOMUXB_LCDC_DENABLE                (1<<7)
#define  IOMUXB_GPIO2_25                    (0<<6)
#define  IOMUXB_LCDC_VSYNC                  (1<<6)
#define  IOMUXB_GPIO2_14_23                 (0<<5)
#define  IOMUXB_HSADC_DATA9_0               (1<<5)
#define  IOMUXB_GPIO2_0_13                  (0<<4)
#define  IOMUXB_HOST_INTERFACE              (1<<4)
#define  IOMUXB_GPIO1_D7                    (0<<3)
#define  IOMUXB_HSADC_CLKIN                 (1<<3)
#define  IOMUXB_GPIO1_D6                    (0<<2)
#define  IOMUXB_EXT_IQ_INDEX                (1<<2)
#define  IOMUXB_I2S_INTERFACE               (0<<1)
#define  IOMUXB_GPIO2_27_31                 (1<<1)
#define  IOMUXB_GPIO1_B6                    (0<<0)
#define  IOMUXB_VIP_CLKOUT                  (1<<0)

//GPIO0_AB_PU_CON;GPIO0_CD_PU_CON;GPIO1_AB_PU_CON;GPIO1_CD_PU_CON
#define  GPIO_NORMAL                        (0)
#define  GPIO_PULLDOWN                      (1)
#define  GPIO_PULLUP                        (2)

/********************************************************************
**                          结构定义                                *
********************************************************************/
//GRF Registers
typedef volatile struct tagGRF_REG
{
    uint32  CPU_APB_REG0;
    uint32  CPU_APB_REG1;
    uint32  CPU_APB_REG2;
    uint32  CPU_APB_REG3;
    uint32  CPU_APB_REG4;
    uint32  CPU_APB_REG5;
    uint32  CPU_APB_REG6;
    uint32  CPU_APB_REG7;
    uint32  IOMUX_A_CON;
    uint32  IOMUX_B_CON;
    uint32  GPIO0_AB_PU_CON;
    uint32  GPIO0_CD_PU_CON;
    uint32  GPIO1_AB_PU_CON;
    uint32  GPIO1_CD_PU_CON;
    uint32  OTGPHY_CON0;
    uint32  OTGPHY_CON1;
}GRF_REG, *pGRF_REG,*pAPB_REG;

/********************************************************************
**                          变量定义                                *
********************************************************************/
#undef EXT
#ifdef IN_GRF
    #define EXT 
#else
    #define EXT extern
#endif
EXT pGRF_REG g_grfReg;

/********************************************************************
**                          结构定义                                *
********************************************************************/
typedef enum
{
    IOMUX_I2C0 = 0,
    IOMUX_I2C0_GPIO
}eIOMUX_I2C0;

typedef enum
{
    IOMUX_I2C1_GPIO = 0,
    IOMUX_I2C1_UART1,
    IOMUX_I2C1
}eIOMUX_I2C1;

typedef enum
{
    IOMUX_TIMER1PWM_GPIO = 0,
    IOMUX_TIMER1PWM_UART1,
    IOMUX_TIMER1PWM
}eIOMUX_TIMER1PWM;

typedef enum
{
    IOMUX_TIMER0PWM_GPIO = 0,
    IOMUX_TIMER0PWM_UART1,
    IOMUX_TIMER0PWM
}eIOMUX_TIMER0PWM;
//UART1
typedef enum
{
    IOMUX_UART1 = 0,
    IOMUX_UART1_SIR,
    IOMUX_UART1_OTHER
}eIOMUX_UART1;

typedef enum
{
    IOMUX_SPI1_GPIO = 0,
    IOMUX_SPI1
}eIOMUX_SPI1;

typedef enum
{
    IOMUX_SPI0_GPIO = 0,
    IOMUX_SPI0_CSN0,
    IOMUX_SPI0_CSN1,
    IOMUX_SPI0_SDMMC0,
    IOMUX_SPI0_SDMMC1
}eIOMUX_SPI0;

typedef enum
{
    IOMUX_UART0_GPIO = 0,
    IOMUX_UART0,
    IOMUX_UART0_MODEM,
    IOMUX_UART0_SDMMC1
}eIOMUX_UART0;
//SDMMC1
typedef enum
{
    IOMUX_SDMMC1 = 0,
    IOMUX_SDMMC1_OTHER
}eIOMUX_SDMMC1;

typedef enum
{
    IOMUX_PWM2_GPIO = 0,
    IOMUX_PWM2,
    IOMUX_PWM2_SDMMC0
}eIOMUX_PWM2;

typedef enum
{
    IOMUX_PWM1_GPIO = 0,
    IOMUX_PWM1,
    IOMUX_PWM1_SDMMC0
}eIOMUX_PWM1;

typedef enum
{
    IOMUX_SMCS1_GPIO = 0,
    IOMUX_SMCS1,
    IOMUX_SMCS1_SDMMC0
}eIOMUX_SMCS1;
//SDMMC0
typedef enum
{
    IOMUX_SDMMC0 = 0,
    IOMUX_SDMMC0_OTHER
}eIOMUX_SDMMC0;
//ATTENTION TO SPI0

typedef enum
{
    IOMUX_GPS_GPIO = 0,
    IOMUX_GPS,
    IOMUX_GPS_HSADC
}eIOMUX_GPS;

typedef enum
{
    IOMUX_TS_HSADC = 0,
    IOMUX_TS
}eIOMUX_TS;

typedef enum
{
    IOMUX_FLASH_GPIO = 0,
    IOMUX_FLASH
}eIOMUX_FLASH;

typedef enum
{
    IOMUX_PWM3_GPIO = 0,
    IOMUX_PWM3_DEMOD,
    IOMUX_PWM3
}eIOMUX_PWM3;

typedef enum
{
    IOMUX_PWM0_GPIO = 0,
    IOMUX_PWM0
}eIOMUX_PWM0;

typedef enum
{
    IOMUX_LCDC_GPIO = 0,
    IOMUX_LCDC
}eIOMUX_LCDC;
//HSADC
typedef enum
{
    IOMUX_HSADC = 0,
    IOMUX_HSADC_OTHER
}eIOMUX_HSADC;

typedef enum
{
    IOMUX_HOST_GPIO = 0,
    IOMUX_HOST
}eIOMUX_HOST;

typedef enum
{
    IOMUX_EXTIQ_GPIO = 0,
    IOMUX_EXTIQ
}eIOMUX_EXTIQ;

typedef enum
{
    IOMUX_I2S = 0,
    IOMUX_I2S_GPIO
}eIOMUX_I2S;

typedef enum
{
    IOMUX_VIP_GPIO = 0,
    IOMUX_VIP
}eIOMUX_VIP;

typedef enum
{
    FLASH_CS1 = 0,
    FLASH_CS2,
    FLASH_CS3
}eFLASH_CS;
/********************************************************************
**                          变量定义                                *
********************************************************************/
#undef EXT
#ifdef IN_PMU
    #define EXT
#else
    #define EXT extern
#endif
        
/********************************************************************
**                          函数声明                                *
********************************************************************/
extern void GRFInit(void);
extern void IOMUXSetSPI0(eIOMUX_SPI0 type);


#endif
#endif
