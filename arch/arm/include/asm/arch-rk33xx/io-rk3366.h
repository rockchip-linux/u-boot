/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3366_IO_H
#define __RK3366_IO_H


/* rk io map base start */
#define RKIO_IOMEMORYMAP_START          0xFF000000


/*
 * RK3366 IO memory map:
 *
 */
#define RKIO_SFC_PHYS                   0xFF000000
#define RKIO_PERI_NOC_PHYS              0xFF080000
#define RKIO_NANDC_PHYS                 0xFF0C0000

#define RKIO_SARADC_PHYS                0xFF100000
#define RKIO_SPI0_PHYS                  0xFF110000
#define RKIO_SPI1_PHYS                  0xFF120000

#define RKIO_I2C2_SENSOR_PHYS           0xFF140000
#define RKIO_I2C3_CAM_PHYS              0xFF150000
#define RKIO_I2C4_TP_PHYS               0xFF160000
#define RKIO_I2C5_HDMI_PHYS             0xFF170000
#define RKIO_UART0_BT_PHYS              0xFF180000
#define RKIO_UART3_GPS_PHYS             0xFF1B0000

#define RKIO_SCR_PHYS                   0xFF1D0000

#define RKIO_DMAC_PERI_PHYS             0xFF250000
#define RKIO_TSADC_PHYS                 0xFF260000
#define RKIO_SDMAC_PERI_PHYS            0xFF280000

#define RKIO_WFIBT_PHYS                 0xFF300000

#define RKIO_SDMMC_PHYS                 0xFF400000
#define RKIO_SDIO_PHYS                  0xFF410000
#define RKIO_EMMC_PHYS                  0xFF420000

#define RKIO_GMAC_PHYS                  0xFF440000

#define RKIO_USBHOST_PHYS               0xFF480000
#define RKIO_USBOTG_PHYS                0xFF4C0000
#define RKIO_USB3_PHYS                  0xFF500000

#define RKIO_DMAC_BUS_PHYS              0xFF600000
#define RKIO_DDR_PCTL_PHYS              0xFF610000
#define RKIO_DDR_PHY_PHYS               0xFF620000
#define RKIO_DFI_MONITOR_PHYS           0xFF630000

#define RKIO_I2C1_AUDIO_PHYS            0xFF660000
#define RKIO_EFUSE_256BITS_PHYS         0xFF670000
#define RKIO_RK_PWM_PHYS                0xFF680000
#define RKIO_UART2_DBG_PHYS             0xFF690000
#define RKIO_DCF_PHYS                   0xFF6A0000
#define RKIO_MAILBOX_PHYS               0xFF6B0000

#define RKIO_PMU_IMEM_PHYS              0xFF720000
#define RKIO_I2C0_PMU_PHYS              0xFF728000
#define RKIO_PMU_PHYS                   0xFF730000
#define RKIO_PMU_GRF_PHYS               0xFF738000
#define RKIO_SECURE_GRF_PHYS            0xFF740000
#define RKIO_GPIO0_PHYS                 0xFF750000
#define RKIO_GPIO1_PHYS                 0xFF758000

#define RKIO_CRU_PHYS                   0xFF760000
#define RKIO_GRF_PHYS                   0xFF770000

#define RKIO_GPIO2_PHYS                 0xFF790000
#define RKIO_GPIO3_PHYS                 0xFF7A0000
#define RKIO_GPIO4_PHYS                 0xFF7B0000
#define RKIO_GPIO5_PHYS                 0xFF7C0000

#define RKIO_WDT_MCU_PHYS               0xFF7F0000
#define RKIO_WDT_CPU_PHYS               0xFF800000
#define RKIO_TIMER_6CH_PHYS             0xFF810000

#define RKIO_SECURE_TIMER_2CH_PHYS      0xFF830000

#define RKIO_I2S_2CH_PHYS               0xFF890000
#define RKIO_I2S_8CH_PHYS               0xFF898000
#define RKIO_CRYPTO_PHYS                0xFF8A0000

#define RKIO_IMEM_PHYS                  0xFF8C0000

#define RKIO_VOP_LITE_PHYS              0xFF8F0000
#define RKIO_IEP_PHYS                   0xFF900000
#define RKIO_TSP_PHYS                   0xFF910000
#define RKIO_RGA_PHYS                   0xFF920000
#define RKIO_VOP_BIG_PHYS               0xFF930000
#define RKIO_HDCP_MMU_PHYS              0xFF940000

#define RKIO_DSI_HOST_PHYS              0xFF960000
#define RKIO_CSI_HOST_PHYS              0xFF964000
#define RKIO_DSI_PHY_PHYS               0xFF968000
#define RKIO_CSI_PHY_PHYS               0xFF96C000

#define RKIO_HDCP_PHYS                  0xFF978000
#define RKIO_HDMI_PHYS                  0xFF980000

#define RKIO_VIDEO_PHYS                 0xFF9A0000
#define RKIO_RKVDEC_PHYS                0xFF9B0000

#define RKIO_GPU_PHYS                   0xFFA30000

#define RKIO_SERVICE_CORE_PHYS          0xFFA80000
#define RKIO_SERVICE_DMA_PHYS           0xFFA90000
#define RKIO_SERVICE_CCI_PHYS           0xFFAA0000
#define RKIO_SERVICE_PERI_PHYS          0xFFAB0000
#define RKIO_SERVICE_BUS_PHYS           0xFFAC0000
#define RKIO_SERVICE_VIO_PHYS           0xFFAD0000
#define RKIO_SERVICE_VPU_PHYS           0xFFAE0000

#define RKIO_EFUSE_1024BITS_PHYS        0xFFB10000
#define RKIO_SECURITY_DMAC_BUS_PHYS     0xFFB20000
#define RKIO_DDR_SGRF_PHYS              0xFFB30000

#define RKIO_GIC400_PHYS                0xFFB70000

#define RKIO_DEBUG_PHYS                 0xFFC00000

#define RKIO_ROM_PHYS                   0xFFFD0000



/* define for gic, from RKIO_GIC400_PHYS */
#define RKIO_GICC_PHYS			0xFFB72000 /* GIC CPU */
#define RKIO_GICD_PHYS			0xFFB71000 /* GIC DIST */


/* define for getting chip version */
#define RKIO_ROM_CHIP_VER_ADDR		(RKIO_ROM_PHYS + 0x4FF0)
#define RKIO_ROM_CHIP_VER_SIZE		16

/* define for pwm configuration */
#define RKIO_PWM0_PHYS                  (RKIO_RK_PWM_PHYS + 0x00)
#define RKIO_PWM1_PHYS                  (RKIO_RK_PWM_PHYS + 0x10)
#define RKIO_PWM2_PHYS                  (RKIO_RK_PWM_PHYS + 0x20)
#define RKIO_PWM3_PHYS                  (RKIO_RK_PWM_PHYS + 0x30)
#define RKIO_VOP0_PWM_PHYS              (RKIO_VOP_BIG_PHYS + 0x01A0)

#endif /* __RK3366_IO_H */
