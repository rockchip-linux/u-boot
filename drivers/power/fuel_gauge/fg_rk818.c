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
#define CHRG_TERM_ANA_SIGNAL 	(0 << 5)
#define CHRG_TERM_DIG_SIGNAL 	(1 << 5)

#define MAX_CAPACITY		0x7fff
#define MAX_SOC			100
#define MAX_PERCENTAGE		100
#define INTERPOLATE_MAX		1000
#define MAX_INT 		0x7FFF


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
#define FINISH_CALI_CURR	1000 	/*mA*/
#define TERM_CALI_CURR		600	/*mA*/

/***********************************************************/
#define PMU_DEBUG 0

int rk818_state_of_chrg = 1;
#define	SUPPORT_USB_CHARGE

static int dbg_enable = 0;
#define DBG(args...) \
	do { \
		if (dbg_enable) { \
			printf(args); \
		} \
	} while (0)

struct battery_info {

	uint16_t 	voltage;
	int 		voltage_ocv;
	int		nac;
	int		temp_nac;
	int		real_soc;
	int		temp_soc;
	int		fcc;
	int 		qmax;
	u32  		*ocv_table ;
	unsigned int  	ocv_size;
	int 		current_offset;
	int		remain_capacity;
	int 		current_avg;
	int 		design_capacity;
	int		dod0;
	int		dod0_status;
	int		dod0_voltage;
	int		dod0_capacity;
	unsigned long	dod0_time;
	u8		dod0_level;

	int		current_k;/* (ICALIB0, ICALIB1) */
	int		current_b;
	int		voltage_k;/* VCALIB0 VCALIB1 */
	int		voltage_b;
	int 		chg_v_lmt;
	int 		chg_i_lmt;
	int 		chg_i_cur;
	int          bat_res;
	int		soc_delta;
	ulong 		start_finish;
	ulong		start_term;

	int   dc_det_gpio;
	int   dc_det_status;
	int   dc_det_gpio_level;
	int   charge_status;

};

struct battery_info g_battery;


#define CHG_VOL_SHIFT	4
#define CHG_ILIM_SHIFT	0
#define CHG_ICUR_SHIFT	0

int CHG_V_LMT[] = {4050, 4100, 4150, 4200, 4300, 4350};
int CHG_I_CUR[] = {1000, 1200, 1400, 1600, 1800, 2000, 2250, 2400, 2600, 2800, 3000};
int CHG_I_LMT[] = {450, 800, 850, 1000, 1250, 1500, 1750, 2000, 2250, 2500, 2750, 3000};

struct rk818_fg {
	struct pmic *p;
};

struct rk818_fg rk818_fg;
typedef enum {
	NO_CHARGE = 0,	
	USB_CHARGE = 1,
	DC_CHARGE = 2
} CHARGE_STATUS;

/**************************************************************/
static  int32_t abs_int(int32_t x)
{
	return (x > 0) ? x : -x;
}

static void dc_gpio_init(void)
{
#if CONFIG_RKCHIP_RK3368
	grf_writel((0x0 << 12) | (0x1 << (12 + 16)), GRF_SOC_CON15);
	
	//io mux
	pmugrf_writel((0x00 << 2) | (0x3 << (2+ 16)),PMU_GRF_GPIO0C_IOMUX);

     //gpio pull down and up
	pmugrf_writel((0x01 << 2) | (0x3 << (2+ 16)),PMU_GRF_GPIO0C_P);
	
	grf_writel((0x1 << 12) | (0x1 << (12 + 16)), GRF_SOC_CON15);
#endif
	return;
}


static int battery_read(u8 reg, u8 *buf)
{
	*buf = i2c_reg_read(rk818_fg.p->hw.i2c.addr, reg);
	return (*buf);
}

static void battery_write(u8 reg, u8 *buf)
{
	i2c_reg_write(rk818_fg.p->hw.i2c.addr, reg, *buf);
}

static int _get_soc(struct battery_info *di)
{	
		return di->remain_capacity * 100 / di->fcc;
}

static int _gauge_enable(struct battery_info *di)
{
	int ret;
	u8 buf;

	ret = battery_read(TS_CTRL_REG, &buf);
	if (ret < 0) {
		DBG( "read TS_CTRL_REG err\n");
		return ret;
	}
	if (!(buf & GG_EN)) {
		buf |= GG_EN;
		battery_write(TS_CTRL_REG, &buf);
		battery_read(TS_CTRL_REG, &buf);
		return 0;
	}

	DBG("%s,TS_CTRL_REG=0x%x\n", __func__, buf);
	return 0;

}

