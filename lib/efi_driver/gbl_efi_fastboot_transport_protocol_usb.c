// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026 Rockchip Electronics Co., Ltd.
 */

#include <config.h>
#include <efi.h>
#include <efi_api.h>
#include <efi_loader.h>
#include <env.h>
#include <g_dnl.h>
#include <gbl_efi_fastboot_transport.h>
#include <gbl_efi_fastboot_transport_usb.h>
#include <log.h>
#include <malloc.h>
#include <usb.h>
#include <watchdog.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>

#define DESCRIPTION "usb"

#define GBL_FASTBOOT_INTERFACE_CLASS		0xff
#define GBL_FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define GBL_FASTBOOT_INTERFACE_PROTOCOL		0x03
#define GBL_FASTBOOT_EP_BUFFER_SIZE		4096
#define GBL_FASTBOOT_TX_TIMEOUT			100000

struct gbl_fastboot_usb_context {
	struct gbl_efi_fastboot_transport_protocol proto;
	struct usb_function function;
	struct usb_ep *in_ep;
	struct usb_ep *out_ep;
	struct usb_request *in_req;
	struct usb_request *out_req;
	struct udevice *udc;
	bool started;
	bool configured;
	bool rx_pending;
	bool tx_busy;
	int tx_status;
	size_t rx_len;
	size_t rx_off;
	u8 rx_buf[GBL_FASTBOOT_EP_BUFFER_SIZE];
};

static struct gbl_fastboot_usb_context gbl_usb_ctx;

static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(64),
};

static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(64),
};

static struct usb_endpoint_descriptor hs_ep_in = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(512),
};

static struct usb_endpoint_descriptor ss_ep_in = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor ss_ep_out = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_bulk_comp_desc = {
	.bLength = sizeof(ss_bulk_comp_desc),
	.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
};

static struct usb_interface_descriptor interface_desc = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0x00,
	.bAlternateSetting = 0x00,
	.bNumEndpoints = 0x02,
	.bInterfaceClass = GBL_FASTBOOT_INTERFACE_CLASS,
	.bInterfaceSubClass = GBL_FASTBOOT_INTERFACE_SUB_CLASS,
	.bInterfaceProtocol = GBL_FASTBOOT_INTERFACE_PROTOCOL,
};

static struct usb_descriptor_header *fs_function[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&fs_ep_in,
	(struct usb_descriptor_header *)&fs_ep_out,
	NULL,
};

static struct usb_descriptor_header *hs_function[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&hs_ep_in,
	(struct usb_descriptor_header *)&hs_ep_out,
	NULL,
};

static struct usb_descriptor_header *ss_function[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&ss_ep_in,
	(struct usb_descriptor_header *)&ss_bulk_comp_desc,
	(struct usb_descriptor_header *)&ss_ep_out,
	(struct usb_descriptor_header *)&ss_bulk_comp_desc,
	NULL,
};

static const char fastboot_name[] = "Android Fastboot";

static struct usb_string string_defs[] = {
	[0].s = fastboot_name,
	{  }
};

static struct usb_gadget_strings string_tab = {
	.language = 0x0409,
	.strings = string_defs,
};

static struct usb_gadget_strings *strings[] = {
	&string_tab,
	NULL,
};

static inline struct gbl_fastboot_usb_context *func_to_ctx(struct usb_function *f)
{
	return container_of(f, struct gbl_fastboot_usb_context, function);
}

static struct usb_endpoint_descriptor *ep_desc(struct usb_gadget *g,
					       struct usb_endpoint_descriptor *fs,
					       struct usb_endpoint_descriptor *hs,
					       struct usb_endpoint_descriptor *ss)
{
	if (gadget_is_superspeed(g) && g->speed >= USB_SPEED_SUPER)
		return ss;
	if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return hs;
	return fs;
}

static void pump_usb(void)
{
	if (!gbl_usb_ctx.udc)
		return;

	schedule();
	dm_usb_gadget_handle_interrupts(gbl_usb_ctx.udc);
}

