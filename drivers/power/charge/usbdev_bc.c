/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>

#include "usbdev_bc.h"

DECLARE_GLOBAL_DATA_PTR;

static char *bc_string[USB_BC_TYPE_MAX] = {
	"DISCONNECT",
	"Stander Downstream Port",
	"Dedicated Charging Port",
	"Charging Downstream Port",
	"UNKNOW"
};

static enum usb_bc_t gusb_bc_type = USB_BC_UNKNOW;

/****** GET and SET REGISTER FIELDS IN GRF UOC ******/
static uoc_field_t *P_BC_UOC_FIELDS;
#define BC_GET(x)	grf_uoc_get_field(&P_BC_UOC_FIELDS[x])
#define BC_SET(x, v)	grf_uoc_set_field(&P_BC_UOC_FIELDS[x], v)

static inline void grf_uoc_set(u32 offset, u8 bitmap, u8 mask, u32 value)
{
	grf_writel((((((1 << mask) - 1) & value) | (((1 << mask) - 1) << 16)) << bitmap), offset);
}

static inline u32 grf_uoc_get(u32 offset, u32 bitmap, u32 mask)
{
	u32 ret;

	ret = (grf_readl(offset) >> bitmap) & ((1 << mask) - 1);

	return ret;
}

static inline bool uoc_field_valid(uoc_field_t *f)
{
	if ((f->b.bitmap < 32) && (f->b.mask < 32))
		return true;
	else {
		printf("%s field invalid\n", __func__);
		return false;
	}
}

static void grf_uoc_set_field(uoc_field_t *field, u32 value)
{
	if (!uoc_field_valid(field))
		return;

	grf_uoc_set(field->b.offset, field->b.bitmap, field->b.mask, value);
}

static u32 grf_uoc_get_field(uoc_field_t *field)
{
	return grf_uoc_get(field->b.offset, field->b.bitmap, field->b.mask);
}


/****** BATTERY CHARGER DETECT FUNCTIONS ******/
static bool is_connected(void)
{
	if (BC_GET(BC_BVALID) && BC_GET(BC_IDDIG))
		return true;
	return false;
}

static enum bc_port_type usb_battery_charger_detect_rk(bool wait)
{
	enum bc_port_type port_type = USB_BC_TYPE_DISCNT;

	if (BC_GET(RK_BC_BVALID) &&
	    BC_GET(RK_BC_IDDIG)) {
		mdelay(10);
		BC_SET(RK_BC_SOFTCTRL, 1);
		BC_SET(RK_BC_OPMODE, 0);
		BC_SET(RK_BC_XCVRSELECT, 1);
		BC_SET(RK_BC_TERMSELECT, 1);

		mdelay(1);
		switch (BC_GET(RK_BC_LINESTATE)) {
		case 1:
			port_type = USB_BC_TYPE_SDP;
			break;

		case 3:
			port_type = USB_BC_TYPE_DCP;
			break;

		default:
			port_type = USB_BC_TYPE_SDP;
		}

	}
	BC_SET(RK_BC_SOFTCTRL, 0);

	debug("%s: usb bc detect port type: %d\n", __func__, port_type);
	return port_type;
}

static enum bc_port_type usb_battery_charger_detect_inno(bool wait)
{
	enum bc_port_type port_type = USB_BC_TYPE_DISCNT;
	int dcd_state = DCD_POSITIVE;
	int timeout = 0, i = 0;

