/*
 * rockchips iomux driver
 */

#ifndef _ASM_ROCKCHIP_IOMUX_H_
#define _ASM_ROCKCHIP_IOMUX_H_

#include <asm/io.h>
#include "iomap.h"

/* The clocks supported by the hardware */
enum iomux_id {
	RK_PWM0_IOMUX,
    RK_PWM1_IOMUX,
    RK_PWM2_IOMUX,
    RK_PWM3_IOMUX,
    RK_PWM4_IOMUX,
    RK_I2C0_IOMUX,
    RK_I2C1_IOMUX,
    RK_I2C2_IOMUX,
    RK_I2C3_IOMUX,
    RK_I2C4_IOMUX,
    RK_SPI0_CS0_IOMUX,
    RK_SPI0_CS1_IOMUX,
    RK_SPI1_CS0_IOMUX,
    RK_SPI1_CS1_IOMUX,
    RK_SPI2_CS0_IOMUX,
    RK_SPI2_CS1_IOMUX,
    RK_UART_BT_IOMUX,
    RK_UART_BB_IOMUX,
    RK_UART_DBG_IOMUX,
    RK_UART_GPS_IOMUX,
    RK_UART_EXP_IOMUX,
    RK_LCDC0_IOMUX,
};

void rk_iomux_config(int iomux_id);


#endif /* _ASM_ROCKCHIP_IOMUX_H_ */