static void save_level(struct battery_info *di, u8 save_soc)
{
	u8 soc;

	soc = save_soc;
	battery_write(UPDAT_LEVE_REG, &soc);
}

static u8 get_level(struct battery_info *di)
{
	u8 soc;

	battery_read(UPDAT_LEVE_REG, &soc);
	return soc;
}

static int _get_vcalib0(struct battery_info *di)
{

	int temp = 0;
	u8 buf;

	battery_read(VCALIB0_REGL, &buf);
	temp = buf;
	battery_read(VCALIB0_REGH, &buf);
	temp |= buf<<8;

	return temp;
}

static int _get_vcalib1(struct  battery_info *di)
{
	int temp = 0;
	u8 buf;

	battery_read(VCALIB1_REGL, &buf);
	temp = buf;
	battery_read(VCALIB1_REGH, &buf);
	temp |= buf<<8;

	return temp;
}

static int _get_ioffset(struct battery_info *di)
{
	u8 buf;
	int temp = 0;

	battery_read(IOFFSET_REGL, &buf);
	temp = buf;
	battery_read(IOFFSET_REGH, &buf);
	temp |= buf<<8;

	return temp;
}

static int _set_cal_offset(struct battery_info *di, u32 value)
{
	u8 buf;

	buf = value&0xff;
	battery_write(CAL_OFFSET_REGL, &buf);
	buf = (value >> 8)&0xff;
	battery_write(CAL_OFFSET_REGH, &buf);

	return 0;
}

static void _get_voltage_offset_value(struct battery_info *di)
{
	int vcalib0, vcalib1;

	vcalib0 = _get_vcalib0(di);
	vcalib1 = _get_vcalib1(di);

	di->voltage_k = (4200 - 3000)*1000/(vcalib1 - vcalib0);
	di->voltage_b = 4200 - (di->voltage_k*vcalib1)/1000;
	DBG("voltage_k=%d(x1000), voltage_b = %d\n", di->voltage_k, di->voltage_b);
}

static uint16_t _get_OCV_voltage(struct battery_info *di)
{
	int ret;
	u8 buf;
	uint16_t temp;
	uint16_t voltage_now = 0;

	ret = battery_read(BAT_OCV_REGL, &buf);
	temp = buf;
	ret = battery_read(BAT_OCV_REGH, &buf);
	temp |= buf<<8;

	if (ret < 0) {
		DBG( "read BAT_OCV_REGH err\n");
		return ret;
	}

	voltage_now = di->voltage_k*temp/1000 + di->voltage_b;

	return voltage_now;
}

static int _get_average_current(struct battery_info *di)
{
	u8  buf;
	int ret;
	int current_now;
	int temp;

	ret = battery_read(BAT_CUR_AVG_REGL, &buf);
	if (ret < 0) {
		DBG("error read BAT_CUR_AVG_REGL");
		return ret;
	}
	current_now = buf;
	ret = battery_read(BAT_CUR_AVG_REGH, &buf);
	if (ret < 0) {
		DBG("error read BAT_CUR_AVG_REGH");
		return ret;
	}
	current_now |= (buf<<8);

	if (current_now & 0x800)
		current_now -= 4096;

	temp = current_now*1506/1000;/*1000*90/14/4096*500/521;*/

	return temp;

}

static int rk_battery_voltage(struct battery_info *di)
{
	int ret;
	int voltage_now = 0;
	u8 buf;
	int temp;

	ret = battery_read(BAT_VOL_REGL, &buf);
	temp = buf;
	ret = battery_read(BAT_VOL_REGH, &buf);
	temp |= buf<<8;

	if (ret < 0) {
		DBG( "error read BAT_VOL_REGH");
		return ret;
	}

	voltage_now = di->voltage_k*temp/1000 + di->voltage_b;

	return voltage_now;

}
static int rk_battery_ocvVoltage(struct battery_info *di)
{
	int voltage_now = 0;
	int voltage_ocv = 0;
	int current_now=0;
	
	voltage_now=rk_battery_voltage(di);
	if(voltage_now<0){
		DBG( "read viltage error!");
		return voltage_now;
	}

	current_now=_get_average_current(di);
	if(current_now<0){
		DBG( "read current error!");
		return current_now;
	}

	voltage_ocv=voltage_now-(di->bat_res*current_now/1000);

	return voltage_ocv;
}