	/* VBUS Valid detect */
	if (BC_GET(INNO_BC_BVALID) &&
		BC_GET(INNO_BC_IDDIG)) {
		if (wait) {
			/* Do DCD */
			dcd_state = DCD_TIMEOUT;
			BC_SET(INNO_BC_RDMPDEN, 1);
			BC_SET(INNO_BC_IDPSRCEN, 1);
			timeout = T_DCD_TIMEOUT;
			while (timeout--) {
				if (BC_GET(INNO_BC_DPATTACH))
					i++;
				if (i >= 3) {
					/* It is a filter here to assure data
					 * lines contacted for at least 3ms */
					dcd_state = DCD_POSITIVE;
					break;
				}
				mdelay(1);
			}
			BC_SET(INNO_BC_RDMPDEN, 0);
			BC_SET(INNO_BC_IDPSRCEN, 0);
		} else {
			dcd_state = DCD_PASSED;
		}
		if (dcd_state == DCD_TIMEOUT) {
			port_type = USB_BC_TYPE_UNKNOW;
			goto out;
		}

		/* Turn on VDPSRC */
		/* Primary Detection */
		BC_SET(INNO_BC_VDPSRCEN, 1);
		BC_SET(INNO_BC_IDMSINKEN, 1);
		udelay(T_BC_CHGDET_VALID);
		/* SDP and CDP/DCP distinguish */
		if (BC_GET(INNO_BC_CPDET)) {
			/* Turn off VDPSRC */
			BC_SET(INNO_BC_VDPSRCEN, 0);
			BC_SET(INNO_BC_IDMSINKEN, 0);

			udelay(T_BC_CHGDET_VALID);

			/* Turn on VDMSRC */
			BC_SET(INNO_BC_VDMSRCEN, 1);
			BC_SET(INNO_BC_IDPSINKEN, 1);
			udelay(T_BC_CHGDET_VALID);
			if (BC_GET(INNO_BC_DCPDET)) {
				port_type = USB_BC_TYPE_DCP;
			} else {
				port_type = USB_BC_TYPE_CDP;
			}
		} else {
			port_type = USB_BC_TYPE_SDP;
		}

		BC_SET(INNO_BC_VDPSRCEN, 0);
		BC_SET(INNO_BC_IDMSINKEN, 0);
		BC_SET(INNO_BC_VDMSRCEN, 0);
		BC_SET(INNO_BC_IDPSINKEN, 0);
	}

out:
	debug("%s: Charger type %s, %s DCD, dcd_state = %d\n", __func__,
	       bc_string[port_type], wait ? "wait" : "pass", dcd_state);

	return port_type;

}

/* When do BC detect PCD pull-up register should be disabled  */
/* wait wait for dcd timeout 900ms */
static enum bc_port_type usb_battery_charger_detect_synop(bool wait)
{
	enum bc_port_type port_type = USB_BC_TYPE_DISCNT;
	int dcd_state = DCD_POSITIVE;
	int timeout = 0, i = 0;

	/* VBUS Valid detect */
	if (BC_GET(SYNOP_BC_BVALID) &&
	    BC_GET(SYNOP_BC_IDDIG)) {
		if (wait) {
			/* Do DCD */
			dcd_state = DCD_TIMEOUT;
			BC_SET(SYNOP_BC_DCDENB, 1);
			timeout = T_DCD_TIMEOUT;
			while (timeout--) {
				if (!BC_GET(SYNOP_BC_FSVPLUS))
					i++;
				if (i >= 3) {
					/* It is a filter here to assure data
					 * lines contacted for at least 3ms */
					dcd_state = DCD_POSITIVE;
					break;
				}

				mdelay(1);
			}
			BC_SET(SYNOP_BC_DCDENB, 0);
		} else {
			dcd_state = DCD_PASSED;
		}
		if (dcd_state == DCD_TIMEOUT) {
			port_type = USB_BC_TYPE_UNKNOW;
			goto out;
		}

		/* Turn on VDPSRC */
		/* Primary Detection */
		BC_SET(SYNOP_BC_VDATSRCENB, 1);
		BC_SET(SYNOP_BC_VDATDETENB, 1);
		BC_SET(SYNOP_BC_CHRGSEL, 0);

		udelay(T_BC_CHGDET_VALID);

		/* SDP and CDP/DCP distinguish */
		if (BC_GET(SYNOP_BC_CHGDET)) {
			/* Turn off VDPSRC */
			BC_SET(SYNOP_BC_VDATSRCENB, 0);
			BC_SET(SYNOP_BC_VDATDETENB, 0);

			udelay(T_BC_CHGDET_VALID);

			/* Turn on VDMSRC */
			BC_SET(SYNOP_BC_VDATSRCENB, 1);
			BC_SET(SYNOP_BC_VDATDETENB, 1);
			BC_SET(SYNOP_BC_CHRGSEL, 1);
			udelay(T_BC_CHGDET_VALID);
			if (BC_GET(SYNOP_BC_CHGDET))
				port_type = USB_BC_TYPE_DCP;
			else
				port_type = USB_BC_TYPE_CDP;
		} else {
			port_type = USB_BC_TYPE_SDP;
		}
		BC_SET(SYNOP_BC_VDATSRCENB, 0);
		BC_SET(SYNOP_BC_VDATDETENB, 0);
		BC_SET(SYNOP_BC_CHRGSEL, 0);

	}

out:
	debug("%s: Charger type %s, %s DCD, dcd_state = %d\n", __func__,
	       bc_string[port_type], wait ? "wait" : "pass", dcd_state);

