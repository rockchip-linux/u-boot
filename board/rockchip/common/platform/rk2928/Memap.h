/******************************************************************/
/*   Copyright (C) 2001 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File    : hw_memmap.h
Desc    : memory map address/device base address
Author  : yk
Date    : 2012-04-20
Notes   : RK2928用头文件, 按地址顺序定义
        : 结尾是ADDR的说明与原来定义兼容, 结尾是BASE的说明是新增定义
*********************************************************************/
#ifndef _RK2928_MEMMAP_H
#define _RK2928_MEMMAP_H

#define     IMEM_BASE_ADDR          0x10080000
#define     GPU_BASE_ADDR           0x10090000
#define     BOOT_ROM_ADDR           0x10100000
#define     BOOT_ROM_CHIP_VER_ADDR  (BOOT_ROM_ADDR+0x27F0)
#define     VCODEC_BASE_ADDR        0x10104000
#define     CIF_BASE_ADDR           0x1010A000
#define     RGA_BASE_ADDR           0x1010C000
#define     LCDC_BASE_ADDR          0x1010E000
#define     CPU_AXI_BUS_BASE        0x10128000
#define     L2C_BASE_ADDR           0x10138000
#define     L2C_BASE                L2C_BASE_ADDR
#define     CORE_SCU_BASE           0x1013c000
#define     CORE_GICC_BASE          0x1013C100
#define     CORE_GTIMER_BASE        0x1013C200
#define     CORE_TIMER_BASE         0x1013C600
#define     CORE_GICD_BASE          0x1013D000
#define     GIC_CPU_BASE_ADDR       CORE_GICC_BASE
#define     GIC_PERI_BASE_ADDR      CORE_GICD_BASE

#define     USB_OTG0_BASE_ADDR       0x10180000
#define     USB_OTG_BASE_ADDR        USB_OTG0_BASE_ADDR
#define     USB_OTG_BASE_ADDR_VA     USB_OTG_BASE_ADDR
#define     USB_HOST_BASE_ADDR      0x101C0000 
#define     SDMMC0_BASE_ADDR        0x10214000
#define     SDMMC1_BASE_ADDR        0x10218000
#define     EMMC_BASE_ADDR          0x1021C000
#define     I2S_BASE_ADDR           0x10220000
#define     ARBITER0_BASE_ADDR      0x10234000
#define     ARBITER1_BASE_ADDR      0x1023c000
#define     PERI_AXI_BUS_BASE       0x10300000
#define     GPS_BASE_ADDR           0x10400000
#define     NANDC_BASE_ADDR         0x10500000

#define     CRU_BASE_ADDR           0x20000000
#define     DDR_PCTL_BASE           0x20004000
#define     GRF_BASE                0x20008000           
#define     REG_FILE_BASE_ADDR      GRF_BASE
#define     DDR_PHY_BASE            0x2000a000
#define     DBG_BASE_ADDR           0x20020000
#define     ACODEC_ANA_BASE         0x20030000
#define     HDMI_ANA_BASE           0x20034000

#define     TIMER0_BASE_ADDR        0x20044000
#define     TIMER1_BASE_ADDR        0x20046000
#define     WDT_BASE_ADDR           0x2004c000
#define     PWM0_BASE_ADDR          0x20050000
#define     I2C1_BASE_ADDR          0x20054000
#define     I2C2_BASE_ADDR          0x20058000
#define     I2C3_BASE_ADDR          0x2005c000
#define     UART0_BASE_ADDR         0x20060000
#define     UART1_BASE_ADDR         0x20064000
#define     UART2_BASE_ADDR         0x20068000
#define     SARADC_BASE_ADDR        0x2006c000
#define     SARADC_BASE         SARADC_BASE_ADDR
#define     I2C0_BASE_ADDR          0x20070000
#define     SPI0_BASE_ADDR           0x20074000
#define     SPI_MASTER_BASE_ADDR    SPI0_BASE_ADDR
#define     SPI_BASE_ADDR           SPI0_BASE_ADDR
#define     DMAC_BASE_ADDR          0x20078000
#define     GPIO0_BASE_ADDR         0x2007c000
#define     GPIO1_BASE_ADDR         0x20080000
#define     GPIO2_BASE_ADDR         0X20084000
#define     GPIO3_BASE_ADDR         0x20088000
#define     EFUSE_BASE_ADDR         0x20090000

#define     SDRAM_ADDR              0x60000000
#endif
/*********************************************************************
 END OF FILE
*********************************************************************/
