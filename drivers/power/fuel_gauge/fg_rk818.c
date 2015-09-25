/*
 *  Copyright (C) 2013 rockchips
 *  zyw < zyw@rock-chips.com >
 *  for battery driver sample
 */

#include <common.h>
#include <malloc.h>
#include <power/battery.h>
#include <power/rk818_pmic.h>
#include <power/rockchip_power.h>
#include <errno.h>
#include <asm/arch/rkplat.h>

/*#define SUPPORT_USB_CONNECT_TO_ADP*/

/***********************************************************/
#include <fdtdec.h>
DECLARE_GLOBAL_DATA_PTR;

/*USB_ILIM_SEL*/
#define ILIM_450MA		(0x00)
#define ILIM_800MA		(0x01)
#define ILIM_850MA		(0x02)
#define ILIM_1000MA		(0x03)
#define ILIM_1250MA		(0x04)
#define ILIM_1500MA		(0x05)
#define ILIM_1750MA		(0x06)
#define ILIM_2000MA		(0x07)
#define ILIM_2250MA		(0x08)
#define ILIM_2500MA		(0x09)
#define ILIM_2750MA		(0x0A)
#define ILIM_3000MA		(0x0B)

/*CHRG_VOL_SEL*/
#define CHRG_VOL4050		(0x00<<4)
#define CHRG_VOL4100		(0x01<<4)
#define CHRG_VOL4150		(0x02<<4)
#define CHRG_VOL4200		(0x03<<4)
#define CHRG_VOL4300		(0x04<<4)
#define CHRG_VOL4350		(0x05<<4)

/*CHRG_CUR_SEL*/
#define CHRG_CUR1000mA		(0x00)
#define CHRG_CUR1200mA		(0x01)
#define CHRG_CUR1400mA		(0x02)
#define CHRG_CUR1600mA		(0x03)
#define CHRG_CUR1800mA		(0x04)
#define CHRG_CUR2000mA		(0x05)
#define CHRG_CUR2200mA		(0x06)
#define CHRG_CUR2400mA		(0x07)
#define CHRG_CUR2600mA		(0x08)
#define CHRG_CUR2800mA		(0x09)
#define CHRG_CUR3000mA		(0x0A)

/*CHRG_CTRL_REG2*/
#define FINISH_100MA		(0x00<<6)
#define FINISH_150MA		(0x01<<6)
#define FINISH_200MA		(0x02<<6)
#define FINISH_250MA		(0x03<<6)

/* CHRG_CTRL_REG2*/
#define CHRG_TERM_ANA_SIGNAL	(0 << 5)
#define CHRG_TERM_DIG_SIGNAL	(1 << 5)

#define MAX_CAPACITY		0x7fff
#define MAX_SOC			100
#define MAX_PERCENTAGE		100
#define INTERPOLATE_MAX		1000
#define MAX_INT			0x7FFF


/*CHRG_CTRL_REG*/
#define CHRG_EN			(1<<7)
#define GG_EN			(1<<7)
#define BAT_CON			(1<<4)
#define PLUG_IN_STS		(1<<6)

/*SUP_STS_REG*/
#define BAT_EXS			(1<<7)
#define CHARGE_OFF		(0x00<<4)
#define DEAD_CHARGE		(0x01<<4)
#define TRICKLE_CHARGE		(0x02<<4)
#define CC_OR_CV		(0x03<<4)
#define CHARGE_FINISH		(0x04<<4)
#define USB_OVER_VOL		(0x05<<4)
#define BAT_TMP_ERR		(0x06<<4)
#define TIMER_ERR		(0x07<<4)
#define USB_EXIST		(1<<1)
#define USB_EFF			(1<<0)

#define SEC_PLUS		1000
#define FINISH_CALI_CURR	2000
#define TERM_CALI_CURR		600

#define ADC_CURRENT_MODE	(1 << 1)
#define ADC_VOLTAGE_MODE	(0 << 1)
/*temp feed back degree*/
#define TEMP_85C		(0x00 << 2)
#define TEMP_95C		(0x01 << 2)
#define TEMP_105C		(0x02 << 2)
#define TEMP_115C		(0x03 << 2)

/* CHRG_CTRL_REG3*/
#define CHRG_TERM_ANA_SIGNAL	(0 << 5)
#define CHRG_TERM_DIG_SIGNAL	(1 << 5)
#define CHRG_TIMER_CCCV_EN	(1 << 2)

#define CHG_CCCV_4HOUR		(0x00)
#define CHG_CCCV_5HOUR		(0x01)
#define CHG_CCCV_6HOUR		(0x02)
#define CHG_CCCV_8HOUR		(0x03)
#define CHG_CCCV_10HOUR		(0x04)
#define CHG_CCCV_12HOUR		(0x05)
#define CHG_CCCV_14HOUR		(0x06)
#define CHG_CCCV_16HOUR		(0x07)

#define CHRG_CT_EN		(1<<7)

#define MIN_FCC			500

#define	OCV_VALID_SHIFT		(0)
#define	OCV_CALIB_SHIFT		(1)
#define FIRST_PWRON_SHIFT	(2)

#define MS_TO_MIN(x)		(x/60000)
#define MS_TO_SEC(x)		(x/1000)

#define	TEST_POWER_VOL		4200
#define	TEST_POWER_SOC		66

#define RELAX_VOL1_UPD		(1<<3)
#define RELAX_VOL2_UPD		(1<<2)

/***********************************************************/
#define DRIVER_VERSION "1.0.0"

#define PMU_DEBUG 0

int rk818_state_of_chrg = 1;
static bool rk81x_fg_loader_init;
#define	SUPPORT_USB_CHARGE

static int dbg_enable;
#define DBG(args...) \
	do { \
		if (dbg_enable) { \
			printf(args); \
		} \
	} while (0)

struct battery_info {

	uint16_t	voltage;
	int		voltage_ocv;
	int		relax_voltage;

	int		nac;
	int		temp_nac;
	int		init_dsoc;
	int		dsoc;
	int		rsoc;
	int		fcc;
	int		qmax;
	u32		*ocv_table;
	unsigned int	ocv_size;
	int		current_offset;
	int		remain_capacity;
	int		current_avg;
	int		design_capacity;
	int		dod0;
	int		dod0_status;
	int		dod0_voltage;
	int		dod0_capacity;
	unsigned long	dod0_time;
	u8		dod0_level;

	int		old_remain_cap;
	int		line_k;

	int		current_k;/* (ICALIB0, ICALIB1) */
	int		current_b;
	int		voltage_k;/* VCALIB0 VCALIB1 */
	int		voltage_b;
	int		chg_v_lmt;
	int		chg_i_lmt;
	int		chg_i_cur;
	int		bat_res;
	int		chrg_diff_res;
	ulong		start_finish;
	ulong		start_term;
	bool		chrg_smooth_state;
	int		pwroff_min;
	struct fdt_gpio_state dc_det;

	int		dc_det_gpio;
	int		dc_det_status;
	int		charge_status;
	ulong		plug_in_base;
	int		chrg_time2full;
	int		chrg_cap2full;
	u8		support_usb_adp;
	u8		support_dc_adp;
	u8		virtual_power;
	u8		pwr_on_dsoc;
	u8		pwr_on_rsoc;
};