static void rx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct gbl_fastboot_usb_context *ctx = &gbl_usb_ctx;

	if (req->status)
		return;

	if (!req->actual) {
		if (ctx->configured && !ctx->rx_pending)
			usb_ep_queue(ctx->out_ep, req, 0);
		return;
	}

	if (ctx->rx_pending)
		return;

	ctx->rx_len = min((size_t)req->actual, sizeof(ctx->rx_buf));
	ctx->rx_off = 0;
	memcpy(ctx->rx_buf, req->buf, ctx->rx_len);
	ctx->rx_pending = true;
}

static void tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	gbl_usb_ctx.tx_status = req->status;
	gbl_usb_ctx.tx_busy = false;
}

static struct usb_request *start_ep(struct usb_ep *ep,
	void (*complete)(struct usb_ep *ep, struct usb_request *req))
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, 0);
	if (!req)
		return NULL;

	req->length = GBL_FASTBOOT_EP_BUFFER_SIZE;
	req->buf = memalign(CONFIG_SYS_CACHELINE_SIZE, GBL_FASTBOOT_EP_BUFFER_SIZE);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	memset(req->buf, 0, req->length);
	req->complete = complete;

	return req;
}

static int queue_out(void)
{
	struct gbl_fastboot_usb_context *ctx = &gbl_usb_ctx;

	if (!ctx->out_req || ctx->rx_pending)
		return 0;

	ctx->out_req->length = GBL_FASTBOOT_EP_BUFFER_SIZE;
	ctx->out_req->actual = 0;
	return usb_ep_queue(ctx->out_ep, ctx->out_req, 0);
}

static void function_disable(struct usb_function *f)
{
	struct gbl_fastboot_usb_context *ctx = func_to_ctx(f);

	if (ctx->out_ep)
		usb_ep_disable(ctx->out_ep);
	if (ctx->in_ep)
		usb_ep_disable(ctx->in_ep);

	if (ctx->out_req) {
		free(ctx->out_req->buf);
		usb_ep_free_request(ctx->out_ep, ctx->out_req);
		ctx->out_req = NULL;
	}
	if (ctx->in_req) {
		free(ctx->in_req->buf);
		usb_ep_free_request(ctx->in_ep, ctx->in_req);
		ctx->in_req = NULL;
	}

	ctx->configured = false;
}

static int function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_gadget *gadget = c->cdev->gadget;
	struct gbl_fastboot_usb_context *ctx = func_to_ctx(f);
	const char *serial;
	int id;

	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	interface_desc.bInterfaceNumber = id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	string_defs[0].id = id;
	interface_desc.iInterface = id;

	ctx->in_ep = usb_ep_autoconfig(gadget, &fs_ep_in);
	if (!ctx->in_ep)
		return -ENODEV;
	ctx->in_ep->driver_data = c->cdev;

	ctx->out_ep = usb_ep_autoconfig(gadget, &fs_ep_out);
	if (!ctx->out_ep)
		return -ENODEV;
	ctx->out_ep->driver_data = c->cdev;

	f->descriptors = fs_function;
	if (gadget_is_dualspeed(gadget)) {
		hs_ep_in.bEndpointAddress = fs_ep_in.bEndpointAddress;
		hs_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;
		f->hs_descriptors = hs_function;
	}
	if (gadget_is_superspeed(gadget)) {
		ss_ep_in.bEndpointAddress = fs_ep_in.bEndpointAddress;
		ss_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;
		f->ss_descriptors = ss_function;
	}

	serial = env_get("serial#");
	if (serial)
		g_dnl_set_serialnumber((char *)serial);

	return 0;
}

static void function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	function_disable(f);
}

