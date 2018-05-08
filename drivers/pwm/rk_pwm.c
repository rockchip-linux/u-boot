/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

#define RKPWM_VERSION			"1.1"

/* PWM registers  */
#define PWM_REG_CNTR			0x00
#define PWM_REG_HRC			0x04
#define PWM_REG_LRC			0x08
#define PWM_REG_CTRL			0x0c /* PWM Control Register */

#define PWM_REG_PERIOD			PWM_REG_HRC  /* Period Register */
#define PWM_REG_DUTY			PWM_REG_LRC  /* Dutby Cycle Register */


/* PWM registers bit */
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

#define PWM_LP_ENABLE                   (1 << 8)
#define PWM_LP_DISABLE                  (0 << 8)


#define DW_PWM_PRESCALE			9
#define RK_PWM_PRESCALE			16

#define PWMCR_MIN_PRESCALE		0x00
#define PWMCR_MAX_PRESCALE		0x07

#define PWMDCR_MIN_DUTY			0x0001
#define PWMDCR_MAX_DUTY			0xFFFF

#define PWMPCR_MIN_PERIOD		0x0001
#define PWMPCR_MAX_PERIOD		0xFFFF
int enable_conf = 0;

void __iomem *rk_pwm_get_base(unsigned pwm_id)
{
	return (void __iomem *)(unsigned long)(RKIO_PWM_BASE + pwm_id * 0x10);
}


uint32 rk_pwm_get_clk(unsigned pwm_id)
{
	return rkclk_get_pwm_clk(pwm_id);
}


int pwm_init(int pwm_id, int div, int invert)
{
	const void __iomem *base = rk_pwm_get_base(pwm_id);

	if (base == NULL)
		return -1;

	debug("pwm init id = %d\n", pwm_id);

	if (invert)
		enable_conf |= PWM_DUTY_NEGATIVE | PWM_INACTIVE_POSTIVE;
	else
		enable_conf |= PWM_DUTY_POSTIVE | PWM_INACTIVE_NEGATIVE;
	div = div;

	return 0;
}


int pwm_config(int pwm_id, int duty_ns, int period_ns)
{
	const void __iomem *base = rk_pwm_get_base(pwm_id);
	uint64 val, div, clk_rate;
	uint64 prescale = PWMCR_MIN_PRESCALE, pv, dc;
	uint32 on;
	uint32 conf = 0;

	if (base == NULL)
		return -1;

	clk_rate = rk_pwm_get_clk(pwm_id);

	debug("pwm config id = %d, clock = %lld, duty_ns = %d, period_ns = %d\n", \
		pwm_id, clk_rate, duty_ns, period_ns);

	on   = RK_PWM_ENABLE;
	conf = PWM_OUTPUT_LEFT|PWM_LP_DISABLE|
			PWM_CONTINUMOUS | enable_conf;
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
	while (1) {
		div = 1000000000;
		div *= 1 + prescale;
		val = clk_rate * period_ns;
		pv = val / div;
		val = clk_rate * duty_ns;
		dc = val / div;

		/* if duty_ns and period_ns are not achievable then return */
		if (pv < PWMPCR_MIN_PERIOD || dc < PWMDCR_MIN_DUTY) {
			printf("pv = %lld, dc = %lld Error!\n", pv, dc);
			return -EINVAL;
		}
		/*
		 * if pv and dc have crossed their upper limit, then increase
		 * prescale and recalculate pv and dc.
		 */
		if (pv > PWMPCR_MAX_PERIOD || dc > PWMDCR_MAX_DUTY) {
			if (++prescale > PWMCR_MAX_PRESCALE) {
				printf("prescale = %lld Error!\n", prescale);
				return -EINVAL;
			}
			continue;
		}
		break;
	}
	conf |= (prescale << RK_PWM_PRESCALE);

	writel(dc, base + PWM_REG_DUTY);
	writel(pv, base + PWM_REG_PERIOD);
	writel(0, base + PWM_REG_CNTR);
	writel(on|conf, base + PWM_REG_CTRL);

	rk_iomux_config(RK_PWM0_IOMUX + pwm_id);

	return 0;
}

int pwm_enable(int pwm_id)
{
	const void __iomem *base = rk_pwm_get_base(pwm_id);
	uint32 ctrl = 0;

	if (base == NULL)
		return -1;

	ctrl = readl(base + PWM_REG_CTRL);
	ctrl |= RK_PWM_ENABLE;
	writel(ctrl, base + PWM_REG_CTRL);

	rk_iomux_config(RK_PWM0_IOMUX + pwm_id);

	return 0;
}

void pwm_disable(int pwm_id)
{
	const void __iomem *base = rk_pwm_get_base(pwm_id);
	uint32 ctrl = 0;

	if (base == NULL)
		return;

	ctrl = readl(base + PWM_REG_CTRL);
	ctrl &= ~RK_PWM_ENABLE;
	writel(RK_PWM_DISABLE, base + PWM_REG_CTRL);
}
