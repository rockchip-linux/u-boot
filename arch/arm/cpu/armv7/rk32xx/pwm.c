#include <common.h>
#include <malloc.h>
#include <fdtdec.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/iomap.h>
#include <asm/arch/reg.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pwm.h>

struct pwm_bl bl;
static inline u64 div64_u64(u64 dividend, u64 divisor)
{
	return dividend / divisor;
}

static int get_pclk_pwm(void)
{
	u32 pclk_pwm, pclk_bus, pclk_bus_div;
	u32 aclk_bus,aclk_bus_div;
	u32 aclk_bus_src, aclk_bus_src_div;
	u32 clksel1 = readl(RK3288_CRU_PHYS + 0x64);
	aclk_bus_div = (clksel1 & 0x7);
	aclk_bus_src_div = (clksel1 & 0xf8) >> 3;
	pclk_bus_div = (clksel1  &0x7000) >> 12;
	if (clksel1 & 0x8000) {
		aclk_bus_src = rk_get_general_pll() / (aclk_bus_src_div + 1);
	} else {
		aclk_bus_src = rk_get_codec_pll() / (aclk_bus_src_div + 1);
	}
	aclk_bus = aclk_bus_src / (aclk_bus_div + 1);
	pclk_pwm = pclk_bus = aclk_bus / (pclk_bus_div +1 );
	return pclk_pwm;
}


static int rk_bl_parse_dt(const void *blob) 
{
	u32 data[3];
	int len;
	bl.node = fdtdec_next_compatible(blob,
					0, COMPAT_ROCKCHIP_BL);
	fdtdec_decode_gpio(blob, bl.node, "enable-gpios", &bl.bl_en);
	bl.bl_en.flags = !(bl.bl_en.flags  & OF_GPIO_ACTIVE_LOW);
	if (fdtdec_get_int_array(blob, bl.node, "pwms", data,
			ARRAY_SIZE(data))) {
		printf("Cannot decode PWM property pwms\n");
		return -1;
	}
	bl.id = data[1];
	bl.period = data[2];

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
	return 0;
	
}

int  rk_pwm_config(int brightness)
{
	u64 val, div, clk_rate;
	unsigned long prescale = 0, pv, dc;
	u32 on;
	int conf=0;
	int id = 0;
	int duty_ns,period_ns;
	if (!bl.node) {
		rk_bl_parse_dt(getenv_hex("fdtaddr", 0));
		if( bl.id == 0)
			g_grfReg->GRF_GPIO7A_IOMUX = (3<<16)|1;	 // 1: PWM0/ 0: GPIO7_A0
		gpio_direction_output(bl.bl_en.gpio, bl.bl_en.flags);
	}

	if (brightness < 0)
		brightness = bl.dft_brightness;
	brightness = bl.levels[brightness];
	duty_ns = (brightness * bl.period)/bl.max_brightness;
	period_ns = bl.period;
	id = bl.id;
	g_grfReg->GRF_SOC_CON[2] = 0x00010001; /*use rk pwm*/
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

	clk_rate = get_pclk_pwm();

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
	write_pwm_reg(id, PWM_REG_DUTY,dc);//0x1900);// dc);
	write_pwm_reg(id, PWM_REG_PERIOD,pv);//0x5dc0);//pv);
	write_pwm_reg(id, PWM_REG_CNTR,0);
	write_pwm_reg(id, PWM_REG_CTRL,on|conf);
	return 0;
}


