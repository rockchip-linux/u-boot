/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _RKXX_IOMUX_H_
#define _RKXX_IOMUX_H_


/* The clocks supported by the hardware */
enum iomux_id {
	RK_PWM0_IOMUX,
	RK_PWM1_IOMUX,
	RK_PWM2_IOMUX,
	RK_PWM3_IOMUX,
	RK_PWM4_IOMUX,
	RK_VOP0_PWM_IOMUX,
	RK_VOP1_PWM_IOMUX,
	RK_I2C0_IOMUX,
	RK_I2C1_IOMUX,
	RK_I2C2_IOMUX,
	RK_I2C3_IOMUX,
	RK_I2C4_IOMUX,
	RK_I2C5_IOMUX,
	RK_I2C6_IOMUX,
	RK_I2C7_IOMUX,
	RK_I2C8_IOMUX,
	RK_SPI0_CS0_IOMUX,
	RK_SPI0_CS1_IOMUX,
	RK_SPI1_CS0_IOMUX,
	RK_SPI1_CS1_IOMUX,
	RK_SPI2_CS0_IOMUX,
	RK_SPI2_CS1_IOMUX,
	RK_UART0_IOMUX,
	RK_UART1_IOMUX,
	RK_UART2_IOMUX,
	RK_UART3_IOMUX,
	RK_UART4_IOMUX,
	RK_LCDC0_IOMUX,
	RK_LCDC1_IOMUX,
	RK_EMMC_IOMUX,
	RK_SDCARD_IOMUX,
	RK_HDMI_IOMUX,
};


#define RK_UART_BT_IOMUX	RK_UART0_IOMUX
#define RK_UART_BB_IOMUX	RK_UART1_IOMUX
#define RK_UART_DBG_IOMUX	RK_UART2_IOMUX
#define RK_UART_GPS_IOMUX	RK_UART3_IOMUX
#define RK_UART_EXP_IOMUX	RK_UART4_IOMUX


void rk_iomux_config(int iomux_id);
#ifdef CONFIG_RK_SDCARD_BOOT_EN
void rk_iomux_sdcard_save(void);
void rk_iomux_sdcard_restore(void);
#endif /* CONFIG_RK_SDCARD_BOOT_EN */

#endif /* _RKXX_IOMUX_H_ */