struct battery_info g_battery;

#define CHG_VOL_SHIFT	4
#define CHG_ILIM_SHIFT	0
#define CHG_ICUR_SHIFT	0

int CHG_V_LMT[] = {4050, 4100, 4150, 4200, 4300, 4350};

int CHG_I_CUR[] = {
			1000, 1200, 1400, 1600, 1800, 2000,
			2250, 2400, 2600, 2800, 3000
		  };

int CHG_I_LMT[] = {
			450, 800, 850, 1000, 1250, 1500,
			1750, 2000, 2250, 2500, 2750, 3000
		  };

struct rk818_fg {
	struct pmic *p;
};

struct rk818_fg rk818_fg;

enum {
	NO_CHARGE = 0,
	USB_CHARGE = 1,
	DC_CHARGE = 2
};

enum hw_support_adp {
	HW_ADP_TYPE_USB = 0,/*'HW' means:hardware*/
	HW_ADP_TYPE_DC,
	HW_ADP_TYPE_DUAL
};

static bool rk81x_bat_support_adp_type(struct  battery_info *di,
				       enum hw_support_adp type)
{
	bool bl = false;

	switch (type) {
	case HW_ADP_TYPE_USB:
		if (di->support_usb_adp)
			bl = true;
		break;
	case HW_ADP_TYPE_DC:
		if (di->support_dc_adp)
			bl = true;
		break;
	case HW_ADP_TYPE_DUAL:
		if (di->support_usb_adp && di->support_dc_adp)
			bl = true;
		break;
	default:
			break;
	}

	return bl;
}

/**************************************************************/
static int div(int val)
{
	return (val == 0) ? 1 : val;
}

static void dc_gpio_init(void)
{
#if CONFIG_RKCHIP_RK3368
	grf_writel((0x0 << 12) | (0x1 << (12 + 16)), GRF_SOC_CON15);

	/*io mux*/
	pmugrf_writel((0x00 << 2) | (0x3 << (2 + 16)), PMU_GRF_GPIO0C_IOMUX);

	/*gpio pull down and up*/
	pmugrf_writel((0x01 << 2) | (0x3 << (2 + 16)), PMU_GRF_GPIO0C_P);

	grf_writel((0x1 << 12) | (0x1 << (12 + 16)), GRF_SOC_CON15);
#endif
}

static int rk81x_bat_read(u8 reg, u8 *buf)
{
	*buf = i2c_reg_read(rk818_fg.p->hw.i2c.addr, reg);
	return *buf;
}

static void rk81x_bat_write(u8 reg, u8 *buf)
{
	i2c_reg_write(rk818_fg.p->hw.i2c.addr, reg, *buf);
}

static void rk81x_bat_clr_bit(struct battery_info *di, u8 reg, u8 shift)
{
	u8 buf;

	rk81x_bat_read(reg, &buf);
	buf &= ~(1 << shift);
	rk81x_bat_write(reg, &buf);
}

static int rk81x_bat_read_bit(struct battery_info *di, u8 reg, u8 shift)
{
	u8 buf;

	rk81x_bat_read(reg, &buf);
	return buf & (1 << shift);
}

static int rk81x_bat_get_rsoc(struct battery_info *di)
{
	/*return di->remain_capacity * 100 / div(di->fcc);*/
	return (di->remain_capacity + di->fcc / 200) * 100 / div(di->fcc);
}

static int rk81x_bat_gauge_enable(struct battery_info *di)
{
	int ret;
	u8 buf;

	ret = rk81x_bat_read(TS_CTRL_REG, &buf);
	if (ret < 0) {
		DBG("read TS_CTRL_REG err\n");
		return ret;
	}
	if (!(buf & GG_EN)) {
		buf |= GG_EN;
		rk81x_bat_write(TS_CTRL_REG, &buf);
		rk81x_bat_read(TS_CTRL_REG, &buf);
		return 0;
	}

	DBG("%s,TS_CTRL_REG=0x%x\n", __func__, buf);
	return 0;

}

static int rk81x_bat_get_vcalib0(struct battery_info *di)
{
	int temp = 0;
	u8 buf;

	rk81x_bat_read(VCALIB0_REGL, &buf);
	temp = buf;
	rk81x_bat_read(VCALIB0_REGH, &buf);
	temp |= buf << 8;

	return temp;
}

static int rk81x_bat_get_vcalib1(struct battery_info *di)
{
	int temp = 0;
	u8 buf;

	rk81x_bat_read(VCALIB1_REGL, &buf);
	temp = buf;
	rk81x_bat_read(VCALIB1_REGH, &buf);
	temp |= buf<<8;

	return temp;
}

static int rk81x_bat_get_ioffset(struct battery_info *di)
{
	u8 buf;
	int temp = 0;

	rk81x_bat_read(IOFFSET_REGL, &buf);
	temp = buf;
	rk81x_bat_read(IOFFSET_REGH, &buf);
	temp |= buf << 8;

	return temp;
}

static int rk81x_bat_set_cal_offset(struct battery_info *di, u32 value)
{
	u8 buf;

	buf = value & 0xff;
	rk81x_bat_write(CAL_OFFSET_REGL, &buf);
	buf = (value >> 8) & 0xff;
	rk81x_bat_write(CAL_OFFSET_REGH, &buf);

	return 0;
}

static void rk81x_bat_get_vol_offset(struct battery_info *di)
{
	int vcalib0, vcalib1;

	vcalib0 = rk81x_bat_get_vcalib0(di);
	vcalib1 = rk81x_bat_get_vcalib1(di);

	di->voltage_k = (4200 - 3000) * 1000 / (vcalib1 - vcalib0);
	di->voltage_b = 4200 - (di->voltage_k * vcalib1) / 1000;
	DBG("voltage_k=%d(x1000), voltage_b = %d\n",
	    di->voltage_k, di->voltage_b);
}

static uint16_t rk81x_bat_get_ocv_vol(struct battery_info *di)
{
	int ret;
	u8 buf;
	uint16_t temp;
	uint16_t voltage_now = 0;

	ret = rk81x_bat_read(BAT_OCV_REGL, &buf);
	temp = buf;
	ret = rk81x_bat_read(BAT_OCV_REGH, &buf);
	temp |= buf << 8;

	if (ret < 0) {
		DBG("read BAT_OCV_REGH err\n");
		return ret;
	}

	voltage_now = di->voltage_k * temp / 1000 + di->voltage_b;

	return voltage_now;
}

static int rk81x_bat_get_avg_current(struct battery_info *di)
{
	u8  buf;
	int ret;
	int current_now;
	int temp;

	ret = rk81x_bat_read(BAT_CUR_AVG_REGL, &buf);
	if (ret < 0) {
		DBG("error read BAT_CUR_AVG_REGL");
		return ret;
	}
	current_now = buf;
	ret = rk81x_bat_read(BAT_CUR_AVG_REGH, &buf);
	if (ret < 0) {
		DBG("error read BAT_CUR_AVG_REGH");
		return ret;
	}
	current_now |= (buf << 8);

	if (current_now & 0x800)
		current_now -= 4096;

	temp = current_now * 1506 / 1000;/*1000*90/14/4096*500/521;*/

	return temp;

}

