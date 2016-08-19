/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK312X_IO_H
#define __RK312X_IO_H


/*
 * RK3036 IO memory map:
 *
 */
#define RKIO_L2MEM_PHYS         0x10000000

#define RKIO_IMEM_PHYS          0x10080000

#define RKIO_GPU_PHYS           0x10090000

#define RKIO_PMU_PHYS           0x100A0000

#define RKIO_CRYPTO_PHYS        0x100FC000

#define RKIO_ROM_PHYS           0x10100000

#define RKIO_VCODEC_PHYS        0x10104000
#define RKIO_IEP_PHYS           0x10108000
#define RKIO_CIF_PHYS           0x1010A000
#define RKIO_RGA_PHYS           0x1010C000
#define RKIO_LCDC0_PHYS         0x1010E000
#define RKIO_MIPI_CTRL_PHYS     0x10110000

#define RKIO_EBC_PHYS           0x10114000

#define RKIO_CPU_AXI_BUS_PHYS   0x10128000

#define RKIO_GICC_PHYS          0x1013A000

#define RKIO_GICD_PHYS          0x10139000

#define RKIO_USBOTG20_PHYS      0x10180000
#define RKIO_USBHOST_EHCI_PHYS  0x101C0000
#define RKIO_USBHOST_OHCI_PHYS  0x101E0000
#define RKIO_I2S0_8CH_PHYS      0x10200000
#define RKIO_SPDIF_PHYS         0x10204000
#define RKIO_TSP_PHYS           0x10208000
#define RKIO_SFC_PHYS           0x1020C000
#define RKIO_SDMMC_PHYS         0x10214000
#define RKIO_SDIO_PHYS          0x10218000
#define RKIO_EMMC_PHYS          0x1021C000
#define RKIO_I2S1_2CH_PHYS      0x10220000

#define RKIO_AHB_ARB0_PHYS      0x10234000
#define RKIO_AHB_ARB1_PHYS      0x1023C000
#define RKIO_PERI_AXI_BUS_PHYS  0x10300000
#define RKIO_GPS_PHYS           0x10400000
#define RKIO_NANDC_PHYS         0x10500000

#define RKIO_CRU_PHYS           0x20000000
#define RKIO_DDR_PCTL_PHYS      0x20004000
#define RKIO_GRF_PHYS           0x20008000
#define RKIO_DDR_PHY_PHYS       0x2000A000

#define RKIO_CPU_DEBUG_PHYS     0x20020000
#define RKIO_ACODEC_PHYS        0x20030000
#define RKIO_HDMI_PHYS          0x20034000
#define RKIO_MIPI_PHYS          0x20038000

#define RKIO_TIMER_PHYS         0x20044000
#define RKIO_SCR_PHYS           0x20048000
#define RKIO_WDT_PHYS           0x2004C000
#define RKIO_PWM_PHYS           0x20050000
#define RKIO_I2C1_PHYS          0x20056000
#define RKIO_I2C2_PHYS          0x2005A000
#define RKIO_I2C3_PHYS          0x2005E000
#define RKIO_UART0_PHYS         0x20060000
#define RKIO_UART1_PHYS         0x20064000
#define RKIO_UART2_PHYS         0x20068000
#define RKIO_SARADC_PHYS        0x2006C000
#define RKIO_I2C0_PHYS          0x20072000
#define RKIO_SPI_PHYS           0x20074000
#define RKIO_DMAC_PHYS          0x20078000
#define RKIO_GPIO0_PHYS         0x2007C000
#define RKIO_GPIO1_PHYS         0x20080000
#define RKIO_GPIO2_PHYS         0x20084000
#define RKIO_GPIO3_PHYS         0x20088000
#define RKIO_GMAC_PHYS          0x2008C000
#define RKIO_EFUSE_PHYS         0x20090000


/* define for getting chip version */
#define RKIO_ROM_CHIP_VER_ADDR		(RKIO_ROM_PHYS + 0x3FF0)
#define RKIO_ROM_CHIP_VER_SIZE		16

/* define for pwm configuration */
#define RKIO_PWM0_PHYS                  (RKIO_PWM_PHYS + 0x00)
#define RKIO_PWM1_PHYS                  (RKIO_PWM_PHYS + 0x10)
#define RKIO_PWM2_PHYS                  (RKIO_PWM_PHYS + 0x20)
#define RKIO_PWM3_PHYS                  (RKIO_PWM_PHYS + 0x30)

#endif /* __RK312X_IO_H */
