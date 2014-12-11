/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3368_IO_H
#define __RK3368_IO_H


/*
 * RK3368 IO memory map:
 *
 */
#define RKIO_SFC_PHYS                   0xFF000000
#define RKIO_SFC_SIZE                   SZ_512K

#define RKIO_SDMMC_PHYS                 0xFF0C0000
#define RKIO_SDMMC_SIZE                 SZ_64K
#define RKIO_SDIO_PHYS                  0xFF0D0000
#define RKIO_SDIO_SIZE                  SZ_64K
#define RKIO_HSADC_PHYS                 0xFF0E0000
#define RKIO_HSADC_SIZE                 SZ_64K
#define RKIO_EMMC_PHYS                  0xFF0F0000
#define RKIO_EMMC_SIZE                  SZ_64K
#define RKIO_SARADC_PHYS                0xFF100000
#define RKIO_SARADC_SIZE                SZ_64K
#define RKIO_SPI0_PHYS                  0xFF110000
#define RKIO_SPI0_SIZE                  SZ_64K
#define RKIO_SPI1_PHYS                  0xFF120000
#define RKIO_SPI1_SIZE                  SZ_64K
#define RKIO_SPI2_PHYS                  0xFF130000
#define RKIO_SPI2_SIZE                  SZ_64K
#define RKIO_I2C2_SENSOR_PHYS           0xFF140000
#define RKIO_I2C2_SENSOR_SIZE           SZ_64K
#define RKIO_I2C3_CAM_PHYS              0xFF150000
#define RKIO_I2C3_CAM_SIZE              SZ_64K
#define RKIO_I2C4_TP_PHYS               0xFF160000
#define RKIO_I2C4_TP_SIZE               SZ_64K
#define RKIO_I2C5_HDMI_PHYS             0xFF170000
#define RKIO_I2C5_HDMI_SIZE             SZ_64K
#define RKIO_UART0_BT_PHYS              0xFF180000
#define RKIO_UART0_BT_SIZE              SZ_64K
#define RKIO_UART1_BB_PHYS              0xFF190000
#define RKIO_UART1_BB_SIZE              SZ_64K

#define RKIO_UART3_GPS_PHYS             0xFF1B0000
#define RKIO_UART3_GPS_SIZE             SZ_64K
#define RKIO_UART4_EXP_PHYS             0xFF1C0000
#define RKIO_UART4_EXP_SIZE             SZ_64K
#define RKIO_SCR_PHYS                   0xFF1D0000
#define RKIO_SCR_SIZE                   SZ_64K

#define RKIO_DMAC_PERI_PHYS             0xFF250000
#define RKIO_DMAC_PERI_SIZE             SZ_64K

#define RKIO_TSADC_PHYS                 0xFF280000
#define RKIO_TSADC_SIZE                 SZ_64K
#define RKIO_GMAC_PHYS                  0xFF290000
#define RKIO_GMAC_SIZE                  SZ_64K
#define RKIO_PERI_MMU_PHYS              0xFF2A0000
#define RKIO_PERI_MMU_SIZE              SZ_64K

#define RKIO_AXI_GPV_PHYS               0xFF300000
#define RKIO_AXI_GPV_SIZE               SZ_1M
#define RKIO_NANDC_PHYS                 0xFF400000
#define RKIO_NANDC_SIZE                 SZ_64K

#define RKIO_USBHOST_PHYS               0xFF500000
#define RKIO_USBHOST_SIZE               SZ_256K
#define RKIO_USBOTG_PHYS                0xFF580000
#define RKIO_USBOTG_SIZE                SZ_256K
#define RKIO_HSIC_PHYS                  0xFF5C0000
#define RKIO_HSIC_SIZE                  SZ_256K
#define RKIO_DMAC_BUS_PHYS              0xFF600000
#define RKIO_DMAC_BUS_SIZE              SZ_64K
#define RKIO_DDR_PCTL_PHYS              0xFF610000
#define RKIO_DDR_PCTL_SIZE              SZ_64K
#define RKIO_DDR_PHY_PHYS               0xFF620000
#define RKIO_DDR_PHY_SIZE               SZ_64K