static int rk81x_bat_get_vol(struct battery_info *di)
{
	int ret;
	int voltage_now = 0;
	u8 buf;
	int temp;

	ret = rk81x_bat_read(BAT_VOL_REGL, &buf);
	temp = buf;
	ret = rk81x_bat_read(BAT_VOL_REGH, &buf);
	temp |= buf << 8;

	if (ret < 0) {
		DBG("error read BAT_VOL_REGH");
		return ret;
	}

	voltage_now = di->voltage_k * temp / 1000 + di->voltage_b;

	return voltage_now;

}
static int rk_battery_ocvVoltage(struct battery_info *di)
{
	int voltage_now = 0;
	int voltage_ocv = 0;
	int current_now = 0;

	voltage_now = rk81x_bat_get_vol(di);
	if (voltage_now < 0) {
		DBG("read viltage error!");
		return voltage_now;
	}

	current_now = rk81x_bat_get_avg_current(di);
	if (current_now < 0) {
		DBG("read current error!");
		return current_now;
	}

	voltage_ocv = voltage_now - (di->bat_res * current_now / 1000);

	return voltage_ocv;
}

static void rk81x_bat_fg_match_param(struct battery_info *di,
				     int chg_vol, int chg_ilim, int chg_cur)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(CHG_V_LMT); i++) {
		if (chg_vol < CHG_V_LMT[i])
			break;

		di->chg_v_lmt = (i << CHG_VOL_SHIFT);
	}

	for (i = 0; i < ARRAY_SIZE(CHG_I_LMT); i++) {
		if (chg_ilim < CHG_I_LMT[i])
			break;

		di->chg_i_lmt = (i << CHG_ILIM_SHIFT);
	}

	for (i = 0; i < ARRAY_SIZE(CHG_I_CUR); i++) {
		if (chg_cur < CHG_I_CUR[i])
			break;

		di->chg_i_cur = (i << CHG_ICUR_SHIFT);
	}
	DBG("vlmt = 0x%x, ilim = 0x%x, cur=0x%x\n",
		di->chg_v_lmt, di->chg_i_lmt, di->chg_i_cur);
}

static void rk81x_bat_charger_init(struct  battery_info *di)
{
	u8 chrg_ctrl_reg1, usb_ctrl_reg, chrg_ctrl_reg2;
	u8 chrg_ctrl_reg3;
	u8 sup_sts_reg, ggcon, thremal_reg;

	rk81x_bat_fg_match_param(di, di->chg_v_lmt,
					di->chg_i_lmt, di->chg_i_cur);

	rk81x_bat_read(USB_CTRL_REG, &usb_ctrl_reg);
	rk81x_bat_read(CHRG_CTRL_REG1, &chrg_ctrl_reg1);
	rk81x_bat_read(CHRG_CTRL_REG2, &chrg_ctrl_reg2);
	rk81x_bat_read(SUP_STS_REG, &sup_sts_reg);
	rk81x_bat_read(CHRG_CTRL_REG3, &chrg_ctrl_reg3);
	rk81x_bat_read(GGCON, &ggcon);
	rk81x_bat_read(RK818_THERMAL_REG, &thremal_reg);

	usb_ctrl_reg &= (~0x0f);

	if (rk81x_bat_support_adp_type(di, HW_ADP_TYPE_USB))
		usb_ctrl_reg |= (CHRG_CT_EN | ILIM_450MA);
	else
		usb_ctrl_reg |= (CHRG_CT_EN | di->chg_i_lmt);

	chrg_ctrl_reg1 &= (0x00);
	chrg_ctrl_reg1 |= (CHRG_EN) | (di->chg_v_lmt | di->chg_i_cur);

	/*chrg_ctrl_reg3 |= CHRG_TERM_DIG_SIGNAL; digital finish mode*/
	chrg_ctrl_reg3 &= ~CHRG_TIMER_CCCV_EN;/*disable*/

	chrg_ctrl_reg2 &= ~(0xc7);
	chrg_ctrl_reg2 |= FINISH_150MA | CHG_CCCV_16HOUR;

	sup_sts_reg &= ~(0x01 << 3);
	sup_sts_reg |= (0x01 << 2);
	ggcon |= ADC_CURRENT_MODE;

	thremal_reg &= (~0x0c);
	thremal_reg |= TEMP_105C;/*temp feed back: 105c*/

	rk81x_bat_write(CHRG_CTRL_REG3, &chrg_ctrl_reg3);
	rk81x_bat_write(USB_CTRL_REG, &usb_ctrl_reg);
	rk81x_bat_write(CHRG_CTRL_REG1, &chrg_ctrl_reg1);
	rk81x_bat_write(CHRG_CTRL_REG2, &chrg_ctrl_reg2);
	rk81x_bat_write(SUP_STS_REG, &sup_sts_reg);
	rk81x_bat_write(GGCON, &ggcon);
	rk81x_bat_write(RK818_THERMAL_REG, &thremal_reg);
}

/* OCV Lookup table
 * Open Circuit Voltage (OCV) correction routine. This function estimates SOC,
 * based on the voltage.
 */

static u32 interpolate(int value, u32 *table, int size)
{
	uint8_t i;
	uint16_t d;

	for (i = 0; i < size; i++) {
		if (value < table[i])
			break;
	}

	if ((i > 0) && (i < size)) {
		d = (value - table[i-1]) * (INTERPOLATE_MAX/(size-1));
		d /= table[i] - table[i-1];
		d = d + (i-1) * (INTERPOLATE_MAX/(size-1));
	} else {
		d = i * ((INTERPOLATE_MAX+size/2)/size);
	}

	if (d > 1000)
		d = 1000;

	return d;
}

/* Returns (a * b) / c */
static int32_t ab_div_c(u32 a, u32 b, u32 c)
{
	bool sign;
	u32 ans = MAX_INT;
	int32_t tmp;

	sign = ((((a^b)^c) & 0x80000000) != 0);

	if (c != 0) {
		if (sign)
			c = -c;

		tmp = ((int32_t) a*b + (c>>1)) / c;

		if (tmp < MAX_INT)
			ans = tmp;
	}

	if (sign)
		ans = -ans;

	return ans;
}

static int rk81x_bat_vol_to_cap(struct battery_info *di, int voltage)
{
	u32 *ocv_table;
	int ocv_size;
	u32 tmp;
	u8 ocv_soc;

	ocv_table = di->ocv_table;
	ocv_size = di->ocv_size;
	tmp = interpolate(voltage, ocv_table, ocv_size);
	ocv_soc = ab_div_c(tmp, MAX_PERCENTAGE, INTERPOLATE_MAX);
	di->temp_nac = ab_div_c(tmp, di->fcc, INTERPOLATE_MAX);

	return ocv_soc;
}

static int rk81x_bat_get_remain_cap(struct battery_info *di)
{
	int temp = 0;
	u8 buf;
	u32 capacity;

	rk81x_bat_read(REMAIN_CAP_REG3, &buf);
	temp = buf << 24;
	rk81x_bat_read(REMAIN_CAP_REG2, &buf);
	temp |= buf << 16;
	rk81x_bat_read(REMAIN_CAP_REG1, &buf);
	temp |= buf << 8;
	rk81x_bat_read(REMAIN_CAP_REG0, &buf);
	temp |= buf;

	capacity = temp;/* /4096*900/14/36*500/521; */

	return capacity;
}

