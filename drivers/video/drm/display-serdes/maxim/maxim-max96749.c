// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * maxim-max96749.c  --  I2C register interface access for max96749 serdes chip
 *
 * Copyright (c) 2025 Rockchip Electronics Co. Ltd.
 *
 * Author: ZITONG CAI <zitong.cai@rock-chips.com>
 */
#include "../core.h"
#include "maxim-max96749.h"
struct serdes_function_data {
	u8 gpio_out_dis:1;
	u8 gpio_io_rx_en:1;
	u8 gpio_tx_en_a:1;
	u8 gpio_tx_en_b:1;
	u8 gpio_rx_en_a:1;
	u8 gpio_rx_en_b:1;
	u8 gpio_tx_id;
	u8 gpio_rx_id;
	u16 mdelay;
};

struct config_desc {
	u16 reg;
	u8 mask;
	u8 val;
};

struct serdes_group_data {
	const struct config_desc *configs;
	int num_configs;
};

static int MAX96749_MFP0_pins[] = {0};
static int MAX96749_MFP1_pins[] = {1};
static int MAX96749_MFP2_pins[] = {2};
static int MAX96749_MFP3_pins[] = {3};
static int MAX96749_MFP4_pins[] = {4};
static int MAX96749_MFP5_pins[] = {5};
static int MAX96749_MFP6_pins[] = {6};
static int MAX96749_MFP7_pins[] = {7};

static int MAX96749_MFP8_pins[] = {8};
static int MAX96749_MFP9_pins[] = {9};
static int MAX96749_MFP10_pins[] = {10};
static int MAX96749_MFP11_pins[] = {11};
static int MAX96749_MFP12_pins[] = {12};
static int MAX96749_MFP13_pins[] = {13};
static int MAX96749_MFP14_pins[] = {14};
static int MAX96749_MFP15_pins[] = {15};

static int MAX96749_MFP16_pins[] = {16};
static int MAX96749_MFP17_pins[] = {17};
static int MAX96749_MFP18_pins[] = {18};
static int MAX96749_MFP19_pins[] = {19};
static int MAX96749_MFP20_pins[] = {20};
static int MAX96749_MFP21_pins[] = {21};
static int MAX96749_MFP22_pins[] = {22};
static int MAX96749_MFP23_pins[] = {23};

static int MAX96749_MFP24_pins[] = {24};
static int MAX96749_MFP25_pins[] = {25};
static int MAX96749_I2C_pins[] = {3, 7};
static int MAX96749_UART_pins[] = {3, 7};