	return port_type;
}

/****** GET REGISTER FIELD INFO FROM Device Tree ******/
#ifdef CONFIG_OF_LIBFDT
static inline int uoc_init_field(const void *blob, int np, const char *name, uoc_field_t *f)
{
	debug("uoc_init_field: name = %s\n", name);
	fdtdec_get_int_array(blob, np, name, f->array, 3);
	debug("uoc_init_field: usb bc detect: 0x%08x %d %d\n", f->b.offset, f->b.bitmap, f->b.mask);
	return 0;
}

static inline void uoc_init_synop(const void *blob, int np)
{
	uoc_init_field(blob, np, "rk_usb,bvalid",
		       &P_BC_UOC_FIELDS[SYNOP_BC_BVALID]);
	uoc_init_field(blob, np, "rk_usb,iddig",
		       &P_BC_UOC_FIELDS[SYNOP_BC_IDDIG]);
	uoc_init_field(blob, np, "rk_usb,dcdenb",
		       &P_BC_UOC_FIELDS[SYNOP_BC_DCDENB]);
	uoc_init_field(blob, np, "rk_usb,vdatsrcenb",
		       &P_BC_UOC_FIELDS[SYNOP_BC_VDATSRCENB]);
	uoc_init_field(blob, np, "rk_usb,vdatdetenb",
		       &P_BC_UOC_FIELDS[SYNOP_BC_VDATDETENB]);
	uoc_init_field(blob, np, "rk_usb,chrgsel",
		       &P_BC_UOC_FIELDS[SYNOP_BC_CHRGSEL]);
	uoc_init_field(blob, np, "rk_usb,chgdet",
		       &P_BC_UOC_FIELDS[SYNOP_BC_CHGDET]);
	uoc_init_field(blob, np, "rk_usb,fsvplus",
		       &P_BC_UOC_FIELDS[SYNOP_BC_FSVPLUS]);
	uoc_init_field(blob, np, "rk_usb,fsvminus",
		       &P_BC_UOC_FIELDS[SYNOP_BC_FSVMINUS]);
}

static inline void uoc_init_rk(const void *blob, int np)
{
	uoc_init_field(blob, np, "rk_usb,bvalid",
		       &P_BC_UOC_FIELDS[RK_BC_BVALID]);
	uoc_init_field(blob, np, "rk_usb,iddig",
		       &P_BC_UOC_FIELDS[RK_BC_IDDIG]);
	uoc_init_field(blob, np, "rk_usb,line",
		       &P_BC_UOC_FIELDS[RK_BC_LINESTATE]);
	uoc_init_field(blob, np, "rk_usb,softctrl",
		       &P_BC_UOC_FIELDS[RK_BC_SOFTCTRL]);
	uoc_init_field(blob, np, "rk_usb,opmode",
		       &P_BC_UOC_FIELDS[RK_BC_OPMODE]);
	uoc_init_field(blob, np, "rk_usb,xcvrsel",
		       &P_BC_UOC_FIELDS[RK_BC_XCVRSELECT]);
	uoc_init_field(blob, np, "rk_usb,termsel",
		       &P_BC_UOC_FIELDS[RK_BC_TERMSELECT]);
}