static void rk81x_bat_save_fcc(struct battery_info *di, u32 capacity)
{
	u8 buf;
	u32 capacity_ma;

	capacity_ma = capacity;
	buf = (capacity_ma >> 24) & 0xff;
	rk81x_bat_write(NEW_FCC_REG3, &buf);
	buf = (capacity_ma >> 16) & 0xff;
	rk81x_bat_write(NEW_FCC_REG2, &buf);
	buf = (capacity_ma >> 8) & 0xff;
	rk81x_bat_write(NEW_FCC_REG1, &buf);
	buf = (capacity_ma & 0xff);
	rk81x_bat_write(NEW_FCC_REG0, &buf);
}

static int rk81x_bat_get_fcc(struct battery_info *di)
{
	int temp = 0;
	u8 buf;
	int capacity;

	rk81x_bat_read(NEW_FCC_REG3, &buf);
	temp = buf << 24;
	rk81x_bat_read(NEW_FCC_REG2, &buf);
	temp |= buf << 16;
	rk81x_bat_read(NEW_FCC_REG1, &buf);
	temp |= buf << 8;
	rk81x_bat_read(NEW_FCC_REG0, &buf);
	temp |= buf;

	capacity = temp;

	if (capacity < MIN_FCC)
		capacity = di->design_capacity;
	else if (capacity > di->qmax)
		capacity = di->qmax;

	return capacity;
}

static int rk81x_bat_get_pwroff_min(struct battery_info *di)
{
	u8 curr_pwroff_min, last_pwroff_min;

	rk81x_bat_read(NON_ACT_TIMER_CNT_REG, &curr_pwroff_min);
	rk81x_bat_read(NON_ACT_TIMER_CNT_REG_SAVE, &last_pwroff_min);
	rk81x_bat_write(NON_ACT_TIMER_CNT_REG_SAVE, &curr_pwroff_min);

	if (curr_pwroff_min == 0)
		return 0;

	return (curr_pwroff_min != last_pwroff_min) ? curr_pwroff_min : -1;
}

static int rk81x_bat_get_calib_vol(struct battery_info *di)
{
	int calib_vol;
	int init_cur, diff;
	int est_vol;
	int relax_vol = di->relax_voltage;
	int ocv_vol = di->voltage_ocv;

	init_cur = rk81x_bat_get_avg_current(di);
	diff = (di->bat_res + di->chrg_diff_res) * init_cur;
	diff /= 1000;
	est_vol = di->voltage - diff;

	if (di->pwroff_min > 8) {
		if (abs(relax_vol - ocv_vol) < 100) {
			calib_vol = ocv_vol;
		} else {
			if (abs(relax_vol - est_vol) > abs(ocv_vol - est_vol))
				calib_vol = ocv_vol;
			else
				calib_vol = relax_vol;
		}
	} else if (di->pwroff_min > 1) { /*normal reboot*/
		calib_vol = ocv_vol;
	} else {
		calib_vol = -1;
	}

	DBG("<%s>. c=%d, v=%d, relax=%d, ocv=%d, est=%d, calib=%d\n",
	    __func__, init_cur, di->voltage, relax_vol, ocv_vol,
	    est_vol, calib_vol);

	return calib_vol;
}

static uint16_t rk81x_bat_get_relax_vol1(struct battery_info *di)
{

	u8 buf;
	uint16_t temp = 0, voltage_now;

	rk81x_bat_read(RELAX_VOL1_REGL, &buf);
	temp = buf;
	rk81x_bat_read(RELAX_VOL1_REGH, &buf);
	temp |= (buf << 8);

	voltage_now = di->voltage_k * temp / 1000 + di->voltage_b;

	return voltage_now;
}

static uint16_t rk81x_bat_get_relax_vol2(struct battery_info *di)
{
	u8 buf;
	uint16_t temp = 0, voltage_now;

	rk81x_bat_read(RELAX_VOL2_REGL, &buf);
	temp = buf;
	rk81x_bat_read(RELAX_VOL2_REGH, &buf);
	temp |= (buf << 8);

	voltage_now = di->voltage_k * temp / 1000 + di->voltage_b;

	return voltage_now;
}

static bool is_rk81x_bat_relax_mode(struct battery_info *di)
{
	u8 status;

	rk81x_bat_read(GGSTS, &status);

	if ((!(status & RELAX_VOL1_UPD)) || (!(status & RELAX_VOL2_UPD)))
		return false;
	else
		return true;
}

static uint16_t rk81x_bat_get_relax_vol(struct battery_info *di)
{
	u8 status;
	uint16_t relax_vol1, relax_vol2;
	u8 ggcon;

	rk81x_bat_read(GGSTS, &status);
	rk81x_bat_read(GGCON, &ggcon);

	relax_vol1 = rk81x_bat_get_relax_vol1(di);
	relax_vol2 = rk81x_bat_get_relax_vol2(di);
	DBG("<%s>. GGSTS=0x%x, GGCON=0x%x, relax_vol1=%d, relax_vol2=%d\n",
	    __func__, status, ggcon, relax_vol1, relax_vol2);

	if (is_rk81x_bat_relax_mode(di))
		return relax_vol1 > relax_vol2 ? relax_vol1 : relax_vol2;
	else
		return 0;
}

static void rk81x_bat_first_pwron(struct battery_info *di)
{
	rk81x_bat_save_fcc(di, di->design_capacity);
	di->fcc = rk81x_bat_get_fcc(di);
	di->rsoc = rk81x_bat_vol_to_cap(di, di->voltage_ocv);
	di->dsoc = di->rsoc;
	di->nac  = di->temp_nac;
	DBG("<%s>. OCV-SOC:%d, OCV-CAP:%d, FCC:%d\n",
	    __func__, di->dsoc, di->nac, di->fcc);
}

