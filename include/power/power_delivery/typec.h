/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright 2026 Rockchip Electronics Co., Ltd
 */

#ifndef __LINUX_USB_TYPEC_H
#define __LINUX_USB_TYPEC_H

#include <linux/types.h>

/* USB Type-C Specification releases */
#define USB_TYPEC_REV_1_0	0x100 /* 1.0 */
#define USB_TYPEC_REV_1_1	0x110 /* 1.1 */
#define USB_TYPEC_REV_1_2	0x120 /* 1.2 */
#define USB_TYPEC_REV_1_3	0x130 /* 1.3 */
#define USB_TYPEC_REV_1_4	0x140 /* 1.4 */
#define USB_TYPEC_REV_2_0	0x200 /* 2.0 */

enum typec_port_type {
	TYPEC_PORT_SRC,
	TYPEC_PORT_SNK,
	TYPEC_PORT_DRP,
};

enum typec_port_data {
	TYPEC_PORT_DFP,
	TYPEC_PORT_UFP,
	TYPEC_PORT_DRD,
};

enum typec_plug_type {
	USB_PLUG_NONE,
	USB_PLUG_TYPE_A,
	USB_PLUG_TYPE_B,
	USB_PLUG_TYPE_C,
	USB_PLUG_CAPTIVE,
};

enum typec_data_role {
	TYPEC_DEVICE,
	TYPEC_HOST,
};

enum typec_role {
	TYPEC_SINK,
	TYPEC_SOURCE,
};

enum typec_pwr_opmode {
	TYPEC_PWR_MODE_USB,
	TYPEC_PWR_MODE_1_5A,
	TYPEC_PWR_MODE_3_0A,
	TYPEC_PWR_MODE_PD,
};

enum typec_accessory {
	TYPEC_ACCESSORY_NONE,
	TYPEC_ACCESSORY_AUDIO,
	TYPEC_ACCESSORY_DEBUG,
};

#define TYPEC_MAX_ACCESSORY	3

enum typec_orientation {
	TYPEC_ORIENTATION_NONE,
	TYPEC_ORIENTATION_NORMAL,
	TYPEC_ORIENTATION_REVERSE,
};

/*
 * struct usb_pd_identity - USB Power Delivery identity data
 * @id_header: ID Header VDO
 * @cert_stat: Cert Stat VDO
 * @product: Product VDO
 * @vdo: Product Type Specific VDOs
 *
 * USB power delivery Discover Identity command response data.
 *
 * REVISIT: This is USB Power Delivery specific information, so this structure
 * probable belongs to USB Power Delivery header file once we have them.
 */
struct usb_pd_identity {
	u32			id_header;
	u32			cert_stat;
	u32			product;
	u32			vdo[3];
};

enum typec_plug_index {
	TYPEC_PLUG_SOP_P,
	TYPEC_PLUG_SOP_PP,
};

/*
 * struct typec_plug_desc - USB Type-C Cable Plug Descriptor
 * @index: SOP Prime for the plug connected to DFP and SOP Double Prime for the
 *         plug connected to UFP
 *
 * Represents USB Type-C Cable Plug.
 */
struct typec_plug_desc {
	enum typec_plug_index	index;
};

/*
 * struct typec_cable_desc - USB Type-C Cable Descriptor
 * @type: The plug type from USB PD Cable VDO
 * @active: Is the cable active or passive
 * @identity: Result of Discover Identity command
 * @pd_revision: USB Power Delivery Specification revision if supported
 *
 * Represents USB Type-C Cable attached to USB Type-C port.
 */
struct typec_cable_desc {
	enum typec_plug_type	type;
	unsigned int		active:1;
	struct usb_pd_identity	*identity;
	u16			pd_revision; /* 0300H = "3.0" */

};

enum usb_pd_svdm_ver {
	SVDM_VER_1_0 = 0,
	SVDM_VER_2_0 = 1,
	SVDM_VER_MAX = SVDM_VER_2_0,
};

/*
 * struct typec_capability - USB Type-C Port Capabilities
 * @type: Supported power role of the port
 * @data: Supported data role of the port
 * @revision: USB Type-C Specification release. Binary coded decimal
 * @pd_revision: USB Power Delivery Specification revision if supported
 * @svdm_version: USB PD Structured VDM version if supported
 * @prefer_role: Initial role preference (DRP ports).
 * @accessory: Supported Accessory Modes
 * @driver_data: Private pointer for driver specific info
 *
 * Static capabilities of a single USB Type-C port.
 */
struct typec_capability {
	enum typec_port_type	type;
	enum typec_port_data	data;
	u16			revision; /* 0120H = "1.2" */
	u16			pd_revision; /* 0300H = "3.0" */
	enum usb_pd_svdm_ver	svdm_version;
	int			prefer_role;
	enum typec_accessory	accessory[TYPEC_MAX_ACCESSORY];
	unsigned int		orientation_aware:1;

	void			*driver_data;
};

#endif /* __LINUX_USB_TYPEC_H */
