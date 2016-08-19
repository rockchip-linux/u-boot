/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK3399_IO_H
#define __RK3399_IO_H


/* rk io map base start */
#define RKIO_IOMEMORYMAP_START          0xF8000000

/*
 * RK3399 IO memory map:
 *
 */
#define RKIO_PCIE_PHYS                  0xF8000000

#define RKIO_SDIO_PHYS                  0xFE310000
#define RKIO_SDMMC_PHYS                 0xFE320000
#define RKIO_EMMC_PHYS                  0xFE330000

#define RKIO_USB2HOST0_PHYS             0xFE380000
#define RKIO_USB2HOST1_PHYS             0xFE3C0000

#define RKIO_USBOTG0_PHYS               0xFE800000
#define RKIO_USBOTG1_PHYS               0xFE900000

#define RKIO_GIC_PHYS                   0xFEE00000

#define RKIO_SARADC_PHYS                0xFF100000

#define RKIO_I2C0_PHYS                  0xFF3C0000
#define RKIO_I2C1_PHYS                  0xFF110000
#define RKIO_I2C2_PHYS                  0xFF120000
#define RKIO_I2C3_PHYS                  0xFF130000
#define RKIO_I2C4_PHYS                  0xFF3D0000
#define RKIO_I2C5_PHYS                  0xFF140000
#define RKIO_I2C6_PHYS                  0xFF150000
#define RKIO_I2C7_PHYS                  0xFF160000
#define RKIO_I2C8_PHYS                  0xFF3E0000

#define RKIO_UART0_PHYS                 0xFF180000
#define RKIO_UART1_PHYS                 0xFF190000
#define RKIO_UART2_PHYS                 0xFF1A0000
#define RKIO_UART3_PHYS                 0xFF1B0000
#define RKIO_UART4_PHYS                 0xFF3C0000

#define RKIO_SPI0_PHYS                  0xFF1C0000
#define RKIO_SPI1_PHYS                  0xFF1D0000
#define RKIO_SPI2_PHYS                  0xFF1E0000
#define RKIO_SPI3_PHYS                  0xFF350000
#define RKIO_SPI4_PHYS                  0xFF1F0000
#define RKIO_SPI5_PHYS                  0xFF200000

#define RKIO_PWM_PHYS                   0xFF420000

#define RKIO_PMU_PHYS                   0xFF310000
#define RKIO_CRU_PHYS                   0xFF760000
#define RKIO_GRF_PHYS                   0xFF770000

#define RKIO_PMU_GRF_PHYS               0xFF320000
#define RKIO_PMU_SGRF_PHYS              0xFF330000
#define RKIO_PMU_CRU_PHYS               0xFF750000

#define RKIO_WDT0_PHYS                  0xFF840000
#define RKIO_WDT1_PHYS                  0xFF848000
#define RKIO_WDT2_PHYS                  0xFF380000

#define RKIO_MAILBOX0_PHYS              0xFF6B0000
#define RKIO_MAILBOX1_PHYS              0xFF390000

#define RKIO_IMEM0_PHYS                 0xFF8C0000
#define RKIO_IMEM1_PHYS                 0xFF3B0000

#define RKIO_EFUSE0_PHYS                0xFF690000
#define RKIO_EFUSE1_PHYS                0xFFFA0000

#define RKIO_DMAC0_PHYS                 0xFF6D0000
#define RKIO_DMAC1_PHYS                 0xFF6E0000

#define RKIO_GPIO0_PHYS                 0xFF720000
#define RKIO_GPIO1_PHYS                 0xFF730000
#define RKIO_GPIO2_PHYS                 0xFF780000
#define RKIO_GPIO3_PHYS                 0xFF788000
#define RKIO_GPIO4_PHYS                 0xFF790000

#define RKIO_PMU_TIMER_2CH_PHYS         0xFF360000
#define RKIO_TIMER0_6CH_PHYS            0xFF850000
#define RKIO_TIMER1_6CH_PHYS            0xFF858000
#define RKIO_STIMER0_6CH_PHYS           0xFF860000
#define RKIO_STIMER1_6CH_PHYS           0xFF868000

#define RKIO_CRYPTO0_PHYS               0xFF8B0000
#define RKIO_CRYPTO1_PHYS               0xFF8B8000

#define RKIO_VOP_LITE_PHYS              0xFF8F0000
#define RKIO_VOP_BIG_PHYS               0xFF900000
#define RKIO_HDMI_PHYS                  0xFF940000
#define RKIO_HDCP_PHYS                  0xFF988000

#define RKIO_BOOTROM_PHYS               0xFFFF0000


/* gicc and gicd */
#define RKIO_GICC_PHYS			0xFFF00000 /* GIC CPU */
#define RKIO_GICD_PHYS			0xFEE00000 /* GIC DIST */
#define RKIO_GICR_PHYS			0xFEF00000

/* define for getting chip version */
#define RKIO_ROM_CHIP_VER_ADDR		(RKIO_BOOTROM_PHYS + 0x7FF0)
#define RKIO_ROM_CHIP_VER_SIZE		16

/* define for pwm configuration */
#define RKIO_PWM0_PHYS                  (RKIO_PWM_PHYS + 0x00)
#define RKIO_PWM1_PHYS                  (RKIO_PWM_PHYS + 0x10)
#define RKIO_PWM2_PHYS                  (RKIO_PWM_PHYS + 0x20)
#define RKIO_PWM3_PHYS                  (RKIO_PWM_PHYS + 0x30)
#define RKIO_VOP0_PWM_PHYS              (RKIO_VOP_BIG_PHYS + 0x01A0)
#define RKIO_VOP1_PWM_PHYS              (RKIO_VOP_LITE_PHYS + 0x01A0)

#endif /* __RK3399_IO_H */
