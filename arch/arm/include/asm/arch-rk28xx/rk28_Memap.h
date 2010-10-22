/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:      Memap.h
Author:         RK28XX Driver Develop Group
Created:        1st Dec 2008
Modified:
Revision:       1.00
********************************************************************************
********************************************************************************/
#ifndef     _DRIVER_MEMAP_H
#define     _DRIVER_MEMAP_H

#define     ITCM_ADDR               0x00000000
#define     DTCM_ADDR               0x00004000
//AHB IP
#define     BOOT_ROM_ADDR           0x10000000
#define     SRAM_BASE_ADDR          0x10002000
#define     USB_OTG_BASE_ADDR       0x10040000
#define     SHARE_MEM0_ADDR         0x10080000
#define     SHARE_MEM1_ADDR         0x10090000
#define     DW_DMA_BASE_ADDR        0x100A0000
#define     HOST_IF_ADDR            0x100A2000
#define     LCDC_BASE_ADDR          0x100A4000
#define     VIP_BASE_ADDR           0x100A6000
#define     SDMMC1_BASE_ADDR        0x100A8000
#define     INTC_BASE_ADDR          0x100AA000
#define     SDMMC0_BASE_ADDR        0x100AC000
#define     NANDC_BASE_ADDR         0x100AE000
#define     SDRAMC_BASE_ADDR        0x100B0000
#define     ARMD_ARBITER_BASE_ADDR  0x100B4000
#define     VIDEO_COP_BASE_ADDR     0x100BA000

//APB IP
#define     UART0_BASE_ADDR         0x18000000
#define     UART1_BASE_ADDR         0x18002000
#define     TIMER_BASE_ADDR         0x18004000
#define     eFUSE_BASE_ADDR         0x18006000
#define     GPIO0_BASE_ADDR         0x18008000
#define     GPIO1_BASE_ADDR         0x18009000
#define     I2S_BASE_ADDR           0x1800A000
#define     I2C0_BASE_ADDR          0x1800C000
#define     I2C1_BASE_ADDR          0x1800D000
#define     SPI_MASTER_BASE_ADDR    0x1800E000
#define     SPI_SLAVE_BASE_ADDR     0x1800F000
#define     WDT_BASE_ADDR           0x18010000
#define     PWM_BASE_ADDR           0x18012000
#define     RTC_BASE_ADDR           0x18014000
#define     ADC_BASE_ADDR           0x18016000
#define     SCU_BASE_ADDR           0x18018000
#define     REG_FILE_BASE_ADDR      0x18019000

//MEM
#define     NORFLASH0_ADDR          0x50000000
#define     NORFLASH1_ADDR          0x51000000
#define     SDRAM_ADDR              0x60000000
#define     DSP_BASE_ADDR           0x80000000
#endif

