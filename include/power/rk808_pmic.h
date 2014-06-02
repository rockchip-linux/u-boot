#ifndef __RK808_PMIC_H__
#define __RK808_PMIC_H__
#include <power/pmic.h>
#include <fdtdec.h>

#define COMPAT_ROCKCHIP_RK808  "rockchip,rk808"
#define RK808_I2C_ADDR 		0x1b
#define RK808_I2C_SPEED		100000
#define RK808_NUM_REGULATORS	14

struct pmic_rk808 {
	struct pmic *pmic;
	int node;	/*device tree node*/
	struct fdt_gpio_state pwr_hold;
};

#endif