static inline void uoc_init_inno(const void *blob, int np)
{
	uoc_init_field(blob, np, "rk_usb,bvalid",
		       &P_BC_UOC_FIELDS[INNO_BC_BVALID]);
	uoc_init_field(blob, np, "rk_usb,iddig",
		       &P_BC_UOC_FIELDS[INNO_BC_IDDIG]);
	uoc_init_field(blob, np, "rk_usb,vdmsrcen",
		       &P_BC_UOC_FIELDS[INNO_BC_VDMSRCEN]);
	uoc_init_field(blob, np, "rk_usb,vdpsrcen",
		       &P_BC_UOC_FIELDS[INNO_BC_VDPSRCEN]);
	uoc_init_field(blob, np, "rk_usb,rdmpden",
		       &P_BC_UOC_FIELDS[INNO_BC_RDMPDEN]);
	uoc_init_field(blob, np, "rk_usb,idpsrcen",
		       &P_BC_UOC_FIELDS[INNO_BC_IDPSRCEN]);
	uoc_init_field(blob, np, "rk_usb,idmsinken",
		       &P_BC_UOC_FIELDS[INNO_BC_IDMSINKEN]);
	uoc_init_field(blob, np, "rk_usb,idpsinken",
		       &P_BC_UOC_FIELDS[INNO_BC_IDPSINKEN]);
	uoc_init_field(blob, np, "rk_usb,dpattach",
		       &P_BC_UOC_FIELDS[INNO_BC_DPATTACH]);
	uoc_init_field(blob, np, "rk_usb,cpdet",
		       &P_BC_UOC_FIELDS[INNO_BC_CPDET]);
	uoc_init_field(blob, np, "rk_usb,dcpdet",
		       &P_BC_UOC_FIELDS[INNO_BC_DCPDET]);
}

static int usb_battery_charger_init_fdt(const void *blob)
{
	int usb_bc_node = -1;
	const char *prop;

	usb_bc_node = fdt_path_offset(blob, "/dwc-control-usb/usb_bc");
	if (usb_bc_node < 0) {
		printf("usb bc: can find node by path: /dwc-control-usb/usb_bc\n");
		return -1;
	}
	prop = (char *)fdt_getprop(blob, usb_bc_node, "compatible", NULL);
	if (!prop)
		return -1;
	debug("usb bc node compatible: %s\n", prop);

	if (strcmp(prop, "rockchip,ctrl") == 0) {
		uoc_init_rk(blob, usb_bc_node);
		gusb_bc_type = USB_BC_RK;
	}
	else if (strcmp(prop, "synopsys,phy") == 0) {
		uoc_init_synop(blob, usb_bc_node);
		gusb_bc_type = USB_BC_SYNOP;
	}
	else if (strcmp(prop, "inno,phy") == 0) {
		uoc_init_inno(blob, usb_bc_node);
		gusb_bc_type = USB_BC_INNO;
	}
	else {
		return -1;
	}

	return 0;
}
#endif

static uoc_field_t rockchip_rk3399_usb2phy_inno[] = {
	[INNO_BC_BVALID]	= { {0xe2ac, 7, 1} },
	[INNO_BC_IDDIG]		= { {0xe2ac, 8, 1} },
	[INNO_BC_VDMSRCEN]	= { {0xe450, 12, 1} },
	[INNO_BC_VDPSRCEN]	= { {0xe450, 11, 1} },
	[INNO_BC_RDMPDEN]	= { {0xe450, 10, 1} },
	[INNO_BC_IDPSRCEN]	= { {0xe450, 9, 1} },
	[INNO_BC_IDMSINKEN]	= { {0xe450, 8, 1} },
	[INNO_BC_IDPSINKEN]	= { {0xe450, 7, 1} },
	[INNO_BC_DPATTACH]	= { {0xe2ac, 0, 1} },
	[INNO_BC_CPDET]		= { {0xe2ac, 2, 1} },
	[INNO_BC_DCPDET]	= { {0xe2ac, 1, 1} },
};

static uoc_field_t rockchip_rk3126_usb2phy_inno[] = {
	[INNO_BC_BVALID]	= { {0x14c, 5, 1} },
	[INNO_BC_IDDIG]		= { {0x14c, 8, 1} },
	[INNO_BC_VDMSRCEN]	= { {0x184, 12, 1} },
	[INNO_BC_VDPSRCEN]	= { {0x184, 11, 1} },
	[INNO_BC_RDMPDEN]	= { {0x184, 10, 1} },
	[INNO_BC_IDPSRCEN]	= { {0x184, 9, 1} },
	[INNO_BC_IDMSINKEN]	= { {0x184, 8, 1} },
	[INNO_BC_IDPSINKEN]	= { {0x184, 7, 1} },
	[INNO_BC_DPATTACH]	= { {0x2c0, 7, 1} },
	[INNO_BC_CPDET]		= { {0x2c0, 6, 1} },
	[INNO_BC_DCPDET]	= { {0x2c0, 5, 1} },
};