static void rk81x_bat_not_first_pwron(struct battery_info *di)
{
	u8 pwron_soc;
	u8 init_soc;
	int remain_capacity;
	int ocv_soc;
	int calib_vol = 0;
	int calib_soc = 0;
	int calib_capacity = 0;

	rk81x_bat_clr_bit(di, MISC_MARK_REG, FIRST_PWRON_SHIFT);
	rk81x_bat_read(SOC_REG, &pwron_soc);
	init_soc = pwron_soc;
	remain_capacity = rk81x_bat_get_remain_cap(di);
	DBG("reg: soc=%d, remain_cap=%d\n", init_soc, remain_capacity);

	calib_vol = rk81x_bat_get_calib_vol(di);
	if (calib_vol > 0) {
		calib_soc = rk81x_bat_vol_to_cap(di, calib_vol);
		calib_capacity = di->temp_nac;

		if (abs(calib_soc - init_soc) > 50) {
			init_soc = calib_soc;
			remain_capacity = calib_capacity;
		}
		DBG("calib: init soc=%d, remain_cap=%d\n",
		    init_soc, remain_capacity);
	}

	ocv_soc = rk81x_bat_vol_to_cap(di, di->voltage_ocv);
	DBG("<%s>, ocv: soc=%d, cap=%d\n", __func__, ocv_soc, di->temp_nac);

	if (di->pwroff_min > 0) {
		if (di->pwroff_min > 30) {
			remain_capacity = di->temp_nac;
			DBG("<%s>pwroff > 30 minute, remain_cap = %d\n",
			    __func__, remain_capacity);

		} else if ((di->pwroff_min > 5) &&
				(abs(ocv_soc - init_soc) >= 10)) {
			if (remain_capacity >= di->temp_nac * 120/100)
				remain_capacity = di->temp_nac * 110/100;
			else if (remain_capacity < di->temp_nac * 8/10)
				remain_capacity = di->temp_nac * 9/10;
			DBG("<%s> pwroff > 5 minute, remain_cap = %d\n",
			    __func__, remain_capacity);
		}
	} else {
		rk81x_bat_clr_bit(di, MISC_MARK_REG, OCV_VALID_SHIFT);
	}

	di->dsoc = init_soc;
	di->nac = remain_capacity;
	if (di->nac <= 0)
		di->nac = 0;
	DBG("<%s> out: init_soc = %d, init_capacity=%d\n",
	    __func__, di->dsoc, di->nac);
}

static bool is_rk81x_bat_first_poweron(struct battery_info *di)
{
	u8 buf;
	u8 temp;

	rk81x_bat_read(GGSTS, &buf);
	DBG("%s GGSTS value is 0x%2x\n", __func__, buf);
	/*di->pwron_bat_con = buf;*/
	if (buf&BAT_CON) {
		buf &= ~(BAT_CON);
		do {
			rk81x_bat_write(GGSTS, &buf);
			rk81x_bat_read(GGSTS, &temp);
		} while (temp & BAT_CON);
		return true;
	}

	return false;
}

static int rk81x_bat_rsoc_init(struct battery_info *di)
{
	u8 calib_en;/*debug*/

	di->voltage  = rk81x_bat_get_vol(di);
	di->voltage_ocv = rk81x_bat_get_ocv_vol(di);
	di->pwroff_min = rk81x_bat_get_pwroff_min(di);
	di->relax_voltage = rk81x_bat_get_relax_vol(di);
	di->current_avg = rk81x_bat_get_avg_current(di);

	DBG("v=%d, ov=%d, rv=%d, c=%d, min=%d\n",
	    di->voltage, di->voltage_ocv, di->relax_voltage,
	    di->current_avg, di->pwroff_min);

	calib_en = rk81x_bat_read_bit(di, MISC_MARK_REG, OCV_CALIB_SHIFT);
	DBG("readbit: calib_en=%d\n", calib_en);
	if (is_rk81x_bat_first_poweron(di) ||
	    ((di->pwroff_min >= 30) && (calib_en == 1))) {
		rk81x_bat_first_pwron(di);
		rk81x_bat_clr_bit(di, MISC_MARK_REG, OCV_CALIB_SHIFT);

	} else {
		rk81x_bat_not_first_pwron(di);
	}


	return 0;
}

static void rk81x_bat_capacity_init(struct battery_info *di, u32 capacity)
{
	u8 buf;
	u32 capacity_ma;

	capacity_ma = capacity * 2390;/* 2134;//36*14/900*4096/521*500; */

	buf = (capacity_ma >> 24) & 0xff;
	rk81x_bat_write(GASCNT_CAL_REG3, &buf);
	buf = (capacity_ma >> 16) & 0xff;
	rk81x_bat_write(GASCNT_CAL_REG2, &buf);
	buf = (capacity_ma >> 8) & 0xff;
	rk81x_bat_write(GASCNT_CAL_REG1, &buf);
	buf = (capacity_ma & 0xff);
	rk81x_bat_write(GASCNT_CAL_REG0, &buf);
}


static int rk81x_bat_get_realtime_cap(struct battery_info *di)
{
	int temp = 0;
	u8 buf;
	u32 capacity;

	rk81x_bat_read(GASCNT3, &buf);
	temp = buf << 24;
	rk81x_bat_read(GASCNT2, &buf);
	temp |= buf << 16;
	rk81x_bat_read(GASCNT1, &buf);
	temp |= buf << 8;
	rk81x_bat_read(GASCNT0, &buf);
	temp |= buf;

	capacity = temp/2390;/* 4096*900/14/36*500/521; */

	return capacity;
}

static int rk81x_bat_calc_linek(struct battery_info *di)
{
	int line_k;
	u8 thresd_soc, delta_soc;

	di->pwr_on_dsoc = di->dsoc;
	di->pwr_on_rsoc = rk81x_bat_get_rsoc(di);

	/*get small one*/
	delta_soc = abs(di->dsoc - di->rsoc);

	if (delta_soc < 10)
		thresd_soc = delta_soc;
	else if (delta_soc <= 15)
		thresd_soc = 15;
	else if (delta_soc <= 20)
		thresd_soc = 20;
	else
		thresd_soc = 30;

	/*plus 10*/
	if (di->dsoc < di->rsoc)
		line_k = 10 * (delta_soc + thresd_soc) / thresd_soc;
	else if (di->dsoc > di->rsoc)
		line_k = 10 * thresd_soc / (delta_soc + thresd_soc);
	else
		line_k = 10;
	DBG("<%s>. meet thresd_soc = %d, link = %d\n",
	    __func__, thresd_soc, line_k);
	return line_k;
}

static void rk81x_bat_restart_relax(struct battery_info *di)
{
	u8 ggcon;
	u8 ggsts;

	rk81x_bat_read(GGCON, &ggcon);
	ggcon &= ~0x0c;
	rk81x_bat_write(GGCON, &ggcon);

	rk81x_bat_read(GGSTS, &ggsts);
	ggsts &= ~0x0c;
	rk81x_bat_write(GGSTS, &ggsts);
}

bool is_rk81x_fg_init(void)
{
	return rk81x_fg_loader_init;
}

static void rk81x_bat_fg_init(struct battery_info *di)
{
	u8 adc_ctrl_val;
	u8 pwron_soc;

	adc_ctrl_val = 0x30;
	rk81x_bat_write(ADC_CTRL_REG, &adc_ctrl_val);
	rk81x_bat_read(SOC_REG, &pwron_soc);

	rk81x_bat_gauge_enable(di);
	rk81x_bat_get_vol_offset(di);
	rk81x_bat_charger_init(di);

	di->current_offset = rk81x_bat_get_ioffset(di);
	rk81x_bat_set_cal_offset(di, di->current_offset+42);
	rk81x_bat_rsoc_init(di);
	rk81x_bat_capacity_init(di, di->nac);

	di->remain_capacity = rk81x_bat_get_realtime_cap(di);
	di->rsoc = rk81x_bat_get_rsoc(di);
	di->init_dsoc = di->dsoc;
	di->current_avg = rk81x_bat_get_avg_current(di);
	di->line_k = rk81x_bat_calc_linek(di);
	di->plug_in_base = get_timer(0);
	rk81x_bat_restart_relax(di);
	rk81x_fg_loader_init = true;

	printf("battery: gl=%d dl=%d rl=%d, v=%d, m=%d\n",
	       pwron_soc, di->dsoc, di->rsoc, di->voltage, di->pwroff_min);
}

