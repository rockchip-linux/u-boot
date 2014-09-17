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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <malloc.h>
#include <fdtdec.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;

#define COMPAT_ROCKCHIP_BL "pwm-backlight"




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
	u32 base;
	int id;				/*pwm id*/
	int node;			/*device node*/
	int status;
	struct fdt_gpio_state bl_en;
	unsigned int	period; 	/* in nanoseconds */
	unsigned int max_brightness;
	unsigned int dft_brightness;
	int *levels;
};

struct pwm_bl bl;
static void write_pwm_reg(struct pwm_bl *bl, int reg, int val) {	
	writel(val, bl->base + reg);
}

static inline u64 div64_u64(u64 dividend, u64 divisor)
{
	return dividend / divisor;
}

static int get_pclk_pwm(uint32 pwm_id)
{
	return rkclk_get_pwm_clk(pwm_id);
}

#ifdef CONFIG_OF_LIBFDT
static int rk_bl_parse_dt(const void *blob) 
{
	u32 data[3];
	int len;
	int pwm_node;
	bl.node = fdt_node_offset_by_compatible(blob,
					0, COMPAT_ROCKCHIP_BL);
	if (bl.node < 0) {
		debug("can't find dts node for backlight\n");
		bl.status = 0;
		return -ENODEV;
	}
	if (!fdt_device_is_available(blob,bl.node)) {
		debug("device backlight is disabled\n");
		bl.status = 0;
		return -EPERM;
	}
	fdtdec_decode_gpio(blob, bl.node, "enable-gpios", &bl.bl_en);
	bl.bl_en.flags = !(bl.bl_en.flags  & OF_GPIO_ACTIVE_LOW);
	if (fdtdec_get_int_array(blob, bl.node, "pwms", data,
			ARRAY_SIZE(data))) {
		debug("Cannot decode PWM property pwms\n");
		bl.status = 0;
		return -ENODEV;
	}

	bl.id = data[1];
	bl.period = data[2];
	pwm_node = fdt_node_offset_by_phandle(blob, data[0]);
	bl.base = fdtdec_get_addr(blob, pwm_node, "reg");
	fdt_getprop(blob, bl.node, "brightness-levels", &len);
	bl.max_brightness = len / sizeof(u32);
	bl.levels = malloc(len);
	if(!bl.levels) {
		printf("malloc for bl levels fail\n");
		return -ENOMEM;
	}
	if (fdtdec_get_int_array(blob, bl.node, "brightness-levels",
		bl.levels, len >> 2 )) {
		printf("Cannot decode brightness-levels\n");
		return -EINVAL;
	}

	bl.dft_brightness = fdtdec_get_int(blob, bl.node, "default-brightness-level", 48);
	bl.status = 1;
	return 0;
}
#endif /* CONFIG_OF_LIBFDT */


int rk_pwm_config(int brightness)
{
	u64 val, div, clk_rate;
	unsigned long prescale = 0, pv, dc;
	u32 on;
	int conf=0;
	int id = 0;
	int duty_ns,period_ns;
	int ret;
	if (!bl.node) {
#ifdef CONFIG_OF_LIBFDT
		ret = rk_bl_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
#endif
		rk_iomux_config(RK_PWM0_IOMUX+bl.id);
		gpio_direction_output(bl.bl_en.gpio, bl.bl_en.flags);
	}

	if (!bl.status)
		return -EPERM;

	if (brightness == 0)
		gpio_set_value(bl.bl_en.gpio, !(bl.bl_en.flags));
	else
		gpio_set_value(bl.bl_en.gpio, bl.bl_en.flags);

	if (brightness < 0)
		brightness = bl.dft_brightness;
	printf("%s:brightness:%d\n", __func__,brightness);
	brightness = bl.levels[brightness];
	duty_ns = (brightness * bl.period)/bl.max_brightness;
	period_ns = bl.period;
	id = bl.id;
	if (gd->arch.chiptype == CONFIG_RK3288)
    		grf_writel(0x00010001, 0x024c);/*use rk pwm*/
	on   =  RK_PWM_ENABLE ;
	conf = PWM_OUTPUT_LEFT|PWM_LP_DISABLE|
	                    PWM_CONTINUMOUS|PWM_DUTY_POSTIVE|PWM_INACTIVE_NEGATIVE;
	
	//dump_pwm_register(pc);

	/*
	 * Find pv, dc and prescale to suit duty_ns and period_ns. This is done
	 * according to formulas described below:
	 *
	 * period_ns = 10^9 * (PRESCALE ) * PV / PWM_CLK_RATE
	 * duty_ns = 10^9 * (PRESCALE + 1) * DC / PWM_CLK_RATE
	 *
	 * PV = (PWM_CLK_RATE * period_ns) / (10^9 * (PRESCALE + 1))
	 * DC = (PWM_CLK_RATE * duty_ns) / (10^9 * (PRESCALE + 1))
	 */

	clk_rate = get_pclk_pwm(bl.id);

	while (1) {
		div = 1000000000;
		div *= 1 + prescale;
		val = clk_rate * period_ns;
		pv = div64_u64(val, div);
		val = clk_rate * duty_ns;
		dc = div64_u64(val, div);
		/* if duty_ns and period_ns are not achievable then return */
		if (pv < PWMPCR_MIN_PERIOD || dc < PWMDCR_MIN_DUTY) {
			return -EINVAL;
		}

		/*
		 * if pv and dc have crossed their upper limit, then increase
		 * prescale and recalculate pv and dc.
		 */
		if (pv > PWMPCR_MAX_PERIOD || dc > PWMDCR_MAX_DUTY) {
			if (++prescale > PWMCR_MAX_PRESCALE) {
				return -EINVAL;
			}
			continue;
		}
		break;
	}

	/*
	 * NOTE: the clock to PWM has to be enabled first before writing to the
	 * registers.
	 */

	conf |= (prescale << RK_PWM_PRESCALE);
	write_pwm_reg(&bl, PWM_REG_DUTY,dc);//0x1900);// dc);
	write_pwm_reg(&bl, PWM_REG_PERIOD,pv);//0x5dc0);//pv);
	write_pwm_reg(&bl, PWM_REG_CNTR,0);
	write_pwm_reg(&bl, PWM_REG_CTRL,on|conf);

	return 0;
}

