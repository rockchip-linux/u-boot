/**
 * ep0.c - DesignWare USB3 DRD Controller Endpoint 0 Handling
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com
 *
 * Authors: Felipe Balbi <balbi@ti.com>,
 *	    Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Taken from Linux Kernel v3.19-rc1 (drivers/usb/dwc3/ep0.c) and ported
 * to uboot.
 *
 * commit c00552ebaf : Merge 3.18-rc7 into usb-next
 *
 * SPDX-License-Identifier:     GPL-2.0
 */

//#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <usb/dwc3_rk_udc.h>

#include "../dwc3/core.h"
#include "../dwc3/gadget.h"
#include "../dwc3/io.h"

#include "../dwc3/linux-compat.h"

#define EP_BUFFER_SIZE			4096
#define USB_BUFSIZ	4096
extern struct rk_dwc3_udc_instance global_rk_instance;
extern int rockusb_clear_feature(struct dwc3 *dwc, u32 wIndex);
static void __dwc3_ep0_do_control_status(struct dwc3 *dwc, struct dwc3_ep *dep);
static void __dwc3_ep0_do_control_data(struct dwc3 *dwc,
		struct dwc3_ep *dep, struct dwc3_request *req);

static void ep0_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length)
		DWC_DBG("%s: setup complete --> %d, %d/%d\n", __func__,
				req->status, req->actual, req->length);
}

static void device_qual(struct usb_request *req)
{
	struct usb_qualifier_descriptor	*qual = req->buf;

	qual->bLength = sizeof(*qual);
	qual->bDescriptorType = USB_DT_DEVICE_QUALIFIER;
	/* POLICY: same bcdUSB and device type info at both speeds */
	qual->bcdUSB = global_rk_instance.device_desc->bcdUSB;
	qual->bDeviceClass = global_rk_instance.device_desc->bDeviceClass;
	qual->bDeviceSubClass = global_rk_instance.device_desc->bDeviceSubClass;
	qual->bDeviceProtocol = global_rk_instance.device_desc->bDeviceProtocol;
	/* ASSUME same EP0 fifo size at both speeds */
	qual->bMaxPacketSize0 = global_rk_instance.device_desc->bMaxPacketSize0;
	qual->bNumConfigurations = global_rk_instance.device_desc->bNumConfigurations;
	qual->bRESERVED = 0;
}

static struct usb_request *rockusb_start_ep(struct usb_ep *ep)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, 0);
	if (!req)
		return NULL;
/*	req->length = EP_BUFFER_SIZE;
	req->buf = memalign(ARCH_DMA_MINALIGN, EP_BUFFER_SIZE);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	memset(req->buf, 0, req->length);
*/
	return req;
}

void rockusb_disable(struct usb_gadget *gadget)
{
	struct rk_udc_private_data *privData = (struct rk_udc_private_data *)get_gadget_data(gadget);

	usb_ep_disable(privData->out_ep);
	usb_ep_disable(privData->in_ep);

	if (privData->out_req) {
		/* free(privData->out_req->buf); */
		usb_ep_free_request(privData->out_ep, privData->out_req);
		privData->out_req = NULL;
	}
	if (privData->in_req) {
		/* free(privData->in_req->buf); */
		usb_ep_free_request(privData->in_ep, privData->in_req);
		privData->in_req = NULL;
	}
	if (privData->out_req2) {
		usb_ep_free_request(privData->out_ep, privData->out_req2);
		privData->out_req2 = NULL;
	}
	if (privData->in_req2) {
		usb_ep_free_request(privData->in_ep, privData->in_req2);
		privData->in_req2 = NULL;
	}
	global_rk_instance.connected = 0;
}

static int rockusb_set_alt(struct usb_gadget *gadget)
{
	struct rk_udc_private_data *privData = (struct rk_udc_private_data *)get_gadget_data(gadget);
	int ret;

	ret = usb_ep_enable(privData->out_ep, global_rk_instance.endpoint_desc[0]);
	if (ret) {
		DWC_ERR("failed to enable out ep\n");
		return ret;
	}
	privData->out_req = rockusb_start_ep(privData->out_ep);
	if (!privData->out_req) {
		DWC_ERR("failed to alloc out req\n");
		ret = -EINVAL;
		goto err;
	}
	privData->out_req->length = global_rk_instance.rx_buffer_size;
	privData->out_req->buf = global_rk_instance.rx_buffer[0];

	privData->out_req2 = rockusb_start_ep(privData->out_ep);
	if (!privData->out_req2) {
		DWC_ERR("failed to alloc out req2\n");
		ret = -EINVAL;
		goto err;
	}
	privData->out_req2->length = global_rk_instance.rx_buffer_size;
	privData->out_req2->buf = global_rk_instance.rx_buffer[1];

	privData->out_req->complete = privData->out_req2->complete = NULL;
	ret = usb_ep_enable(privData->in_ep, global_rk_instance.endpoint_desc[1]);
	if (ret) {
		DWC_ERR("failed to enable in ep\n");
		return ret;
	}
	privData->in_req = rockusb_start_ep(privData->in_ep);
	if (!privData->in_req) {
		DWC_ERR("failed to alloc in req\n");
		ret = -EINVAL;
		goto err;
	}
	privData->in_req->length = global_rk_instance.tx_buffer_size;
	privData->in_req->buf = global_rk_instance.tx_buffer[0];

	privData->in_req2 = rockusb_start_ep(privData->in_ep);
	if (!privData->in_req2) {
		DWC_ERR("failed to alloc in req2\n");
		ret = -EINVAL;
		goto err;
	}
	privData->in_req2->length = global_rk_instance.tx_buffer_size;
	privData->in_req2->buf = global_rk_instance.tx_buffer[1];

	privData->in_req->complete = privData->in_req2->complete = NULL;
	global_rk_instance.connected = 1;
	return 0;

err:
	rockusb_disable(gadget);
	return ret;
}

static int
dwc3_ep0_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	u16				w_length = le16_to_cpu(ctrl->wLength);
#ifdef DWCDEBUG
	u16				w_index = le16_to_cpu(ctrl->wIndex);