static int usb_battery_charger_init_nofdt(const char *phy)
{
	if (strcmp(phy, "RK3399,inno,usb2phy") == 0) {
		P_BC_UOC_FIELDS = rockchip_rk3399_usb2phy_inno;
		gusb_bc_type = USB_BC_INNO;
	} else if (strcmp(phy, "RK3126,inno,usb2phy") == 0) {
		P_BC_UOC_FIELDS = rockchip_rk3126_usb2phy_inno;
		gusb_bc_type = USB_BC_INNO;
	} else {
		return -1;
	}

	return 0;
}

static void usb_battery_charger_init(void)
{
	static int usb_bc_init_flag = 0;
	uint32 usb_bc_max = 0;
	int ret;
	if (!usb_bc_init_flag) {
		usb_bc_init_flag = !0;

		gusb_bc_type = USB_BC_UNKNOW;

		usb_bc_max = ((INNO_BC_MAX - SYNOP_BC_MAX) > 0) ? INNO_BC_MAX : SYNOP_BC_MAX;
		usb_bc_max = (usb_bc_max > RK_BC_MAX) ? usb_bc_max : RK_BC_MAX;
		debug("usb bc init: usb bc max = %u\n", usb_bc_max);

		P_BC_UOC_FIELDS = (uoc_field_t *)malloc(usb_bc_max
							* sizeof(uoc_field_t));
		if (P_BC_UOC_FIELDS == NULL) {
			printf("BC_UOC_FIELDS malloc error!\n");
			return;
		}

		memset(P_BC_UOC_FIELDS, 0, usb_bc_max * sizeof(uoc_field_t));

#ifdef CONFIG_OF_LIBFDT
		ret = usb_battery_charger_init_fdt(gd->fdt_blob);
		if (!ret)
			return;
#endif
#ifdef CONFIG_RKCHIP_RK3399
		usb_battery_charger_init_nofdt("RK3399,inno,usb2phy");
#endif
#ifdef CONFIG_RKCHIP_RK3126
		usb_battery_charger_init_nofdt("RK3126,inno,usb2phy");
#endif
	}
}

static enum bc_port_type usb_battery_charger_detect(bool wait)
{
	enum bc_port_type ret = USB_BC_TYPE_UNKNOW;

	if (gusb_bc_type == USB_BC_RK) {
		ret = usb_battery_charger_detect_rk(wait);
	}
	else if (gusb_bc_type == USB_BC_SYNOP) {
		ret = usb_battery_charger_detect_synop(wait);
	}
	else if (gusb_bc_type == USB_BC_INNO) {
		ret = usb_battery_charger_detect_inno(wait);
	}

	if (ret == USB_BC_TYPE_UNKNOW)
		ret = USB_BC_TYPE_DCP;

	return ret;
}

int dwc_otg_check_dpdm(void)
{
	enum bc_port_type status;

	usb_battery_charger_init();

	if (gusb_bc_type == USB_BC_UNKNOW) {
		printf("%s: usb bc unknow type\n", __func__);
		return 0;
	}

	if (is_connected() == false) {
		printf("%s: usb bc disconnected\n", __func__);
		return 0;
	}

	if (P_BC_UOC_FIELDS == NULL) {
		printf("%s: usb bc error\n", __func__);
		return 0;
	}

	status = usb_battery_charger_detect(1);

	switch (status) {
	case USB_BC_TYPE_DISCNT:
		return 0;
	case USB_BC_TYPE_SDP:
		return 1;
	case USB_BC_TYPE_DCP:
	case USB_BC_TYPE_CDP:
	case USB_BC_TYPE_UNKNOW:
		return 2;
	default:
		return 0;
	}
}