static void fg_match_param(struct battery_info *di, int chg_vol, int chg_ilim, int chg_cur)
{
	int i;
	
	for (i=0; i<ARRAY_SIZE(CHG_V_LMT); i++){
		if (chg_vol < CHG_V_LMT[i])
			break;
		else
			di->chg_v_lmt = (i << CHG_VOL_SHIFT);
	}

	for (i=0; i<ARRAY_SIZE(CHG_I_LMT); i++){
		if (chg_ilim < CHG_I_LMT[i])
			break;
		else
			di->chg_i_lmt = (i << CHG_ILIM_SHIFT);
	}

	for (i=0; i<ARRAY_SIZE(CHG_I_CUR); i++){
		if (chg_cur < CHG_I_CUR[i])
			break;
		else
			di->chg_i_cur = (i << CHG_ICUR_SHIFT);
	}
	DBG("vlmt = 0x%x, ilim = 0x%x, cur=0x%x\n",
		di->chg_v_lmt, di->chg_i_lmt, di->chg_i_cur);
}

static void rk_battery_charger_init(struct  battery_info *di)
{
	u8 chrg_ctrl_reg1, usb_ctrl_reg, chrg_ctrl_reg2, chrg_ctrl_reg3;
	u8 sup_sts_reg;

	fg_match_param(di, di->chg_v_lmt, di->chg_i_lmt, di->chg_i_cur);
	battery_read(USB_CTRL_REG, &usb_ctrl_reg);
	battery_read(CHRG_CTRL_REG1, &chrg_ctrl_reg1);
	battery_read(CHRG_CTRL_REG2, &chrg_ctrl_reg2);
	battery_read(SUP_STS_REG, &sup_sts_reg);
	battery_read(CHRG_CTRL_REG3, &chrg_ctrl_reg3);

	usb_ctrl_reg &= (~0x0f);
#ifdef SUPPORT_USB_CHARGE
	usb_ctrl_reg |= (ILIM_450MA);
#else
	usb_ctrl_reg |= (di->chg_i_lmt);
#endif
	chrg_ctrl_reg1 &= (0x00);
	chrg_ctrl_reg1 |= (CHRG_EN) | (di->chg_v_lmt | di->chg_i_cur);

	chrg_ctrl_reg3 |= CHRG_TERM_DIG_SIGNAL;/* digital finish mode*/
	chrg_ctrl_reg2 &= ~(0xc0);
	chrg_ctrl_reg2 |= FINISH_100MA;

	sup_sts_reg &= ~(0x01 << 3);
	sup_sts_reg |= (0x01 << 2);

	battery_write(CHRG_CTRL_REG3, &chrg_ctrl_reg3);
	battery_write(USB_CTRL_REG, &usb_ctrl_reg);
	battery_write(CHRG_CTRL_REG1, &chrg_ctrl_reg1);
	battery_write(CHRG_CTRL_REG2, &chrg_ctrl_reg2);
	battery_write(SUP_STS_REG, &sup_sts_reg);
}