#endif
	u16				w_value = le16_to_cpu(ctrl->wValue);
	int				value = -EOPNOTSUPP;
	struct rk_udc_private_data *privData = (struct rk_udc_private_data *)get_gadget_data(gadget);
	int				standard;

	/*
	 * partial re-init of the response message; the function or the
	 * gadget might need to intercept e.g. a control-OUT completion
	 * when we delegate to it.
	 */
	privData->ep0_req->zero = 0;
	privData->ep0_req->complete = ep0_setup_complete;
	privData->ep0_req->length = USB_BUFSIZ;

	standard = (ctrl->bRequestType & USB_TYPE_MASK)
						== USB_TYPE_STANDARD;
	if (!standard)
		goto unknown;

	switch (ctrl->bRequest) {

	/* we handle all standard USB descriptors */
	case USB_REQ_GET_DESCRIPTOR:
		DWC_DBG("USB_REQ_GET_DESCRIPTOR,w_value=%d\n", w_value>>8);
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		switch (w_value >> 8) {
		case USB_DT_DEVICE:
			value = min(w_length, (u16) sizeof(struct usb_device_descriptor));
			memcpy(privData->ep0_req->buf, global_rk_instance.device_desc, value);
			break;
		case USB_DT_DEVICE_QUALIFIER:
			if (!gadget_is_dualspeed(gadget))
				break;
			device_qual(privData->ep0_req);
			value = min_t(int, w_length,
				      sizeof(struct usb_qualifier_descriptor));
			break;
		case USB_DT_OTHER_SPEED_CONFIG:
			if (!gadget_is_dualspeed(gadget))
				break;

		case USB_DT_CONFIG:
			value = min(w_length, (u16) global_rk_instance.whole_config_size);
			memcpy(privData->ep0_req->buf, global_rk_instance.whole_config_desc, value);
			break;
		case USB_DT_STRING:
			if ((w_value & 0xff) >= 6)
				value = EINVAL;
			else {
				value = min(w_length, global_rk_instance.string_desc[w_value&0xff]->bLength);
				memcpy(privData->ep0_req->buf, global_rk_instance.string_desc[w_value&0xff], value);
			}
			break;
		case USB_DT_BOS:
			/*
			 * The USB compliance test (USB 2.0 Command Verifier)
			 * issues this request. We should not run into the
			 * default path here. But return for now until
			 * the superspeed support is added.
			 */
			value = min(w_length, (u16) sizeof(global_rk_instance.bos_desc));
			memcpy(privData->ep0_req->buf, global_rk_instance.bos_desc, value);
			break;
		default:
			goto unknown;
		}
		break;

	/* any number of configs can work */
	case USB_REQ_SET_CONFIGURATION:
		DWC_DBG("USB_REQ_SET_CONFIGURATION\n");
		if (ctrl->bRequestType != 0)
			goto unknown;
		if (gadget_is_otg(gadget)) {
			if (gadget->a_hnp_support)
				DWC_DBG("HNP available\n");
			else if (gadget->a_alt_hnp_support)
				DWC_DBG("HNP on another port\n");
			else
				DWC_DBG("HNP inactive\n");
		}

		value = rockusb_set_alt(gadget);
		return value;
	case USB_REQ_GET_CONFIGURATION:
		DWC_DBG("USB_REQ_GET_CONFIGURATION\n");
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		struct usb_config_descriptor *config_desc = (struct usb_config_descriptor *)global_rk_instance.config_desc;
		*(u8 *)privData->ep0_req->buf = config_desc->bConfigurationValue;
		value = min(w_length, (u16) 1);
		break;
	default:
unknown:
		DWC_DBG("non-core control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);

		goto done;
	}

	/* respond with data transfer before status phase? */
	if (value >= 0) {
		privData->ep0_req->length = value;
		privData->ep0_req->zero = value < w_length;
		value = usb_ep_queue(gadget->ep0, privData->ep0_req, GFP_KERNEL);
		if (value < 0) {
			DWC_DBG("ep_queue --> %d\n", value);
			privData->ep0_req->status = 0;
			ep0_setup_complete(gadget->ep0, privData->ep0_req);
		}
	}

done:
	/* device either stalls (value < 0) or reports success */
	return value;
}

#ifdef DWCDEBUG
static const char *dwc3_ep0_state_string(enum dwc3_ep0_state state)
{
	switch (state) {
	case EP0_UNCONNECTED:
		return "Unconnected";
	case EP0_SETUP_PHASE:
		return "Setup Phase";
	case EP0_DATA_PHASE:
		return "Data Phase";
	case EP0_STATUS_PHASE:
		return "Status Phase";
	default:
		return "UNKNOWN";
	}
}
#endif

static int dwc3_ep0_start_trans(struct dwc3 *dwc, u8 epnum, dma_addr_t buf_dma,
				u32 len, u32 type, unsigned chain)
{
	struct dwc3_gadget_ep_cmd_params params;
	struct dwc3_trb			*trb;
	struct dwc3_ep			*dep;

	int				ret;

	dep = dwc->eps[epnum];
	if (dep->flags & DWC3_EP_BUSY) {
		DWC_DBG("%s still busy", dep->name);
		return 0;
	}

	trb = &dwc->ep0_trb[dep->free_slot];

	if (chain)
		dep->free_slot++;

	trb->bpl = lower_32_bits(buf_dma);
	trb->bph = upper_32_bits(buf_dma);
	trb->size = len;
	trb->ctrl = type;

	trb->ctrl |= (DWC3_TRB_CTRL_HWO
			| DWC3_TRB_CTRL_ISP_IMI);

	if (chain)
		trb->ctrl |= DWC3_TRB_CTRL_CHN;
	else
		trb->ctrl |= (DWC3_TRB_CTRL_IOC
				| DWC3_TRB_CTRL_LST);

	dwc3_flush_cache((long)buf_dma, len);
	dwc3_flush_cache((long)trb, sizeof(*trb));

	if (chain)
		return 0;

	memset(&params, 0, sizeof(params));
	params.param0 = upper_32_bits(dwc->ep0_trb_addr);
	params.param1 = lower_32_bits(dwc->ep0_trb_addr);

	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number,
			DWC3_DEPCMD_STARTTRANSFER, &params);
	if (ret < 0) {
		DWC_ERR("%s STARTTRANSFER failed", dep->name);
		return ret;
	}

	dep->flags |= DWC3_EP_BUSY;
	dep->resource_index = dwc3_gadget_ep_get_transfer_index(dwc,
			dep->number);

	dwc->ep0_next_event = DWC3_EP0_COMPLETE;
	return 0;
}

