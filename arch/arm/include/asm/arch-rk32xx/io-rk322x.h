/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK322X_IO_H
#define __RK322X_IO_H


/*
 * RK3036 IO memory map:
 *
 */
#define RKIO_L2MEM_PHYS                 0x10000000

#define RKIO_IMEM_PHYS                  0x10080000

#define RKIO_CRYPTO_PHYS                0x100A0000
#define RKIO_I2S1_8CH_PHYS              0x100B0000
#define RKIO_I2S0_8CH_PHYS              0x100C0000
#define RKIO_SPDIF_PHYS                 0x100D0000
#define RKIO_I2S2_2CH_PHYS              0x100E0000
#define RKIO_TSP_PHYS                   0x100F0000
#define RKIO_ROM_PHYS                   0x10100000
#define RKIO_EFUSE_1024BITS_PHYS        0x10120000
#define RKIO_SECURE_DMAC_BUS_PHYS       0x10130000
#define RKIO_SECURE_GRF_PHYS            0x10140000
#define RKIO_DDR_SGRF_PHYS              0x10150000

#define RKIO_GRF_PHYS                   0x11000000
#define RKIO_UART0_PHYS                 0x11010000
#define RKIO_UART1_PHYS                 0x11020000
#define RKIO_UART2_PHYS                 0x11030000
#define RKIO_EFUSE_256BITS_PHYS         0x11040000
#define RKIO_I2C0_PHYS                  0x11050000
#define RKIO_I2C1_PHYS                  0x11060000
#define RKIO_I2C2_PHYS                  0x11070000
#define RKIO_I2C3_PHYS                  0x11080000
#define RKIO_SPI_PHYS                   0x11090000
#define RKIO_WDT_PHYS                   0x110A0000
#define RKIO_PWM_PHYS                   0x110B0000
#define RKIO_TIMER_6CH_PHYS             0x110C0000
#define RKIO_STIMER_2CH_PHYS            0x110D0000
#define RKIO_CRU_PHYS                   0x110E0000
#define RKIO_DMAC_PHYS                  0x110F0000
#define RKIO_SIM_PHYS                   0x11100000
#define RKIO_GPIO0_PHYS                 0x11110000
#define RKIO_GPIO1_PHYS                 0x11120000
#define RKIO_GPIO2_PHYS                 0x11130000
#define RKIO_GPIO3_PHYS                 0x11140000
#define RKIO_TSADC_PHYS                 0x11150000

#define RKIO_DDR_PCTL_PHYS              0x11210000
#define RKIO_DFIM_PHYS                  0x11220000

#define RKIO_DDR_PHY_PHYS               0x12000000
#define RKIO_ACODEC_PHY_PHYS            0x12010000
#define RKIO_VDAC_PHY_PHYS              0x12020000
#define RKIO_HDMI_PHY_PHYS              0x12030000

#define RKIO_GPU_PHYS                   0x20000000

#define RKIO_VPU_PHYS                   0x20020000
#define RKIO_VCODEC_PHYS                0x20030000

#define RKIO_VOP_PHYS                   0x20050000
#define RKIO_RGA_PHYS                   0x20060000
#define RKIO_IEP_PHYS                   0x20070000
#define RKIO_HDCPMMU_PHYS               0x20080000
#define RKIO_HDCP_PHYS                  0x20090000
#define RKIO_HDMI_PHYS                  0x200A0000

#define RKIO_SDMMC_PHYS                 0x30000000
#define RKIO_SDIO_PHYS                  0x30010000
#define RKIO_EMMC_PHYS                  0x30020000
#define RKIO_NANDC_PHYS                 0x30030000
#define RKIO_USBOTG20_PHYS              0x30040000
#define RKIO_USBHOST0_PHYS              0x30080000
#define RKIO_USBHOST1_PHYS              0x300C0000
#define RKIO_USBHOST2_PHYS              0x30100000

#define RKIO_GMAC_PHYS                  0x30200000

#define RKIO_SERVICE_CPU_PHYS           0x31000000
#define RKIO_SERVICE_PERI_PHYS          0x31010000
#define RKIO_SERVICE_BUS_PHYS           0x31020000
#define RKIO_SERVICE_VIO_PHYS           0x31030000
#define RKIO_SERVICE_VPU_PHYS           0x31040000
#define RKIO_SERVICE_GPU_PHYS           0x31050000
#define RKIO_SERVICE_VOP_PHYS           0x31060000
#define RKIO_SERVICE_VDEC_PHYS          0x31070000
#define RKIO_SERVICE_MSCH_PHYS          0x31090000

#define RKIO_CPU_DEBUG_PHYS             0x32000000

#define RKIO_GICC_PHYS                  0x32012000
#define RKIO_GICD_PHYS                  0x32011000


/* define for getting chip version */
#define RKIO_ROM_CHIP_VER_ADDR		(RKIO_ROM_PHYS + 0x4FF0)
#define RKIO_ROM_CHIP_VER_SIZE		16

/* define for pwm configuration */
#define RKIO_PWM0_PHYS                  (RKIO_PWM_PHYS + 0x00)
#define RKIO_PWM1_PHYS                  (RKIO_PWM_PHYS + 0x10)
#define RKIO_PWM2_PHYS                  (RKIO_PWM_PHYS + 0x20)
#define RKIO_PWM3_PHYS                  (RKIO_PWM_PHYS + 0x30)

#endif /* __RK322X_IO_H */