static bool _is_first_poweron(struct  battery_info *di)
{
	u8 buf;
	u8 temp;

	battery_read(GGSTS, &buf);
	if (buf&BAT_CON) {
		buf &= ~(BAT_CON);
		do {
			battery_write(GGSTS, &buf);
			battery_read(GGSTS, &temp);
		} while (temp&BAT_CON);
		return true;
	}
	return false;
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

static int _voltage_to_capacity(struct battery_info *di, int voltage)
{
	u32 *ocv_table;
	int ocv_size;
	u32 tmp;

	ocv_table = di->ocv_table;
	ocv_size = di->ocv_size;
	tmp = interpolate(voltage, ocv_table, ocv_size);
	di->temp_soc = ab_div_c(tmp, MAX_PERCENTAGE, INTERPOLATE_MAX);
	di->temp_nac = ab_div_c(tmp, di->fcc, INTERPOLATE_MAX);

	return 0;
}

static int _get_remain_capacity(struct battery_info *di)
{
	int temp = 0;
	u8 buf;
	u32 capacity;

	battery_read(REMAIN_CAP_REG3, &buf);
	temp = buf << 24;
	battery_read(REMAIN_CAP_REG2, &buf);
	temp |= buf << 16;
	battery_read(REMAIN_CAP_REG1, &buf);
	temp |= buf << 8;
	battery_read(REMAIN_CAP_REG0, &buf);
	temp |= buf;

	capacity = temp;/* /4096*900/14/36*500/521; */

	return capacity;
}

static void  _save_FCC_capacity(struct battery_info *di, u32 capacity)
{
	u8 buf;
	u32 capacity_ma;

	capacity_ma = capacity;
	buf = (capacity_ma>>24)&0xff;
	battery_write(NEW_FCC_REG3, &buf);
	buf = (capacity_ma>>16)&0xff;
	battery_write(NEW_FCC_REG2, &buf);
	buf = (capacity_ma>>8)&0xff;
	battery_write(NEW_FCC_REG1, &buf);
	buf = (capacity_ma&0xff) | 0x01;
	battery_write(NEW_FCC_REG0, &buf);
}

static int _get_FCC_capacity(struct battery_info *di)
{
	int temp = 0;
	u8 buf;
	u32 capacity;

	battery_read(NEW_FCC_REG3, &buf);
	temp = buf << 24;
	battery_read(NEW_FCC_REG2, &buf);
	temp |= buf << 16;
	battery_read(NEW_FCC_REG1, &buf);
	temp |= buf << 8;
	battery_read(NEW_FCC_REG0, &buf);
	temp |= buf;

	if (temp > 1)
		capacity = temp-1;/* 4096*900/14/36*500/521 */
	else
		capacity = temp;

	return capacity;
}

static int _rsoc_init(struct  battery_info *di)
{
	u8 pwron_soc;
	u8 init_soc;
	u32 remain_capacity;
	u8 last_shtd_time;
	u8 curr_shtd_time;
#ifdef SUPPORT_USB_CHARGE
	int otg_status;
#else
	u8 buf;
#endif
	di->voltage  = rk_battery_voltage(di);
	di->voltage_ocv = _get_OCV_voltage(di);
	DBG("OCV voltage = %d\n" , di->voltage_ocv);

	if (_is_first_poweron(di)) {
		_save_FCC_capacity(di, di->design_capacity);
		di->fcc = _get_FCC_capacity(di);

		_voltage_to_capacity(di, di->voltage_ocv);
		di->real_soc = di->temp_soc;
		di->nac      = di->temp_nac;
		DBG("<%s>.this is first poweron: OCV-SOC = %d, OCV-CAPACITY = %d, FCC = %d\n", __func__, di->real_soc, di->nac, di->fcc);

	} else {
		battery_read(SOC_REG, &pwron_soc);
		init_soc = pwron_soc;

#ifdef SUPPORT_USB_CHARGE
		otg_status = dwc_otg_check_dpdm();
		if ((pwron_soc == 0) && (otg_status == 1)) { /*usb charging*/
			init_soc = 1;
			battery_write(SOC_REG, &init_soc);
		}
#else
		battery_read(VB_MOD_REG, &buf);
		if ((pwron_soc == 0) && ((buf&PLUG_IN_STS) != 0)) {
			init_soc = 1;
			battery_write(SOC_REG, &init_soc);
		}
#endif

		remain_capacity = _get_remain_capacity(di);
		battery_read(NON_ACT_TIMER_CNT_REG, &curr_shtd_time);
		battery_read(NON_ACT_TIMER_CNT_REG_SAVE, &last_shtd_time);
		battery_write(NON_ACT_TIMER_CNT_REG_SAVE, &curr_shtd_time);
		DBG("<%s>, now_shtd_time = %d, last_shtd_time = %d, otg_status = %d\n", __func__, curr_shtd_time, last_shtd_time, otg_status);

		_voltage_to_capacity(di, di->voltage_ocv);
		DBG("<%s>Not first pwron, real_remain_cap = %d, ocv-remain_cp=%d\n", __func__, remain_capacity, di->temp_nac);

		/* if plugin, make sure current shtd_time different from last_shtd_time.*/
		if (last_shtd_time != curr_shtd_time) {

			if (curr_shtd_time > 30) {
				remain_capacity = di->temp_nac;
				DBG("<%s>shutdown_time > 30 minute,  remain_cap = %d\n", __func__, remain_capacity);

			} else if ((curr_shtd_time > 5) && (abs(di->temp_soc - init_soc) >= 10)) {
				if (remain_capacity >= di->temp_nac*120/100)
					remain_capacity = di->temp_nac*110/100;
				else if (remain_capacity < di->temp_nac*8/10)
					remain_capacity = di->temp_nac*9/10;

				DBG("<%s> shutdown_time > 3 minute,  remain_cap = %d\n", __func__, remain_capacity);
			}
		}

		di->real_soc = init_soc;
		di->nac = remain_capacity;
		if (di->nac <= 0)
			di->nac = 0;
		DBG("<%s> init_soc = %d, init_capacity=%d\n", __func__, di->real_soc, di->nac);
	}

	return 0;
}

static void _capacity_init(struct battery_info *di, u32 capacity)
{
	u8 buf;
	u32 capacity_ma;

	capacity_ma = capacity*2390;/* 2134;//36*14/900*4096/521*500; */
	do {
		buf = (capacity_ma>>24)&0xff;
		battery_write(GASCNT_CAL_REG3, &buf);
		buf = (capacity_ma>>16)&0xff;
		battery_write(GASCNT_CAL_REG2, &buf);
		buf = (capacity_ma>>8)&0xff;
		battery_write(GASCNT_CAL_REG1, &buf);
		buf = (capacity_ma&0xff) | 0x01;
		battery_write(GASCNT_CAL_REG0, &buf);
		battery_read(GASCNT_CAL_REG0, &buf);

	} while (buf == 0);
}


static int _get_realtime_capacity(struct battery_info *di)
{
	int temp = 0;
	u8 buf;
	u32 capacity;

	battery_read(GASCNT3, &buf);
	temp = buf << 24;
	battery_read(GASCNT2, &buf);
	temp |= buf << 16;
	battery_read(GASCNT1, &buf);
	temp |= buf << 8;
	battery_read(GASCNT0, &buf);
	temp |= buf;

	capacity = temp/2390;/* 4096*900/14/36*500/521; */

	return capacity;
}

static void power_on_save(struct battery_info *di, int voltage)
{
	u8 buf;
	u8 save_soc;

	/*default status=0*/
	battery_write(DOD0_ST_REG, (u8*)(&di->dod0_status));

	battery_read(NON_ACT_TIMER_CNT_REG, &buf);
	if (_is_first_poweron(di) || buf > 30) { /* first power-on or power off time > 30min */
		_voltage_to_capacity(di, voltage);
		if (di->temp_soc < 20) {
			di->dod0_voltage = voltage;
			di->dod0_capacity = di->nac;
			di->dod0_status = 1;
			di->dod0 = di->temp_soc;/* _voltage_to_capacity(di, voltage); */
			di->dod0_level = 80;

			if (di->temp_soc <= 0)
				di->dod0_level = 100;
			else if (di->temp_soc < 5)
				di->dod0_level = 95;
			else if (di->temp_soc < 10)
				di->dod0_level = 90;
			/* save_soc = di->dod0_level; */
			save_soc = get_level(di);
			if (save_soc <  di->dod0_level)
				save_soc = di->dod0_level;
			save_level(di, save_soc);
			/*save for kernel*/
			battery_write(DOD0_ST_REG, (u8*)(&di->dod0_status));
			battery_write(DOD0_CAP_REG, (u8*)(&di->dod0_capacity));
			battery_write(DOD0_LVL_REG, (u8*)(&di->dod0_level));
			battery_write(DOD0_TEMP_REG, (u8*)(&di->temp_soc));

			DBG("<%s>UPDATE-FCC POWER ON : dod0_voltage = %d, dod0_capacity = %d ", __func__, di->dod0_voltage, di->dod0_capacity);
		}
	}

}

static void rk818_fg_init(struct battery_info *di)
{
	u8 adc_ctrl_val;

	adc_ctrl_val = 0x30;
	battery_write(ADC_CTRL_REG, &adc_ctrl_val);

	_gauge_enable(di);
	_get_voltage_offset_value(di);
	rk_battery_charger_init(di);

	di->current_offset = _get_ioffset(di);
	_set_cal_offset(di, di->current_offset+42);
	_rsoc_init(di);
	_capacity_init(di, di->nac);

	di->remain_capacity = _get_realtime_capacity(di);
	di->current_avg = _get_average_current(di);
	power_on_save(di, di->voltage_ocv);//???

	printf("<%s> :\n"
	    "nac = %d , remain_capacity = %d\n"
	    "OCV_voltage = %d, voltage = %d\n"
	    "SOC = %d, fcc = %d\n, current=%d\n,soc_temp=%d\n",
	    __func__,
	    di->nac, di->remain_capacity,
	    di->voltage_ocv, di->voltage,
	    di->real_soc, di->fcc, di->current_avg,di->temp_soc);
}

static void battery_info_init(struct battery_info *di)
{
	int fcc_capacity;

	di->start_finish = get_timer(0);
	di->start_term = get_timer(0);

	di->soc_delta = _get_soc(di) - di->real_soc;
	fcc_capacity = _get_FCC_capacity(di);
	if (fcc_capacity > 1000)
		di->fcc = fcc_capacity;
	else
		di->fcc = di->design_capacity;
}

static bool is_bat_exist(struct  battery_info *di)  
{
	u8 buf;

	battery_read(SUP_STS_REG, &buf);
	return (buf & BAT_EXS) ? true : false;
}

static void set_charge_current(struct battery_info *di, int charge_current)
{
	u8 usb_ctrl_reg;

	battery_read(USB_CTRL_REG, &usb_ctrl_reg);
	usb_ctrl_reg &= (~0x0f);/* (VLIM_4400MV | ILIM_1200MA) |(0x01 << 7); */
	usb_ctrl_reg |= (charge_current);
	battery_write(USB_CTRL_REG, &usb_ctrl_reg);
}

/**************************************************************/
/*
0. disable charging
1. usb charging
2. ac adapter charging
*/
static void  rk818_charger_setting(struct battery_info *di, int charger_st)
{
	if((charger_st>USB_CHARGE)&&(!is_bat_exist(di))){
			set_charge_current(di, ILIM_2000MA);
			return;
	}

	if(di->charge_status!=charger_st){		
		if(charger_st==NO_CHARGE){		
			  set_charge_current(di, ILIM_450MA);
			  
		}else if(charger_st>NO_CHARGE){
			if (USB_CHARGE== charger_st)
				set_charge_current(di, ILIM_450MA);

			else if (DC_CHARGE == charger_st) {
				set_charge_current(di, di->chg_i_lmt);
			}
		}else{
			printf("rk818 charger setting error %d\n",charger_st);	
		}
		di->charge_status=charger_st;
	}
	
}

static int rk818_chrg_det(struct pmic *pmic)
{
	u8 val;
	unsigned int chrg_state = 0;
	unsigned int dc_state = 0;
	
	dc_state=gpio_get_value(g_battery.dc_det_gpio);
	val = i2c_reg_read(pmic->hw.i2c.addr, RK818_VB_MON_REG);
	if (val & PLUG_IN_STS) {			
		if(dc_state == 0)
			chrg_state = DC_CHARGE;//dc charger
		else{
			chrg_state = dwc_otg_check_dpdm();
			if(chrg_state == 0)
				chrg_state = NO_CHARGE;
			else if(chrg_state==1)
				chrg_state = USB_CHARGE;
			else if(chrg_state==2)
				chrg_state = DC_CHARGE;
			else
				printf("chrg det error!");
		}
	} else {		
		chrg_state = NO_CHARGE;
		
	}

	return chrg_state;
}

static int rk818_check_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	battery->state_of_chrg = rk818_chrg_det(bat);
	return 0;
}