static int function_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	struct gbl_fastboot_usb_context *ctx = func_to_ctx(f);
	const struct usb_endpoint_descriptor *desc;
	int ret;

	desc = ep_desc(gadget, &fs_ep_out, &hs_ep_out, &ss_ep_out);
	ret = usb_ep_enable(ctx->out_ep, desc);
	if (ret)
		return ret;

	ctx->out_req = start_ep(ctx->out_ep, rx_complete);
	if (!ctx->out_req) {
		ret = -ENOMEM;
		goto err;
	}

	desc = ep_desc(gadget, &fs_ep_in, &hs_ep_in, &ss_ep_in);
	ret = usb_ep_enable(ctx->in_ep, desc);
	if (ret)
		goto err;

	ctx->in_req = start_ep(ctx->in_ep, tx_complete);
	if (!ctx->in_req) {
		ret = -ENOMEM;
		goto err;
	}

	ctx->configured = true;
	ret = queue_out();
	if (ret)
		goto err;

	return 0;
err:
	function_disable(f);
	return ret;
}

static int usb_gbl_fastboot_add(struct usb_configuration *c)
{
	struct gbl_fastboot_usb_context *ctx = &gbl_usb_ctx;

	ctx->function.name = "f_gbl_fastboot";
	ctx->function.bind = function_bind;
	ctx->function.unbind = function_unbind;
	ctx->function.set_alt = function_set_alt;
	ctx->function.disable = function_disable;
	ctx->function.strings = strings;

	return usb_add_function(c, &ctx->function);
}
DECLARE_GADGET_BIND_CALLBACK(usb_dnl_fastboot_gbl, usb_gbl_fastboot_add);

