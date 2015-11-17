/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
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

extern void DRVDelayUs(uint32 count);
extern uint32 StorageSysDataLoad(uint32 Index, void *Buf);
extern uint32 StorageSysDataStore(uint32 Index, void *Buf);

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
	int state_of_chrg;
};
struct adc_battery fg_adc;
static int adc_get_status(void)
{
	int ac_charging, usb_charging;

	if (fg_adc.support_ac_charge == 1) {
		if (gpio_get_value(fg_adc.dc_det.gpio) == (fg_adc.dc_det.flags ? 0x0 : 0x01)){
			ac_charging = 2;
			return ac_charging;
		}
		if(fg_adc.support_usb_charge != 1){
			ac_charging = 0;
			return ac_charging;
		}
	}
	if (fg_adc.support_usb_charge == 1) {
		usb_charging = dwc_otg_check_dpdm();
		return usb_charging;
	}

	return 0;
}

int adc_charge_status(void)
{
	return fg_adc.state_of_chrg;
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
	if (fg_adc.state_of_chrg == 2 && (voltage < CONFIG_SYSTEM_ON_VOL_THRESD))
		voltage = CONFIG_SYSTEM_ON_VOL_THRESD + 10;
	return voltage;
}
static int vol_to_capacity(int BatVoltage)
{
	int capacity = 0;
	int charge_level = 0;
	int  *p;
	int i;

	p = (int *)fg_adc.bat_table;
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


#define STORAGE_SYSDATA_SECTOR2		2 // sector2, first 8 bytes has been define.
#define ADC_CAPACITY_OFFSET		8 // sector2, byte 8.
#define ADC_CHARGE_FLAG_OFFSET		9 // sector2, byte 9.
static int g_fg_adc_flag = 0;
int fg_adc_storage_load(void)
{
	if(g_fg_adc_flag != 0)
	{
		ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_buf, 512);
		StorageSysDataLoad(STORAGE_SYSDATA_SECTOR2, tmp_buf);
		debug("tmp_buf[%d] is %d\n", ADC_CAPACITY_OFFSET, tmp_buf[ADC_CAPACITY_OFFSET]);
		return tmp_buf[ADC_CAPACITY_OFFSET];
	}

	return 0;
}

int fg_adc_storage_store(u8 capacity)
{
	if(g_fg_adc_flag != 0)
	{
		ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_buf, 512);
		tmp_buf[ADC_CAPACITY_OFFSET] = capacity;
		StorageSysDataStore(STORAGE_SYSDATA_SECTOR2, tmp_buf);
	}

	return 0;
}

int fg_adc_storage_flag_store(bool flag)
{
	if(g_fg_adc_flag != 0)
	{
		ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_buf, 512);
		tmp_buf[ADC_CHARGE_FLAG_OFFSET] = flag;
		StorageSysDataStore(STORAGE_SYSDATA_SECTOR2, tmp_buf);
	}

	return 0;
}

int fg_adc_storage_flag_load(void)
{
	if(g_fg_adc_flag != 0)
	{
		ALLOC_CACHE_ALIGN_BUFFER(u8, tmp_buf, 512);
		StorageSysDataLoad(STORAGE_SYSDATA_SECTOR2, tmp_buf);
		debug("tmp_buf[%d] is %d\n", ADC_CHARGE_FLAG_OFFSET, tmp_buf[ADC_CHARGE_FLAG_OFFSET]);
		return tmp_buf[ADC_CHARGE_FLAG_OFFSET];
	}

	return 0;
}

static int get_capacity(int volt)
{
	fg_adc.capacity = vol_to_capacity(volt);
	return fg_adc.capacity;
}
static int adc_update_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	
	battery->state_of_chrg = adc_get_status();
	fg_adc.state_of_chrg = battery->state_of_chrg;
	//printf("fg_adc.state_of_chrg is %d\n",fg_adc.state_of_chrg);
	battery->voltage_uV = adc_get_vol();
	//printf("battery->voltage_uV is %d\n",battery->voltage_uV);
	battery->capacity = get_capacity(battery->voltage_uV);
	//printf("battery->capacity is %d\n",battery->capacity);
	
	return 0;
}

static int adc_check_battery(struct pmic *p, struct pmic *bat)
{
	struct battery *battery = bat->pbat->bat;
	battery->state_of_chrg = adc_get_status();
	return 0;
}

static struct power_fg adc_fg_ops = {
	.fg_battery_check = adc_check_battery,
	.fg_battery_update = adc_update_battery,
};

static int rk_adcbat_parse_dt(const void *blob)
{
	int node;

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

	fdtdec_get_int_array(blob, node, "bat_table", fg_adc.bat_table,
				   ARRAY_SIZE(fg_adc.bat_table));

	fg_adc.p = pmic_alloc();
	fg_adc.adc.index = 0;
	fg_adc.adc.data = SARADC_BASE;
	fg_adc.adc.stas = SARADC_BASE+4;
	fg_adc.adc.ctrl = SARADC_BASE+8;

	g_fg_adc_flag = 1;
	return 0;
}


int adc_battery_init(void)
{
	static const char name[] = "RK-ADC-FG";
	int ret;
	int voltage;
	if (!fg_adc.p) {
		if (!gd->fdt_blob)
			return -1;
		ret = rk_adcbat_parse_dt(gd->fdt_blob);
		if (ret < 0)
			return ret;
	}
	adc_init();
	voltage = adc_get_vol();
	 adc_get_status();
	fg_adc.capacity_poweron = get_capacity(voltage);
	fg_adc.p->name = name;
	fg_adc.p->fg = &adc_fg_ops;
	fg_adc.p->pbat = calloc(sizeof(struct  power_battery), 1);
	fg_adc_storage_load();
	return 0;
}