static int _copy_soc(struct  battery_info *di, u8 save_soc)
{
	u8 soc;

	soc = save_soc;
	battery_write(SOC_REG, &soc);
	return 0;
}

static void _save_remain_capacity(struct battery_info *di, u32 capacity)
{
	u8 buf;
	u32 capacity_ma;

	if (capacity >= di->qmax)
		capacity = di->qmax;

	capacity_ma = capacity;
	buf = (capacity_ma>>24)&0xff;
	battery_write(REMAIN_CAP_REG3, &buf);
	buf = (capacity_ma>>16)&0xff;
	battery_write(REMAIN_CAP_REG2, &buf);
	buf = (capacity_ma>>8)&0xff;
	battery_write(REMAIN_CAP_REG1, &buf);
	buf = (capacity_ma&0xff) | 0x01;
	battery_write(REMAIN_CAP_REG0, &buf);
}

static u8 get_charge_status(struct battery_info *di)
{
	u8 status;
	u8 ret = 0;

	battery_read(SUP_STS_REG, &status);
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

	case  TRICKLE_CHARGE:				/* (0x02 << 4) */
		ret = DEAD_CHARGE;
		DBG("  TRICKLE CHARGE ...\n ");
		break;

	case  CC_OR_CV:					/* (0x03 << 4) */
		ret = CC_OR_CV;
		DBG("  CC or CV ...\n");
		break;

	case  CHARGE_FINISH:				/* (0x04 << 4) */
		ret = CHARGE_FINISH;
		DBG("  CHARGE FINISH ...\n");
		break;

	case  USB_OVER_VOL:					/* (0x05 << 4) */
		ret = USB_OVER_VOL;
		DBG("  USB OVER VOL ...\n");
		break;

	case  BAT_TMP_ERR:					/* (0x06 << 4) */
		ret = BAT_TMP_ERR;
		DBG("  BAT TMP ERROR ...\n");
		break;

	case  TIMER_ERR:					/* (0x07 << 4) */
		ret = TIMER_ERR;
		DBG("  TIMER ERROR ...\n");
		break;

	case  USB_EXIST:					/* (1 << 1)// usb is exists */
		ret = USB_EXIST;
		DBG("  USB EXIST ...\n");
		break;

	case  USB_EFF:						/* (1 << 0)// usb is effective */
		ret = USB_EFF;
		DBG("  USB EFF...\n");
		break;

	default:
		return -EINVAL;
	}

	return ret;

}