static void rk81x_bat_info_init(struct battery_info *di)
{
	di->start_finish = get_timer(0);
	di->start_term = get_timer(0);
	di->chrg_smooth_state = false;
	di->fcc = rk81x_bat_get_fcc(di);
}

static bool is_rk81x_bat_exist(struct  battery_info *di)
{
	u8 buf;

	rk81x_bat_read(SUP_STS_REG, &buf);
	return (buf & BAT_EXS) ? true : false;
}

static void rk81x_bat_set_chrg_current(struct battery_info *di,
				       int charge_current)
{
	u8 usb_ctrl_reg;

	rk81x_bat_read(USB_CTRL_REG, &usb_ctrl_reg);
	usb_ctrl_reg &= (~0x0f);
	usb_ctrl_reg |= (charge_current);
	rk81x_bat_write(USB_CTRL_REG, &usb_ctrl_reg);
}

/**************************************************************/
/*
0. disable charging
1. usb charging
2. ac adapter charging
*/
static void  rk81x_bat_charger_setting(struct battery_info *di, int charger_st)
{
	if ((charger_st > USB_CHARGE) && (!is_rk81x_bat_exist(di))) {
		rk81x_bat_set_chrg_current(di, di->chg_i_lmt);
		return;
	}

	if (di->charge_status != charger_st) {
		if (charger_st == NO_CHARGE) {
			rk81x_bat_set_chrg_current(di, ILIM_450MA);
		} else if (charger_st > NO_CHARGE) {
			if (USB_CHARGE == charger_st)
				rk81x_bat_set_chrg_current(di, ILIM_450MA);

			else if (DC_CHARGE == charger_st)
				rk81x_bat_set_chrg_current(di, di->chg_i_lmt);
		} else {
			printf("rk81x charger setting error %d\n", charger_st);
		}
		di->charge_status = charger_st;
	}
}

static int rk81x_bat_get_usb_state(struct battery_info *di)
{
	int charger_type;
	int usb_id;

	usb_id = dwc_otg_check_dpdm();
	switch (usb_id) {
	case 0:
		charger_type = NO_CHARGE;
		break;
	case 1:
	case 3:
		charger_type = USB_CHARGE;
		break;
	case 2:
		charger_type = DC_CHARGE;
		break;
	default:
		charger_type = NO_CHARGE;
	}

	return charger_type;
}

static int rk81x_bat_get_dc_state(struct battery_info *di,
				  struct pmic *pmic)
{
	int charger_type = NO_CHARGE;
	u8 buf;
	int dc_state;

	buf = i2c_reg_read(pmic->hw.i2c.addr, RK818_VB_MON_REG);
	/*only HW_ADP_TYPE_DC: det by rk818 is easily and will be successful*/
	if (!rk81x_bat_support_adp_type(di, HW_ADP_TYPE_USB)) {
		if ((buf & PLUG_IN_STS) != 0)
			charger_type = DC_CHARGE;
		else
			charger_type = NO_CHARGE;
		return charger_type;
	}
	/*det by gpio level*/
	dc_state = gpio_get_value(di->dc_det.gpio);
	if (dc_state == di->dc_det.flags) {
		charger_type = DC_CHARGE;/*dc charger*/
	} else if (rk81x_bat_support_adp_type(di, HW_ADP_TYPE_DUAL)) {
		if ((buf & PLUG_IN_STS) != 0) {
			charger_type = dwc_otg_check_dpdm();
			if (charger_type == 0)
				charger_type = DC_CHARGE;
			else
				charger_type = NO_CHARGE;
		}
	}

	return charger_type;
}

static int rk81x_bat_get_charger_type(struct battery_info *di,
				      struct pmic *pmic)
{
	u8 buf;
	int charger_type = NO_CHARGE;

	/*check by ic hardware: this check make check work safer*/
	buf = i2c_reg_read(pmic->hw.i2c.addr, RK818_VB_MON_REG);
	if ((buf & PLUG_IN_STS) == 0)
		return NO_CHARGE;

	if (di->virtual_power)
		return DC_CHARGE;

	/*check DC first*/
	if (rk81x_bat_support_adp_type(di, HW_ADP_TYPE_DC)) {
		charger_type = rk81x_bat_get_dc_state(di, pmic);
		if (charger_type == DC_CHARGE)
			return charger_type;
	}

	/*HW_ADP_TYPE_USB*/
	charger_type = rk81x_bat_get_usb_state(di);

	return charger_type;
}

static int rk81x_batttery_check(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;

	battery->state_of_chrg = rk81x_bat_get_charger_type(&g_battery, bat);

	return 0;
}

static int rk81x_bat_save_dsoc(struct  battery_info *di, u8 save_soc)
{
	static u8 old_soc;

	if (old_soc != save_soc) {
		old_soc = save_soc;
		rk81x_bat_write(SOC_REG, &save_soc);
	}

	return 0;
}

static void rk81x_bat_save_remain_cap(struct battery_info *di, int capacity)
{
	u8 buf;
	int capacity_ma;
	static int old_capacity;

	if (old_capacity == capacity)
		return;

	if (capacity >= di->qmax)
		capacity = di->qmax;
	old_capacity = capacity;

	capacity_ma = capacity;
	buf = (capacity_ma >> 24) & 0xff;
	rk81x_bat_write(REMAIN_CAP_REG3, &buf);
	buf = (capacity_ma >> 16) & 0xff;
	rk81x_bat_write(REMAIN_CAP_REG2, &buf);
	buf = (capacity_ma >> 8) & 0xff;
	rk81x_bat_write(REMAIN_CAP_REG1, &buf);
	buf = (capacity_ma & 0xff) | 0x01;
	rk81x_bat_write(REMAIN_CAP_REG0, &buf);
}

static u8 rk81x_bat_get_chrg_status(struct battery_info *di)
{
	u8 status;
	u8 ret = 0;

	rk81x_bat_read(SUP_STS_REG, &status);
	status &= (0x70);
	switch (status) {
	case CHARGE_OFF:
		ret = CHARGE_OFF;
		DBG("  CHARGE-OFF ...\n");
		break;

	case DEAD_CHARGE:
		ret = DEAD_CHARGE;
		DBG("  DEAD CHARGE ...\n");
		break;

	case  TRICKLE_CHARGE:
		ret = DEAD_CHARGE;
		DBG("  TRICKLE CHARGE ...\n ");
		break;

	case  CC_OR_CV:
		ret = CC_OR_CV;
		DBG("  CC or CV ...\n");
		break;

	case  CHARGE_FINISH:
		ret = CHARGE_FINISH;
		DBG("  CHARGE FINISH ...\n");
		break;

	case  USB_OVER_VOL:
		ret = USB_OVER_VOL;
		DBG("  USB OVER VOL ...\n");
		break;

	case  BAT_TMP_ERR:
		ret = BAT_TMP_ERR;
		DBG("  BAT TMP ERROR ...\n");
		break;

	case  TIMER_ERR:
		ret = TIMER_ERR;
		DBG("  TIMER ERROR ...\n");
		break;

	case  USB_EXIST:
		ret = USB_EXIST;
		DBG("  USB EXIST ...\n");
		break;

	case  USB_EFF:
		ret = USB_EFF;
		DBG("  USB EFF...\n");
		break;

	default:
		return -EINVAL;
	}

	return ret;

}

