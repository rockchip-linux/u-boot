/******************************************************************/
/*   Copyright (C) 2007 ROCK-CHIPS FUZHOU . All Rights Reserved.  */
/*******************************************************************
File :  Memap.h
Desc :  memory map address/device base address

Author : huangxinyu
Date : 2007-05-30
Notes :



*********************************************************************/
#ifndef _RK30_MEMMAP_H
#define _RK30_MEMMAP_H

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3026)
#define     IMEM_BASE_ADDR          0x10080000
#define     GPU_BASE_ADDR           0x10090000
#define     BOOT_ROM_ADDR           0x10100000
#define     BOOT_ROM_CHIP_VER_ADDR  (BOOT_ROM_ADDR+0x27F0)
#define     VCODEC_BASE_ADDR        0x10104000
#define     CIF_BASE_ADDR           0x1010A000
#define     RGA_BASE_ADDR           0x1010C000
#define     LCDC_BASE_ADDR          0x1010E000
#define     LCDC0_BASE              LCDC_BASE_ADDR
#define      EBC_BASE_ADDR          0x10114000
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

#else
#define     L2MEM_BASE              0x10000000
#define     BOOT_ROM_ADDR           0x10100000
#define     BOOT_ROM_CHIP_VER_ADDR  (BOOT_ROM_ADDR+0x27F0)
#define     VCODEC_BASE_ADDR        0x10104000
#define     CIF0_BASE               0x10108000
#define     CIF1_BASE               0x1010A000
#define     VIP_BASE_ADDR           CIF0_BASE
#define     LCDC0_BASE              0x1010C000
#define     LCDC1_BASE              0x1010E000
#define     LCDC_BASE_ADDR          LCDC0_BASE
#define     IPP_BASE_ADDR           0x10110000
#define     RGA_BASE                0x10114000
#define     HDMI_TX_BASE            0x10116000
#define     I2S_BASE_ADDR_8CH       0x10118000
#define     I2S_BASE_ADDR_2CH       0x1011A000
#define     I2S2_2CH_BASE           0x1011C000
#define     SPDIF_BASE_ADDR         0x1011E000
#define     UART0_BASE_ADDR         0x10124000	//0x20030000
#define     UART1_BASE_ADDR         0x10126000	//0x20060000
#define     CPU_AXI_BUS_BASE        0x10128000
#define     INTMEM_TZMA_ADDR        0x10080000
#define     GPU_BASE_ADDR           0x10090000
#define     L2C_BASE                0x10138000
#define     CORE_SCU_BASE           0x1013C000
#define     CORE_GICC_BASE          0x1013C100	//0x1012C000
#define     CORE_GTIMER_BASE        0x1013C200
#define     CORE_TIMER_BASE         0x1013C600
#define     CORE_GICD_BASE          0x1013D000	//0x1012E000
#define     GIC_CPU_BASE_ADDR       CORE_GICC_BASE
#define     GIC_PERI_BASE_ADDR      CORE_GICD_BASE
#define     USB_OTG0_BASE_ADDR      0x10180000
#define     USB_OTG_BASE_ADDR       USB_OTG0_BASE_ADDR
#define     USB_OTG_BASE_ADDR_VA       USB_OTG_BASE_ADDR
#define     USB_OTG1_BASE_ADDR      0x101C0000
#define     MAC_BASE_ADDR           0x10204000
#define     GPS_BASE                0x1020C000
#define     HSADC_BASE_ADDR         0x10210000
#define     SDMMC0_BASE_ADDR        0x10214000
#define     SDMMC1_BASE_ADDR        0x10218000
#define     EMMC_BASE_ADDR          0x1021C000
#define     PIDF_BASE_ADDR          0x10220000
#define     ARBITER0_BASE_ADDR      0x10224000
#define     ARBITER1_BASE_ADDR      0x10228000
#define     PERI_AXI_BUS_BASE       0x10300000
#define     PERI_AXI_BUS0_BASE_ADDR PERI_AXI_BUS_BASE
#define     NANDC_BASE_ADDR         0x10500000
#define     SMC0_ADDR               0x11000000
#define     SMC1_ADDR               0x12000000
#define     DEBUG_BASE_ADDR         0x1FFE0000
#define     CRU_BASE_ADDR           0x20000000
#define     PMU_BASE_ADDR           0x20004000
#define     GRF_BASE                0x20008000
#define     REG_FILE_BASE_ADDR      GRF_BASE
#define     GPIO6_BASE_ADDR         0x2000A000
#define     TIMER3_BASE_ADDR        0x2000E000
#define     eFUSE_BASE_ADDR         0x20010000
#define     TZPC_BASE_ADDR          0x20014000
#define     DMACS1_BASE             0x20018000
#define     DMAC1_BASE              0x2001C000
#define     DDR_PCTL_BASE           0x20020000
#define     SDRAMC_BASE_ADDR        DDR_PCTL_BASE
#define     I2C0_BASE_ADDR          0x2002C000
#define     I2C1_BASE_ADDR          0x2002E000
#define     PWM01_BASE_ADDR         0x20030000
#define     PWM_BASE_ADDR           PWM01_BASE_ADDR
#define     GPIO0_BASE_ADDR         0x20034000
#define     TIMER0_BASE_ADDR        0x20038000
#define     TIMER1_BASE_ADDR        0x2003A000
#define     GPIO1_BASE_ADDR         0x2003C000
#define     GPIO2_BASE_ADDR         0x2003E000
#define     DDR_PUBL_BASE           0x20040000
#define     WDT_BASE_ADDR           0x2004C000
#define     PWM23_BASE_ADDR         0x20050000
#define     I2C2_BASE_ADDR          0x20054000
#define     I2C3_BASE_ADDR          0x20058000
#define     I2C4_BASE               0x2005C000
#define     TSADC_BASE              0x20060000
#define     UART2_BASE_ADDR         0x20064000
#define     UART3_BASE_ADDR         0x20068000
#define     SARADC_BASE             0x2006C000
#define     ADC_BASE_ADDR           SARADC_BASE
#define     SPI0_BASE_ADDR          0x20070000
#define     SPI_MASTER_BASE_ADDR    SPI0_BASE_ADDR
#define     SPI_BASE_ADDR           SPI0_BASE_ADDR
#define     SPI1_BASE_ADDR          0x20074000
#define     DMA2_BASE_ADDR          0x20078000
#define     SMC_BASE_ADDR           0x2007C000
#define     GPIO3_BASE_ADDR         0x20080000
#define     GPIO4_BASE_ADDR         0x20084000
#define     GPIO5_BASE_ADDR         0x20088000
#define     SDRAM_ADDR              0x60000000
#endif




#endif /* _HW_MEMMAP_H */



/*********************************************************************
 END OF FILE
*********************************************************************/