static void do_finish_work(struct battery_info *di)
{
	_capacity_init(di, di->fcc);
	if (di->real_soc < 100){
		di->real_soc=100;
		#if 0
		sec = di->fcc*3600/100/FINISH_CALI_CURR;/* 1000mA*/
		if (get_timer(di->start_finish) > SEC_PLUS*sec) {
			di->start_finish= get_timer(0);
			di->real_soc++;
		}
		#endif 
	}

}

static bool do_term_chrg_cali(struct battery_info *di, int chg_st)
{
	u32 sec;

	if ((rk818_state_of_chrg >0) &&
	    (di->real_soc >= 90) &&
	    (di->current_avg > 600)) {

		sec = di->fcc*3600/100/TERM_CALI_CURR;
		if (get_timer(di->start_term) > SEC_PLUS*sec) {
			di->start_term = get_timer(0);
			di->real_soc++;
			if(di->real_soc>=100)
				di->real_soc=99;
		}

		return true;
	}

	return false;
}

static void dump_debug_info(struct battery_info *di)
{
	u8 sup_tst_reg, ggcon_reg, ggsts_reg, vb_mod_reg;
	u8 usb_ctrl_reg, chrg_ctrl_reg1;
	u8 chrg_ctrl_reg2, chrg_ctrl_reg3, rtc_val;

	battery_read(GGCON, &ggcon_reg);
	battery_read(GGSTS, &ggsts_reg);
	battery_read(SUP_STS_REG, &sup_tst_reg);
	battery_read(RK818_VB_MON_REG, &vb_mod_reg);
	battery_read(USB_CTRL_REG, &usb_ctrl_reg);
	battery_read(CHRG_CTRL_REG1, &chrg_ctrl_reg1);
	battery_read(CHRG_CTRL_REG2, &chrg_ctrl_reg2);
	battery_read(CHRG_CTRL_REG3, &chrg_ctrl_reg3);
	battery_read(0x00, &rtc_val);

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
	    "-----------------------------------------------------------------\n"
	    "voltage = %d, current-avg = %d\n"
	    "fcc = %d, remain_capacity = %d, ocv_volt = %d\n"
	    "diplay_soc = %d, cpapacity_soc = %d\n"
	    "ADP = %d, soc_delta = %d\n",
	    rk_battery_voltage(di), _get_average_current(di),
	    di->fcc, di->remain_capacity, _get_OCV_voltage(di),
	    di->real_soc, _get_soc(di),
	    rk818_state_of_chrg, di->soc_delta
	   );
	get_charge_status(di);
	DBG("################################################################\n");
}

