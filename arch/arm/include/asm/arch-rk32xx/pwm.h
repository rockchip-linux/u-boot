#ifndef __RK32XX_PWM_H_
#define __RK32XX_PWM_H_

#ifdef CONFIG_RK3288SDK
#define write_pwm_reg(id, addr, val)        (*(unsigned long *)(addr+RKIO_RK_PWM_PHYS+id*0x10)=val)
#else
#define write_pwm_reg(id, addr, val)        (*(unsigned long *)(addr+(PWM01_BASE_ADDR+(id>>1)*0x20000)+id*0x10)=val)
#endif

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
	struct fdt_gpio_state bl_en;
	unsigned int	period; 	/* in nanoseconds */
	unsigned int max_brightness;
	unsigned int dft_brightness;
	u32 *levels;
};
extern int rk_pwm_config(int brightness);
#endif