#define GROUP_DESC(nm) \
{ \
	.name = #nm, \
	.pins = nm ## _pins, \
	.num_pins = ARRAY_SIZE(nm ## _pins), \
}

static const char *const serdes_gpio_groups[] = {
	"MAX96749_MFP0", "MAX96749_MFP1", "MAX96749_MFP2", "MAX96749_MFP3",
	"MAX96749_MFP4", "MAX96749_MFP5", "MAX96749_MFP6", "MAX96749_MFP7",

	"MAX96749_MFP8", "MAX96749_MFP9", "MAX96749_MFP10", "MAX96749_MFP11",
	"MAX96749_MFP12", "MAX96749_MFP13", "MAX96749_MFP14", "MAX96749_MFP15",

	"MAX96749_MFP16", "MAX96749_MFP17", "MAX96749_MFP18", "MAX96749_MFP19",
	"MAX96749_MFP20", "MAX96749_MFP21", "MAX96749_MFP22", "MAX96749_MFP23",

	"MAX96749_MFP24", "MAX96749_MFP25",
};

static const char *const MAX96749_I2C_groups[] = { "MAX96749_I2C" };
static const char *const MAX96749_UART_groups[] = { "MAX96749_UART" };

#define FUNCTION_DESC(nm) \
{ \
	.name = #nm, \
	.group_names = nm##_groups, \
	.num_group_names = ARRAY_SIZE(nm##_groups), \
} \

#define FUNCTION_DESC_GPIO_OUTPUT_A(id) \
{ \
	.name = "SER_TXID" #id "_TO_DES_LINKA", \
	.group_names = serdes_gpio_groups, \
	.num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
	.data = (void *)(const struct serdes_function_data []) { \
		{ .gpio_out_dis = 1, .gpio_tx_en_a = 1, \
		  .gpio_io_rx_en = 1, .gpio_tx_id = id } \
	}, \
} \

#define FUNCTION_DESC_GPIO_OUTPUT_B(id) \
{ \
	.name = "SER_TXID" #id "_TO_DES_LINKB", \
	.group_names = serdes_gpio_groups, \
	.num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
	.data = (void *)(const struct serdes_function_data []) { \
		{ .gpio_out_dis = 1, .gpio_tx_en_b = 1, \
		  .gpio_io_rx_en = 1, .gpio_tx_id = id } \
	}, \
} \

#define FUNCTION_DESC_GPIO_INPUT_A(id) \
{ \
	.name = "DES_RXID" #id "_TO_SER_LINKA", \
	.group_names = serdes_gpio_groups, \
	.num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
	.data = (void *)(const struct serdes_function_data []) { \
		{ .gpio_rx_en_a = 1, .gpio_rx_id = id } \
	}, \
} \

#define FUNCTION_DESC_GPIO_INPUT_B(id) \
{ \
	.name = "DES_RXID" #id "_TO_SER_LINKB", \
	.group_names = serdes_gpio_groups, \
	.num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
	.data = (void *)(const struct serdes_function_data []) { \
		{ .gpio_rx_en_b = 1, .gpio_rx_id = id } \
	}, \
} \

#define FUNCTION_DESC_GPIO() \
{ \
	.name = "MAX96749_GPIO", \
	.group_names = serdes_gpio_groups, \
	.num_group_names = ARRAY_SIZE(serdes_gpio_groups), \
	.data = (void *)(const struct serdes_function_data []) { \
		{ } \
	}, \
} \

static struct pinctrl_pin_desc max96749_pins_desc[] = {
	PINCTRL_PIN(MAXIM_MAX96749_MFP0, "MAX96749_MFP0"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP1, "MAX96749_MFP1"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP2, "MAX96749_MFP2"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP3, "MAX96749_MFP3"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP4, "MAX96749_MFP4"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP5, "MAX96749_MFP5"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP6, "MAX96749_MFP6"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP7, "MAX96749_MFP7"),

	PINCTRL_PIN(MAXIM_MAX96749_MFP8, "MAX96749_MFP8"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP9, "MAX96749_MFP9"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP10, "MAX96749_MFP10"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP11, "MAX96749_MFP11"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP12, "MAX96749_MFP12"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP13, "MAX96749_MFP13"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP14, "MAX96749_MFP14"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP15, "MAX96749_MFP15"),

	PINCTRL_PIN(MAXIM_MAX96749_MFP16, "MAX96749_MFP16"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP17, "MAX96749_MFP17"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP18, "MAX96749_MFP18"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP19, "MAX96749_MFP19"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP20, "MAX96749_MFP20"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP21, "MAX96749_MFP21"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP22, "MAX96749_MFP22"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP23, "MAX96749_MFP23"),

	PINCTRL_PIN(MAXIM_MAX96749_MFP24, "MAX96749_MFP24"),
	PINCTRL_PIN(MAXIM_MAX96749_MFP25, "MAX96749_MFP25"),
};

static struct group_desc max96749_groups_desc[] = {
	GROUP_DESC(MAX96749_MFP0),
	GROUP_DESC(MAX96749_MFP1),
	GROUP_DESC(MAX96749_MFP2),
	GROUP_DESC(MAX96749_MFP3),
	GROUP_DESC(MAX96749_MFP4),
	GROUP_DESC(MAX96749_MFP5),
	GROUP_DESC(MAX96749_MFP6),
	GROUP_DESC(MAX96749_MFP7),

	GROUP_DESC(MAX96749_MFP8),
	GROUP_DESC(MAX96749_MFP9),
	GROUP_DESC(MAX96749_MFP10),
	GROUP_DESC(MAX96749_MFP11),
	GROUP_DESC(MAX96749_MFP12),
	GROUP_DESC(MAX96749_MFP13),
	GROUP_DESC(MAX96749_MFP14),
	GROUP_DESC(MAX96749_MFP15),

	GROUP_DESC(MAX96749_MFP16),
	GROUP_DESC(MAX96749_MFP17),
	GROUP_DESC(MAX96749_MFP18),
	GROUP_DESC(MAX96749_MFP19),
	GROUP_DESC(MAX96749_MFP20),
	GROUP_DESC(MAX96749_MFP21),
	GROUP_DESC(MAX96749_MFP22),
	GROUP_DESC(MAX96749_MFP23),

	GROUP_DESC(MAX96749_MFP24),
	GROUP_DESC(MAX96749_MFP25),

	GROUP_DESC(MAX96749_I2C),
	GROUP_DESC(MAX96749_UART),
};

static struct function_desc max96749_functions_desc[] = {
	FUNCTION_DESC_GPIO_INPUT_A(0),
	FUNCTION_DESC_GPIO_INPUT_A(1),
	FUNCTION_DESC_GPIO_INPUT_A(2),
	FUNCTION_DESC_GPIO_INPUT_A(3),
	FUNCTION_DESC_GPIO_INPUT_A(4),
	FUNCTION_DESC_GPIO_INPUT_A(5),
	FUNCTION_DESC_GPIO_INPUT_A(6),
	FUNCTION_DESC_GPIO_INPUT_A(7),

	FUNCTION_DESC_GPIO_INPUT_A(8),
	FUNCTION_DESC_GPIO_INPUT_A(9),
	FUNCTION_DESC_GPIO_INPUT_A(10),
	FUNCTION_DESC_GPIO_INPUT_A(11),
	FUNCTION_DESC_GPIO_INPUT_A(12),
	FUNCTION_DESC_GPIO_INPUT_A(13),
	FUNCTION_DESC_GPIO_INPUT_A(14),
	FUNCTION_DESC_GPIO_INPUT_A(15),

	FUNCTION_DESC_GPIO_INPUT_A(16),
	FUNCTION_DESC_GPIO_INPUT_A(17),
	FUNCTION_DESC_GPIO_INPUT_A(18),
	FUNCTION_DESC_GPIO_INPUT_A(19),
	FUNCTION_DESC_GPIO_INPUT_A(20),
	FUNCTION_DESC_GPIO_INPUT_A(21),
	FUNCTION_DESC_GPIO_INPUT_A(22),
	FUNCTION_DESC_GPIO_INPUT_A(23),

	FUNCTION_DESC_GPIO_INPUT_A(24),
	FUNCTION_DESC_GPIO_INPUT_A(25),

	FUNCTION_DESC_GPIO_OUTPUT_A(0),
	FUNCTION_DESC_GPIO_OUTPUT_A(1),
	FUNCTION_DESC_GPIO_OUTPUT_A(2),
	FUNCTION_DESC_GPIO_OUTPUT_A(3),
	FUNCTION_DESC_GPIO_OUTPUT_A(4),
	FUNCTION_DESC_GPIO_OUTPUT_A(5),
	FUNCTION_DESC_GPIO_OUTPUT_A(6),
	FUNCTION_DESC_GPIO_OUTPUT_A(7),

	FUNCTION_DESC_GPIO_OUTPUT_A(8),
	FUNCTION_DESC_GPIO_OUTPUT_A(9),
	FUNCTION_DESC_GPIO_OUTPUT_A(10),
	FUNCTION_DESC_GPIO_OUTPUT_A(11),
	FUNCTION_DESC_GPIO_OUTPUT_A(12),
	FUNCTION_DESC_GPIO_OUTPUT_A(13),
	FUNCTION_DESC_GPIO_OUTPUT_A(14),
	FUNCTION_DESC_GPIO_OUTPUT_A(15),

	FUNCTION_DESC_GPIO_OUTPUT_A(16),
	FUNCTION_DESC_GPIO_OUTPUT_A(17),
	FUNCTION_DESC_GPIO_OUTPUT_A(18),
	FUNCTION_DESC_GPIO_OUTPUT_A(19),
	FUNCTION_DESC_GPIO_OUTPUT_A(20),
	FUNCTION_DESC_GPIO_OUTPUT_A(21),
	FUNCTION_DESC_GPIO_OUTPUT_A(22),
	FUNCTION_DESC_GPIO_OUTPUT_A(23),

	FUNCTION_DESC_GPIO_OUTPUT_A(24),
	FUNCTION_DESC_GPIO_OUTPUT_A(25),

	FUNCTION_DESC_GPIO_INPUT_B(0),
	FUNCTION_DESC_GPIO_INPUT_B(1),
	FUNCTION_DESC_GPIO_INPUT_B(2),
	FUNCTION_DESC_GPIO_INPUT_B(3),
	FUNCTION_DESC_GPIO_INPUT_B(4),
	FUNCTION_DESC_GPIO_INPUT_B(5),
	FUNCTION_DESC_GPIO_INPUT_B(6),
	FUNCTION_DESC_GPIO_INPUT_B(7),

	FUNCTION_DESC_GPIO_INPUT_B(8),
	FUNCTION_DESC_GPIO_INPUT_B(9),
	FUNCTION_DESC_GPIO_INPUT_B(10),
	FUNCTION_DESC_GPIO_INPUT_B(11),
	FUNCTION_DESC_GPIO_INPUT_B(12),
	FUNCTION_DESC_GPIO_INPUT_B(13),
	FUNCTION_DESC_GPIO_INPUT_B(14),
	FUNCTION_DESC_GPIO_INPUT_B(15),

	FUNCTION_DESC_GPIO_INPUT_B(16),
	FUNCTION_DESC_GPIO_INPUT_B(17),
	FUNCTION_DESC_GPIO_INPUT_B(18),
	FUNCTION_DESC_GPIO_INPUT_B(19),
	FUNCTION_DESC_GPIO_INPUT_B(20),
	FUNCTION_DESC_GPIO_INPUT_B(21),
	FUNCTION_DESC_GPIO_INPUT_B(22),
	FUNCTION_DESC_GPIO_INPUT_B(23),

	FUNCTION_DESC_GPIO_INPUT_B(24),
	FUNCTION_DESC_GPIO_INPUT_B(25),

	FUNCTION_DESC_GPIO_OUTPUT_B(0),
	FUNCTION_DESC_GPIO_OUTPUT_B(1),
	FUNCTION_DESC_GPIO_OUTPUT_B(2),
	FUNCTION_DESC_GPIO_OUTPUT_B(3),
	FUNCTION_DESC_GPIO_OUTPUT_B(4),
	FUNCTION_DESC_GPIO_OUTPUT_B(5),
	FUNCTION_DESC_GPIO_OUTPUT_B(6),
	FUNCTION_DESC_GPIO_OUTPUT_B(7),

	FUNCTION_DESC_GPIO_OUTPUT_B(8),
	FUNCTION_DESC_GPIO_OUTPUT_B(9),
	FUNCTION_DESC_GPIO_OUTPUT_B(10),
	FUNCTION_DESC_GPIO_OUTPUT_B(11),
	FUNCTION_DESC_GPIO_OUTPUT_B(12),
	FUNCTION_DESC_GPIO_OUTPUT_B(13),
	FUNCTION_DESC_GPIO_OUTPUT_B(14),
	FUNCTION_DESC_GPIO_OUTPUT_B(15),

	FUNCTION_DESC_GPIO_OUTPUT_B(16),
	FUNCTION_DESC_GPIO_OUTPUT_B(17),
	FUNCTION_DESC_GPIO_OUTPUT_B(18),
	FUNCTION_DESC_GPIO_OUTPUT_B(19),
	FUNCTION_DESC_GPIO_OUTPUT_B(20),
	FUNCTION_DESC_GPIO_OUTPUT_B(21),
	FUNCTION_DESC_GPIO_OUTPUT_B(22),
	FUNCTION_DESC_GPIO_OUTPUT_B(23),

	FUNCTION_DESC_GPIO_OUTPUT_B(24),
	FUNCTION_DESC_GPIO_OUTPUT_B(25),

	FUNCTION_DESC_GPIO(),

	FUNCTION_DESC(MAX96749_I2C),
	FUNCTION_DESC(MAX96749_UART),
};

static struct serdes_chip_pinctrl_info max96749_pinctrl_info = {
	.pins = max96749_pins_desc,
	.num_pins = ARRAY_SIZE(max96749_pins_desc),
	.groups = max96749_groups_desc,
	.num_groups = ARRAY_SIZE(max96749_groups_desc),
	.functions = max96749_functions_desc,
	.num_functions = ARRAY_SIZE(max96749_functions_desc),
};

static bool max96749_bridge_linka_locked(struct serdes *serdes)
{
	u32 val = 0, i = 0;

	for (i = 0; i < 80; i++) {
		mdelay(5);

		if (serdes_reg_read(serdes, 0x002a, &val)) {
			SERDES_DBG_CHIP("serdes %s unlock val=0x%x\n",
					serdes->dev->name, val);
			continue;
		}

		if (!FIELD_GET(LINK_LOCKED, val)) {
			SERDES_DBG_CHIP("serdes %s unlock val=0x%x\n",
					serdes->dev->name, val);
			continue;
		}

		SERDES_DBG_CHIP("serdes %s reg locked 0x%x\n",
				serdes->dev->name, val);

		return true;
	}

	return false;
}

static bool max96749_bridge_linkb_locked(struct serdes *serdes)
{
	u32 val = 0, i = 0;

	for (i = 0; i < 80; i++) {
		mdelay(5);

		if (serdes_reg_read(serdes, 0x0034, &val)) {
			SERDES_DBG_CHIP("serdes %s unlock val=0x%x\n",
					serdes->dev->name, val);
			continue;
		}

		if (!FIELD_GET(LINK_LOCKED, val)) {
			SERDES_DBG_CHIP("serdes %s unlock val=0x%x\n",
					serdes->dev->name, val);
			continue;
		}

		SERDES_DBG_CHIP("serdes %s reg locked 0x%x\n",
				serdes->dev->name, val);

		return true;
	}

	return false;
}

static int max96749_select(struct serdes *serdes, int link)
{
	int ret;
	u32 i, status;
	struct udevice *dev;
	struct serdes *deser;

	/*0076 for linkA and 0086 for linkB*/
	if (link == SER_DUAL_LINK) {
		dev = serdes->serdes_bridge->bridge->conn->panel->dev;
		deser = dev_get_priv(dev->parent);
		serdes_reg_write(deser, 0x10, 0x00);
		serdes_set_bits(serdes, 0x45, DUAL_LINK_MODE,
				FIELD_PREP(DUAL_LINK_MODE, 1));
	} else if (link == SER_LINKA) {
		serdes_set_bits(serdes, 0x0076, DIS_REM_CC,
				FIELD_PREP(DIS_REM_CC, 0));
		serdes_set_bits(serdes, 0x0086, DIS_REM_CC,
				FIELD_PREP(DIS_REM_CC, 1));
		SERDES_DBG_CHIP("%s: only enable %s remote i2c of linkA\n",
				__func__,
				serdes->chip_data->name);
	} else if (link == SER_LINKB) {
		serdes_set_bits(serdes, 0x0076, DIS_REM_CC,
				FIELD_PREP(DIS_REM_CC, 1));
		serdes_set_bits(serdes, 0x0086, DIS_REM_CC,
				FIELD_PREP(DIS_REM_CC, 0));
		SERDES_DBG_CHIP("%s: only enable %s remote i2c of linkB\n",
				__func__,
				serdes->chip_data->name);
	} else if (link == SER_SPLITTER_MODE) {
		serdes_set_bits(serdes, 0x0076, DIS_REM_CC,
				FIELD_PREP(DIS_REM_CC, 0));
		serdes_set_bits(serdes, 0x0086, DIS_REM_CC,
				FIELD_PREP(DIS_REM_CC, 0));
		SERDES_DBG_CHIP("%s: enable %s remote i2c of linkA/B\n",
				__func__,
				serdes->chip_data->name);
	}

	for (i = 0; i < 80; i++) {
		mdelay(5);
		ret = serdes_reg_read(serdes, 0x0021, &status);
		if (ret)
			continue;

		if (serdes->dual_link && link != SER_DUAL_LINK)
			return 0;

		switch (link) {
		case SER_DUAL_LINK:
		case SER_SPLITTER_MODE:
			if ((status & LINKA_LOCKED) &&
			    (status & LINKB_LOCKED))
				goto out;
			break;
		case SER_LINKA:
			if (status & LINKA_LOCKED)
				goto out;
			break;
		case SER_LINKB:
			if (status & LINKB_LOCKED)
				goto out;
			break;
		}
	}

	printf("%s: %s link lock timeout, mode=%d val=0x%x\n",
	       serdes->dev->name, __func__, link, status);
	return -1;

out:
	printf("%s: %s link locked, mode=%d, val=0x%x\n",
	       serdes->dev->name, __func__, link, status);

	return 0;
}

static int max96749_deselect(struct serdes *serdes, int link)
{
	if (link == SER_DUAL_LINK) {
		serdes_set_bits(serdes, 0x45, DUAL_LINK_MODE,
				FIELD_PREP(DUAL_LINK_MODE, 0));

		SERDES_DBG_CHIP("%s: serdes %s disable dual link\n", __func__,
				serdes->dev->name);
	}

	return 0;
}

static struct serdes_chip_split_ops max96749_split_ops = {
	.select = max96749_select,
	.deselect = max96749_deselect,
};

static bool
max96749_bridge_detect(struct serdes *serdes, int link)
{
	int ret;
	bool status;

	if (link == SER_LINKA)
		status = max96749_bridge_linka_locked(serdes);
	else
		status = max96749_bridge_linkb_locked(serdes);

	if (serdes->dual_link) {
		printf("serdes %s disconnect, try to change dual link\n",
		       serdes->dev->name);

		ret = max96749_select(serdes, SER_DUAL_LINK);
		if (ret) {
			printf("serdes %s disconnect, close dual link\n",
			       serdes->dev->name);
			max96749_deselect(serdes, SER_DUAL_LINK);
		} else {
			status = true;
		}
	}

	return status;
}

static int max96749_bridge_enable(struct serdes *serdes)
{
	return 0;
}

static int max96749_bridge_disable(struct serdes *serdes)
{
	if (serdes->dual_link)
		max96749_deselect(serdes, SER_DUAL_LINK);

	SERDES_DBG_CHIP("%s: serdes %s\n", __func__, serdes->dev->name);
	return 0;
}

static struct serdes_chip_bridge_ops max96749_bridge_ops = {
	.detect = max96749_bridge_detect,
	.enable = max96749_bridge_enable,
	.disable = max96749_bridge_disable,
};

static int max96749_pinctrl_set_pin_mux(struct serdes *serdes,
					unsigned int pin_selector,
					unsigned int func_selector)
{
	struct function_desc *func;
	struct pinctrl_pin_desc *pin;
	struct serdes_function_data *data;
	int offset;
	u16 ms;

	func = &serdes->chip_data->pinctrl_info->functions[func_selector];
	if (!func) {
		printf("%s: func is null\n", __func__);
		return -EINVAL;
	}

	pin = &serdes->chip_data->pinctrl_info->pins[pin_selector];
	if (!pin) {
		printf("%s: pin is null\n", __func__);
		return -EINVAL;
	}

	SERDES_DBG_CHIP("%s: serdes %s func=%s data=%p pin=%s num=%d\n",
			__func__, serdes->chip_data->name,
			func->name, func->data,
			pin->name, pin->number);

	data = func->data;
	if (!data)
		return 0;

	ms = data->mdelay;
	offset = pin->number;
	if (offset > 32)
		dev_err(serdes->dev, "%s offset=%d > 32\n",
			serdes->dev->name, offset);
	else
		SERDES_DBG_CHIP("%s: serdes %s txid=%d rxid=%d off=%d\n",
				__func__, serdes->dev->name,
				data->gpio_tx_id, data->gpio_rx_id, offset);

	if (ms) {
		mdelay(ms);
		SERDES_DBG_CHIP("%s: delay %dms\n", __func__, ms);
		return 0;
	}

	serdes_set_bits(serdes,
			GPIO_A_REG(offset),
			GPIO_OUT_DIS,
			FIELD_PREP(GPIO_OUT_DIS, data->gpio_out_dis));
	if (data->gpio_tx_en_a || data->gpio_tx_en_b)
		serdes_set_bits(serdes,
				GPIO_B_REG(offset),
				GPIO_TX_ID,
				FIELD_PREP(GPIO_TX_ID, data->gpio_tx_id));
	if (data->gpio_rx_en_a || data->gpio_rx_en_b)
		serdes_set_bits(serdes,
				GPIO_C_REG(offset),
				GPIO_RX_ID,
				FIELD_PREP(GPIO_RX_ID, data->gpio_rx_id));
	serdes_set_bits(serdes,
			GPIO_D_REG(offset),
			GPIO_TX_EN_A | GPIO_TX_EN_B | GPIO_IO_RX_EN |
			GPIO_RX_EN_A | GPIO_RX_EN_B,
			FIELD_PREP(GPIO_TX_EN_A, data->gpio_tx_en_a) |
			FIELD_PREP(GPIO_TX_EN_B, data->gpio_tx_en_b) |
			FIELD_PREP(GPIO_RX_EN_A, data->gpio_rx_en_a) |
			FIELD_PREP(GPIO_RX_EN_B, data->gpio_rx_en_b) |
			FIELD_PREP(GPIO_IO_RX_EN, data->gpio_io_rx_en));

	return 0;
}

static int max96749_pinctrl_set_grp_mux(struct serdes *serdes,
					unsigned int group_selector,
					unsigned int func_selector)
{
	struct serdes_pinctrl *pinctrl = serdes->serdes_pinctrl;
	struct function_desc *func;
	struct group_desc *grp;
	struct serdes_function_data *data;
	int i, offset;

	func = &serdes->chip_data->pinctrl_info->functions[func_selector];
	if (!func) {
		printf("%s: func is null\n", __func__);
		return -EINVAL;
	}

	grp = &serdes->chip_data->pinctrl_info->groups[group_selector];
	if (!grp) {
		printf("%s: grp is null\n", __func__);
		return -EINVAL;
	}

	SERDES_DBG_CHIP("%s: serdes %s func=%s data=%p grp=%s data=%p num=%d\n",
			__func__, serdes->chip_data->name,
			func->name, func->data,
			grp->name, grp->data, grp->num_pins);

	data = func->data;
	if (!data)
		return 0;

	for (i = 0; i < grp->num_pins; i++) {
		offset = grp->pins[i] - pinctrl->pin_base;
		if (offset > 32)
			dev_err(serdes->dev, "%s offset=%d > 32\n",
				serdes->dev->name, offset);
		else
			SERDES_DBG_CHIP("serdes %s txid=%d rxid=%d off=%d\n",
					serdes->dev->name, data->gpio_tx_id,
					data->gpio_rx_id, offset);

		serdes_set_bits(serdes,
				GPIO_A_REG(offset),
				GPIO_OUT_DIS,
				FIELD_PREP(GPIO_OUT_DIS, data->gpio_out_dis));
		if (data->gpio_tx_en_a || data->gpio_tx_en_b)
			serdes_set_bits(serdes,
					GPIO_B_REG(offset), GPIO_TX_ID,
					FIELD_PREP(GPIO_TX_ID,
						   data->gpio_tx_id));
		if (data->gpio_rx_en_a || data->gpio_rx_en_b)
			serdes_set_bits(serdes,
					GPIO_C_REG(offset), GPIO_RX_ID,
					FIELD_PREP(GPIO_RX_ID,
						   data->gpio_rx_id));
		serdes_set_bits(serdes,
				GPIO_D_REG(offset),
				GPIO_TX_EN_A | GPIO_TX_EN_B | GPIO_IO_RX_EN |
				GPIO_RX_EN_A | GPIO_RX_EN_B,
				FIELD_PREP(GPIO_TX_EN_A, data->gpio_tx_en_a) |
				FIELD_PREP(GPIO_TX_EN_B, data->gpio_tx_en_b) |
				FIELD_PREP(GPIO_RX_EN_A, data->gpio_rx_en_a) |
				FIELD_PREP(GPIO_RX_EN_B, data->gpio_rx_en_b) |
				FIELD_PREP(GPIO_IO_RX_EN, data->gpio_io_rx_en));
	}

	return 0;
}

static int max96749_pinctrl_config_set(struct serdes *serdes,
				       unsigned int pin_selector,
				       unsigned int param,
				       unsigned int argument)
{
	u8 res_cfg;

	switch (param) {
	case PIN_CONFIG_DRIVE_OPEN_DRAIN:
		serdes_set_bits(serdes, GPIO_B_REG(pin_selector),
				OUT_TYPE, FIELD_PREP(OUT_TYPE, 0));
		break;
	case PIN_CONFIG_DRIVE_PUSH_PULL:
		serdes_set_bits(serdes, GPIO_B_REG(pin_selector),
				OUT_TYPE, FIELD_PREP(OUT_TYPE, 1));
		break;
	case PIN_CONFIG_BIAS_DISABLE:
		serdes_set_bits(serdes, GPIO_C_REG(pin_selector),
				PULL_UPDN_SEL,
				FIELD_PREP(PULL_UPDN_SEL, 0));
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		switch (argument) {
		case 40000:
			res_cfg = 0;
			break;
		case 1000000:
			res_cfg = 1;
			break;
		default:
			return -EINVAL;
		}

		serdes_set_bits(serdes, GPIO_A_REG(pin_selector),
				RES_CFG, FIELD_PREP(RES_CFG, res_cfg));
		serdes_set_bits(serdes, GPIO_C_REG(pin_selector),
				PULL_UPDN_SEL,
				FIELD_PREP(PULL_UPDN_SEL, 1));
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		switch (argument) {
		case 40000:
			res_cfg = 0;
			break;
		case 1000000:
			res_cfg = 1;
			break;
		default:
			return -EINVAL;
		}

		serdes_set_bits(serdes, GPIO_A_REG(pin_selector),
				RES_CFG, FIELD_PREP(RES_CFG, res_cfg));
		serdes_set_bits(serdes, GPIO_C_REG(pin_selector),
				PULL_UPDN_SEL,
				FIELD_PREP(PULL_UPDN_SEL, 2));
		break;
	case PIN_CONFIG_OUTPUT:
			serdes_set_bits(serdes, GPIO_A_REG(pin_selector),
					GPIO_OUT_DIS | GPIO_OUT,
					FIELD_PREP(GPIO_OUT_DIS, 0) |
					FIELD_PREP(GPIO_OUT, argument));
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static struct serdes_chip_pinctrl_ops max96749_pinctrl_ops = {
	.pinconf_set = max96749_pinctrl_config_set,
	.pinmux_set = max96749_pinctrl_set_pin_mux,
	.pinmux_group_set = max96749_pinctrl_set_grp_mux,
};

static int max96749_gpio_direction_input(struct serdes *serdes, int gpio)
{
	return 0;
}

static int max96749_gpio_direction_output(struct serdes *serdes,
					  int gpio, int value)
{
	return 0;
}

static int max96749_gpio_get_level(struct serdes *serdes, int gpio)
{
	return 0;
}

static int max96749_gpio_set_level(struct serdes *serdes, int gpio, int value)
{
	return 0;
}

static int max96749_gpio_set_config(struct serdes *serdes,
				    int gpio, unsigned long config)
{
	return 0;
}

static int max96749_gpio_to_irq(struct serdes *serdes, int gpio)
{
	return 0;
}

static struct serdes_chip_gpio_ops max96749_gpio_ops = {
	.direction_input = max96749_gpio_direction_input,
	.direction_output = max96749_gpio_direction_output,
	.get_level = max96749_gpio_get_level,
	.set_level = max96749_gpio_set_level,
	.set_config = max96749_gpio_set_config,
	.to_irq = max96749_gpio_to_irq,
};

struct serdes_chip_data serdes_max96749_data = {
	.name		= "max96749",
	.serdes_type	= TYPE_SER,
	.serdes_id	= MAXIM_ID_MAX96749,
	.connector_type	= DRM_MODE_CONNECTOR_eDP,
	.pinctrl_info	= &max96749_pinctrl_info,
	.bridge_ops	= &max96749_bridge_ops,
	.pinctrl_ops	= &max96749_pinctrl_ops,
	.gpio_ops	= &max96749_gpio_ops,
	.split_ops	= &max96749_split_ops,
};
EXPORT_SYMBOL_GPL(serdes_max96749_data);

MODULE_LICENSE("GPL");