static void update_battery_info(struct battery_info *di)
{
	int now_current, soc_time;
	u8 chg_st = get_charge_status(di);

	di->remain_capacity = _get_realtime_capacity(di);
	di->temp_soc=_get_soc(di);
	if(di->temp_soc>100)
		di->temp_soc=100;
	else if(di->temp_soc<0)
		di->temp_soc=0;
	
	if (di->remain_capacity > di->fcc){
		_capacity_init(di, di->fcc);	
	}

	if (chg_st == CHARGE_FINISH) {
		do_finish_work(di);

	} else {
		di->start_finish = get_timer(0);/*clear finish time*/	

		if(di->real_soc>=100)
			di->real_soc=99;
		
		if (do_term_chrg_cali(di, chg_st))
			return;
		else {
			now_current = _get_average_current(di);
			if (now_current == 0)
				now_current = 1;

			soc_time =( di->fcc*3600/100/(abs_int(now_current)))*1000;   /* 1%  time; ms*/
		
			di->temp_soc = _get_soc(di);

			if ((di->temp_soc != di->real_soc) && (now_current != 0)) {
				if(di->temp_soc < di->real_soc){
					if(get_timer(di->start_term) > soc_time*3/2){			
						di->real_soc++;
						di->start_term = get_timer(0);/*clear term calib time*/
					}
				}
				else if(di->temp_soc > di-> real_soc){
					if(get_timer(di->start_term) > soc_time*3/4){			
						di->real_soc++;
						di->start_term = get_timer(0);/*clear term calib time*/
					}

				}else if(di->temp_soc==di->real_soc){
						di->real_soc=di->temp_soc;
						di->start_term = get_timer(0);/*clear term calib time*/
				}	
				
			}
		}

	}

	
     
	_copy_soc(di, di->real_soc);
	_save_remain_capacity(di, di->remain_capacity);
	dump_debug_info(di);

}

