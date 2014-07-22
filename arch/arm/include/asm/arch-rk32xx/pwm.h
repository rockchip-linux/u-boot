/*
 * (C) Copyright 2008-2014 Rockchip Electronics
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
#ifndef __RK_PWM_H_
#define __RK_PWM_H_

#include "typedef.h"

#define COMPAT_ROCKCHIP_BL "pwm-backlight"

#define write_pwm_reg(id, addr, val)	(*(unsigned long *)(addr+RKIO_RK_PWM_PHYS+id*0x10)=val)


#define RK_PWM_DISABLE                  (0 << 0) 
#define RK_PWM_ENABLE                   (1 << 0)

#define PWM_SHOT                        (0 << 1)
#define PWM_CONTINUMOUS                 (1 << 1)
#define RK_PWM_CAPTURE                  (1 << 2)

#define PWM_DUTY_POSTIVE                (1 << 3)
#define PWM_DUTY_NEGATIVE               (0 << 3)

#define PWM_INACTIVE_POSTIVE            (1 << 4)
#define PWM_INACTIVE_NEGATIVE           (0 << 4)

#define PWM_OUTPUT_LEFT                 (0 << 5)
#define PWM_OUTPUT_ENTER                (1 << 5)

#define PWM_LP_ENABLE                   (1<<8)
#define PWM_LP_DISABLE                  (0<<8)

#define DW_PWM_PRESCALE			9
#define RK_PWM_PRESCALE			16

#define PWMCR_MIN_PRESCALE		0x00
#define PWMCR_MAX_PRESCALE		0x07

#define PWMDCR_MIN_DUTY			0x0001
#define PWMDCR_MAX_DUTY			0xFFFF

#define PWMPCR_MIN_PERIOD		0x0001
#define PWMPCR_MAX_PERIOD		0xFFFF

#define DW_PWM				0x00
#define RK_PWM				0x01
#define PWM_REG_CNTR    		0x00
#define PWM_REG_HRC     		0x04
#define PWM_REG_LRC     		0x08
#define PWM_REG_CTRL    		0x0c
#define PWM_REG_PERIOD			PWM_REG_HRC  /* Period Register */
#define PWM_REG_DUTY        		PWM_REG_LRC  /* Dutby Cycle Register */


struct pwm_bl {
	int id;				/*pwm id*/
	int node;			/*device node*/
	int status;
	struct fdt_gpio_state bl_en;
	unsigned int	period; 	/* in nanoseconds */
	unsigned int max_brightness;
	unsigned int dft_brightness;
	u32 *levels;
};

extern int rk_pwm_config(int brightness);

#endif /* __RK_PWM_H_ */