static int __dwc3_gadget_ep0_queue(struct dwc3_ep *dep,
		struct dwc3_request *req)
{
	struct dwc3		*dwc = dep->dwc;
	req->request.actual	= 0;
	req->request.status	= -EINPROGRESS;
	req->epnum		= dep->number;

	list_add_tail(&req->list, &dep->request_list);

	/*
	 * Gadget driver might not be quick enough to queue a request
	 * before we get a Transfer Not Ready event on this endpoint.
	 *
	 * In that case, we will set DWC3_EP_PENDING_REQUEST. When that
	 * flag is set, it's telling us that as soon as Gadget queues the
	 * required request, we should kick the transfer here because the
	 * IRQ we were waiting for is long gone.
	 */
	if (dep->flags & DWC3_EP_PENDING_REQUEST) {
		unsigned	direction;

		direction = !!(dep->flags & DWC3_EP0_DIR_IN);

		if (dwc->ep0state != EP0_DATA_PHASE) {
			DWC_WARN("Unexpected pending request\n");
			return 0;
		}

		__dwc3_ep0_do_control_data(dwc, dwc->eps[direction], req);

		dep->flags &= ~(DWC3_EP_PENDING_REQUEST |
				DWC3_EP0_DIR_IN);

		return 0;
	}

	/*
	 * In case gadget driver asked us to delay the STATUS phase,
	 * handle it here.
	 */
	if (dwc->delayed_status) {
		unsigned	direction;

		direction = !dwc->ep0_expect_in;
		dwc->delayed_status = false;
		usb_gadget_set_state(&dwc->gadget, USB_STATE_CONFIGURED);

		if (dwc->ep0state == EP0_STATUS_PHASE)
			__dwc3_ep0_do_control_status(dwc, dwc->eps[direction]);
		else
			DWC_DBG("too early for delayed status");

		return 0;
	}

	/*
	 * Unfortunately we have uncovered a limitation wrt the Data Phase.
	 *
	 * Section 9.4 says we can wait for the XferNotReady(DATA) event to
	 * come before issueing Start Transfer command, but if we do, we will
	 * miss situations where the host starts another SETUP phase instead of
	 * the DATA phase.  Such cases happen at least on TD.7.6 of the Link
	 * Layer Compliance Suite.
	 *
	 * The problem surfaces due to the fact that in case of back-to-back
	 * SETUP packets there will be no XferNotReady(DATA) generated and we
	 * will be stuck waiting for XferNotReady(DATA) forever.
	 *
	 * By looking at tables 9-13 and 9-14 of the Databook, we can see that
	 * it tells us to start Data Phase right away. It also mentions that if
	 * we receive a SETUP phase instead of the DATA phase, core will issue
	 * XferComplete for the DATA phase, before actually initiating it in
	 * the wire, with the TRB's status set to "SETUP_PENDING". Such status
	 * can only be used to print some debugging logs, as the core expects
	 * us to go through to the STATUS phase and start a CONTROL_STATUS TRB,
	 * just so it completes right away, without transferring anything and,
	 * only then, we can go back to the SETUP phase.
	 *
	 * Because of this scenario, SNPS decided to change the programming
	 * model of control transfers and support on-demand transfers only for
	 * the STATUS phase. To fix the issue we have now, we will always wait
	 * for gadget driver to queue the DATA phase's struct usb_request, then
	 * start it right away.
	 *
	 * If we're actually in a 2-stage transfer, we will wait for
	 * XferNotReady(STATUS).
	 */
	if (dwc->three_stage_setup) {
		unsigned        direction;

		direction = dwc->ep0_expect_in;
		dwc->ep0state = EP0_DATA_PHASE;

		__dwc3_ep0_do_control_data(dwc, dwc->eps[direction], req);

		dep->flags &= ~DWC3_EP0_DIR_IN;
	}

	return 0;
}

int dwc3_gadget_ep0_queue(struct usb_ep *ep, struct usb_request *request,
		gfp_t gfp_flags)
{
	struct dwc3_request		*req = to_dwc3_request(request);
	struct dwc3_ep			*dep = to_dwc3_ep(ep);
#ifdef DWCDEBUG	
	struct dwc3			*dwc = dep->dwc;
#endif
	int				ret;

	if (!dep->endpoint.desc) {
		DWC_ERR("trying to queue request %p to disabled %s",
				request, dep->name);
		ret = -ESHUTDOWN;
		goto out;
	}

	/* we share one TRB for ep0/1 */
	if (!list_empty(&dep->request_list)) {
		ret = -EBUSY;
		goto out;
	}

	DWC_DBG("queueing request %p to %s length %d state '%s'\n",
			request, dep->name, request->length,
			dwc3_ep0_state_string(dwc->ep0state));

	ret = __dwc3_gadget_ep0_queue(dep, req);

out:
	return ret;
}

static void dwc3_ep0_stall_and_restart(struct dwc3 *dwc)
{
	struct dwc3_ep		*dep;

	/* reinitialize physical ep1 */
	dep = dwc->eps[1];
	dep->flags = DWC3_EP_ENABLED;

	/* stall is always issued on EP0 */
	dep = dwc->eps[0];
	__dwc3_gadget_ep_set_halt(dep, 1, false);
	dep->flags = DWC3_EP_ENABLED;
	dwc->delayed_status = false;

	if (!list_empty(&dep->request_list)) {
		struct dwc3_request	*req;

		req = next_request(&dep->request_list);
		dwc3_gadget_giveback(dep, req, -ECONNRESET);
	}

	dwc->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dwc);
}

