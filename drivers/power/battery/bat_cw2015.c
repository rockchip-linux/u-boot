#include <common.h>
#include <fdtdec.h>
#include <power/battery.h>
#include <errno.h>
#include <asm/arch/rkplat.h>
#include <i2c.h>

DECLARE_GLOBAL_DATA_PTR;
static int state_of_chrg = 0;
#define CW201X_I2C_ADDR		0x62
#define CW201X_I2C_CH		0
#define CW201X_I2C_SPEED	100000

#define REG_VERSION             0x0
#define REG_VCELL               0x2
#define REG_SOC                 0x4
#define REG_RRT_ALERT           0x6
#define REG_CONFIG              0x8
#define REG_MODE                0xA
#define REG_BATINFO             0x10

int volt_tab[6] = {3466, 3586, 3670, 3804, 4014, 4316};

struct cw201x {
	int node;
	struct fdt_gpio_state dc_det;
	int i2c_ch;
};

struct cw201x cw;
int cw201x_parse_dt(const void* blob)
{
	int err;
	cw.node = fdtdec_next_compatible(blob,
					0, COMPAT_ROCKCHIP_CW201X);
	err = fdtdec_decode_gpio(blob, cw.node, "dc_det_gpio", &cw.dc_det);
	if (err) {
		printf("decode dc_det_gpio err\n");
		return err;
	}
	cw.dc_det.gpio = rk_gpio_base_to_bank(cw.dc_det.gpio & RK_GPIO_BANK_MASK) |
					(cw.dc_det.gpio & RK_GPIO_PIN_MASK);
	cw.dc_det.flags = !(cw.dc_det.flags  & OF_GPIO_ACTIVE_LOW);
}

static int cw201x_i2c_init(void)
{
	cw201x_parse_dt(gd->fdt_blob);
	i2c_set_bus_num(CW201X_I2C_CH);
	i2c_init (CW201X_I2C_SPEED, 0);
	return 0;
}

static int cw_read_word(int reg)
{
	u8 vall, valh;
	u16 val;
	valh = i2c_reg_read(CW201X_I2C_ADDR, reg);
	vall = i2c_reg_read(CW201X_I2C_ADDR, reg + 1);
	val = ((u16)valh << 8) | vall;
	return val;
}
static int cw_get_vol(void)
{
        int ret;
        u16 value16, value16_1, value16_2, value16_3;
        int voltage;

	value16 = cw_read_word(REG_VCELL);
	if (value16 < 0)
		return -1;
        
	value16_1 = cw_read_word(REG_VCELL);
	if (value16_1 < 0)
		return -1;

	value16_2 = cw_read_word(REG_VCELL);
        if (value16_2 < 0)
                return -1;
		
		
        if (value16 > value16_1) {	 
		value16_3 = value16;
		value16 = value16_1;
		value16_1 = value16_3;
        }
		
        if (value16_1 > value16_2) {
		value16_3 =value16_1;
		value16_1 =value16_2;
		value16_2 =value16_3;
	}
			
        if (value16 >value16_1) {	 
		value16_3 =value16;
		value16 =value16_1;
		value16_1 =value16_3;
        }			

        voltage = value16_1 * 305;
        return voltage/1000;
}

/*
for chack charger status in boot
return 0, no charger
return 1, charging
*/
int check_charge(void)
{
	int ret = 0;
	if (!state_of_chrg) 
		cw201x_i2c_init();
	if (gpio_get_value(cw.dc_det.gpio) == cw.dc_det.flags)
		state_of_chrg = 2;
	else
		state_of_chrg = 0;
	ret = !!state_of_chrg;
	return ret;
}

static int get_capcity(int volt)
{
	int i = 0;
	int level0, level1;
	int cap;
	int step = 100 / (ARRAY_SIZE(volt_tab) -1);


	for (i = 0 ; i < ARRAY_SIZE(volt_tab); i++) {
		if (volt <= (volt_tab[i]))
			break;
	}

	if (i == 0) 
		return 0;
	
	level0 = volt_tab[i -1]; 
	level1 = volt_tab[i];

	cap = step * (i-1) + step *(volt - level0)/(level1 - level0);
	/*printf("cap%d step:%d level0 %d level1 %d  diff %d\n",
			cap, step, level0 ,level1, diff);*/
	return cap;
}

int get_power_bat_status(struct battery *batt_status)
{
    
	if (!state_of_chrg) 
		cw201x_i2c_init();
	if (gpio_get_value(cw.dc_det.gpio) == cw.dc_det.flags)
		state_of_chrg = 2;
	else
		state_of_chrg = 0;
	batt_status->voltage_uV = cw_get_vol();
	batt_status->capacity = get_capcity(batt_status->voltage_uV);
	batt_status->state_of_chrg = state_of_chrg;
	printf("%s capacity = %d, voltage_uV = %d,state_of_chrg=%d\n",__func__,batt_status->capacity,batt_status->voltage_uV,batt_status->state_of_chrg);
	return 0;
}

