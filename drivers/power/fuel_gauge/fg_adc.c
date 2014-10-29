#include <common.h>
#include <malloc.h>
#include <fdtdec.h>
#include <power/battery.h>
#include <errno.h>
#include <asm/arch/rkplat.h>
#include <power/rockchip_power.h>

DECLARE_GLOBAL_DATA_PTR;

#define SARADC_BASE             RKIO_SARADC_PHYS
#define GRF_BASE		RKIO_GRF_PHYS
#define BATT_NUM  11

#define read_XDATA(address)	(*((uint16 volatile*)(address)))
#define read_XDATA32(address)	(*((uint32 volatile*)(address)))
#define write_XDATA(address, value)	(*((uint16 volatile*)(address)) = value)
#define write_XDATA32(address, value)	(*((uint32 volatile*)(address)) = value)

typedef struct {
	uint32	index;
	uint32	data;
	uint32	stas;
	uint32	ctrl;
} adc_battery_conf;

struct adc_battery {
	struct pmic *p;
	int node;
	struct fdt_gpio_state dc_det;
	struct fdt_gpio_state charge_ctrl_gpios;
	struct fdt_gpio_state pwr_hold;
	u32 bat_table[28];
	u32 support_ac_charge;
	u32 support_usb_charge;
	adc_battery_conf adc;
	int capacity_poweron;
	int capacity;
	int is_pwr_hold;
};
struct adc_battery fg_adc;
static int get_status(void)
{
	int ac_charging, usb_charging;

	if (fg_adc.support_ac_charge == 1) {
		if (gpio_get_value(fg_adc.dc_det.gpio) == (fg_adc.dc_det.flags ? 0x0 : 0x01))
			ac_charging = 1;
		else
			ac_charging = 0;
		return ac_charging;
	}
	if (fg_adc.support_usb_charge == 1) {
		usb_charging = dwc_otg_check_dpdm();
		return usb_charging;
	}

	return 0;
}

static int adc_init(void)
{
	uint32 value;
	uint32 timeout = 0;

	write_XDATA32(fg_adc.adc.ctrl, 0);
	DRVDelayUs(1);
	write_XDATA32(fg_adc.adc.ctrl, 0x0028|(fg_adc.adc.index));
	DRVDelayUs(1);
	do {
		value = read_XDATA32(fg_adc.adc.ctrl);
		timeout++;
	} while ((value&0x40) == 0);
	value = read_XDATA32(fg_adc.adc.data);
	return value;
}

static int adc_get_vol(void)
{
	int value;
	int voltage;
	adc_init();
	do {
		value = read_XDATA32(fg_adc.adc.ctrl);
	} while ((value&0x40) == 0);
	value = read_XDATA32(fg_adc.adc.data);
	voltage = (value * 3300 * (fg_adc.bat_table[4] + fg_adc.bat_table[5])) / (1024 * fg_adc.bat_table[5]);

	if ((get_status() == 1) && (voltage < CONFIG_SYSTEM_ON_VOL_THRESD))
		voltage = CONFIG_SYSTEM_ON_VOL_THRESD/1000 + 10;
	return voltage;
}
static int vol_to_capacity(int BatVoltage)
{
	int capacity;
	int charge_level;
	int  *p;
	int i;

	p = fg_adc.bat_table;
	if (0 == charge_level) {
		if (BatVoltage >=  (p[2*BATT_NUM + 5])) {
			capacity = 100;
		} else {
			if (BatVoltage <= (p[BATT_NUM + 6])) {
				capacity = 0;
			} else {
				for (i = BATT_NUM + 6; i < 2 * BATT_NUM + 5; i++) {
					if (((p[i]) <= BatVoltage) && (BatVoltage <  (p[i + 1]))) {
							capacity = (i - (BATT_NUM + 6)) * 10 + ((BatVoltage - p[i]) *  10) / (p[i + 1] - p[i]);
						break;
					}
				}
			}
		}
	}
	return capacity;
}
static int get_capacity(int volt)
{
	fg_adc.capacity = vol_to_capacity(volt);
	return fg_adc.capacity;
}
static int adc_update_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;

	battery->voltage_uV = adc_get_vol() * 1000;
	battery->capacity = get_capacity(battery->voltage_uV);
	battery->state_of_chrg = 0;
	return 0;
}

static int adc_check_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	battery->state_of_chrg = 0;
	return 0;
}

static struct power_fg adc_fg_ops = {
	.fg_battery_check = adc_check_battery,
	.fg_battery_update = adc_update_battery,
};

static int rk_adcbat_parse_dt(const void *blob)
{
	int node;
	struct fdt_gpio_state dc_det_gpios;
	struct fdt_gpio_state charge_ctrl_gpios;
	u32 support_ac_charge;
	u32 support_usb_charge;
	int err;

	struct fdt_gpio_state gpios;


	node = fdt_node_offset_by_compatible(blob,
					0, "rk30-adc-battery");
	if (node < 0) {
		printf("can't find dts node for ADC\n");
		return -ENODEV;
	}
	if (!fdt_device_is_available(blob, node)) {
		printf("device ADC-BATTERY is disabled\n");
		return -1;
	}
	fdtdec_decode_gpio(blob, node, "dc_det_gpio",
		&(fg_adc.dc_det));
	fdtdec_decode_gpio(blob, node, "charge_ctrl_gpios",
		&(fg_adc.charge_ctrl_gpios));
	fg_adc.support_ac_charge  =
		fdtdec_get_int(blob, node, "is_dc_charge", 1);
	fg_adc.support_usb_charge  =
		fdtdec_get_int(blob, node, "is_usb_charge", 1);

	err = fdtdec_get_int_array(blob, node, "bat_table", fg_adc.bat_table,
				   ARRAY_SIZE(fg_adc.bat_table));

	fg_adc.p = pmic_alloc();
	fg_adc.adc.index = 0;
	fg_adc.adc.data = SARADC_BASE;
	fg_adc.adc.stas = SARADC_BASE+4;
	fg_adc.adc.ctrl = SARADC_BASE+8;

	node = fdt_node_offset_by_compatible(blob,
					0, "gpio-poweroff");
	if (node < 0) {
		fg_adc.is_pwr_hold = 0;
		printf("can't find node(gpio-poweroff) for adc\n");
		return 0;
	}
	fg_adc.is_pwr_hold = 1;
	fdtdec_decode_gpios(blob, node, "gpios", &gpios, 1);
	fg_adc.pwr_hold.gpio = gpios.gpio;
	fg_adc.pwr_hold.flags = !(gpios.flags  & OF_GPIO_ACTIVE_LOW);

	return 0;
}
void adc_shut_down(void)
{
	printf("shut downk\n");
	if (fg_adc.is_pwr_hold == 0)
		return;
	gpio_direction_output(fg_adc.pwr_hold.gpio, !!fg_adc.pwr_hold.flags);
}


int adc_battery_init(void)
{
	static const char name[] = "RK-ADC-FG";
	int ret;
	int voltage;
	if (!fg_adc.p) {
		ret = rk_adcbat_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}
	adc_init();
	voltage = adc_get_vol();
	 get_status();
	fg_adc.capacity_poweron = get_capacity(voltage);
	fg_adc.p->name = name;
	fg_adc.p->fg = &adc_fg_ops;
	fg_adc.p->pbat = calloc(sizeof(struct  power_battery), 1);

	return 0;
}