int __dwc3_gadget_ep0_set_halt(struct usb_ep *ep, int value)
{
	struct dwc3_ep			*dep = to_dwc3_ep(ep);
	struct dwc3			*dwc = dep->dwc;

	dwc3_ep0_stall_and_restart(dwc);

	return 0;
}

int dwc3_gadget_ep0_set_halt(struct usb_ep *ep, int value)
{
	unsigned long			flags;
	int				ret;

	spin_lock_irqsave(&dwc->lock, flags);
	ret = __dwc3_gadget_ep0_set_halt(ep, value);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

void dwc3_ep0_out_start(struct dwc3 *dwc)
{
	int				ret;
	ret = dwc3_ep0_start_trans(dwc, 0, dwc->ctrl_req_addr, 8,
				   DWC3_TRBCTL_CONTROL_SETUP, 0);
	WARN_ON(ret < 0);
}

static struct dwc3_ep *dwc3_wIndex_to_dep(struct dwc3 *dwc, __le16 wIndex_le)
{
	struct dwc3_ep		*dep;
	u32			windex = le16_to_cpu(wIndex_le);
	u32			epnum;

	epnum = (windex & USB_ENDPOINT_NUMBER_MASK) << 1;
	if ((windex & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
		epnum |= 1;

	dep = dwc->eps[epnum];
	if (dep->flags & DWC3_EP_ENABLED)
		return dep;

	return NULL;
}

static void dwc3_ep0_status_cmpl(struct usb_ep *ep, struct usb_request *req)
{
}

/*
 * ch 9.4.5
 */
static int dwc3_ep0_handle_status(struct dwc3 *dwc,
		struct usb_ctrlrequest *ctrl)
{
	struct dwc3_ep		*dep;
	u32			recip;
	u32			reg;
	u16			usb_status = 0;
	__le16			*response_pkt;

	recip = ctrl->bRequestType & USB_RECIP_MASK;
	switch (recip) {
	case USB_RECIP_DEVICE:
		/*
		 * LTM will be set once we know how to set this in HW.
		 */
		usb_status |= dwc->is_selfpowered << USB_DEVICE_SELF_POWERED;

		if (dwc->speed == DWC3_DSTS_SUPERSPEED) {
			reg = dwc3_readl(dwc->regs, DWC3_DCTL);
			if (reg & DWC3_DCTL_INITU1ENA)
				usb_status |= 1 << USB_DEV_STAT_U1_ENABLED;
			if (reg & DWC3_DCTL_INITU2ENA)
				usb_status |= 1 << USB_DEV_STAT_U2_ENABLED;
		}

		break;

	case USB_RECIP_INTERFACE:
		/*
		 * Function Remote Wake Capable	D0
		 * Function Remote Wakeup	D1
		 */
		break;

	case USB_RECIP_ENDPOINT:
		dep = dwc3_wIndex_to_dep(dwc, ctrl->wIndex);
		if (!dep)
			return -EINVAL;

		if (dep->flags & DWC3_EP_STALL)
			usb_status = 1 << USB_ENDPOINT_HALT;
		break;
	default:
		return -EINVAL;
	}

	response_pkt = (__le16 *) dwc->setup_buf;
	*response_pkt = cpu_to_le16(usb_status);

	dep = dwc->eps[0];
	dwc->ep0_usb_req.dep = dep;
	dwc->ep0_usb_req.request.length = sizeof(*response_pkt);
	dwc->ep0_usb_req.request.buf = dwc->setup_buf;
	dwc->ep0_usb_req.request.complete = dwc3_ep0_status_cmpl;

	return __dwc3_gadget_ep0_queue(dep, &dwc->ep0_usb_req);
}

static int dwc3_ep0_handle_feature(struct dwc3 *dwc,
		struct usb_ctrlrequest *ctrl, int set)
{
	struct dwc3_ep		*dep;
	u32			recip;
	u32			wValue;
	u32			wIndex;
	u32			reg;
	int			ret;
	enum usb_device_state	state;

	wValue = le16_to_cpu(ctrl->wValue);
	wIndex = le16_to_cpu(ctrl->wIndex);
	recip = ctrl->bRequestType & USB_RECIP_MASK;
	state = dwc->gadget.state;

	switch (recip) {
	case USB_RECIP_DEVICE:

		switch (wValue) {
		case USB_DEVICE_REMOTE_WAKEUP:
			break;
		/*
		 * 9.4.1 says only only for SS, in AddressState only for
		 * default control pipe
		 */
		case USB_DEVICE_U1_ENABLE:
			if (state != USB_STATE_CONFIGURED)
				return -EINVAL;
			if (dwc->speed != DWC3_DSTS_SUPERSPEED)
				return -EINVAL;

			reg = dwc3_readl(dwc->regs, DWC3_DCTL);
			if (set)
				reg |= DWC3_DCTL_INITU1ENA;
			else
				reg &= ~DWC3_DCTL_INITU1ENA;
			dwc3_writel(dwc->regs, DWC3_DCTL, reg);
			break;

		case USB_DEVICE_U2_ENABLE:
			if (state != USB_STATE_CONFIGURED)
				return -EINVAL;
			if (dwc->speed != DWC3_DSTS_SUPERSPEED)
				return -EINVAL;

			reg = dwc3_readl(dwc->regs, DWC3_DCTL);
			if (set)
				reg |= DWC3_DCTL_INITU2ENA;
			else
				reg &= ~DWC3_DCTL_INITU2ENA;
			dwc3_writel(dwc->regs, DWC3_DCTL, reg);
			break;

		case USB_DEVICE_LTM_ENABLE:
			return -EINVAL;

		case USB_DEVICE_TEST_MODE:
			if ((wIndex & 0xff) != 0)
				return -EINVAL;
			if (!set)
				return -EINVAL;

			dwc->test_mode_nr = wIndex >> 8;
			dwc->test_mode = true;
			break;
		default:
			return -EINVAL;
		}
		break;

	case USB_RECIP_INTERFACE:
		switch (wValue) {
		case USB_INTRF_FUNC_SUSPEND:
			if (wIndex & USB_INTRF_FUNC_SUSPEND_LP)
				/* XXX enable Low power suspend */
				;
			if (wIndex & USB_INTRF_FUNC_SUSPEND_RW)
				/* XXX enable remote wakeup */
				;
			break;
		default:
			return -EINVAL;
		}
		break;

	case USB_RECIP_ENDPOINT:
		switch (wValue) {
		case USB_ENDPOINT_HALT:
			dep = dwc3_wIndex_to_dep(dwc, wIndex);
			if (!dep)
				return -EINVAL;
			if (set == 0 && (dep->flags & DWC3_EP_WEDGE))
				break;
			ret = __dwc3_gadget_ep_set_halt(dep, set, true);
			if (ret)
				return -EINVAL;
			break;
		default:
			return -EINVAL;
		}
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int dwc3_ep0_set_address(struct dwc3 *dwc, struct usb_ctrlrequest *ctrl)
{
	enum usb_device_state state = dwc->gadget.state;
	u32 addr;
	u32 reg;

	addr = le16_to_cpu(ctrl->wValue);
	if (addr > 127) {
		DWC_ERR("invalid device address %d", addr);
		return -EINVAL;
	}

	if (state == USB_STATE_CONFIGURED) {
		DWC_ERR("trying to set address when configured");
		return -EINVAL;
	}

	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_DEVADDR_MASK);
	reg |= DWC3_DCFG_DEVADDR(addr);
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	if (addr)
		usb_gadget_set_state(&dwc->gadget, USB_STATE_ADDRESS);
	else
		usb_gadget_set_state(&dwc->gadget, USB_STATE_DEFAULT);

	return 0;
}

static int dwc3_ep0_delegate_req(struct dwc3 *dwc, struct usb_ctrlrequest *ctrl)
{
	int ret;

	spin_unlock(&dwc->lock);
	ret = dwc3_ep0_setup(&dwc->gadget, ctrl);
	spin_lock(&dwc->lock);
	return ret;
}

static int dwc3_ep0_set_config(struct dwc3 *dwc, struct usb_ctrlrequest *ctrl)
{
	enum usb_device_state state = dwc->gadget.state;
	u32 cfg;
	int ret;
	u32 reg;

	dwc->start_config_issued = false;
	cfg = le16_to_cpu(ctrl->wValue);

	switch (state) {
	case USB_STATE_DEFAULT:
		return -EINVAL;

	case USB_STATE_ADDRESS:
		ret = dwc3_ep0_delegate_req(dwc, ctrl);
		/* if the cfg matches and the cfg is non zero */
		if (cfg && (!ret || (ret == USB_GADGET_DELAYED_STATUS))) {

			/*
			 * only change state if set_config has already
			 * been processed. If gadget driver returns
			 * USB_GADGET_DELAYED_STATUS, we will wait
			 * to change the state on the next usb_ep_queue()
			 */
			if (ret == 0)
				usb_gadget_set_state(&dwc->gadget,
						USB_STATE_CONFIGURED);

			/*
			 * Enable transition to U1/U2 state when
			 * nothing is pending from application.
			 */
			reg = dwc3_readl(dwc->regs, DWC3_DCTL);
			reg |= (DWC3_DCTL_ACCEPTU1ENA | DWC3_DCTL_ACCEPTU2ENA);
			dwc3_writel(dwc->regs, DWC3_DCTL, reg);

			dwc->resize_fifos = false;
		}
		break;

	case USB_STATE_CONFIGURED:
		ret = dwc3_ep0_delegate_req(dwc, ctrl);
		if (!cfg && !ret)
			usb_gadget_set_state(&dwc->gadget,
					USB_STATE_ADDRESS);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static void dwc3_ep0_set_sel_cmpl(struct usb_ep *ep, struct usb_request *req)
{
	struct dwc3_ep	*dep = to_dwc3_ep(ep);
	struct dwc3	*dwc = dep->dwc;

	u32		param = 0;
	u32		reg;

	struct timing {
		u8	u1sel;
		u8	u1pel;
		u16	u2sel;
		u16	u2pel;
	} __packed timing;

	int		ret;

	memcpy(&timing, req->buf, sizeof(timing));

	dwc->u1sel = timing.u1sel;
	dwc->u1pel = timing.u1pel;
	dwc->u2sel = le16_to_cpu(timing.u2sel);
	dwc->u2pel = le16_to_cpu(timing.u2pel);

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	if (reg & DWC3_DCTL_INITU2ENA)
		param = dwc->u2pel;
	if (reg & DWC3_DCTL_INITU1ENA)
		param = dwc->u1pel;

	/*
	 * According to Synopsys Databook, if parameter is
	 * greater than 125, a value of zero should be
	 * programmed in the register.
	 */
	if (param > 125)
		param = 0;

	/* now that we have the time, issue DGCMD Set Sel */
	ret = dwc3_send_gadget_generic_command(dwc,
			DWC3_DGCMD_SET_PERIODIC_PAR, param);
	WARN_ON(ret < 0);
}

static int dwc3_ep0_set_sel(struct dwc3 *dwc, struct usb_ctrlrequest *ctrl)
{
	struct dwc3_ep	*dep;
	enum usb_device_state state = dwc->gadget.state;
	u16		wLength;

	if (state == USB_STATE_DEFAULT)
		return -EINVAL;

	wLength = le16_to_cpu(ctrl->wLength);

	if (wLength != 6) {
		DWC_ERR("Set SEL should be 6 bytes, got %d\n",
				wLength);
		return -EINVAL;
	}

	/*
	 * To handle Set SEL we need to receive 6 bytes from Host. So let's
	 * queue a usb_request for 6 bytes.
	 *
	 * Remember, though, this controller can't handle non-wMaxPacketSize
	 * aligned transfers on the OUT direction, so we queue a request for
	 * wMaxPacketSize instead.
	 */
	dep = dwc->eps[0];
	dwc->ep0_usb_req.dep = dep;
	dwc->ep0_usb_req.request.length = dep->endpoint.maxpacket;
	dwc->ep0_usb_req.request.buf = dwc->setup_buf;
	dwc->ep0_usb_req.request.complete = dwc3_ep0_set_sel_cmpl;

	return __dwc3_gadget_ep0_queue(dep, &dwc->ep0_usb_req);
}

static int dwc3_ep0_set_isoch_delay(struct dwc3 *dwc, struct usb_ctrlrequest *ctrl)
{
	u16		wLength;
	u16		wValue;
	u16		wIndex;

	wValue = le16_to_cpu(ctrl->wValue);
	wLength = le16_to_cpu(ctrl->wLength);
	wIndex = le16_to_cpu(ctrl->wIndex);

	if (wIndex || wLength)
		return -EINVAL;

	/*
	 * REVISIT It's unclear from Databook what to do with this
	 * value. For now, just cache it.
	 */
	dwc->isoch_delay = wValue;

	return 0;
}

static int dwc3_ep0_std_request(struct dwc3 *dwc, struct usb_ctrlrequest *ctrl)
{
	int ret, nEvent;

	switch (ctrl->bRequest) {
	case USB_REQ_GET_STATUS:
		DWC_DBG("USB_REQ_GET_STATUS\n");
		ret = dwc3_ep0_handle_status(dwc, ctrl);
		break;
	case USB_REQ_CLEAR_FEATURE:
		DWC_DBG("USB_REQ_CLEAR_FEATURE\n");
		/* ret = dwc3_ep0_handle_feature(dwc, ctrl, 0); */
		ret = rockusb_clear_feature(dwc, ctrl->wIndex);
		nEvent = ctrl->wIndex;
		nEvent = nEvent << 16;
		nEvent |= 4;
		global_rk_instance.device_event(nEvent);
		break;
	case USB_REQ_SET_FEATURE:
		DWC_DBG("USB_REQ_SET_FEATURE\n");
		ret = dwc3_ep0_handle_feature(dwc, ctrl, 1);
		break;
	case USB_REQ_SET_ADDRESS:
		DWC_DBG("USB_REQ_SET_ADDRESS\n");
		ret = dwc3_ep0_set_address(dwc, ctrl);
		global_rk_instance.device_event(3);
		break;
	case USB_REQ_SET_CONFIGURATION:
		DWC_DBG("USB_REQ_SET_CONFIGURATION\n");
		ret = dwc3_ep0_set_config(dwc, ctrl);
		if (!ret)
			global_rk_instance.device_event(2);
		break;
	case USB_REQ_SET_SEL:
		DWC_DBG("USB_REQ_SET_SEL\n");
		ret = dwc3_ep0_set_sel(dwc, ctrl);
		break;
	case USB_REQ_SET_ISOCH_DELAY:
		DWC_DBG("USB_REQ_SET_ISOCH_DELAY\n");
		ret = dwc3_ep0_set_isoch_delay(dwc, ctrl);
		break;
	default:
		DWC_DBG("Forwarding to gadget driver\n");
		ret = dwc3_ep0_delegate_req(dwc, ctrl);
		break;
	}

	return ret;
}

static void dwc3_ep0_inspect_setup(struct dwc3 *dwc,
		const struct dwc3_event_depevt *event)
{
	struct usb_ctrlrequest *ctrl = dwc->ctrl_req;
	int ret = -EINVAL;
	u32 len;

	len = le16_to_cpu(ctrl->wLength);
	if (!len) {
		dwc->three_stage_setup = false;
		dwc->ep0_expect_in = false;
		dwc->ep0_next_event = DWC3_EP0_NRDY_STATUS;
	} else {
		dwc->three_stage_setup = true;
		dwc->ep0_expect_in = !!(ctrl->bRequestType & USB_DIR_IN);
		dwc->ep0_next_event = DWC3_EP0_NRDY_DATA;
	}

	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD)
		ret = dwc3_ep0_std_request(dwc, ctrl);
	else
		ret = dwc3_ep0_delegate_req(dwc, ctrl);

	if (ret == USB_GADGET_DELAYED_STATUS)
		dwc->delayed_status = true;

	if (ret < 0)
		dwc3_ep0_stall_and_restart(dwc);
}

static void dwc3_ep0_complete_data(struct dwc3 *dwc,
		const struct dwc3_event_depevt *event)
{
	struct dwc3_request	*r = NULL;
	struct usb_request	*ur;
	struct dwc3_trb		*trb;
	struct dwc3_ep		*ep0;
	unsigned		transfer_size = 0;
	unsigned		maxp;
	void			*buf;
	u32			transferred = 0;
	u32			status;
	u32			length;
	u8			epnum;
	epnum = event->endpoint_number;
	ep0 = dwc->eps[0];

	dwc->ep0_next_event = DWC3_EP0_NRDY_STATUS;

	trb = dwc->ep0_trb;

	r = next_request(&ep0->request_list);
	if (!r)
		return;

	dwc3_flush_cache((long)trb, sizeof(*trb));

	status = DWC3_TRB_SIZE_TRBSTS(trb->size);
	if (status == DWC3_TRBSTS_SETUP_PENDING) {
		DWC_DBG("Setup Pending received");

		if (r)
			dwc3_gadget_giveback(ep0, r, -ECONNRESET);

		return;
	}

	ur = &r->request;
	buf = ur->buf;

	length = trb->size & DWC3_TRB_SIZE_MASK;

	maxp = ep0->endpoint.maxpacket;

	if (dwc->ep0_bounced) {
		/*
		 * Handle the first TRB before handling the bounce buffer if
		 * the request length is greater than the bounce buffer size.
		 */
		if (ur->length > DWC3_EP0_BOUNCE_SIZE) {
			transfer_size = (ur->length / maxp) * maxp;
			transferred = transfer_size - length;
			buf = (u8 *)buf + transferred;
			ur->actual += transferred;

			trb++;
			dwc3_flush_cache((long)trb, sizeof(*trb));
			length = trb->size & DWC3_TRB_SIZE_MASK;

			ep0->free_slot = 0;
		}

		transfer_size = roundup((ur->length - transfer_size),
					maxp);
		transferred = min_t(u32, ur->length - transferred,
				    transfer_size - length);
		dwc3_flush_cache((long)dwc->ep0_bounce, DWC3_EP0_BOUNCE_SIZE);
		memcpy(buf, dwc->ep0_bounce, transferred);
	} else {
		transferred = ur->length - length;
	}

	ur->actual += transferred;

	if ((epnum & 1) && ur->actual < ur->length) {
		/* for some reason we did not get everything out */

		dwc3_ep0_stall_and_restart(dwc);
	} else {
		dwc3_gadget_giveback(ep0, r, 0);
		if (IS_ALIGNED(ur->length, ep0->endpoint.maxpacket) &&
				ur->length && ur->zero) {
			int ret;

			dwc->ep0_next_event = DWC3_EP0_COMPLETE;

			ret = dwc3_ep0_start_trans(dwc, epnum,
					dwc->ctrl_req_addr, 0,
					DWC3_TRBCTL_CONTROL_DATA, 0);
			WARN_ON(ret < 0);
		}
	}
}

int dwc3_gadget_set_test_mode(struct dwc3 *dwc, int mode)
{
	u32		reg;

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_TSTCTRL_MASK;

	switch (mode) {
	case TEST_J:
	case TEST_K:
	case TEST_SE0_NAK:
	case TEST_PACKET:
	case TEST_FORCE_EN:
		reg |= mode << 1;
		break;
	default:
		return -EINVAL;
	}

	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	return 0;
}

static void dwc3_ep0_complete_status(struct dwc3 *dwc,
		const struct dwc3_event_depevt *event)
{
	struct dwc3_request	*r;
	struct dwc3_ep		*dep;
	struct dwc3_trb		*trb;
	u32			status;

	dep = dwc->eps[0];
	trb = dwc->ep0_trb;

	if (!list_empty(&dep->request_list)) {
		r = next_request(&dep->request_list);

		dwc3_gadget_giveback(dep, r, 0);
	}

	if (dwc->test_mode) {
		int ret;

		ret = dwc3_gadget_set_test_mode(dwc, dwc->test_mode_nr);
		if (ret < 0) {
			DWC_ERR("Invalid Test #%d",
					dwc->test_mode_nr);
			dwc3_ep0_stall_and_restart(dwc);
			return;
		}
	}

	status = DWC3_TRB_SIZE_TRBSTS(trb->size);
	if (status == DWC3_TRBSTS_SETUP_PENDING)
		DWC_DBG("Setup Pending received");

	dwc->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dwc);
}

static void dwc3_ep0_xfer_complete(struct dwc3 *dwc,
			const struct dwc3_event_depevt *event)
{
	struct dwc3_ep		*dep = dwc->eps[event->endpoint_number];

	dep->flags &= ~DWC3_EP_BUSY;
	dep->resource_index = 0;
	dwc->setup_packet_pending = false;

	switch (dwc->ep0state) {
	case EP0_SETUP_PHASE:
		DWC_DBG("Setup Phase\n");
		dwc3_ep0_inspect_setup(dwc, event);
		break;

	case EP0_DATA_PHASE:
		DWC_DBG("Data Phase\n");
		dwc3_ep0_complete_data(dwc, event);
		break;

	case EP0_STATUS_PHASE:
		DWC_DBG("Status Phase\n");
		dwc3_ep0_complete_status(dwc, event);
		break;
	default:
		DWC_WARN("UNKNOWN ep0state %d\n", dwc->ep0state);
	}
}

static void __dwc3_ep0_do_control_data(struct dwc3 *dwc,
		struct dwc3_ep *dep, struct dwc3_request *req)
{
	int			ret;
	req->direction = !!dep->number;

	if (req->request.length == 0) {
		ret = dwc3_ep0_start_trans(dwc, dep->number,
					   dwc->ctrl_req_addr, 0,
					   DWC3_TRBCTL_CONTROL_DATA, 0);
	} else if (!IS_ALIGNED(req->request.length, dep->endpoint.maxpacket) &&
			(dep->number == 0)) {
		u32	transfer_size = 0;
		u32	maxpacket;

		ret = usb_gadget_map_request(&dwc->gadget, &req->request,
				dep->number);
		if (ret) {
			DWC_ERR("failed to map request\n");
			return;
		}

		maxpacket = dep->endpoint.maxpacket;
		if (req->request.length > DWC3_EP0_BOUNCE_SIZE) {
			transfer_size = (req->request.length / maxpacket) *
						maxpacket;
			ret = dwc3_ep0_start_trans(dwc, dep->number,
						   req->request.dma,
						   transfer_size,
						   DWC3_TRBCTL_CONTROL_DATA, 1);
		}

		transfer_size = roundup((req->request.length - transfer_size),
					maxpacket);

		dwc->ep0_bounced = true;

		/*
		 * REVISIT in case request length is bigger than
		 * DWC3_EP0_BOUNCE_SIZE we will need two chained
		 * TRBs to handle the transfer.
		 */
		ret = dwc3_ep0_start_trans(dwc, dep->number,
					   dwc->ep0_bounce_addr, transfer_size,
					   DWC3_TRBCTL_CONTROL_DATA, 0);
	} else {
		ret = usb_gadget_map_request(&dwc->gadget, &req->request,
				dep->number);
		if (ret) {
			DWC_ERR("failed to map request\n");
			return;
		}

		ret = dwc3_ep0_start_trans(dwc, dep->number, req->request.dma,
					   req->request.length,
					   DWC3_TRBCTL_CONTROL_DATA, 0);
	}

	WARN_ON(ret < 0);
}

static int dwc3_ep0_start_control_status(struct dwc3_ep *dep)
{
	struct dwc3		*dwc = dep->dwc;
	u32			type;
	type = dwc->three_stage_setup ? DWC3_TRBCTL_CONTROL_STATUS3
		: DWC3_TRBCTL_CONTROL_STATUS2;

	return dwc3_ep0_start_trans(dwc, dep->number,
			dwc->ctrl_req_addr, 0, type, 0);
}

int dwc3_gadget_resize_tx_fifos(struct dwc3 *dwc)
{
	int		last_fifo_depth = 0;
	int		fifo_size;
	int		mdwidth;
	int		num;

	if (!dwc->needs_fifo_resize)
		return 0;

	mdwidth = DWC3_MDWIDTH(dwc->hwparams.hwparams0);

	/* MDWIDTH is represented in bits, we need it in bytes */
	mdwidth >>= 3;

	/*
	 * FIXME For now we will only allocate 1 wMaxPacketSize space
	 * for each enabled endpoint, later patches will come to
	 * improve this algorithm so that we better use the internal
	 * FIFO space
	 */
	for (num = 0; num < dwc->num_in_eps; num++) {
		/* bit0 indicates direction; 1 means IN ep */
		struct dwc3_ep	*dep = dwc->eps[(num << 1) | 1];
		int		mult = 1;
		int		tmp;

		if (!(dep->flags & DWC3_EP_ENABLED))
			continue;

		if (usb_endpoint_xfer_bulk(dep->endpoint.desc)
				|| usb_endpoint_xfer_isoc(dep->endpoint.desc))
			mult = 3;

		/*
		 * REVISIT: the following assumes we will always have enough
		 * space available on the FIFO RAM for all possible use cases.
		 * Make sure that's true somehow and change FIFO allocation
		 * accordingly.
		 *
		 * If we have Bulk or Isochronous endpoints, we want
		 * them to be able to be very, very fast. So we're giving
		 * those endpoints a fifo_size which is enough for 3 full
		 * packets
		 */
		tmp = mult * (dep->endpoint.maxpacket + mdwidth);
		tmp += mdwidth;

		fifo_size = DIV_ROUND_UP(tmp, mdwidth);

		fifo_size |= (last_fifo_depth << 16);

		DWC_DBG("%s: Fifo Addr %04x Size %d\n",
				dep->name, last_fifo_depth, fifo_size & 0xffff);

		dwc3_writel(dwc->regs, DWC3_GTXFIFOSIZ(num), fifo_size);

		last_fifo_depth += (fifo_size & 0xffff);
	}

	return 0;
}

static void __dwc3_ep0_do_control_status(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	if (dwc->resize_fifos) {
		DWC_DBG("Resizing FIFOs\n");
		dwc3_gadget_resize_tx_fifos(dwc);
		dwc->resize_fifos = 0;
	}

	WARN_ON(dwc3_ep0_start_control_status(dep));
}

static void dwc3_ep0_do_control_status(struct dwc3 *dwc,
		const struct dwc3_event_depevt *event)
{
	struct dwc3_ep		*dep = dwc->eps[event->endpoint_number];

	__dwc3_ep0_do_control_status(dwc, dep);
}

static void dwc3_ep0_end_control_data(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;
	u32			cmd;
	int			ret;

	if (!dep->resource_index)
		return;

	cmd = DWC3_DEPCMD_ENDTRANSFER;
	cmd |= DWC3_DEPCMD_CMDIOC;
	cmd |= DWC3_DEPCMD_PARAM(dep->resource_index);
	memset(&params, 0, sizeof(params));
	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number, cmd, &params);
	WARN_ON_ONCE(ret);
	dep->resource_index = 0;
}

static void dwc3_ep0_xfernotready(struct dwc3 *dwc,
		const struct dwc3_event_depevt *event)
{
	dwc->setup_packet_pending = true;

	switch (event->status) {
	case DEPEVT_STATUS_CONTROL_DATA:
		DWC_DBG("Control Data\n");

		/*
		 * We already have a DATA transfer in the controller's cache,
		 * if we receive a XferNotReady(DATA) we will ignore it, unless
		 * it's for the wrong direction.
		 *
		 * In that case, we must issue END_TRANSFER command to the Data
		 * Phase we already have started and issue SetStall on the
		 * control endpoint.
		 */
		if (dwc->ep0_expect_in != event->endpoint_number) {
			struct dwc3_ep	*dep = dwc->eps[dwc->ep0_expect_in];

			DWC_DBG("Wrong direction for Data phase\n");
			dwc3_ep0_end_control_data(dwc, dep);
			dwc3_ep0_stall_and_restart(dwc);
			return;
		}

		break;

	case DEPEVT_STATUS_CONTROL_STATUS:
		if (dwc->ep0_next_event != DWC3_EP0_NRDY_STATUS)
			return;

		DWC_DBG("Control Status\n");

		dwc->ep0state = EP0_STATUS_PHASE;

		if (dwc->delayed_status) {
			WARN_ON_ONCE(event->endpoint_number != 1);
			DWC_DBG("Delayed Status\n");
			return;
		}

		dwc3_ep0_do_control_status(dwc, event);
	}
}

void dwc3_ep0_interrupt(struct dwc3 *dwc,
		const struct dwc3_event_depevt *event)
{
#ifdef DWCDEBUG
	u8 epnum = event->endpoint_number;
#endif
	DWC_DBG("%s while ep%d%s in state '%s'\n",
			dwc3_ep_event_string(event->endpoint_event),
			epnum >> 1, (epnum & 1) ? "in" : "out",
			dwc3_ep0_state_string(dwc->ep0state));

	switch (event->endpoint_event) {
	case DWC3_DEPEVT_XFERCOMPLETE:
		dwc3_ep0_xfer_complete(dwc, event);
		break;

	case DWC3_DEPEVT_XFERNOTREADY:
		dwc3_ep0_xfernotready(dwc, event);
		break;

	case DWC3_DEPEVT_XFERINPROGRESS:
	case DWC3_DEPEVT_RXTXFIFOEVT:
	case DWC3_DEPEVT_STREAMEVT:
	case DWC3_DEPEVT_EPCMDCMPLT:
		break;
	}
}