#define RKIO_I2C0_PMU_PHYS              0xFF650000
#define RKIO_I2C0_PMU_SIZE              SZ_64K
#define RKIO_I2C1_AUDIO_PHYS            0xFF660000
#define RKIO_I2C1_AUDIO_SIZE            SZ_64K
#define RKIO_DW_PWM_PHYS                0xFF670000
#define RKIO_DW_PWM_SIZE                SZ_64K
#define RKIO_RK_PWM_PHYS                0xFF680000
#define RKIO_RK_PWM_SIZE                SZ_64K
#define RKIO_UART2_DBG_PHYS             0xFF690000
#define RKIO_UART2_DBG_SIZE             SZ_64K
#define RKIO_RK_SCR_PHYS                0xFF6A0000
#define RKIO_RK_SCR_SIZE                SZ_64K
#define RKIO_RK_MAILBOX_PHYS            0xFF6B0000
#define RKIO_RK_MAILBOX_SIZE            SZ_64K

#define RKIO_PMU_IMEM_PHYS              0xFF720000
#define RKIO_PMU_IMEM_SIZE              SZ_4K
#define RKIO_PMU_PHYS                   0xFF730000
#define RKIO_PMU_SIZE                   SZ_32K
#define RKIO_PMU_GRF_PHYS               0xFF738000
#define RKIO_PMU_GRF_SIZE               SZ_32K
#define RKIO_SECURE_GRF_PHYS            0xFF740000
#define RKIO_SECURE_GRF_SIZE            SZ_64K
#define RKIO_GPIO0_PHYS                 0xFF750000
#define RKIO_GPIO0_SIZE                 SZ_64K
#define RKIO_CRU_PHYS                   0xFF760000
#define RKIO_CRU_SIZE                   SZ_64K
#define RKIO_GRF_PHYS                   0xFF770000
#define RKIO_GRF_SIZE                   SZ_64K
#define RKIO_GPIO1_PHYS                 0xFF780000
#define RKIO_GPIO1_SIZE                 SZ_64K
#define RKIO_GPIO2_PHYS                 0xFF790000
#define RKIO_GPIO2_SIZE                 SZ_64K
#define RKIO_GPIO3_PHYS                 0xFF7A0000
#define RKIO_GPIO3_SIZE                 SZ_64K

#define RKIO_WDT_MCU_PHYS               0xFF7F0000
#define RKIO_WDT_MCU_SIZE               SZ_64K
#define RKIO_WDT_CPU_PHYS               0xFF800000
#define RKIO_WDT_CPU_SIZE               SZ_64K

#define RKIO_TIMER0_6CH_PHYS            0xFF810000
#define RKIO_TIMER0_6CH_SIZE            SZ_64K
#define RKIO_TIMER1_6CH_PHYS            0xFF820000
#define RKIO_TIMER1_6CH_SIZE            SZ_64K
#define RKIO_SECURE_TIMER_2CH_PHYS      0xFF830000
#define RKIO_SECURE_TIMER_2CH_SIZE      SZ_64K

#define RKIO_SPDIF_PHYS                 0xFF880000
#define RKIO_SPDIF_SIZE                 SZ_64K
#define RKIO_I2S_2CH_PHYS               0xFF890000
#define RKIO_I2S_2CH_SIZE               SZ_32K
#define RKIO_I2S_8CH_PHYS               0xFF898000
#define RKIO_I2S_8CH_SIZE               SZ_32K
#define RKIO_CRYPTO_PHYS                0xFF8A0000
#define RKIO_CRYPTO_SIZE                SZ_64K
#define RKIO_TSP_PHYS                   0xFF8B0000
#define RKIO_TSP_SIZE                   SZ_64K
#define RKIO_IMEM_PHYS                  0xFF8C0000
#define RKIO_IMEM_SIZE                  SZ_64K

#define RKIO_IEP_PHYS                   0xFF900000
#define RKIO_IEP_SIZE                   SZ_64K
#define RKIO_ISP_MINI_PHYS              0xFF910000
#define RKIO_ISP_MINI_SIZE              SZ_64K
#define RKIO_RGA_PHYS                   0xFF920000
#define RKIO_RGA_SIZE                   SZ_64K
#define RKIO_VOP_PHYS                   0xFF930000
#define RKIO_VOP_SIZE                   SZ_64K