static efi_status_t EFIAPI start(struct gbl_efi_fastboot_transport_protocol *this)
{
	struct gbl_fastboot_usb_context *ctx = &gbl_usb_ctx;
	int ret;

	EFI_ENTRY("%p", this);

	if (this != &ctx->proto)
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	if (ctx->started)
		return EFI_EXIT(EFI_ALREADY_STARTED);

	memset((char *)ctx + sizeof(ctx->proto), 0, sizeof(*ctx) - sizeof(ctx->proto));
	g_dnl_clear_detach();

	ret = udc_device_get_by_index(CONFIG_FASTBOOT_USB_DEV, &ctx->udc);
	if (ret) {
		log_err("# GBL fastboot USB init failed: %d\n", ret);
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	ret = g_dnl_register("usb_dnl_fastboot_gbl");
	if (ret) {
		udc_device_put(ctx->udc);
		ctx->udc = NULL;
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	ctx->started = true;
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI stop(struct gbl_efi_fastboot_transport_protocol *this)
{
	struct gbl_fastboot_usb_context *ctx = &gbl_usb_ctx;

	EFI_ENTRY("%p", this);

	if (this != &ctx->proto)
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	if (!ctx->started)
		return EFI_EXIT(EFI_NOT_STARTED);

	g_dnl_unregister();
	udc_device_put(ctx->udc);
	g_dnl_clear_detach();
	ctx->udc = NULL;
	ctx->started = false;
	ctx->configured = false;
	ctx->rx_pending = false;
	ctx->tx_busy = false;

	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI receive(struct gbl_efi_fastboot_transport_protocol *this,
				   size_t *bufsize, void *buf,
				   gbl_efi_fastboot_rx_mode mode)
{
	struct gbl_fastboot_usb_context *ctx = &gbl_usb_ctx;
	size_t remain;
	size_t copy_len;

	EFI_ENTRY_NO_LOG("%p, %p, %p, %u", this, bufsize, buf, mode);

	if (this != &ctx->proto || !bufsize || (*bufsize > 0 && !buf))
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	if (!ctx->started)
		return EFI_EXIT(EFI_NOT_STARTED);

	pump_usb();
	if (g_dnl_detach())
		return EFI_EXIT(EFI_DEVICE_ERROR);
	if (!ctx->configured || !ctx->rx_pending)
		return EFI_EXIT_NO_LOG(EFI_NOT_READY);

	remain = ctx->rx_len - ctx->rx_off;
	if (mode == GBL_EFI_FASTBOOT_RX_MODE_SINGLE_PACKET && *bufsize < remain) {
		*bufsize = remain;
		return EFI_EXIT(EFI_BUFFER_TOO_SMALL);
	}

	copy_len = min(*bufsize, remain);
	if (copy_len)
		memcpy(buf, ctx->rx_buf + ctx->rx_off, copy_len);
	ctx->rx_off += copy_len;
	*bufsize = copy_len;

	if (ctx->rx_off == ctx->rx_len) {
		ctx->rx_pending = false;
		ctx->rx_len = 0;
		ctx->rx_off = 0;
		queue_out();
	}

	return EFI_EXIT_NO_LOG(EFI_SUCCESS);
}

static efi_status_t EFIAPI send(struct gbl_efi_fastboot_transport_protocol *this,
				size_t *bufsize, const void *buf)
{
	struct gbl_fastboot_usb_context *ctx = &gbl_usb_ctx;
	size_t send_len;
	int timeout = GBL_FASTBOOT_TX_TIMEOUT;
	int ret;

	EFI_ENTRY_NO_LOG("%p, %p, %p", this, bufsize, buf);

	if (this != &ctx->proto || !bufsize || (*bufsize > 0 && !buf))
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	if (!ctx->started)
		return EFI_EXIT(EFI_NOT_STARTED);

	pump_usb();
	if (!ctx->configured || ctx->tx_busy || !ctx->in_req)
		return EFI_EXIT_NO_LOG(EFI_NOT_READY);

	send_len = min(*bufsize, (size_t)GBL_FASTBOOT_EP_BUFFER_SIZE);
	if (!send_len) {
		*bufsize = 0;
		return EFI_EXIT_NO_LOG(EFI_SUCCESS);
	}

	memcpy(ctx->in_req->buf, buf, send_len);
	ctx->in_req->length = send_len;
	ctx->in_req->actual = 0;
	ctx->tx_busy = true;
	ctx->tx_status = 0;

	ret = usb_ep_queue(ctx->in_ep, ctx->in_req, 0);
	if (ret) {
		ctx->tx_busy = false;
		return EFI_EXIT(EFI_DEVICE_ERROR);
	}

	while (ctx->tx_busy && timeout--)
		pump_usb();

	if (ctx->tx_busy)
		return EFI_EXIT(EFI_NOT_READY);
	if (ctx->tx_status)
		return EFI_EXIT(EFI_DEVICE_ERROR);

	*bufsize = send_len;
	return EFI_EXIT_NO_LOG(EFI_SUCCESS);
}

static efi_status_t EFIAPI gbl_usb_flush(struct gbl_efi_fastboot_transport_protocol *this)
{
	struct gbl_fastboot_usb_context *ctx = &gbl_usb_ctx;

	EFI_ENTRY_NO_LOG("%p", this);
	if (this != &ctx->proto)
		return EFI_EXIT(EFI_INVALID_PARAMETER);
	if (!ctx->started)
		return EFI_EXIT(EFI_NOT_STARTED);

	while (ctx->tx_busy)
		pump_usb();

	return EFI_EXIT_NO_LOG(EFI_SUCCESS);
}

efi_status_t gbl_efi_fastboot_transport_usb_register(void)
{
	efi_handle_t handle = NULL;
	efi_status_t ret;

	gbl_usb_ctx.proto.revision = GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_REVISION;
	gbl_usb_ctx.proto.description = DESCRIPTION;
	gbl_usb_ctx.proto.start = start;
	gbl_usb_ctx.proto.stop = stop;
	gbl_usb_ctx.proto.receive = receive;
	gbl_usb_ctx.proto.send = send;
	gbl_usb_ctx.proto.flush = gbl_usb_flush;

	ret = efi_install_multiple_protocol_interfaces(
		&handle, &gbl_efi_fastboot_transport_guid, &gbl_usb_ctx.proto,
		NULL);
	if (ret != EFI_SUCCESS)
		log_err("Failed to install USB GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL: 0x%lx\n", ret);

	return ret;
}
