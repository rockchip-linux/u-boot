/*
 * rockchip gpio driver
 */

#ifndef _ASM_ROCKCHIP_GPIO_H_
#define _ASM_ROCKCHIP_GPIO_H_

#include <asm/io.h>

#define RK_GPIO_WRITE_REG        0
#define RK_GPIO_DIR_REG          0x4
#define RK_GPIO_INT_EN           0x30
#define RK_GPIO_INT_MASK         0x34
#define RK_GPIO_INT_LEVEL        0x38
#define RK_GPIO_INT_POLARITY     0x3c
#define RK_GPIO_INT_STATUS       0x40
#define RK_GPIO_DEBOUNCE_REG     0x48
#define RK_GPIO_INT_EOI          0x4c
#define RK_GPIO_READ_REG         0x50


#define	GPIO_A0			0
#define	GPIO_A1			1
#define	GPIO_A2			2
#define	GPIO_A3			3
#define	GPIO_A4			4
#define	GPIO_A5			5
#define	GPIO_A6			6
#define	GPIO_A7			7
#define	GPIO_B0			8
#define	GPIO_B1			9
#define	GPIO_B2			10
#define	GPIO_B3			11
#define	GPIO_B4			12
#define	GPIO_B5			13
#define	GPIO_B6			14
#define	GPIO_B7			15
#define	GPIO_C0			16
#define	GPIO_C1			17
#define	GPIO_C2			18
#define	GPIO_C3			19
#define	GPIO_C4			20
#define	GPIO_C5			21
#define	GPIO_C6			22
#define	GPIO_C7			23
#define	GPIO_D0			24
#define	GPIO_D1			25
#define	GPIO_D2			26
#define	GPIO_D3			27
#define	GPIO_D4			28
#define	GPIO_D5			29
#define	GPIO_D6			30
#define	GPIO_D7			31

/*
 * This is Linux-specific flags. By default controllers' and Linux' mapping
 * match, but GPIO controllers are free to translate their own flags to
 * Linux-specific in their .xlate callback. Though, 1:1 mapping is recommended.
 */
enum of_gpio_flags {
	OF_GPIO_ACTIVE_LOW = 0x1,
};


int gpio_request(unsigned gpio, const char *label);
int gpio_free(unsigned gpio);
int gpio_direction_input(unsigned gpio);
int gpio_direction_output(unsigned gpio, int value);
int gpio_get_value(unsigned gpio);
int gpio_set_value(unsigned gpio, int value);
int gpio_is_valid(int number);
int gpio_irq_request(unsigned gpio, int type);

#endif /* _ASM_ROCKCHIP_GPIO_H_ */