#define RKIO_VIP_PHYS                   0xFF950000
#define RKIO_VIP_SIZE                   SZ_64K
#define RKIO_MIPI_DSI_HOST_PHYS         0xFF960000
#define RKIO_MIPI_DSI_HOST_SIZE         SZ_16K
#define RKIO_MIPI_CSI_HOST_PHYS         0xFF964000
#define RKIO_MIPI_CSI_HOST_SIZE         SZ_16K
#define RKIO_MIPI_DSI_PHY_PHYS          0xFF968000
#define RKIO_MIPI_DSI_PHY_SIZE          SZ_16K
#define RKIO_MIPI_CSI_PHY_PHYS          0xFF96C000
#define RKIO_MIPI_CSI_PHY_SIZE          SZ_16K
#define RKIO_EDP_PHYS                   0xFF970000
#define RKIO_EDP_SIZE                   SZ_32K

#define RKIO_HDCP_PHYS                  0xFF978000
#define RKIO_HDCP_SIZE                  SZ_32K
#define RKIO_HDMI_PHYS                  0xFF980000
#define RKIO_HDMI_SIZE                  SZ_32K
#define RKIO_VIDEO_PHYS                 0xFF9A0000
#define RKIO_VIDEO_SIZE                 SZ_64K

#define RKIO_GPU_PHYS                   0xFFA30000
#define RKIO_GPU_SIZE                   SZ_64K

#define RKIO_SERVICE_CORE_PHYS          0xFFA80000
#define RKIO_SERVICE_CORE_SIZE          SZ_64K
#define RKIO_SERVICE_DMA_PHYS           0xFFA90000
#define RKIO_SERVICE_DMA_SIZE           SZ_64K
#define RKIO_SERVICE_CCI_PHYS           0xFFAA0000
#define RKIO_SERVICE_CCI_SIZE           SZ_64K
#define RKIO_SERVICE_PERI_PHYS          0xFFAB0000
#define RKIO_SERVICE_PERI_SIZE          SZ_64K
#define RKIO_SERVICE_BUS_PHYS           0xFFAC0000
#define RKIO_SERVICE_BUS_SIZE           SZ_64K
#define RKIO_SERVICE_VIO_PHYS           0xFFAD0000
#define RKIO_SERVICE_VIO_SIZE           SZ_64K
#define RKIO_SERVICE_VPU_PHYS           0xFFAE0000
#define RKIO_SERVICE_VPU_SIZE           SZ_64K

#define RKIO_EFUSE_256BITS_PHYS         0xFFB00000
#define RKIO_EFUSE_256BITS_SIZE         SZ_64K
#define RKIO_EFUSE_1024BITS_PHYS        0xFFB10000
#define RKIO_EFUSE_1024BITS_SIZE        SZ_64K
#define RKIO_SECURITY_DMAC_BUS_PHYS     0xFFB20000
#define RKIO_SECURITY_DMAC_BUS_SIZE     SZ_64K

#define RKIO_GIC400_PHYS                0xFFB70000
#define RKIO_GIC400_SIZE                SZ_64K

#define RKIO_CCI400_PHYS                0xFFB90000
#define RKIO_CCI400_SIZE                SZ_64K

#define RKIO_DEBUG_PHYS                 0xFFC00000
#define RKIO_DEBUG_SIZE                 SZ_1M

#define RKIO_ROM_PHYS                   0xFFFD0000
#define RKIO_ROM_SIZE                   SZ_64K

#define RKIO_BUS_IMEM_PHYS              0xFFFE0000
#define RKIO_BUS_IMEM_SIZE              SZ_64K



/* define for gic, from RKIO_GIC400_PHYS */
#define RKIO_GICC_PHYS			0xFFB72000 /* GIC CPU */
#define RKIO_GICD_PHYS			0xFFB71000 /* GIC DIST */


/* define for getting chip version */
#define RKIO_ROM_CHIP_VER_ADDR		(RKIO_ROM_PHYS + 0x4FF0)
#define RKIO_ROM_CHIP_VER_SIZE		16


#endif /* __RK3368_IO_H */