static void rk81x_bat_finish_chrg(struct battery_info *di)
{
	u32 sec;

	if (di->dsoc < 100) {
		sec = di->fcc * 3600 / 100 / FINISH_CALI_CURR;
		if (get_timer(di->start_finish) > SEC_PLUS * sec) {
			di->start_finish = get_timer(0);
			di->dsoc++;
		}
		DBG("<%s>. sec = %d, finish_sec = %lu\n",
		    __func__, sec, get_timer(di->start_finish));
	}
}

static bool rk81x_bat_term_chrg(struct battery_info *di, int chg_st)
{
	u32 sec;
	bool ret;

	sec = di->fcc * 3600 / 100 / TERM_CALI_CURR;
	if ((rk818_state_of_chrg > 0 && di->dsoc >= 90) &&
	    (di->current_avg > 600 ||
	     di->voltage < (di->ocv_table[18] + 20))) {

		if (get_timer(di->start_term) > SEC_PLUS * sec) {
			di->start_term = get_timer(0);
			di->dsoc++;
			if (di->dsoc >= 100)
				di->dsoc = 99;
		}
		ret = true;
		DBG("<%s>. sec = %d, start_term = %lu\n",
		    __func__, sec, get_timer(di->start_term));
	} else {
		ret = false;
	}

	return ret;
}

static void rk81x_bat_dmp_dbg_info(struct battery_info *di)
{
	u8 sup_tst_reg, ggcon_reg, ggsts_reg, vb_mod_reg;
	u8 usb_ctrl_reg, chrg_ctrl_reg1;
	u8 chrg_ctrl_reg2, chrg_ctrl_reg3, rtc_val;

	rk81x_bat_read(GGCON, &ggcon_reg);
	rk81x_bat_read(GGSTS, &ggsts_reg);
	rk81x_bat_read(SUP_STS_REG, &sup_tst_reg);
	rk81x_bat_read(RK818_VB_MON_REG, &vb_mod_reg);
	rk81x_bat_read(USB_CTRL_REG, &usb_ctrl_reg);
	rk81x_bat_read(CHRG_CTRL_REG1, &chrg_ctrl_reg1);
	rk81x_bat_read(CHRG_CTRL_REG2, &chrg_ctrl_reg2);
	rk81x_bat_read(CHRG_CTRL_REG3, &chrg_ctrl_reg3);
	rk81x_bat_read(0x00, &rtc_val);

	DBG("\n------------- dump_debug_regs -----------------\n"
	    "GGCON = 0x%2x, GGSTS = 0x%2x, RTC	= 0x%2x\n"
	    "SUP_STS_REG  = 0x%2x, VB_MOD_REG	= 0x%2x\n"
	    "USB_CTRL_REG  = 0x%2x, CHRG_CTRL_REG1 = 0x%2x\n"
	    "CHRG_CTRL_REG2 = 0x%2x, CHRG_CTRL_REG3 = 0x%2x\n\n",
	    ggcon_reg, ggsts_reg, rtc_val,
	    sup_tst_reg, vb_mod_reg,
	    usb_ctrl_reg, chrg_ctrl_reg1,
	    chrg_ctrl_reg2, chrg_ctrl_reg3
	   );

	DBG(
	    "-----------------------------------------------------------\n"
	    "voltage = %d, current-avg = %d\n"
	    "fcc = %d, remain_capacity = %d, ocv_volt = %d\n"
	    "diplay_soc = %d, cpapacity_soc = %d, ADP = %d\n",
	    rk81x_bat_get_vol(di), rk81x_bat_get_avg_current(di),
	    di->fcc, di->remain_capacity, rk81x_bat_get_ocv_vol(di),
	    di->dsoc, rk81x_bat_get_rsoc(di), rk818_state_of_chrg
	   );
	rk81x_bat_get_chrg_status(di);
	DBG("##########################################################\n");
}

static void rk81x_bat_linek_chrg_algorithm(struct battery_info *di)
{
	int delta_rsoc, delta_dsoc;
	u8 chg_st = rk81x_bat_get_chrg_status(di);

	if (!di->old_remain_cap)
		di->old_remain_cap = di->remain_capacity;

	/*plus 10*/
	delta_rsoc = 10 * (di->remain_capacity - di->old_remain_cap) * 100 /
			div(di->fcc);
	delta_dsoc = di->line_k * delta_rsoc / 100;

	if (delta_dsoc > 0) {
		di->dsoc += delta_dsoc;
		di->old_remain_cap = di->remain_capacity;
		if (di->dsoc == di->rsoc)
			di->line_k = 10;
	}

	/*force dsoc to match rsoc*/
	if ((di->line_k == 10) &&
	    (di->dsoc != di->rsoc) &&
	    (chg_st != CHARGE_FINISH)) {
		di->dsoc = di->rsoc;
		DBG("<%s>. force dsoc to match rsoc\n", __func__);
	}

	DBG("<%s>. line_k=%d, delta_rsoc=%d, delta_dsoc=%d, old_cap=%d\n"
	    "pwr_on_dsoc=%d, pwr_on_rsoc=%d\n",
	    __func__, di->line_k, delta_rsoc, delta_dsoc, di->old_remain_cap,
	    di->pwr_on_dsoc, di->pwr_on_rsoc);

}

static void rk81x_chrg_term_mode_set(struct battery_info *di, int mode)
{
	u8 buf;
	u8 mask = 0x20;

	rk81x_bat_read(CHRG_CTRL_REG3, &buf);
	buf &= ~mask;
	buf |= mode;
	rk81x_bat_write(CHRG_CTRL_REG3, &buf);

	DBG("set charge to %s termination mode\n",
		 mode ? "digital" : "analog");
}

static int rk81x_chrg_term_mode_get(struct battery_info *di)
{
	u8 buf;
	u8 mask = 0x20;

	rk81x_bat_read(CHRG_CTRL_REG3, &buf);
	return (buf & mask);
}

static void rk81x_bat_update_info(struct battery_info *di)
{
	u8 chg_st = rk81x_bat_get_chrg_status(di);
	bool ret;

	if (rk81x_chrg_term_mode_get(di) == CHRG_TERM_ANA_SIGNAL) {
		if (MS_TO_SEC(get_timer(di->plug_in_base)) > 0) /* 1s*/
			rk81x_chrg_term_mode_set(di, CHRG_TERM_DIG_SIGNAL);
	}

	/*check rsoc and dsoc*/
	di->remain_capacity = rk81x_bat_get_realtime_cap(di);
	di->rsoc = rk81x_bat_get_rsoc(di);
	if (di->dsoc > 100)
		di->dsoc = 100;
	else if (di->dsoc < 0)
		di->dsoc = 0;

	if (di->remain_capacity > di->fcc)
		rk81x_bat_capacity_init(di, di->fcc);

	if ((chg_st == CHARGE_FINISH) || (di->rsoc == 100)) {
		di->start_term = get_timer(0);
		rk81x_bat_finish_chrg(di);
		rk81x_bat_capacity_init(di, di->fcc);

	} else {
		if ((di->dsoc >= 100) && (di->init_dsoc != 100))
			di->dsoc = 99;

		di->start_finish = get_timer(0);/*clear finish time*/
		ret = rk81x_bat_term_chrg(di, chg_st);
		if (ret) {
			goto out;
		} else {
			di->start_term = get_timer(0);
			rk81x_bat_linek_chrg_algorithm(di);
		}
	}
out:
	rk81x_bat_save_dsoc(di, di->dsoc);
	rk81x_bat_save_remain_cap(di, di->remain_capacity);
	rk81x_bat_dmp_dbg_info(di);
}

