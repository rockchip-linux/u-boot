/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef __RK3036_IO_H
#define __RK3036_IO_H


/*
 * RK3036 IO memory map:
 *
 */
#define RKIO_L2MEM_PHYS         0x10000000
#define RKIO_L2MEM_SIZE         SZ_128K

#define RKIO_IMEM_PHYS          0x10080000
#define RKIO_IMEM_SIZE          SZ_8K

#define RKIO_GPU_PHYS           0x10090000
#define RKIO_GPU_SIZE           SZ_64K

#define RKIO_ROM_PHYS           0x10100000
#define RKIO_ROM_SIZE           SZ_16K

#define RKIO_VCODEC_PHYS        0x10108000
#define RKIO_VCODEC_SIZE        SZ_16K
#define RKIO_HEVC_PHYS          0x1010C000
#define RKIO_HEVC_SIZE          SZ_16K

#define RKIO_VOP_PHYS           0x10118000
#define RKIO_VOP_SIZE           SZ_16K

#define RKIO_CPU_AXI_BUS_PHYS   0x10128000
#define RKIO_CPU_AXI_BUS_SIZE   SZ_32K

#define RKIO_GICC_PHYS          0x1013A000
#define RKIO_GICC_SIZE          SZ_4K

#define RKIO_GICD_PHYS          0x10139000
#define RKIO_GICD_SIZE          SZ_4K

#define RKIO_USBOTG20_PHYS      0x10180000
#define RKIO_USBOTG20_SIZE      SZ_256K
#define RKIO_USBHOST20_PHYS     0x101c0000
#define RKIO_USBHOST20_SIZE     SZ_256K
#define RKIO_EMAC_PHYS          0x10200000
#define RKIO_EMAC_SIZE          SZ_16K
#define RKIO_SPDIF_PHYS         0x10204000
#define RKIO_SPDIF_SIZE         SZ_16K
#define RKIO_SFC_PHYS           0x10208000
#define RKIO_SFC_SIZE           SZ_16K

#define RKIO_SDMMC_PHYS         0x10214000
#define RKIO_SDMMC_SIZE         SZ_16K
#define RKIO_SDIO_PHYS          0x10218000
#define RKIO_SDIO_SIZE          SZ_16K
#define RKIO_EMMC_PHYS          0x1021C000
#define RKIO_EMMC_SIZE          SZ_16K
#define RKIO_I2S_PHYS           0x10220000
#define RKIO_I2S_SIZE           SZ_16K

#define RKIO_AHB_ARB0_PHYS      0x10234000
#define RKIO_AHB_ARB0_SIZE      SZ_32K
#define RKIO_AHB_ARB1_PHYS      0x1023C000
#define RKIO_AHB_ARB1_SIZE      SZ_784K
#define RKIO_PERI_AXI_BUS_PHYS  0x10300000
#define RKIO_PERI_AXI_BUS_SIZE  SZ_1M

#define RKIO_NANDC_PHYS         0x10500000
#define RKIO_NANDC_SIZE         SZ_16K

#define RKIO_CRU_PHYS           0x20000000
#define RKIO_CRU_SIZE           SZ_16K
#define RKIO_DDR_PCTL_PHYS      0x20004000
#define RKIO_DDR_PCTL_SIZE      SZ_16K
#define RKIO_GRF_PHYS           0x20008000
#define RKIO_GRF_SIZE           SZ_8K
#define RKIO_DDR_PHY_PHYS       0x2000A000
#define RKIO_DDR_PHY_SIZE       SZ_24K

#define RKIO_CPU_DEBUG_PHYS     0x20020000
#define RKIO_CPU_DEBUG_SIZE     SZ_64K
#define RKIO_ACODEC_PHYS        0x20030000
#define RKIO_ACODEC_SIZE        SZ_16K
#define RKIO_HDMI_PHYS          0x20034000
#define RKIO_HDMI_SIZE          SZ_16K

#define RKIO_TIMER_PHYS         0x20044000
#define RKIO_TIMER_SIZE         SZ_16K

#define RKIO_WDT_PHYS           0x2004C000
#define RKIO_WDT_SIZE           SZ_16K
#define RKIO_PWM_PHYS           0x20050000
#define RKIO_PWM_SIZE           SZ_24K
#define RKIO_I2C1_PHYS          0x20056000
#define RKIO_I2C1_SIZE          SZ_16K
#define RKIO_I2C2_PHYS          0x2005A000
#define RKIO_I2C2_SIZE          SZ_8K

#define RKIO_UART0_PHYS         0x20060000
#define RKIO_UART0_SIZE         SZ_16K
#define RKIO_UART1_PHYS         0x20064000
#define RKIO_UART1_SIZE         SZ_16K
#define RKIO_UART2_PHYS         0x20068000
#define RKIO_UART2_SIZE         SZ_16K

#define RKIO_I2C0_PHYS          0x20072000
#define RKIO_I2C0_SIZE          SZ_8K
#define RKIO_SPI_PHYS           0x20074000
#define RKIO_SPI_SIZE           SZ_16K
#define RKIO_DMAC_PHYS          0x20078000
#define RKIO_DMAC_SIZE          SZ_16K
#define RKIO_GPIO0_PHYS         0x2007C000
#define RKIO_GPIO0_SIZE         SZ_16K
#define RKIO_GPIO1_PHYS         0x20080000
#define RKIO_GPIO1_SIZE         SZ_16K
#define RKIO_GPIO2_PHYS         0x20084000
#define RKIO_GPIO2_SIZE         SZ_16K

#define RKIO_EFUSE_PHYS         0x20090000
#define RKIO_EFUSE_SIZE         SZ_16K


/* define for getting chip version */
#define RKIO_ROM_CHIP_VER_ADDR		(RKIO_ROM_PHYS + 0x3FF0)
#define RKIO_ROM_CHIP_VER_SIZE		16


#endif /* __RK3036_IO_H */