/*
get battery status, contain capacity, voltage, status
struct battery *batt_status:
voltage_uV. battery voltage
capacity.   battery capacity
state_of_chrg: 0. no charger; 1. usb charging; 2. AC charging
*/

static int rk818_update_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	
	rk818_state_of_chrg = rk818_chrg_det(bat);
	i2c_set_bus_num(bat->bus);
	i2c_init(RK818_I2C_SPEED, bat->hw.i2c.addr);
	rk818_charger_setting(&g_battery,rk818_state_of_chrg);
	update_battery_info(&g_battery);

	battery->voltage_uV = rk_battery_ocvVoltage(&g_battery);
	battery->capacity = g_battery.real_soc;
	battery->state_of_chrg = rk818_state_of_chrg;
	battery->isexistbat=is_bat_exist(&g_battery);
			
	return 0;
}

static struct power_fg fg_ops = {
	.fg_battery_check = rk818_check_battery,
	.fg_battery_update = rk818_update_battery,
};

static int rk818_battery_parse_dt(struct battery_info *di, void const *blob)
{
	int node, parent;
	int len;
	int err;
	const char *prop;
	struct fdt_gpio_state gpioflags;

	DBG("<%s>.\n",__func__);
	DBG("SOC_REG = 0x%2x\n", i2c_reg_read(rk818_fg.p->hw.i2c.addr, SOC_REG));

	parent = fdt_node_offset_by_compatible(blob, 0, "rockchip,rk818");
	if (parent < 0) {
		printf("Could not find rockchip,rk818 node!\n");
		return -ENODEV;
	}

	if (!fdt_device_is_available(blob,parent)) {
		printf("device rk818 is disabled\n");
		return -1;
	}

	node = fdt_subnode_offset_namelen(blob, parent, "battery", 7);
	if (node < 0){
		printf("%s: Cannot find battery node!\n", __func__);
		return -EINVAL;
	}


	prop = fdt_getprop(blob, node, "ocv_table", &len);
	if (!prop){
		printf("Could not find battery ocv_table node.\n");
		return -EINVAL;
	}

	di->ocv_table= calloc(sizeof(len), 1);
	if (!di->ocv_table) {
		printf("%s: No available memory for ocv_table allocation!\n", __func__);
		return -ENOMEM;
	}

	di->ocv_size = len / 4;
	err = fdtdec_get_int_array(blob, node, "ocv_table", di->ocv_table, di->ocv_size);
	if (err < 0){
		printf("read ocv_table error!\n");
		free(di->ocv_table);
		return -EINVAL;
	}

	di->design_capacity = fdtdec_get_int(blob, node, "design_capacity",  0);
	if (di->design_capacity == 0) {
		printf("read design_capacity error!\n");
		return -EINVAL;
	}
	di->qmax = fdtdec_get_int(blob, node, "design_qmax",  0);
	if (di->qmax == 0) {
		printf("read design_qmax error!\n");
		return -EINVAL;
	}

	 if(fdtdec_decode_gpio(blob, node, "dc_det_gpio", &gpioflags)<0){
	 	printf("dc det not found!\n");
		return -EINVAL;
	 }
	 else{
	 	di->dc_det_gpio=gpioflags.gpio;
		dc_gpio_init();		
	    	gpio_direction_input(di->dc_det_gpio);
	 }

	di->chg_v_lmt = fdtdec_get_int(blob, node, "max_charge_voltagemV", 4200);;
	di->chg_i_lmt = fdtdec_get_int(blob, node, "max_charge_ilimitmA", 2000);;
	di->chg_i_cur = fdtdec_get_int(blob, node, "max_charge_currentmA", 1600);;
	di->bat_res=fdtdec_get_int(blob, node, "bat_res", 136);;
	di->fcc = di->design_capacity;

	DBG("\n--------- the battery OCV TABLE dump:\n");
	DBG("max_charge_ilimitmA :%d\n", di->chg_i_lmt);
	DBG("max_charge_currentmA :%d\n", di->chg_i_cur);
	DBG("max_charge_voltagemV :%d\n", di->chg_v_lmt);
	DBG("design_capacity :%d\n", di->design_capacity);
	DBG("design_qmax :%d\n", di->qmax);
	DBG("bat_res :%d\n", di->bat_res);
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
	
int fg_rk818_init(unsigned char bus,uchar addr)
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
	i2c_init(RK818_I2C_SPEED,addr);

	ret = rk818_battery_parse_dt(di, gd->fdt_blob);
	if (ret < 0) {
		printf("rk818_battery_parse_dt failed!\n");
		return ret;
	}

	//power_gpio_init();
	rk818_fg_init(di);
	battery_info_init(di);

	return 0;
}