/*
get battery status, contain capacity, voltage, status
struct battery *batt_status:
voltage_uV. battery voltage
capacity.   battery capacity
state_of_chrg: 0. no charger; 1. usb charging; 2. AC charging
*/

static int rk81x_batttery_update(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	int voltage;

	rk818_state_of_chrg = rk81x_bat_get_charger_type(&g_battery, bat);
	i2c_set_bus_num(bat->bus);
	i2c_init(RK818_I2C_SPEED, bat->hw.i2c.addr);
	rk81x_bat_charger_setting(&g_battery, rk818_state_of_chrg);
	rk81x_bat_update_info(&g_battery);
	voltage = rk81x_bat_get_vol(&g_battery);

	/*
	 * if voltage < 2500, it means the machine's power suspply is a
	 * DC power supply, we need report a virtual vol and soc to
	 * make start kernel normally
	 */
	if (is_rk81x_bat_exist(&g_battery) && voltage > 2500 &&
	    !g_battery.virtual_power) {
		battery->voltage_uV = rk_battery_ocvVoltage(&g_battery);
		battery->capacity = g_battery.dsoc;
	} else {
		battery->voltage_uV = TEST_POWER_VOL;
		battery->capacity = TEST_POWER_SOC;
	}
	battery->state_of_chrg = rk818_state_of_chrg;
	battery->isexistbat = is_rk81x_bat_exist(&g_battery);

	return 0;
}

static struct power_fg fg_ops = {
	.fg_battery_check = rk81x_batttery_check,
	.fg_battery_update = rk81x_batttery_update,
};

static int rk81x_bat_parse_dt(struct battery_info *di, void const *blob)
{
	int node, parent;
	int len;
	int err;
	const char *prop;

	parent = fdt_node_offset_by_compatible(blob, 0, "rockchip,rk818");
	if (parent < 0) {
		printf("Could not find rockchip,rk818 node!\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob, parent)) {
		printf("device rk818 is disabled\n");
		return -1;
	}

	node = fdt_subnode_offset_namelen(blob, parent, "battery", 7);
	if (node < 0) {
		printf("%s: Cannot find battery node!\n", __func__);
		return -EINVAL;
	}


	prop = fdt_getprop(blob, node, "ocv_table", &len);
	if (!prop) {
		printf("Could not find battery ocv_table node.\n");
		return -EINVAL;
	}

	di->ocv_table = calloc(len, 1);
	if (!di->ocv_table) {
		printf("%s: No available memory for ocv_table allocation!\n",
		       __func__);
		return -ENOMEM;
	}

	di->ocv_size = len / 4;
	err = fdtdec_get_int_array(blob, node, "ocv_table",
				   di->ocv_table, di->ocv_size);
	if (err < 0) {
		printf("read ocv_table error!\n");
		free(di->ocv_table);
		return -EINVAL;
	}

	di->design_capacity = fdtdec_get_int(blob, node, "design_capacity", 0);
	if (di->design_capacity == 0) {
		printf("read design_capacity error!\n");
		return -EINVAL;
	}
	di->qmax = fdtdec_get_int(blob, node, "design_qmax", 0);
	if (di->qmax == 0) {
		printf("read design_qmax error!\n");
		return -EINVAL;
	}

	if (fdtdec_decode_gpio(blob, node, "dc_det_gpio", &di->dc_det) < 0) {
		printf("dc det not found!\n");
	} else {
		di->dc_det.flags = !(di->dc_det.flags & OF_GPIO_ACTIVE_LOW);
		dc_gpio_init();
		gpio_direction_input(di->dc_det.gpio);
	}

	di->chg_v_lmt = fdtdec_get_int(blob, node,
				       "max_charge_voltagemV", 4200);
	di->chg_i_lmt = fdtdec_get_int(blob, node, "max_input_currentmA", 2000);
	di->chg_i_cur = fdtdec_get_int(blob, node, "max_chrg_currentmA", 1200);
	di->bat_res = fdtdec_get_int(blob, node, "bat_res", 135);
	di->chrg_diff_res = fdtdec_get_int(blob, node, "chrg_diff_vol", 0);
	di->support_usb_adp = fdtdec_get_int(blob, node, "support_usb_adp", 1);
	di->support_dc_adp = fdtdec_get_int(blob, node, "support_dc_adp", 0);
	di->virtual_power = fdtdec_get_int(blob, node, "virtual_power", 0);
	di->fcc = di->design_capacity;

	DBG("\n--------- the battery OCV TABLE dump:\n");
	DBG("max_input_currentmA :%d\n", di->chg_i_lmt);
	DBG("max_chrg_currentmA :%d\n", di->chg_i_cur);
	DBG("max_charge_voltagemV :%d\n", di->chg_v_lmt);
	DBG("design_capacity :%d\n", di->design_capacity);
	DBG("design_qmax :%d\n", di->qmax);
	DBG("bat_res :%d\n", di->bat_res);
	DBG("support_usb_adp :%d\n", di->support_usb_adp);
	DBG("support_dc_adp :%d\n", di->support_dc_adp);
	DBG("--------- rk818_battery dt_parse ok.\n");

	return 0;
}

/*
// dc detect iomux
void power_gpio_init(){
	int value=0;

	grf_writel((0x0 << 12) | (0x1 << (12 + 16)), GRF_SOC_CON15);

	//io mux
	pmugrf_writel((0x00 << 6) | (0x3 << (6+ 16)),PMU_GRF_GPIO0C_IOMUX);

	grf_writel((0x1 << 12) | (0x1 << (12 + 16)), GRF_SOC_CON15);
}
*/

int fg_rk818_init(unsigned char bus, uchar addr)
{
	static const char name[] = "RK818_FG";
	struct battery_info *di = &g_battery;
	int ret;

	if (!rk818_fg.p)
		rk818_fg.p = pmic_alloc();
	rk818_fg.p->name = name;
	rk818_fg.p->bus = bus;
	rk818_fg.p->hw.i2c.addr = addr;
	rk818_fg.p->interface = PMIC_I2C;
	rk818_fg.p->fg = &fg_ops;
	rk818_fg.p->pbat = calloc(sizeof(struct  power_battery), 1);
	i2c_set_bus_num(bus);
	i2c_init(RK818_I2C_SPEED, addr);

	ret = rk81x_bat_parse_dt(di, gd->fdt_blob);
	if (ret < 0) {
		printf("rk81x_bat_parse_dt failed!\n");
		return ret;
	}

	/*power_gpio_init();*/
	rk81x_bat_fg_init(di);
	rk81x_bat_info_init(di);
	return 0;
}

