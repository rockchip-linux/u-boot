/*
 * Based on drivers/usb/gadget/omap1510_udc.c
 * TI OMAP1510 USB bus interface driver
 *
 * (C) Copyright 2009
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
 *
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <dwc3-uboot.h>
#include <asm/dma-mapping.h>
#include <linux/bug.h>
#include <asm/arch/rkplat.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <usb/dwc3_rk_udc.h>
#include "../dwc3/core.h"
#include "../dwc3/io.h"
#include "../dwc3/gadget.h"
#include "../dwc3/linux-compat.h"

#define CONFIG_USB_DWC3_GADGET
#define POINTER_ALIGN(p, a)		((typeof(p))ALIGN((unsigned long)(p), (a)))



#ifndef RKIO_USB3_BASE
	#error "PLS config usb3  base!"
#endif
extern void rockusb_disable(struct usb_gadget *gadget);

static struct dwc3_device dwc3_device_data;
static struct usb_endpoint_descriptor dwc3_gadget_ep0_desc = {
	.bLength	= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bmAttributes	= USB_ENDPOINT_XFER_CONTROL,
};



#define DWC3_ALIGN_MASK		(16 - 1)

#define USB_BUFSIZ	4096

struct rk_dwc3_udc_instance global_rk_instance;
static struct rk_udc_private_data private_data = {
.in_ep = NULL,
.out_ep = NULL,
.in_req = NULL,
.in_req2 = NULL,
.out_req = NULL,
.out_req2 = NULL,
.ep0_req = NULL,
};

static struct dwc3	*global_dwc;

int usb_gadget_map_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return 0;

	req->dma = dma_map_single(req->buf, req->length,
				  is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	return 0;
}

void usb_gadget_unmap_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return;

	dma_unmap_single((void *)(uintptr_t)req->dma, req->length,
			 is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
}

void usb_gadget_giveback_request(struct usb_ep *ep,
		struct usb_request *req)
{
	DWC_DBG("ep_name=%s,ep_addr=0x%02x\n", ep->name, ep->desc->bEndpointAddress);
	if ((ep->desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_CONTROL)
		return;
	if (ep->desc->bEndpointAddress & 0x80)
		global_rk_instance.tx_handler(req->status, req->actual, req->buf);
	else
		global_rk_instance.rx_handler(req->status, req->actual, req->buf);
}

void usb_gadget_set_state(struct usb_gadget *gadget,
		enum usb_device_state state)
{
	gadget->state = state;
}

static int dwc3_alloc_trb_pool(struct dwc3_ep *dep)
{
	if (dep->trb_pool)
		return 0;

	if (dep->number == 0 || dep->number == 1)
		return 0;

	dep->trb_pool = dma_alloc_coherent(sizeof(struct dwc3_trb) *
					   DWC3_TRB_NUM,
					   (unsigned long *)&dep->trb_pool_dma);
	if (!dep->trb_pool) {
		DWC_ERR("failed to allocate trb pool for %s\n",
				dep->name);
		return -ENOMEM;
	}

	return 0;
}

static void dwc3_free_trb_pool(struct dwc3_ep *dep)
{
	dma_free_coherent(dep->trb_pool);

	dep->trb_pool = NULL;
	dep->trb_pool_dma = 0;
}

static int dwc3_gadget_ep0_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	return -EINVAL;
}

static int dwc3_gadget_ep0_disable(struct usb_ep *ep)
{
	return -EINVAL;
}

static struct usb_request *dwc3_gadget_ep_alloc_request(struct usb_ep *ep,
	gfp_t gfp_flags)
{
	struct dwc3_request		*req;
	struct dwc3_ep			*dep = to_dwc3_ep(ep);

	req = kzalloc(sizeof(*req), gfp_flags);
	if (!req)
		return NULL;

	req->epnum	= dep->number;
	req->dep	= dep;

	return &req->request;
}

static void dwc3_gadget_ep_free_request(struct usb_ep *ep,
		struct usb_request *request)
{
	struct dwc3_request		*req = to_dwc3_request(request);

	kfree(req);
}

int dwc3_send_gadget_ep_cmd(struct dwc3 *dwc, unsigned ep,
		unsigned cmd, struct dwc3_gadget_ep_cmd_params *params)
{
	u32			timeout = 500;
	u32			reg;
	DWC_DBG("<<DWC3>> ep=%02x,cmd=%x\n", ep, cmd);
	dwc3_writel(dwc->regs, DWC3_DEPCMDPAR0(ep), params->param0);
	dwc3_writel(dwc->regs, DWC3_DEPCMDPAR1(ep), params->param1);
	dwc3_writel(dwc->regs, DWC3_DEPCMDPAR2(ep), params->param2);

	dwc3_writel(dwc->regs, DWC3_DEPCMD(ep), cmd | DWC3_DEPCMD_CMDACT);
	do {
		reg = dwc3_readl(dwc->regs, DWC3_DEPCMD(ep));
		if (!(reg & DWC3_DEPCMD_CMDACT)) {
			DWC_DBG("Command Complete --> %d\n",
					DWC3_DEPCMD_STATUS(reg));
			return 0;
		}

		/*
		 * We can't sleep here, because it is also called from
		 * interrupt context.
		 */
		timeout--;
		if (!timeout)
			return -ETIMEDOUT;

		udelay(1);
	} while (1);
}

static void dwc3_stop_active_transfer(struct dwc3 *dwc, u32 epnum, bool force)
{
	struct dwc3_ep *dep;
	struct dwc3_gadget_ep_cmd_params params;
	u32 cmd;
	int ret;

	dep = dwc->eps[epnum];

	if (!dep->resource_index)
		return;

	/*
	 * NOTICE: We are violating what the Databook says about the
	 * EndTransfer command. Ideally we would _always_ wait for the
	 * EndTransfer Command Completion IRQ, but that's causing too
	 * much trouble synchronizing between us and gadget driver.
	 *
	 * We have discussed this with the IP Provider and it was
	 * suggested to giveback all requests here, but give HW some
	 * extra time to synchronize with the interconnect. We're using
	 * an arbitraty 100us delay for that.
	 *
	 * Note also that a similar handling was tested by Synopsys
	 * (thanks a lot Paul) and nothing bad has come out of it.
	 * In short, what we're doing is:
	 *
	 * - Issue EndTransfer WITH CMDIOC bit set
	 * - Wait 100us
	 */

	cmd = DWC3_DEPCMD_ENDTRANSFER;
	cmd |= force ? DWC3_DEPCMD_HIPRI_FORCERM : 0;
	cmd |= DWC3_DEPCMD_CMDIOC;
	cmd |= DWC3_DEPCMD_PARAM(dep->resource_index);
	memset(&params, 0, sizeof(params));
	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number, cmd, &params);
	WARN_ON_ONCE(ret);
	dep->resource_index = 0;
	dep->flags &= ~DWC3_EP_BUSY;
	udelay(100);
}

void dwc3_gadget_giveback(struct dwc3_ep *dep, struct dwc3_request *req,
		int status)
{
	struct dwc3 *dwc = dep->dwc;
	if (req->queued) {
		dep->busy_slot++;
		/*
		 * Skip LINK TRB. We can't use req->trb and check for
		 * DWC3_TRBCTL_LINK_TRB because it points the TRB we
		 * just completed (not the LINK TRB).
		 */
		if (((dep->busy_slot & DWC3_TRB_MASK) ==
			DWC3_TRB_NUM - 1) &&
			usb_endpoint_xfer_isoc(dep->endpoint.desc))
			dep->busy_slot++;
		req->queued = false;
	}

	list_del(&req->list);
	req->trb = NULL;
	dwc3_flush_cache((long)req->request.dma, req->request.length);

	if (req->request.status == -EINPROGRESS)
		req->request.status = status;

	if (dwc->ep0_bounced && dep->number == 0)
		dwc->ep0_bounced = false;
	else
		usb_gadget_unmap_request(&dwc->gadget, &req->request,
				req->direction);

	DWC_DBG("request %p from %s completed %d/%d ===> %d\n",
			req, dep->name, req->request.actual,
			req->request.length, status);

	spin_unlock(&dwc->lock);
	usb_gadget_giveback_request(&dep->endpoint, &req->request);
	spin_lock(&dwc->lock);
}

static int dwc3_gadget_ep_dequeue(struct usb_ep *ep,
		struct usb_request *request)
{
	struct dwc3_request		*req = to_dwc3_request(request);
	struct dwc3_request		*r = NULL;

	struct dwc3_ep			*dep = to_dwc3_ep(ep);
	struct dwc3			*dwc = dep->dwc;

	unsigned long			flags;
	int				ret = 0;

	spin_lock_irqsave(&dwc->lock, flags);

	list_for_each_entry(r, &dep->request_list, list) {
		if (r == req)
			break;
	}

	if (r != req) {
		list_for_each_entry(r, &dep->req_queued, list) {
			if (r == req)
				break;
		}
		if (r == req) {
			/* wait until it is processed */
			dwc3_stop_active_transfer(dwc, dep->number, true);
			goto out1;
		}
		DWC_ERR("request %p was not queued to %s\n",
				request, ep->name);
		ret = -EINVAL;
		goto out0;
	}

out1:
	/* giveback the request */
	dwc3_gadget_giveback(dep, req, -ECONNRESET);

out0:
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

int __dwc3_gadget_ep_set_halt(struct dwc3_ep *dep, int value, int protocol)
{
	struct dwc3_gadget_ep_cmd_params	params;
	struct dwc3				*dwc = dep->dwc;
	int					ret;

	if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
		DWC_ERR("%s is of Isochronous type\n", dep->name);
		return -EINVAL;
	}

	memset(&params, 0x00, sizeof(params));

	if (value) {
		if (!protocol && ((dep->direction && dep->flags & DWC3_EP_BUSY) ||
				(!list_empty(&dep->req_queued) ||
				 !list_empty(&dep->request_list)))) {
			DWC_DBG("%s: pending request, cannot halt\n",
					dep->name);
			return -EAGAIN;
		}

		ret = dwc3_send_gadget_ep_cmd(dwc, dep->number,
			DWC3_DEPCMD_SETSTALL, &params);
		if (ret)
			DWC_ERR("failed to set STALL on %s\n",
					dep->name);
		else
			dep->flags |= DWC3_EP_STALL;
	} else {
		ret = dwc3_send_gadget_ep_cmd(dwc, dep->number,
			DWC3_DEPCMD_CLEARSTALL, &params);
		if (ret)
			DWC_ERR("failed to clear STALL on %s\n",
					dep->name);
		else
			dep->flags &= ~(DWC3_EP_STALL | DWC3_EP_WEDGE);
	}

	return ret;
}

static int dwc3_gadget_ep_set_wedge(struct usb_ep *ep)
{
	struct dwc3_ep			*dep = to_dwc3_ep(ep);
	unsigned long			flags;
	int				ret;

	spin_lock_irqsave(&dwc->lock, flags);
	dep->flags |= DWC3_EP_WEDGE;

	if (dep->number == 0 || dep->number == 1)
		ret = __dwc3_gadget_ep0_set_halt(ep, 1);
	else
		ret = __dwc3_gadget_ep_set_halt(dep, 1, false);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_start_config(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;
	u32			cmd;
	int			ret;
	memset(&params, 0x00, sizeof(params));

	if (dep->number != 1) {
		cmd = DWC3_DEPCMD_DEPSTARTCFG;
		/* XferRscIdx == 0 for ep0 and 2 for the remaining */
		if (dep->number > 1) {
			if (dwc->start_config_issued)
				return 0;
			dwc->start_config_issued = true;
			cmd |= DWC3_DEPCMD_PARAM(2);
		}
		ret = dwc3_send_gadget_ep_cmd(dwc, 0, cmd, &params);

		return ret;
	}

	return 0;
}

static int dwc3_gadget_set_ep_config(struct dwc3 *dwc, struct dwc3_ep *dep,
		const struct usb_endpoint_descriptor *desc,
		const struct usb_ss_ep_comp_descriptor *comp_desc,
		bool ignore, bool restore)
{
	struct dwc3_gadget_ep_cmd_params params;
	int ret;
	memset(&params, 0x00, sizeof(params));

	params.param0 = DWC3_DEPCFG_EP_TYPE(usb_endpoint_type(desc))
		| DWC3_DEPCFG_MAX_PACKET_SIZE(usb_endpoint_maxp(desc));

	/* Burst size is only needed in SuperSpeed mode */
	if (dwc->gadget.speed == USB_SPEED_SUPER) {
		u32 burst = dep->endpoint.maxburst - 1;

		params.param0 |= DWC3_DEPCFG_BURST_SIZE(burst);
	}

	if (ignore)
		params.param0 |= DWC3_DEPCFG_IGN_SEQ_NUM;

	if (restore) {
		params.param0 |= DWC3_DEPCFG_ACTION_RESTORE;
		params.param2 |= dep->saved_state;
	}

	params.param1 = DWC3_DEPCFG_XFER_COMPLETE_EN
		| DWC3_DEPCFG_XFER_NOT_READY_EN;

	if (usb_ss_max_streams(comp_desc) && usb_endpoint_xfer_bulk(desc)) {
		params.param1 |= DWC3_DEPCFG_STREAM_CAPABLE
			| DWC3_DEPCFG_STREAM_EVENT_EN;
		dep->stream_capable = true;
	}

	if (!usb_endpoint_xfer_control(desc))
		params.param1 |= DWC3_DEPCFG_XFER_IN_PROGRESS_EN;

	/*
	 * We are doing 1:1 mapping for endpoints, meaning
	 * Physical Endpoints 2 maps to Logical Endpoint 2 and
	 * so on. We consider the direction bit as part of the physical
	 * endpoint number. So USB endpoint 0x81 is 0x03.
	 */
	params.param1 |= DWC3_DEPCFG_EP_NUMBER(dep->number);

	/*
	 * We must use the lower 16 TX FIFOs even though
	 * HW might have more
	 */
	if (dep->direction) {
		params.param0 |= DWC3_DEPCFG_FIFO_NUMBER(dep->number >> 1);
	}

	if (desc->bInterval) {
		params.param1 |= DWC3_DEPCFG_BINTERVAL_M1(desc->bInterval - 1);
		dep->interval = 1 << (desc->bInterval - 1);
	}
	DWC_DBG("<<Dwc3>> param0=%x,param1=%x,param2=%x.\n", params.param0,
														params.param1,
														params.param2);
	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number, DWC3_DEPCMD_SETEPCONFIG, &params);

	return ret;
}

static int dwc3_gadget_set_xfer_resource(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	struct dwc3_gadget_ep_cmd_params params;
	int ret;
	memset(&params, 0x00, sizeof(params));

	params.param0 = DWC3_DEPXFERCFG_NUM_XFER_RES(1);
	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number, DWC3_DEPCMD_SETTRANSFRESOURCE, &params);

	return ret;
}

static void dwc3_remove_requests(struct dwc3 *dwc, struct dwc3_ep *dep)
{
	struct dwc3_request		*req;

	if (!list_empty(&dep->req_queued)) {
		dwc3_stop_active_transfer(dwc, dep->number, true);

		/* - giveback all requests to gadget driver */
		while (!list_empty(&dep->req_queued)) {
			req = next_request(&dep->req_queued);

			dwc3_gadget_giveback(dep, req, -ESHUTDOWN);
		}
	}

	while (!list_empty(&dep->request_list)) {
		req = next_request(&dep->request_list);

		dwc3_gadget_giveback(dep, req, -ESHUTDOWN);
	}
}

static int __dwc3_gadget_ep_disable(struct dwc3_ep *dep)
{
	struct dwc3		*dwc = dep->dwc;
	u32			reg;

	dwc3_remove_requests(dwc, dep);

	/* make sure HW endpoint isn't stalled */
	if (dep->flags & DWC3_EP_STALL)
		__dwc3_gadget_ep_set_halt(dep, 0, false);

	reg = dwc3_readl(dwc->regs, DWC3_DALEPENA);
	reg &= ~DWC3_DALEPENA_EP(dep->number);
	dwc3_writel(dwc->regs, DWC3_DALEPENA, reg);

	dep->stream_capable = false;
	dep->endpoint.desc = NULL;
	dep->comp_desc = NULL;
	dep->type = 0;
	dep->flags = 0;

	return 0;
}

static dma_addr_t dwc3_trb_dma_offset(struct dwc3_ep *dep,
		struct dwc3_trb *trb)
{
	u32		offset = (char *) trb - (char *) dep->trb_pool;

	return dep->trb_pool_dma + offset;
}

static int __dwc3_gadget_ep_enable(struct dwc3_ep *dep,
		const struct usb_endpoint_descriptor *desc,
		const struct usb_ss_ep_comp_descriptor *comp_desc,
		bool ignore, bool restore)
{
	struct dwc3		*dwc = dep->dwc;
	u32			reg;
	int			ret;

	DWC_DBG("Enabling %s\n", dep->name);

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		ret = dwc3_gadget_start_config(dwc, dep);
		if (ret)
			return ret;
	}

	ret = dwc3_gadget_set_ep_config(dwc, dep, desc, comp_desc, ignore,
			restore);
	if (ret)
		return ret;

	if (!(dep->flags & DWC3_EP_ENABLED)) {
		struct dwc3_trb	*trb_st_hw;
		struct dwc3_trb	*trb_link;

		ret = dwc3_gadget_set_xfer_resource(dwc, dep);
		if (ret)
			return ret;

		dep->endpoint.desc = desc;
		dep->comp_desc = comp_desc;
		dep->type = usb_endpoint_type(desc);
		dep->flags |= DWC3_EP_ENABLED;

		reg = dwc3_readl(dwc->regs, DWC3_DALEPENA);
		reg |= DWC3_DALEPENA_EP(dep->number);
		dwc3_writel(dwc->regs, DWC3_DALEPENA, reg);

		if (!usb_endpoint_xfer_isoc(desc))
			return 0;
		/* Link TRB for ISOC. The HWO bit is never reset */
		trb_st_hw = &dep->trb_pool[0];

		trb_link = &dep->trb_pool[DWC3_TRB_NUM - 1];
		memset(trb_link, 0, sizeof(*trb_link));

		trb_link->bpl = lower_32_bits(dwc3_trb_dma_offset(dep, trb_st_hw));
		trb_link->bph = upper_32_bits(dwc3_trb_dma_offset(dep, trb_st_hw));
		trb_link->ctrl |= DWC3_TRBCTL_LINK_TRB;
		trb_link->ctrl |= DWC3_TRB_CTRL_HWO;
	}

	return 0;
}

static int dwc3_gadget_ep_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct dwc3_ep			*dep;
	int				ret;

	if (!ep || !desc || desc->bDescriptorType != USB_DT_ENDPOINT) {
		DWC_ERR("dwc3: invalid parameters\n");
		return -EINVAL;
	}

	if (!desc->wMaxPacketSize) {
		DWC_ERR("dwc3: missing wMaxPacketSize\n");
		return -EINVAL;
	}

	dep = to_dwc3_ep(ep);
	if (dep->flags & DWC3_EP_ENABLED) {
		DWC_WARN("%s is already enabled\n",
				dep->name);
		return 0;
	}

	switch (usb_endpoint_type(desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		strlcat(dep->name, "-control", sizeof(dep->name));
		break;
	case USB_ENDPOINT_XFER_ISOC:
		strlcat(dep->name, "-isoc", sizeof(dep->name));
		break;
	case USB_ENDPOINT_XFER_BULK:
		strlcat(dep->name, "-bulk", sizeof(dep->name));
		break;
	case USB_ENDPOINT_XFER_INT:
		strlcat(dep->name, "-int", sizeof(dep->name));
		break;
	default:
		DWC_ERR("invalid endpoint transfer type\n");
	}

	ret = __dwc3_gadget_ep_enable(dep, desc, ep->comp_desc, false, false);

	return ret;
}

static int dwc3_gadget_ep_disable(struct usb_ep *ep)
{
	struct dwc3_ep			*dep;
	int				ret;

	if (!ep) {
		DWC_ERR("dwc3: invalid parameters\n");
		return -EINVAL;
	}

	dep = to_dwc3_ep(ep);
	if (!(dep->flags & DWC3_EP_ENABLED)) {
		DWC_WARN("%s is already disabled\n",
				dep->name);
		return 0;
	}

	snprintf(dep->name, sizeof(dep->name), "ep%d%s",
			dep->number >> 1,
			(dep->number & 1) ? "in" : "out");

	ret = __dwc3_gadget_ep_disable(dep);

	return ret;
}

static void dwc3_prepare_one_trb(struct dwc3_ep *dep,
		struct dwc3_request *req, dma_addr_t dma,
		unsigned length, unsigned last, unsigned chain, unsigned node)
{
	struct dwc3_trb		*trb;

	DWC_DBG("%s: req %p dma %08llx length %d%s%s\n",
			dep->name, req, (unsigned long long) dma,
			length, last ? " last" : "",
			chain ? " chain" : "");


	trb = &dep->trb_pool[dep->free_slot & DWC3_TRB_MASK];

	if (!req->trb) {
		dwc3_gadget_move_request_queued(req);
		req->trb = trb;
		req->trb_dma = dwc3_trb_dma_offset(dep, trb);
		req->start_slot = dep->free_slot & DWC3_TRB_MASK;
	}

	dep->free_slot++;
	/* Skip the LINK-TRB on ISOC */
	if (((dep->free_slot & DWC3_TRB_MASK) == DWC3_TRB_NUM - 1) &&
			usb_endpoint_xfer_isoc(dep->endpoint.desc))
		dep->free_slot++;

	trb->size = DWC3_TRB_SIZE_LENGTH(length);
	trb->bpl = lower_32_bits(dma);
	trb->bph = upper_32_bits(dma);

	switch (usb_endpoint_type(dep->endpoint.desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		trb->ctrl = DWC3_TRBCTL_CONTROL_SETUP;
		break;

	case USB_ENDPOINT_XFER_ISOC:
		if (!node)
			trb->ctrl = DWC3_TRBCTL_ISOCHRONOUS_FIRST;
		else
			trb->ctrl = DWC3_TRBCTL_ISOCHRONOUS;
		break;

	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		trb->ctrl = DWC3_TRBCTL_NORMAL;
		break;
	default:
		/*
		 * This is only possible with faulty memory because we
		 * checked it already :)
		 */
		BUG();
	}

	if (!req->request.no_interrupt && !chain)
		trb->ctrl |= DWC3_TRB_CTRL_IOC;

	if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
		trb->ctrl |= DWC3_TRB_CTRL_ISP_IMI;
		trb->ctrl |= DWC3_TRB_CTRL_CSP;
	} else if (last) {
		trb->ctrl |= DWC3_TRB_CTRL_LST;
	}

	if (chain)
		trb->ctrl |= DWC3_TRB_CTRL_CHN;

	if (usb_endpoint_xfer_bulk(dep->endpoint.desc) && dep->stream_capable)
		trb->ctrl |= DWC3_TRB_CTRL_SID_SOFN(req->request.stream_id);

	trb->ctrl |= DWC3_TRB_CTRL_HWO;

	dwc3_flush_cache((long)dma, length);
	dwc3_flush_cache((long)trb, sizeof(*trb));
}

static void dwc3_prepare_trbs(struct dwc3_ep *dep, bool starting)
{
	struct dwc3_request	*req, *n;
	u32			trbs_left;
	u32			max;

	BUILD_BUG_ON_NOT_POWER_OF_2(DWC3_TRB_NUM);

	/* the first request must not be queued */
	trbs_left = (dep->busy_slot - dep->free_slot) & DWC3_TRB_MASK;

	/* Can't wrap around on a non-isoc EP since there's no link TRB */
	if (!usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
		max = DWC3_TRB_NUM - (dep->free_slot & DWC3_TRB_MASK);
		if (trbs_left > max)
			trbs_left = max;
	}

	/*
	 * If busy & slot are equal than it is either full or empty. If we are
	 * starting to process requests then we are empty. Otherwise we are
	 * full and don't do anything
	 */
	if (!trbs_left) {
		if (!starting)
			return;
		trbs_left = DWC3_TRB_NUM;
		/*
		 * In case we start from scratch, we queue the ISOC requests
		 * starting from slot 1. This is done because we use ring
		 * buffer and have no LST bit to stop us. Instead, we place
		 * IOC bit every TRB_NUM/4. We try to avoid having an interrupt
		 * after the first request so we start at slot 1 and have
		 * 7 requests proceed before we hit the first IOC.
		 * Other transfer types don't use the ring buffer and are
		 * processed from the first TRB until the last one. Since we
		 * don't wrap around we have to start at the beginning.
		 */
		if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			dep->busy_slot = 1;
			dep->free_slot = 1;
		} else {
			dep->busy_slot = 0;
			dep->free_slot = 0;
		}
	}

	/* The last TRB is a link TRB, not used for xfer */
	if ((trbs_left <= 1) && usb_endpoint_xfer_isoc(dep->endpoint.desc))
		return;

	list_for_each_entry_safe(req, n, &dep->request_list, list) {
		unsigned	length;
		dma_addr_t	dma;

		dma = req->request.dma;
		length = req->request.length;

		dwc3_prepare_one_trb(dep, req, dma, length,
				     true, false, 0);

		break;
	}
}

static int __dwc3_gadget_kick_transfer(struct dwc3_ep *dep, u16 cmd_param,
		int start_new)
{
	struct dwc3_gadget_ep_cmd_params params;
	struct dwc3_request		*req;
	struct dwc3			*dwc = dep->dwc;
	int				ret;
	u32				cmd;

	if (start_new && (dep->flags & DWC3_EP_BUSY)) {
		DWC_DBG("%s: endpoint busy\n", dep->name);
		return -EBUSY;
	}
	dep->flags &= ~DWC3_EP_PENDING_REQUEST;

	/*
	 * If we are getting here after a short-out-packet we don't enqueue any
	 * new requests as we try to set the IOC bit only on the last request.
	 */
	if (start_new) {
		if (list_empty(&dep->req_queued))
			dwc3_prepare_trbs(dep, start_new);

		/* req points to the first request which will be sent */
		req = next_request(&dep->req_queued);
	} else {
		dwc3_prepare_trbs(dep, start_new);

		/*
		 * req points to the first request where HWO changed from 0 to 1
		 */
		req = next_request(&dep->req_queued);
	}
	if (!req) {
		dep->flags |= DWC3_EP_PENDING_REQUEST;
		return 0;
	}

	memset(&params, 0, sizeof(params));

	if (start_new) {
		params.param0 = upper_32_bits(req->trb_dma);
		params.param1 = lower_32_bits(req->trb_dma);
		cmd = DWC3_DEPCMD_STARTTRANSFER;
	} else {
		cmd = DWC3_DEPCMD_UPDATETRANSFER;
	}

	cmd |= DWC3_DEPCMD_PARAM(cmd_param);
	ret = dwc3_send_gadget_ep_cmd(dwc, dep->number, cmd, &params);
	if (ret < 0) {
		DWC_ERR("failed to send STARTTRANSFER command\n");

		/*
		 * FIXME we need to iterate over the list of requests
		 * here and stop, unmap, free and del each of the linked
		 * requests instead of what we do now.
		 */
		usb_gadget_unmap_request(&dwc->gadget, &req->request,
				req->direction);
		list_del(&req->list);
		return ret;
	}

	dep->flags |= DWC3_EP_BUSY;

	if (start_new) {
		dep->resource_index = dwc3_gadget_ep_get_transfer_index(dwc,
				dep->number);
		DWC_DBG("ep=%d,resource_index=%d\n", dep->number, dep->resource_index);
	}

	return 0;
}

static int __dwc3_gadget_ep_queue(struct dwc3_ep *dep, struct dwc3_request *req)
{
	struct dwc3		*dwc = dep->dwc;
	int			ret;

	req->request.actual	= 0;
	req->request.status	= -EINPROGRESS;
	req->direction		= dep->direction;
	req->epnum		= dep->number;

	/*
	 * DWC3 hangs on OUT requests smaller than maxpacket size,
	 * so HACK the request length
	 */
	if (dep->direction == 0) {
		if (req->request.length < dep->endpoint.maxpacket)
			req->request.length = dep->endpoint.maxpacket;
		else
			req->request.length = (((req->request.length - 1) / dep->endpoint.maxpacket) + 1) * dep->endpoint.maxpacket;
	}

	/*
	 * We only add to our list of requests now and
	 * start consuming the list once we get XferNotReady
	 * IRQ.
	 *
	 * That way, we avoid doing anything that we don't need
	 * to do now and defer it until the point we receive a
	 * particular token from the Host side.
	 *
	 * This will also avoid Host cancelling URBs due to too
	 * many NAKs.
	 */
	ret = usb_gadget_map_request(&dwc->gadget, &req->request,
			dep->direction);
	if (ret)
		return ret;

	list_add_tail(&req->list, &dep->request_list);

	/*
	 * There are a few special cases:
	 *
	 * 1. XferNotReady with empty list of requests. We need to kick the
	 *    transfer here in that situation, otherwise we will be NAKing
	 *    forever. If we get XferNotReady before gadget driver has a
	 *    chance to queue a request, we will ACK the IRQ but won't be
	 *    able to receive the data until the next request is queued.
	 *    The following code is handling exactly that.
	 *
	 */
	if (dep->flags & DWC3_EP_PENDING_REQUEST) {
		/*
		 * If xfernotready is already elapsed and it is a case
		 * of isoc transfer, then issue END TRANSFER, so that
		 * you can receive xfernotready again and can have
		 * notion of current microframe.
		 */
		if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			if (list_empty(&dep->req_queued)) {
				dwc3_stop_active_transfer(dwc, dep->number, true);
				dep->flags = DWC3_EP_ENABLED;
			}
			return 0;
		}

		ret = __dwc3_gadget_kick_transfer(dep, 0, true);
		if (ret && ret != -EBUSY)
			DWC_ERR("%s: failed to kick transfers\n",
					dep->name);
		return ret;
	}

	/*
	 * 2. XferInProgress on Isoc EP with an active transfer. We need to
	 *    kick the transfer here after queuing a request, otherwise the
	 *    core may not see the modified TRB(s).
	 */
	if (usb_endpoint_xfer_isoc(dep->endpoint.desc) &&
			(dep->flags & DWC3_EP_BUSY) &&
			!(dep->flags & DWC3_EP_MISSED_ISOC)) {
		ret = __dwc3_gadget_kick_transfer(dep, dep->resource_index,
				false);
		if (ret && ret != -EBUSY)
			DWC_ERR("%s: failed to kick transfers\n",
					dep->name);
		return ret;
	}

	/*
	 * 4. Stream Capable Bulk Endpoints. We need to start the transfer
	 * right away, otherwise host will not know we have streams to be
	 * handled.
	 */
	if (dep->stream_capable) {
		int	ret;

		ret = __dwc3_gadget_kick_transfer(dep, 0, true);
		if (ret && ret != -EBUSY) {
			DWC_ERR("%s: failed to kick transfers\n",
					dep->name);
		}
	}

	return 0;
}

static int dwc3_gadget_ep_set_halt(struct usb_ep *ep, int value)
{
	struct dwc3_ep			*dep = to_dwc3_ep(ep);

	unsigned long			flags;

	int				ret;

	spin_lock_irqsave(&dwc->lock, flags);
	ret = __dwc3_gadget_ep_set_halt(dep, value, false);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_ep_queue(struct usb_ep *ep, struct usb_request *request,
	gfp_t gfp_flags)
{
	struct dwc3_request		*req = to_dwc3_request(request);
	struct dwc3_ep			*dep = to_dwc3_ep(ep);

	int				ret;

	if (!dep->endpoint.desc) {
		DWC_ERR("trying to queue request %p to disabled %s\n",
				request, ep->name);
		ret = -ESHUTDOWN;
		goto out;
	}

	if (req->dep != dep) {
		DWC_ERR("request %p belongs to '%s'\n",
				request, req->dep->name);
		ret = -EINVAL;
		goto out;
	}

	DWC_DBG("queing request %p to %s length %d\n",
			request, ep->name, request->length);

	ret = __dwc3_gadget_ep_queue(dep, req);

out:
	if (!ret) {
		if (gfp_flags == 0xFF)
			ret = __dwc3_gadget_kick_transfer(dep, 0, 1);
	}
	return ret;
}

static int dwc3_gadget_get_frame(struct usb_gadget *g)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	u32			reg;

	reg = dwc3_readl(dwc->regs, DWC3_DSTS);
	return DWC3_DSTS_SOFFN(reg);
}

int dwc3_gadget_set_link_state(struct dwc3 *dwc, enum dwc3_link_state state)
{
	int		retries = 10000;
	u32		reg;

	/*
	 * Wait until device controller is ready. Only applies to 1.94a and
	 * later RTL.
	 */
	if (dwc->revision >= DWC3_REVISION_194A) {
		while (--retries) {
			reg = dwc3_readl(dwc->regs, DWC3_DSTS);
			if (reg & DWC3_DSTS_DCNRD)
				udelay(5);
			else
				break;
		}

		if (retries <= 0)
			return -ETIMEDOUT;
	}

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_ULSTCHNGREQ_MASK;

	/* set requested state */
	reg |= DWC3_DCTL_ULSTCHNGREQ(state);
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	/*
	 * The following code is racy when called from dwc3_gadget_wakeup,
	 * and is not needed, at least on newer versions
	 */
	if (dwc->revision >= DWC3_REVISION_194A)
		return 0;

	/* wait for a change in DSTS */
	retries = 10000;
	while (--retries) {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);

		if (DWC3_DSTS_USBLNKST(reg) == state)
			return 0;

		udelay(5);
	}

	DWC_DBG("link state change request timed out\n");

	return -ETIMEDOUT;
}

static int dwc3_gadget_wakeup(struct usb_gadget *g)
{
	struct dwc3		*dwc = gadget_to_dwc(g);

	unsigned long		timeout;
	unsigned long		flags;

	u32			reg;

	int			ret = 0;

	u8			link_state;
	u8			speed;

	spin_lock_irqsave(&dwc->lock, flags);

	/*
	 * According to the Databook Remote wakeup request should
	 * be issued only when the device is in early suspend state.
	 *
	 * We can check that via USB Link State bits in DSTS register.
	 */
	reg = dwc3_readl(dwc->regs, DWC3_DSTS);

	speed = reg & DWC3_DSTS_CONNECTSPD;
	if (speed == DWC3_DSTS_SUPERSPEED) {
		DWC_ERR("no wakeup on SuperSpeed\n");
		ret = -EINVAL;
		goto out;
	}

	link_state = DWC3_DSTS_USBLNKST(reg);

	switch (link_state) {
	case DWC3_LINK_STATE_RX_DET:	/* in HS, means Early Suspend */
	case DWC3_LINK_STATE_U3:	/* in HS, means SUSPEND */
		break;
	default:
		DWC_ERR("can't wakeup from link state %d\n",
				link_state);
		ret = -EINVAL;
		goto out;
	}

	ret = dwc3_gadget_set_link_state(dwc, DWC3_LINK_STATE_RECOV);
	if (ret < 0) {
		DWC_ERR("failed to put link in Recovery\n");
		goto out;
	}

	/* Recent versions do this automatically */
	if (dwc->revision < DWC3_REVISION_194A) {
		/* write zeroes to Link Change Request */
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~DWC3_DCTL_ULSTCHNGREQ_MASK;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	}

	/* poll until Link State changes to ON */
	timeout = 1000;

	while (timeout--) {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);

		/* in HS, means ON */
		if (DWC3_DSTS_USBLNKST(reg) == DWC3_LINK_STATE_U0)
			break;
	}

	if (DWC3_DSTS_USBLNKST(reg) != DWC3_LINK_STATE_U0) {
		DWC_ERR("failed to send remote wakeup\n");
		ret = -EINVAL;
	}

out:
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_set_selfpowered(struct usb_gadget *g,
		int is_selfpowered)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	unsigned long		flags;

	spin_lock_irqsave(&dwc->lock, flags);
	dwc->is_selfpowered = !!is_selfpowered;
	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;
}

static int dwc3_gadget_run_stop(struct dwc3 *dwc, int is_on, int suspend)
{
	u32			reg;
	u32			timeout = 500;
	DWC_DBG("<<Dwc3>> is_on=%d,suspend=%d.\n", is_on, suspend);
	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	if (is_on) {
		if (dwc->revision <= DWC3_REVISION_187A) {
			reg &= ~DWC3_DCTL_TRGTULST_MASK;
			reg |= DWC3_DCTL_TRGTULST_RX_DET;
		}

		if (dwc->revision >= DWC3_REVISION_194A)
			reg &= ~DWC3_DCTL_KEEP_CONNECT;
		reg |= DWC3_DCTL_RUN_STOP;

		if (dwc->has_hibernation)
			reg |= DWC3_DCTL_KEEP_CONNECT;

		dwc->pullups_connected = true;
	} else {
		reg &= ~DWC3_DCTL_RUN_STOP;

		if (dwc->has_hibernation && !suspend)
			reg &= ~DWC3_DCTL_KEEP_CONNECT;

		dwc->pullups_connected = false;
	}

	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	do {
		reg = dwc3_readl(dwc->regs, DWC3_DSTS);
		if (is_on) {
			if (!(reg & DWC3_DSTS_DEVCTRLHLT))
				break;
		} else {
			if (reg & DWC3_DSTS_DEVCTRLHLT)
				break;
		}
		timeout--;
		if (!timeout)
			return -ETIMEDOUT;
		udelay(1);
	} while (1);

	DWC_DBG("gadget %s data soft-%s\n",
			dwc->gadget_driver
			? dwc->gadget_driver->function : "no-function",
			is_on ? "connect" : "disconnect");

	return 0;
}

static int dwc3_gadget_pullup(struct usb_gadget *g, int is_on)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	unsigned long		flags;
	int			ret;
	is_on = !!is_on;

	spin_lock_irqsave(&dwc->lock, flags);
	ret = dwc3_gadget_run_stop(dwc, is_on, false);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static void dwc3_gadget_enable_irq(struct dwc3 *dwc)
{
	u32			reg;

	/* Enable all but Start and End of Frame IRQs */
	reg = (DWC3_DEVTEN_VNDRDEVTSTRCVEDEN |
			DWC3_DEVTEN_EVNTOVERFLOWEN |
			DWC3_DEVTEN_CMDCMPLTEN |
			DWC3_DEVTEN_ERRTICERREN |
			DWC3_DEVTEN_WKUPEVTEN |
			DWC3_DEVTEN_ULSTCNGEN |
			DWC3_DEVTEN_CONNECTDONEEN |
			DWC3_DEVTEN_USBRSTEN |
			DWC3_DEVTEN_DISCONNEVTEN);

	dwc3_writel(dwc->regs, DWC3_DEVTEN, reg);
}

static void dwc3_gadget_disable_irq(struct dwc3 *dwc)
{
	/* mask all interrupts */
	dwc3_writel(dwc->regs, DWC3_DEVTEN, 0x00);
}

static int dwc3_gadget_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	struct dwc3_ep		*dep;
	unsigned long		flags;
	int			ret = 0;
	u32			reg;

	spin_lock_irqsave(&dwc->lock, flags);

	dwc->gadget_driver	= driver;

	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_SPEED_MASK);

	/**
	 * WORKAROUND: DWC3 revision < 2.20a have an issue
	 * which would cause metastability state on Run/Stop
	 * bit if we try to force the IP to USB2-only mode.
	 *
	 * Because of that, we cannot configure the IP to any
	 * speed other than the SuperSpeed
	 *
	 * Refers to:
	 *
	 * STAR#9000525659: Clock Domain Crossing on DCTL in
	 * USB 2.0 Mode
	 */
	if (dwc->revision < DWC3_REVISION_220A) {
		reg |= DWC3_DCFG_SUPERSPEED;
	} else {
		switch (dwc->maximum_speed) {
		case USB_SPEED_LOW:
			reg |= DWC3_DSTS_LOWSPEED;
			break;
		case USB_SPEED_FULL:
			reg |= DWC3_DSTS_FULLSPEED1;
			break;
		case USB_SPEED_HIGH:
			reg |= DWC3_DSTS_HIGHSPEED;
			break;
		case USB_SPEED_SUPER:	/* FALLTHROUGH */
		case USB_SPEED_UNKNOWN:	/* FALTHROUGH */
		default:
			reg |= DWC3_DSTS_SUPERSPEED;
		}
	}
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);

	dwc->start_config_issued = false;
	/* Start with SuperSpeed Default */
	dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);

	dep = dwc->eps[0];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false,
			false);
	if (ret) {
		DWC_ERR("failed to enable %s\n", dep->name);
		goto err2;
	}

	dep = dwc->eps[1];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, false,
			false);
	if (ret) {
		DWC_ERR("failed to enable %s\n", dep->name);
		goto err3;
	}

	/* begin to receive SETUP packets */
	dwc->ep0state = EP0_SETUP_PHASE;
	dwc3_ep0_out_start(dwc);

	dwc3_gadget_enable_irq(dwc);

	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;

err3:
	__dwc3_gadget_ep_disable(dwc->eps[0]);

err2:
	dwc->gadget_driver = NULL;

	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static int dwc3_gadget_stop(struct usb_gadget *g)
{
	struct dwc3		*dwc = gadget_to_dwc(g);
	unsigned long		flags;

	spin_lock_irqsave(&dwc->lock, flags);

	dwc3_gadget_disable_irq(dwc);
	__dwc3_gadget_ep_disable(dwc->eps[0]);
	__dwc3_gadget_ep_disable(dwc->eps[1]);

	dwc->gadget_driver	= NULL;

	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;
}

static const struct usb_gadget_ops dwc3_gadget_ops = {
	.get_frame		= dwc3_gadget_get_frame,
	.wakeup			= dwc3_gadget_wakeup,
	.set_selfpowered	= dwc3_gadget_set_selfpowered,
	.pullup			= dwc3_gadget_pullup,
	.udc_start		= dwc3_gadget_start,
	.udc_stop		= dwc3_gadget_stop,
};

static const struct usb_ep_ops dwc3_gadget_ep0_ops = {
	.enable		= dwc3_gadget_ep0_enable,
	.disable	= dwc3_gadget_ep0_disable,
	.alloc_request	= dwc3_gadget_ep_alloc_request,
	.free_request	= dwc3_gadget_ep_free_request,
	.queue		= dwc3_gadget_ep0_queue,
	.dequeue	= dwc3_gadget_ep_dequeue,
	.set_halt	= dwc3_gadget_ep0_set_halt,
	.set_wedge	= dwc3_gadget_ep_set_wedge,
};

static const struct usb_ep_ops dwc3_gadget_ep_ops = {
	.enable		= dwc3_gadget_ep_enable,
	.disable	= dwc3_gadget_ep_disable,
	.alloc_request	= dwc3_gadget_ep_alloc_request,
	.free_request	= dwc3_gadget_ep_free_request,
	.queue		= dwc3_gadget_ep_queue,
	.dequeue	= dwc3_gadget_ep_dequeue,
	.set_halt	= dwc3_gadget_ep_set_halt,
	.set_wedge	= dwc3_gadget_ep_set_wedge,
};

static int dwc3_gadget_init_hw_endpoints(struct dwc3 *dwc,
		u8 num, u32 direction)
{
	struct dwc3_ep			*dep;
	u8				i;

	for (i = 0; i < num; i++) {
		u8 epnum = (i << 1) | (!!direction);

		dep = kzalloc(sizeof(*dep), GFP_KERNEL);
		if (!dep)
			return -ENOMEM;

		dep->dwc = dwc;
		dep->number = epnum;
		dep->direction = !!direction;
		dwc->eps[epnum] = dep;

		snprintf(dep->name, sizeof(dep->name), "ep%d%s", epnum >> 1,
				(epnum & 1) ? "in" : "out");

		dep->endpoint.name = dep->name;

		dev_dbg(dwc->dev, "initializing %s\n", dep->name);

		if (epnum == 0 || epnum == 1) {
			usb_ep_set_maxpacket_limit(&dep->endpoint, 512);
			dep->endpoint.maxburst = 1;
			dep->endpoint.ops = &dwc3_gadget_ep0_ops;
			if (!epnum) {
				dwc->gadget.ep0 = &dep->endpoint;
			}
		} else {
			int		ret;

			usb_ep_set_maxpacket_limit(&dep->endpoint, 512);
			dep->endpoint.max_streams = 15;
			dep->endpoint.ops = &dwc3_gadget_ep_ops;
			list_add_tail(&dep->endpoint.ep_list,
					&dwc->gadget.ep_list);

			ret = dwc3_alloc_trb_pool(dep);
			if (ret)
				return ret;
		}

		INIT_LIST_HEAD(&dep->request_list);
		INIT_LIST_HEAD(&dep->req_queued);
	}

	return 0;
}

static int dwc3_gadget_init_endpoints(struct dwc3 *dwc)
{
	int				ret;
	struct usb_ep	*ep;
	struct dwc3_ep			*dep;
	INIT_LIST_HEAD(&dwc->gadget.ep_list);

	ret = dwc3_gadget_init_hw_endpoints(dwc, dwc->num_out_eps, 0);
	if (ret < 0) {
		DWC_ERR("failed to allocate OUT endpoints\n");
		return ret;
	}

	ret = dwc3_gadget_init_hw_endpoints(dwc, dwc->num_in_eps, 1);
	if (ret < 0) {
		DWC_ERR("failed to allocate IN endpoints\n");
		return ret;
	}
	/* bind bulk in and out ep */
	list_for_each_entry(ep, &dwc->gadget.ep_list, ep_list) {
		dep = to_dwc3_ep(ep);
			if (dep->direction & 1) {
				if (!private_data.in_ep)
					private_data.in_ep = ep;
			} else {
				if (!private_data.out_ep)
					private_data.out_ep = ep;
			}
	}
	if (!private_data.in_ep) {
		DWC_DBG("failed to bind IN endpoints\n");
	}

	if (!private_data.out_ep) {
		DWC_DBG("failed to bind OUT endpoints\n");
	}
	return 0;
}

static void dwc3_gadget_free_endpoints(struct dwc3 *dwc)
{
	struct dwc3_ep			*dep;
	u8				epnum;

	for (epnum = 0; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		dep = dwc->eps[epnum];
		if (!dep)
			continue;
		/*
		 * Physical endpoints 0 and 1 are special; they form the
		 * bi-directional USB endpoint 0.
		 *
		 * For those two physical endpoints, we don't allocate a TRB
		 * pool nor do we add them the endpoints list. Due to that, we
		 * shouldn't do these two operations otherwise we would end up
		 * with all sorts of bugs when removing dwc3.ko.
		 */
		if (epnum != 0 && epnum != 1) {
			dwc3_free_trb_pool(dep);
			list_del(&dep->endpoint.ep_list);
		}

		kfree(dep);
	}
}

int dwc3_gadget_init(struct dwc3 *dwc)
{
	int					ret;
	struct usb_request	*req = NULL;
	dwc->ctrl_req = dma_alloc_coherent(sizeof(*dwc->ctrl_req),
					(unsigned long *)&dwc->ctrl_req_addr);
	if (!dwc->ctrl_req) {
		DWC_ERR("failed to allocate ctrl request\n");
		ret = -ENOMEM;
		goto err0;
	}
	dwc->ep0_trb = dma_alloc_coherent(sizeof(*dwc->ep0_trb) * 2,
					  (unsigned long *)&dwc->ep0_trb_addr);
	if (!dwc->ep0_trb) {
		DWC_ERR("failed to allocate ep0 trb\n");
		ret = -ENOMEM;
		goto err1;
	}
	dwc->setup_buf = memalign(ARCH_DMA_MINALIGN,
				  DWC3_EP0_BOUNCE_SIZE);
	if (!dwc->setup_buf) {
		ret = -ENOMEM;
		goto err2;
	}
	dwc->ep0_bounce = dma_alloc_coherent(DWC3_EP0_BOUNCE_SIZE,
					(unsigned long *)&dwc->ep0_bounce_addr);
	if (!dwc->ep0_bounce) {
		DWC_ERR("failed to allocate ep0 bounce buffer\n");
		ret = -ENOMEM;
		goto err3;
	}
	dwc->gadget.ops			= &dwc3_gadget_ops;
	dwc->gadget.max_speed		= USB_SPEED_HIGH;
	dwc->gadget.speed		= USB_SPEED_UNKNOWN;
	dwc->gadget.name		= "dwc3-gadget";
	dwc->gadget.dev.device_data = NULL;

	/*
	 * Per databook, DWC3 needs buffer size to be aligned to MaxPacketSize
	 * on ep out.
	 */
	dwc->gadget.quirk_ep_out_aligned_size = true;

	/*
	 * REVISIT: Here we should clear all pending IRQs to be
	 * sure we're starting from a well known location.
	 */

	ret = dwc3_gadget_init_endpoints(dwc);
	if (ret)
		goto err4;
	req = usb_ep_alloc_request(dwc->gadget.ep0, GFP_KERNEL);
	if (!req)
		goto err4;
	req->buf = memalign(ARCH_DMA_MINALIGN, USB_BUFSIZ);
	if (!req->buf)
		goto err5;
	private_data.ep0_req = req;
	set_gadget_data(&dwc->gadget, (void *)&private_data);
	return 0;
err5:
	usb_ep_free_request(dwc->gadget.ep0, req);
err4:
	dwc3_gadget_free_endpoints(dwc);
	dma_free_coherent(dwc->ep0_bounce);

err3:
	kfree(dwc->setup_buf);

err2:
	dma_free_coherent(dwc->ep0_trb);

err1:
	dma_free_coherent(dwc->ctrl_req);

err0:
	return ret;
}

static struct dwc3_event_buffer *dwc3_alloc_one_event_buffer(struct dwc3 *dwc,
		unsigned length)
{
	struct dwc3_event_buffer	*evt;

	evt = devm_kzalloc(dwc->dev, sizeof(*evt), GFP_KERNEL);
	if (!evt)
		return ERR_PTR(-ENOMEM);

	evt->dwc	= dwc;
	evt->length	= length;
	evt->buf	= dma_alloc_coherent(length,
					     (unsigned long *)&evt->dma);
	if (!evt->buf)
		return ERR_PTR(-ENOMEM);

	return evt;
}

static int dwc3_alloc_event_buffers(struct dwc3 *dwc, unsigned length)
{
	int			num;
	int			i;

	num = DWC3_NUM_INT(dwc->hwparams.hwparams1);
	dwc->num_event_buffers = num;
	dwc->ev_buffs = memalign(ARCH_DMA_MINALIGN,
				 sizeof(*dwc->ev_buffs) * num);
	if (!dwc->ev_buffs)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		struct dwc3_event_buffer	*evt;
		evt = dwc3_alloc_one_event_buffer(dwc, length);
		if (IS_ERR(evt)) {
			DWC_ERR("can't allocate event buffer\n");
			return PTR_ERR(evt);
		}
		dwc->ev_buffs[i] = evt;
	}

	return 0;
}

static void dwc3_cache_hwparams(struct dwc3 *dwc)
{
	struct dwc3_hwparams	*parms = &dwc->hwparams;
	parms->hwparams0 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS0);
	parms->hwparams1 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS1);
	parms->hwparams2 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS2);
	parms->hwparams3 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS3);
	parms->hwparams4 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS4);
	parms->hwparams5 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS5);
	parms->hwparams6 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS6);
	parms->hwparams7 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS7);
	parms->hwparams8 = dwc3_readl(dwc->regs, DWC3_GHWPARAMS8);
}

static int dwc3_core_soft_reset(struct dwc3 *dwc)
{
	u32		reg;

	/* Before Resetting PHY, put Core in Reset */
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg |= DWC3_GCTL_CORESOFTRESET;
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

	/* Assert USB3 PHY reset */
	reg = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
	reg |= DWC3_GUSB3PIPECTL_PHYSOFTRST;
	dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);

	/* Assert USB2 PHY reset */
	reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));
	reg |= DWC3_GUSB2PHYCFG_PHYSOFTRST;
	dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);

	mdelay(100);

	/* Clear USB3 PHY reset */
	reg = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));
	reg &= ~DWC3_GUSB3PIPECTL_PHYSOFTRST;
	dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);

	/* Clear USB2 PHY reset */
	reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));
	reg &= ~DWC3_GUSB2PHYCFG_PHYSOFTRST;
	dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);

	mdelay(100);

	/* After PHYs are stable we can take Core out of reset state */
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~DWC3_GCTL_CORESOFTRESET;
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

	return 0;
}

static void dwc3_core_num_eps(struct dwc3 *dwc)
{
	struct dwc3_hwparams	*parms = &dwc->hwparams;

	dwc->num_in_eps = DWC3_NUM_IN_EPS(parms);
	dwc->num_out_eps = DWC3_NUM_EPS(parms) - dwc->num_in_eps;

	DWC_DBG("found %d IN and %d OUT endpoints\n",
			dwc->num_in_eps, dwc->num_out_eps);
}

static void dwc3_phy_setup(struct dwc3 *dwc)
{
	u32 reg;
	u32 trdtim;
	reg = dwc3_readl(dwc->regs, DWC3_GUSB3PIPECTL(0));

	/*
	 * Above 1.94a, it is recommended to set DWC3_GUSB3PIPECTL_SUSPHY
	 * to '0' during coreConsultant configuration. So default value
	 * will be '0' when the core is reset. Application needs to set it
	 * to '1' after the core initialization is completed.
	 */
	/* if (dwc->revision > DWC3_REVISION_194A) */
		reg |= DWC3_GUSB3PIPECTL_SUSPHY;
/*
	if (dwc->u2ss_inp3_quirk)
		reg |= DWC3_GUSB3PIPECTL_U2SSINP3OK;

	if (dwc->req_p1p2p3_quirk)
		reg |= DWC3_GUSB3PIPECTL_REQP1P2P3;

	if (dwc->del_p1p2p3_quirk)
		reg |= DWC3_GUSB3PIPECTL_DEP1P2P3_EN;

	if (dwc->del_phy_power_chg_quirk)
		reg |= DWC3_GUSB3PIPECTL_DEPOCHANGE;

	if (dwc->lfps_filter_quirk)
		reg |= DWC3_GUSB3PIPECTL_LFPSFILT;

	if (dwc->rx_detect_poll_quirk)
		reg |= DWC3_GUSB3PIPECTL_RX_DETOPOLL;

	if (dwc->tx_de_emphasis_quirk)
		reg |= DWC3_GUSB3PIPECTL_TX_DEEPH(dwc->tx_de_emphasis);

	if (dwc->dis_u3_susphy_quirk)
		reg &= ~DWC3_GUSB3PIPECTL_SUSPHY;
*/
	dwc3_writel(dwc->regs, DWC3_GUSB3PIPECTL(0), reg);

	mdelay(100);

	reg = dwc3_readl(dwc->regs, DWC3_GUSB2PHYCFG(0));

	/*
	 * Above 1.94a, it is recommended to set DWC3_GUSB2PHYCFG_SUSPHY to
	 * '0' during coreConsultant configuration. So default value will
	 * be '0' when the core is reset. Application needs to set it to
	 * '1' after the core initialization is completed.
	 */
	/* if (dwc->revision > DWC3_REVISION_194A)
		reg |= DWC3_GUSB2PHYCFG_SUSPHY;

	if (dwc->dis_u2_susphy_quirk) */
		reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;
#if defined(CONFIG_RKCHIP_RK3399)
	reg |= DWC3_GUSB2PHYCFG_PHYIF;
#else
	reg &= ~DWC3_GUSB2PHYCFG_PHYIF;
#endif
	reg &= ~(U2_FREECLK_EXISTS | DWC3_GUSB2PHYCFG_ENBLSLPM);

	trdtim = (reg & GUSBCFG_PHYIF16) ? 5 : 9;
	reg &= ~GUSBCFG_USBTRDTIM_MASK;
    reg |= (trdtim << GUSBCFG_USBTRDTIM_SHIFT);

	dwc3_writel(dwc->regs, DWC3_GUSB2PHYCFG(0), reg);

	mdelay(100);
}

int dwc3_send_gadget_generic_command(struct dwc3 *dwc, unsigned cmd, u32 param)
{
	u32		timeout = 500;
	u32		reg;

	dwc3_writel(dwc->regs, DWC3_DGCMDPAR, param);
	dwc3_writel(dwc->regs, DWC3_DGCMD, cmd | DWC3_DGCMD_CMDACT);

	do {
		reg = dwc3_readl(dwc->regs, DWC3_DGCMD);
		if (!(reg & DWC3_DGCMD_CMDACT)) {
			DWC_DBG("Command Complete --> %d\n",
					DWC3_DGCMD_STATUS(reg));
			return 0;
		}

		/*
		 * We can't sleep here, because it's also called from
		 * interrupt context.
		 */
		timeout--;
		if (!timeout)
			return -ETIMEDOUT;
		udelay(1);
	} while (1);
}

static int dwc3_alloc_scratch_buffers(struct dwc3 *dwc)
{
	if (!dwc->has_hibernation)
		return 0;

	if (!dwc->nr_scratch)
		return 0;

	dwc->scratchbuf = kmalloc_array(dwc->nr_scratch,
			DWC3_SCRATCHBUF_SIZE, GFP_KERNEL);
	if (!dwc->scratchbuf)
		return -ENOMEM;

	return 0;
}

static int dwc3_setup_scratch_buffers(struct dwc3 *dwc)
{
	dma_addr_t scratch_addr;
	u32 param;
	int ret;

	if (!dwc->has_hibernation)
		return 0;

	if (!dwc->nr_scratch)
		return 0;

	scratch_addr = dma_map_single(dwc->scratchbuf,
				      dwc->nr_scratch * DWC3_SCRATCHBUF_SIZE,
				      DMA_BIDIRECTIONAL);
	if (dma_mapping_error(dwc->dev, scratch_addr)) {
		DWC_ERR("failed to map scratch buffer\n");
		ret = -EFAULT;
		goto err0;
	}

	dwc->scratch_addr = scratch_addr;

	param = lower_32_bits(scratch_addr);

	ret = dwc3_send_gadget_generic_command(dwc,
			DWC3_DGCMD_SET_SCRATCHPAD_ADDR_LO, param);
	if (ret < 0)
		goto err1;

	param = upper_32_bits(scratch_addr);

	ret = dwc3_send_gadget_generic_command(dwc,
			DWC3_DGCMD_SET_SCRATCHPAD_ADDR_HI, param);
	if (ret < 0)
		goto err1;

	return 0;

err1:
	dma_unmap_single((void *)(uintptr_t)dwc->scratch_addr, dwc->nr_scratch *
			 DWC3_SCRATCHBUF_SIZE, DMA_BIDIRECTIONAL);

err0:
	return ret;
}

static void dwc3_free_scratch_buffers(struct dwc3 *dwc)
{
	if (!dwc->has_hibernation)
		return;

	if (!dwc->nr_scratch)
		return;

	dma_unmap_single((void *)(uintptr_t)dwc->scratch_addr, dwc->nr_scratch *
			 DWC3_SCRATCHBUF_SIZE, DMA_BIDIRECTIONAL);
	kfree(dwc->scratchbuf);
}

static int rk_dwc3_core_init(struct dwc3 *dwc)
{
	unsigned long		timeout;
	u32			reg;
	int			ret;

	reg = dwc3_readl(dwc->regs, DWC3_GSNPSID);
	dwc->revision = reg;

	/* Handle USB2.0-only core configuration */
	if (DWC3_GHWPARAMS3_SSPHY_IFC(dwc->hwparams.hwparams3) ==
			DWC3_GHWPARAMS3_SSPHY_IFC_DIS) {
		if (dwc->maximum_speed == USB_SPEED_SUPER)
			dwc->maximum_speed = USB_SPEED_HIGH;
	}

	/* issue device SoftReset too */
	timeout = 5000;
	dwc3_writel(dwc->regs, DWC3_DCTL, DWC3_DCTL_CSFTRST);
	while (timeout--) {
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		if (!(reg & DWC3_DCTL_CSFTRST))
			break;
	};

	if (!timeout) {
		DWC_ERR("Reset Timed Out\n");
		ret = -ETIMEDOUT;
		goto err0;
	}

	ret = dwc3_core_soft_reset(dwc);
	if (ret)
		goto err0;

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~DWC3_GCTL_SCALEDOWN_MASK;
/*
	switch (DWC3_GHWPARAMS1_EN_PWROPT(dwc->hwparams.hwparams1)) {
	case DWC3_GHWPARAMS1_EN_PWROPT_CLK:

		if ((dwc->dr_mode == USB_DR_MODE_HOST ||
				dwc->dr_mode == USB_DR_MODE_OTG) &&
				(dwc->revision >= DWC3_REVISION_210A &&
				dwc->revision <= DWC3_REVISION_250A))
			reg |= DWC3_GCTL_DSBLCLKGTNG | DWC3_GCTL_SOFITPSYNC;
		else
			reg &= ~DWC3_GCTL_DSBLCLKGTNG;
		break;
	case DWC3_GHWPARAMS1_EN_PWROPT_HIB:

		dwc->nr_scratch = DWC3_GHWPARAMS4_HIBER_SCRATCHBUFS(hwparams4);
		reg |= DWC3_GCTL_GBLHIBERNATIONEN;
		break;
	default:
		dev_dbg(dwc->dev, "No power optimization available\n");
	}

	if (dwc->hwparams.hwparams6 & DWC3_GHWPARAMS6_EN_FPGA) {
		dev_dbg(dwc->dev, "it is on FPGA board\n");
		dwc->is_fpga = true;
	}

	if(dwc->disable_scramble_quirk && !dwc->is_fpga)
		WARN(true,
		     "disable_scramble cannot be used on non-FPGA builds\n");
*/
	if (dwc->disable_scramble_quirk && dwc->is_fpga)
		reg |= DWC3_GCTL_DISSCRAMBLE;
	else
		reg &= ~DWC3_GCTL_DISSCRAMBLE;

	if (dwc->u2exit_lfps_quirk)
		reg |= DWC3_GCTL_U2EXIT_LFPS;
	dwc->is_selfpowered = 1;

	/*
	 * WORKAROUND: DWC3 revisions <1.90a have a bug
	 * where the device can fail to connect at SuperSpeed
	 * and falls back to high-speed mode which causes
	 * the device to enter a Connect/Disconnect loop
	 */
	if (dwc->revision < DWC3_REVISION_190A)
		reg |= DWC3_GCTL_U2RSTECN;

	dwc3_core_num_eps(dwc);

	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

	if (dwc->maximum_speed == USB_SPEED_HIGH) {
	    /*
	     * This bit is applicable (and to be set) for device mode
	     * (DCFG.Speed != SS) only.In the 3.0 device core, if the
	     * core is programmed to operate in 2.0 only (i.e., Device
	     * Speed is programmed to 2.0 speeds in DCFG[Speed]), then
	     * setting this bit makes the internal 2.0 (utmi/ulpi) clock
	     * to be routed as the 3.0 (pipe) clock.
	     */
	    DWC_DBG("force dwc3 to high speed.\n");
		reg = dwc3_readl(dwc->regs, DWC3_GUCTL1);
	    reg |= DEV_FORCE_20_CLK_FOR_30_CLK;
	    dwc3_writel(dwc->regs, DWC3_GUCTL1, reg);
	}

	dwc3_phy_setup(dwc);

	reg = dwc3_readl(dwc->regs, DWC3_GRXFIFOSIZ(0));
    reg = (reg & 0xffff0000) | 0x200;
    dwc3_writel(dwc->regs, DWC3_GRXFIFOSIZ(0), reg);

	ret = dwc3_alloc_scratch_buffers(dwc);
	if (ret)
		goto err0;

	ret = dwc3_setup_scratch_buffers(dwc);
	if (ret)
		goto err1;

	return 0;

err1:
	dwc3_free_scratch_buffers(dwc);

err0:
	return ret;
}

static void dwc3_core_exit(struct dwc3 *dwc)
{
	dwc3_free_scratch_buffers(dwc);
}

static void dwc3_set_mode(struct dwc3 *dwc, u32 mode)
{
	u32 reg;

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~(DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG));
	reg |= DWC3_GCTL_PRTCAPDIR(mode);
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);
}

static int dwc3_core_init_mode(struct dwc3 *dwc)
{
	int ret;

	if (dwc->dr_mode == USB_DR_MODE_PERIPHERAL) {
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_DEVICE);
		ret = dwc3_gadget_init(dwc);
		if (ret) {
			DWC_ERR("failed to initialize gadget\n");
			return ret;
		}
	} else {
		DWC_ERR("Unsupported mode of operation %d\n", dwc->dr_mode);
		return -EINVAL;
	}
	return 0;
}

static int dwc3_event_buffers_setup(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	int				n;
	for (n = 0; n < dwc->num_event_buffers; n++) {
		evt = dwc->ev_buffs[n];
		DWC_DBG("Event buf %p dma %08llx length %d\n",
				evt->buf, (unsigned long long) evt->dma,
				evt->length);

		evt->lpos = 0;

		dwc3_writel(dwc->regs, DWC3_GEVNTADRLO(n),
				lower_32_bits(evt->dma));
		dwc3_writel(dwc->regs, DWC3_GEVNTADRHI(n),
				upper_32_bits(evt->dma));
		dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(n),
				DWC3_GEVNTSIZ_SIZE(evt->length));
		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(n), 0);
	}

	return 0;
}

static void dwc3_event_buffers_cleanup(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	int				n;

	for (n = 0; n < dwc->num_event_buffers; n++) {
		evt = dwc->ev_buffs[n];

		evt->lpos = 0;

		dwc3_writel(dwc->regs, DWC3_GEVNTADRLO(n), 0);
		dwc3_writel(dwc->regs, DWC3_GEVNTADRHI(n), 0);
		dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(n), DWC3_GEVNTSIZ_INTMASK
				| DWC3_GEVNTSIZ_SIZE(0));
		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(n), 0);
	}
}

static void dwc3_free_one_event_buffer(struct dwc3 *dwc,
		struct dwc3_event_buffer *evt)
{
	dma_free_coherent(evt->buf);
}

static void dwc3_free_event_buffers(struct dwc3 *dwc)
{
	struct dwc3_event_buffer	*evt;
	int i;

	for (i = 0; i < dwc->num_event_buffers; i++) {
		evt = dwc->ev_buffs[i];
		if (evt)
			dwc3_free_one_event_buffer(dwc, evt);
	}
}

void dwc3_gadget_exit(struct dwc3 *dwc)
{
	struct rk_udc_private_data *privData = NULL;
	privData = (struct rk_udc_private_data *)get_gadget_data(&dwc->gadget);
	if (privData->ep0_req) {
		kfree(privData->ep0_req->buf);
		usb_ep_free_request(dwc->gadget.ep0, privData->ep0_req);
	}
	dwc3_gadget_free_endpoints(dwc);
	dma_free_coherent(dwc->ep0_bounce);

	kfree(dwc->setup_buf);

	dma_free_coherent(dwc->ep0_trb);

	dma_free_coherent(dwc->ctrl_req);
}

void dwc3_uboot_exit(int index)
{
	struct dwc3 *dwc;

	if (global_dwc) {
		dwc = global_dwc;
		dwc3_gadget_exit(dwc);
		dwc3_event_buffers_cleanup(dwc);
		dwc3_free_event_buffers(dwc);
		dwc3_core_exit(dwc);
		kfree(dwc->mem);
	}
}

int dwc3_uboot_init(struct dwc3_device *dwc3_dev)
{
	struct dwc3 *dwc;
	struct device		*dev = NULL;
	u8			lpm_nyet_threshold;
	u8			tx_de_emphasis;
	u8			hird_threshold;
	int			ret;
	void			*mem;

	memset(dwc3_dev, 0, sizeof(struct dwc3_device));
	dwc3_dev->base = RKIO_USB3_BASE;
	dwc3_dev->dr_mode = USB_DR_MODE_PERIPHERAL;
	dwc3_dev->maximum_speed = USB_SPEED_HIGH;

	mem = devm_kzalloc(dev, sizeof(*dwc) + DWC3_ALIGN_MASK, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;
	global_dwc = dwc = (void *)ALIGN((unsigned long)mem, DWC3_ALIGN_MASK + 1);
	dwc->mem = mem;
	dwc->regs = (void *)(RKIO_USB3_BASE + DWC3_GLOBALS_REGS_START);
	/* default to highest possible threshold */
	lpm_nyet_threshold = 0xff;

	/* default to -3.5dB de-emphasis */
	tx_de_emphasis = 1;

	/*
	 * default to assert utmi_sleep_n and use maximum allowed HIRD
	 * threshold value of 0b1100
	 */
	hird_threshold = 12;

	dwc->maximum_speed = dwc3_dev->maximum_speed;
	dwc->has_lpm_erratum = dwc3_dev->has_lpm_erratum;
	if (dwc3_dev->lpm_nyet_threshold)
		lpm_nyet_threshold = dwc3_dev->lpm_nyet_threshold;
	dwc->is_utmi_l1_suspend = dwc3_dev->is_utmi_l1_suspend;
	if (dwc3_dev->hird_threshold)
		hird_threshold = dwc3_dev->hird_threshold;

	dwc->needs_fifo_resize = dwc3_dev->tx_fifo_resize;
	dwc->dr_mode = dwc3_dev->dr_mode;

	dwc->disable_scramble_quirk = dwc3_dev->disable_scramble_quirk;
	dwc->u2exit_lfps_quirk = dwc3_dev->u2exit_lfps_quirk;
	dwc->u2ss_inp3_quirk = dwc3_dev->u2ss_inp3_quirk;
	dwc->req_p1p2p3_quirk = dwc3_dev->req_p1p2p3_quirk;
	dwc->del_p1p2p3_quirk = dwc3_dev->del_p1p2p3_quirk;
	dwc->del_phy_power_chg_quirk = dwc3_dev->del_phy_power_chg_quirk;
	dwc->lfps_filter_quirk = dwc3_dev->lfps_filter_quirk;
	dwc->rx_detect_poll_quirk = dwc3_dev->rx_detect_poll_quirk;
	dwc->dis_u3_susphy_quirk = dwc3_dev->dis_u3_susphy_quirk;
	dwc->dis_u2_susphy_quirk = dwc3_dev->dis_u2_susphy_quirk;

	dwc->tx_de_emphasis_quirk = dwc3_dev->tx_de_emphasis_quirk;
	if (dwc3_dev->tx_de_emphasis)
		tx_de_emphasis = dwc3_dev->tx_de_emphasis;

	/* default to superspeed if no maximum_speed passed */
	if (dwc->maximum_speed == USB_SPEED_UNKNOWN)
		dwc->maximum_speed = USB_SPEED_SUPER;

	dwc->lpm_nyet_threshold = lpm_nyet_threshold;
	dwc->tx_de_emphasis = tx_de_emphasis;

	dwc->hird_threshold = hird_threshold
		| (dwc->is_utmi_l1_suspend << 4);

	dwc->index = dwc3_dev->index;

	dwc3_cache_hwparams(dwc);
	ret = dwc3_alloc_event_buffers(dwc, DWC3_EVENT_BUFFERS_SIZE);
	if (ret) {
		DWC_ERR("failed to allocate event buffers\n");
		return -ENOMEM;
	}
	dwc->dr_mode = USB_DR_MODE_PERIPHERAL;

	ret = rk_dwc3_core_init(dwc);
	if (ret) {
		DWC_ERR("failed to initialize core\n");
		goto err0;
	}
	ret = dwc3_event_buffers_setup(dwc);
	if (ret) {
		DWC_ERR("failed to setup event buffers\n");
		goto err1;
	}
	ret = dwc3_core_init_mode(dwc);
	if (ret) {
		DWC_ERR("failed to initialize gadget mode\n");
		goto err2;
	}

	return 0;

err2:
	dwc3_event_buffers_cleanup(dwc);

err1:
	dwc3_core_exit(dwc);

err0:
	dwc3_free_event_buffers(dwc);

	return ret;
}

static void dwc3_disconnect_gadget(struct dwc3 *dwc)
{
	if (global_rk_instance.connected) {
		spin_unlock(&dwc->lock);
		rockusb_disable(&dwc->gadget);
		spin_lock(&dwc->lock);
	}
}

static void dwc3_gadget_disconnect_interrupt(struct dwc3 *dwc)
{
	int			reg;

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_INITU1ENA;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);

	reg &= ~DWC3_DCTL_INITU2ENA;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);
#if defined(CONFIG_RKCHIP_RK3399)
	ClearVbus();
#endif
	dwc3_disconnect_gadget(dwc);
	dwc->start_config_issued = false;

	dwc->gadget.speed = USB_SPEED_UNKNOWN;
	dwc->setup_packet_pending = false;

	usb_gadget_set_state(&dwc->gadget, USB_STATE_NOTATTACHED);

}
static void dwc3_reset_gadget(struct dwc3 *dwc)
{

	if (dwc->gadget.speed != USB_SPEED_UNKNOWN) {
		spin_unlock(&dwc->lock);
		if (global_rk_instance.connected)
			rockusb_disable(&dwc->gadget);

		usb_gadget_set_state(&dwc->gadget, USB_STATE_DEFAULT);
		spin_lock(&dwc->lock);
	}
}

static void dwc3_stop_active_transfers(struct dwc3 *dwc)
{
	u32 epnum;

	for (epnum = 2; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		struct dwc3_ep *dep;

		dep = dwc->eps[epnum];
		if (!dep)
			continue;

		if (!(dep->flags & DWC3_EP_ENABLED))
			continue;

		dwc3_remove_requests(dwc, dep);
	}
}

static void dwc3_clear_stall_all_ep(struct dwc3 *dwc)
{
	u32 epnum;

	for (epnum = 1; epnum < DWC3_ENDPOINTS_NUM; epnum++) {
		struct dwc3_ep *dep;
		struct dwc3_gadget_ep_cmd_params params;

		dep = dwc->eps[epnum];
		if (!dep)
			continue;

		if (!(dep->flags & DWC3_EP_STALL))
			continue;

		dep->flags &= ~DWC3_EP_STALL;

		memset(&params, 0, sizeof(params));
		dwc3_send_gadget_ep_cmd(dwc, dep->number,
				DWC3_DEPCMD_CLEARSTALL, &params);
	}
}

static void dwc3_gadget_reset_interrupt(struct dwc3 *dwc)
{
	u32			reg;

	if (dwc->revision < DWC3_REVISION_188A) {
		if (dwc->setup_packet_pending)
			dwc3_gadget_disconnect_interrupt(dwc);
	}

	dwc3_reset_gadget(dwc);

	reg = dwc3_readl(dwc->regs, DWC3_DCTL);
	reg &= ~DWC3_DCTL_TSTCTRL_MASK;
	dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	dwc->test_mode = false;

	dwc3_stop_active_transfers(dwc);
	dwc3_clear_stall_all_ep(dwc);
	dwc->start_config_issued = false;

	/* Reset device address to zero */
	reg = dwc3_readl(dwc->regs, DWC3_DCFG);
	reg &= ~(DWC3_DCFG_DEVADDR_MASK);
	dwc3_writel(dwc->regs, DWC3_DCFG, reg);
}

static void dwc3_update_ram_clk_sel(struct dwc3 *dwc, u32 speed)
{
	u32 reg;
	u32 usb30_clock = DWC3_GCTL_CLK_BUS;

	/*
	 * We change the clock only at SS but I dunno why I would want to do
	 * this. Maybe it becomes part of the power saving plan.
	 */

	if (speed != DWC3_DSTS_SUPERSPEED)
		return;

	/*
	 * RAMClkSel is reset to 0 after USB reset, so it must be reprogrammed
	 * each time on Connect Done.
	 */
	if (!usb30_clock)
		return;

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg |= DWC3_GCTL_RAMCLKSEL(usb30_clock);
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);
}

static void dwc3_gadget_conndone_interrupt(struct dwc3 *dwc)
{
	struct dwc3_ep		*dep;
	int			ret;
	u32			reg;
	u8			speed;

	reg = dwc3_readl(dwc->regs, DWC3_DSTS);
	speed = reg & DWC3_DSTS_CONNECTSPD;
	dwc->speed = speed;

	dwc3_update_ram_clk_sel(dwc, speed);

	switch (speed) {
	case DWC3_DCFG_SUPERSPEED:
		if (dwc->revision < DWC3_REVISION_190A)
			dwc3_gadget_reset_interrupt(dwc);

		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);
		dwc->gadget.ep0->maxpacket = 512;
		dwc->gadget.speed = USB_SPEED_SUPER;
		break;
	case DWC3_DCFG_HIGHSPEED:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);
		dwc->gadget.ep0->maxpacket = 64;
		dwc->gadget.speed = USB_SPEED_HIGH;
		break;
	case DWC3_DCFG_FULLSPEED2:
	case DWC3_DCFG_FULLSPEED1:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);
		dwc->gadget.ep0->maxpacket = 64;
		dwc->gadget.speed = USB_SPEED_FULL;
		break;
	case DWC3_DCFG_LOWSPEED:
		dwc3_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(8);
		dwc->gadget.ep0->maxpacket = 8;
		dwc->gadget.speed = USB_SPEED_LOW;
		break;
	}

	/* Enable USB2 LPM Capability */

	if ((dwc->revision > DWC3_REVISION_194A)
			&& (speed != DWC3_DCFG_SUPERSPEED)) {
		reg = dwc3_readl(dwc->regs, DWC3_DCFG);
		reg |= DWC3_DCFG_LPM_CAP;
		dwc3_writel(dwc->regs, DWC3_DCFG, reg);

		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~(DWC3_DCTL_HIRD_THRES_MASK | DWC3_DCTL_L1_HIBER_EN);

		reg |= DWC3_DCTL_HIRD_THRES(dwc->hird_threshold);

		/*
		 * When dwc3 revisions >= 2.40a, LPM Erratum is enabled and
		 * DCFG.LPMCap is set, core responses with an ACK and the
		 * BESL value in the LPM token is less than or equal to LPM
		 * NYET threshold.
		 */
		if (dwc->revision < DWC3_REVISION_240A 	&& dwc->has_lpm_erratum)
			DWC_WARN("LPM Erratum not available on dwc3 revisisions < 2.40a\n");

		if (dwc->has_lpm_erratum && dwc->revision >= DWC3_REVISION_240A)
			reg |= DWC3_DCTL_LPM_ERRATA(dwc->lpm_nyet_threshold);

		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	} else {
		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg &= ~DWC3_DCTL_HIRD_THRES_MASK;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);
	}

	dep = dwc->eps[0];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, true,
			false);
	if (ret) {
		DWC_ERR("failed to enable %s\n", dep->name);
		return;
	}

	dep = dwc->eps[1];
	ret = __dwc3_gadget_ep_enable(dep, &dwc3_gadget_ep0_desc, NULL, true,
			false);
	if (ret) {
		DWC_ERR("failed to enable %s\n", dep->name);
		return;
	}
}

static void dwc3_suspend_gadget(struct dwc3 *dwc)
{
	spin_unlock(&dwc->lock);
	global_rk_instance.suspended = 1;
	spin_lock(&dwc->lock);
}

static void dwc3_resume_gadget(struct dwc3 *dwc)
{
	if (global_rk_instance.suspended) {
		spin_unlock(&dwc->lock);
		global_rk_instance.suspended = 0;
		spin_lock(&dwc->lock);
	}
}

static void dwc3_gadget_hibernation_interrupt(struct dwc3 *dwc,
		unsigned int evtinfo)
{
	unsigned int is_ss = evtinfo & (1UL << 4);

	if (is_ss ^ (dwc->speed == USB_SPEED_SUPER))
		return;

	/* enter hibernation here */
}

static void dwc3_gadget_linksts_change_interrupt(struct dwc3 *dwc,
		unsigned int evtinfo)
{
	enum dwc3_link_state	next = evtinfo & DWC3_LINK_STATE_MASK;
	unsigned int		pwropt;

	pwropt = DWC3_GHWPARAMS1_EN_PWROPT(dwc->hwparams.hwparams1);
	if ((dwc->revision < DWC3_REVISION_250A) &&
			(pwropt != DWC3_GHWPARAMS1_EN_PWROPT_HIB)) {
		if ((dwc->link_state == DWC3_LINK_STATE_U3) &&
				(next == DWC3_LINK_STATE_RESUME)) {
			DWC_DBG("ignoring transition U3 -> Resume\n");
			return;
		}
	}

	if (dwc->revision < DWC3_REVISION_183A) {
		if (next == DWC3_LINK_STATE_U0) {
			u32	u1u2;
			u32	reg;

			switch (dwc->link_state) {
			case DWC3_LINK_STATE_U1:
			case DWC3_LINK_STATE_U2:
				reg = dwc3_readl(dwc->regs, DWC3_DCTL);
				u1u2 = reg & (DWC3_DCTL_INITU2ENA
						| DWC3_DCTL_ACCEPTU2ENA
						| DWC3_DCTL_INITU1ENA
						| DWC3_DCTL_ACCEPTU1ENA);

				if (!dwc->u1u2)
					dwc->u1u2 = reg & u1u2;

				reg &= ~u1u2;

				dwc3_writel(dwc->regs, DWC3_DCTL, reg);
				break;
			default:
				/* do nothing */
				break;
			}
		}
	}

	switch (next) {
	case DWC3_LINK_STATE_U1:
		if (dwc->speed == USB_SPEED_SUPER)
			dwc3_suspend_gadget(dwc);
		break;
	case DWC3_LINK_STATE_U2:
	case DWC3_LINK_STATE_U3:
		dwc3_suspend_gadget(dwc);
		break;
	case DWC3_LINK_STATE_RESUME:
		dwc3_resume_gadget(dwc);
		break;
	default:
		/* do nothing */
		break;
	}

	dwc->link_state = next;
}

static void dwc3_gadget_interrupt(struct dwc3 *dwc,
		const struct dwc3_event_devt *event)
{
	DWC_DBG("<<Dwc3>> gadget interrupt comes,type=%d.\n", event->type);
	switch (event->type) {
	case DWC3_DEVICE_EVENT_DISCONNECT:
		dwc3_gadget_disconnect_interrupt(dwc);
		global_rk_instance.device_event(5);
		break;
	case DWC3_DEVICE_EVENT_RESET:
		dwc3_gadget_reset_interrupt(dwc);
		global_rk_instance.device_event(1);
		break;
	case DWC3_DEVICE_EVENT_CONNECT_DONE:
		dwc3_gadget_conndone_interrupt(dwc);
		break;
	case DWC3_DEVICE_EVENT_WAKEUP:
		dwc3_resume_gadget(dwc);
		break;
	case DWC3_DEVICE_EVENT_HIBER_REQ:
		if (!dwc->has_hibernation) {
			DWC_WARN("unexpected hibernation event\n");
			break;
		}
		dwc3_gadget_hibernation_interrupt(dwc, event->event_info);
		break;
	case DWC3_DEVICE_EVENT_LINK_STATUS_CHANGE:
		dwc3_gadget_linksts_change_interrupt(dwc, event->event_info);
		break;
	case DWC3_DEVICE_EVENT_EOPF:
		DWC_DBG("End of Periodic Frame\n");
		break;
	case DWC3_DEVICE_EVENT_SOF:
		DWC_DBG("Start of Periodic Frame\n");
		break;
	case DWC3_DEVICE_EVENT_ERRATIC_ERROR:
		DWC_DBG("Erratic Error\n");
		break;
	case DWC3_DEVICE_EVENT_CMD_CMPL:
		DWC_DBG("Command Complete\n");
		break;
	case DWC3_DEVICE_EVENT_OVERFLOW:
		DWC_DBG("Overflow\n");
		break;
	default:
		DWC_DBG("UNKNOWN IRQ %d\n", event->type);
	}
}

static int __dwc3_cleanup_done_trbs(struct dwc3 *dwc, struct dwc3_ep *dep,
		struct dwc3_request *req, struct dwc3_trb *trb,
		const struct dwc3_event_depevt *event, int status)
{
	unsigned int		count;
	unsigned int		s_pkt = 0;
	unsigned int		trb_status;

	if ((trb->ctrl & DWC3_TRB_CTRL_HWO) && status != -ESHUTDOWN)
		/*
		 * We continue despite the error. There is not much we
		 * can do. If we don't clean it up we loop forever. If
		 * we skip the TRB then it gets overwritten after a
		 * while since we use them in a ring buffer. A BUG()
		 * would help. Lets hope that if this occurs, someone
		 * fixes the root cause instead of looking away :)
		 */
		DWC_ERR("%s's TRB (%p) still owned by HW\n",
				dep->name, trb);
	count = trb->size & DWC3_TRB_SIZE_MASK;

	if (dep->direction) {
		if (count) {
			trb_status = DWC3_TRB_SIZE_TRBSTS(trb->size);
			if (trb_status == DWC3_TRBSTS_MISSED_ISOC) {
				DWC_DBG("incomplete IN transfer %s\n",
						dep->name);
				/*
				 * If missed isoc occurred and there is
				 * no request queued then issue END
				 * TRANSFER, so that core generates
				 * next xfernotready and we will issue
				 * a fresh START TRANSFER.
				 * If there are still queued request
				 * then wait, do not issue either END
				 * or UPDATE TRANSFER, just attach next
				 * request in request_list during
				 * giveback.If any future queued request
				 * is successfully transferred then we
				 * will issue UPDATE TRANSFER for all
				 * request in the request_list.
				 */
				dep->flags |= DWC3_EP_MISSED_ISOC;
			} else {
				DWC_ERR("incomplete IN transfer %s\n",
						dep->name);
				status = -ECONNRESET;
			}
		} else {
			dep->flags &= ~DWC3_EP_MISSED_ISOC;
		}
	} else {
		if (count && (event->status & DEPEVT_STATUS_SHORT))
			s_pkt = 1;
	}

	/*
	 * We assume here we will always receive the entire data block
	 * which we should receive. Meaning, if we program RX to
	 * receive 4K but we receive only 2K, we assume that's all we
	 * should receive and we simply bounce the request back to the
	 * gadget driver for further processing.
	 */
	req->request.actual += req->request.length - count;
	if (s_pkt)
		return 1;
	if ((event->status & DEPEVT_STATUS_LST) &&
			(trb->ctrl & (DWC3_TRB_CTRL_LST |
				DWC3_TRB_CTRL_HWO)))
		return 1;
	if ((event->status & DEPEVT_STATUS_IOC) &&
			(trb->ctrl & DWC3_TRB_CTRL_IOC))
		return 1;
	return 0;
}

static int dwc3_cleanup_done_reqs(struct dwc3 *dwc, struct dwc3_ep *dep,
		const struct dwc3_event_depevt *event, int status)
{
	struct dwc3_request	*req;
	struct dwc3_trb		*trb;
	unsigned int		slot;

	req = next_request(&dep->req_queued);
	if (!req) {
		return 1;
	}

	slot = req->start_slot;
	if ((slot == DWC3_TRB_NUM - 1) &&
	    usb_endpoint_xfer_isoc(dep->endpoint.desc))
		slot++;
	slot %= DWC3_TRB_NUM;
	trb = &dep->trb_pool[slot];

	dwc3_flush_cache((long)trb, sizeof(*trb));
	__dwc3_cleanup_done_trbs(dwc, dep, req, trb, event, status);
	dwc3_gadget_giveback(dep, req, status);

	if (usb_endpoint_xfer_isoc(dep->endpoint.desc) &&
			list_empty(&dep->req_queued)) {
		if (list_empty(&dep->request_list)) {
			/*
			 * If there is no entry in request list then do
			 * not issue END TRANSFER now. Just set PENDING
			 * flag, so that END TRANSFER is issued when an
			 * entry is added into request list.
			 */
			dep->flags = DWC3_EP_PENDING_REQUEST;
		} else {
			dwc3_stop_active_transfer(dwc, dep->number, true);
			dep->flags = DWC3_EP_ENABLED;
		}
		return 1;
	}

	return 1;
}

static void dwc3_endpoint_transfer_complete(struct dwc3 *dwc,
		struct dwc3_ep *dep, const struct dwc3_event_depevt *event)
{
	unsigned		status = 0;
	int			clean_busy;

	if (event->status & DEPEVT_STATUS_BUSERR)
		status = -ECONNRESET;

	clean_busy = dwc3_cleanup_done_reqs(dwc, dep, event, status);
	if (clean_busy)
		dep->flags &= ~DWC3_EP_BUSY;

	/*
	 * WORKAROUND: This is the 2nd half of U1/U2 -> U0 workaround.
	 * See dwc3_gadget_linksts_change_interrupt() for 1st half.
	 */
	if (dwc->revision < DWC3_REVISION_183A) {
		u32		reg;
		int		i;

		for (i = 0; i < DWC3_ENDPOINTS_NUM; i++) {
			dep = dwc->eps[i];

			if (!(dep->flags & DWC3_EP_ENABLED))
				continue;

			if (!list_empty(&dep->req_queued))
				return;
		}

		reg = dwc3_readl(dwc->regs, DWC3_DCTL);
		reg |= dwc->u1u2;
		dwc3_writel(dwc->regs, DWC3_DCTL, reg);

		dwc->u1u2 = 0;
	}
}
/*
static void __dwc3_gadget_start_isoc(struct dwc3 *dwc,
		struct dwc3_ep *dep, u32 cur_uf)
{
	u32 uf;

	if (list_empty(&dep->request_list)) {
		DWC_DBG("ISOC ep %s run out for requests.\n",
			dep->name);
		dep->flags |= DWC3_EP_PENDING_REQUEST;
		return;
	}

	uf = cur_uf + dep->interval * 4;

	__dwc3_gadget_kick_transfer(dep, uf, 1);
}

static void dwc3_gadget_start_isoc(struct dwc3 *dwc,
		struct dwc3_ep *dep, const struct dwc3_event_depevt *event)
{
	u32 cur_uf, mask;

	mask = ~(dep->interval - 1);
	cur_uf = event->parameters & mask;

	__dwc3_gadget_start_isoc(dwc, dep, cur_uf);
}
*/

static void dwc3_endpoint_interrupt(struct dwc3 *dwc,
		const struct dwc3_event_depevt *event)
{
	struct dwc3_ep		*dep;
	u8			epnum = event->endpoint_number;
	dep = dwc->eps[epnum];

	if (!(dep->flags & DWC3_EP_ENABLED))
		return;

	if (epnum == 0 || epnum == 1) {
		dwc3_ep0_interrupt(dwc, event);
		return;
	}
	DWC_DBG("%s interrupt comes.\n", dep->name);
	switch (event->endpoint_event) {
	case DWC3_DEPEVT_XFERCOMPLETE:
		dep->resource_index = 0;

		if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			DWC_DBG("%s is an Isochronous endpoint\n",
					dep->name);
			return;
		}

		dwc3_endpoint_transfer_complete(dwc, dep, event);
		break;
	case DWC3_DEPEVT_XFERINPROGRESS:
		dwc3_endpoint_transfer_complete(dwc, dep, event);
		break;
	case DWC3_DEPEVT_XFERNOTREADY:
		/* if (usb_endpoint_xfer_isoc(dep->endpoint.desc)) {
			dwc3_gadget_start_isoc(dwc, dep, event);
		} else {
			int ret;

			DWC_DBG("%s: reason %s\n",
					dep->name, event->status &
					DEPEVT_STATUS_TRANSFER_ACTIVE
					? "Transfer Active"
					: "Transfer Not Active");

			ret = __dwc3_gadget_kick_transfer(dep, 0, 1);
			if (!ret || ret == -EBUSY)
				return;

			DWC_DBG("%s: failed to kick transfers\n",
					dep->name);
		} */

		break;
	case DWC3_DEPEVT_STREAMEVT:
		if (!usb_endpoint_xfer_bulk(dep->endpoint.desc)) {
			DWC_ERR("Stream event for non-Bulk %s\n",
					dep->name);
			return;
		}

		switch (event->status) {
		case DEPEVT_STREAMEVT_FOUND:
			DWC_DBG("Stream %d found and started\n",
					event->parameters);

			break;
		case DEPEVT_STREAMEVT_NOTFOUND:
			/* FALLTHROUGH */
		default:
			DWC_DBG("Couldn't find suitable stream\n");
		}
		break;
	case DWC3_DEPEVT_RXTXFIFOEVT:
		DWC_DBG("%s FIFO Overrun\n", dep->name);
		break;
	case DWC3_DEPEVT_EPCMDCMPLT:
		DWC_DBG("Endpoint Command Complete\n");
		break;
	}
}

static void dwc3_process_event_entry(struct dwc3 *dwc,
		const union dwc3_event *event)
{
	/* Endpoint IRQ, handle it and return early */
	DWC_DBG("<<Dwc3>> process event,event=0x%08x\n", event->raw);
	if (event->type.is_devspec == 0) {
		/* depevt */
		return dwc3_endpoint_interrupt(dwc, &event->depevt);
	}

	switch (event->type.type) {
	case DWC3_EVENT_TYPE_DEV:
		dwc3_gadget_interrupt(dwc, &event->devt);
		break;
	/* REVISIT what to do with Carkit and I2C events ? */
	default:
		DWC_ERR("UNKNOWN IRQ type %d\n", event->raw);
	}
}

static irqreturn_t dwc3_process_event_buf(struct dwc3 *dwc, u32 buf)
{
	struct dwc3_event_buffer *evt;
	irqreturn_t ret = IRQ_NONE;
	int left;
	u32 reg;
	evt = dwc->ev_buffs[buf];
	left = evt->count;

	if (!(evt->flags & DWC3_EVENT_PENDING))
		return IRQ_NONE;

	while (left > 0) {
		union dwc3_event event;

		event.raw = *(u32 *) (evt->buf + evt->lpos);

		dwc3_process_event_entry(dwc, &event);

		/*
		 * FIXME we wrap around correctly to the next entry as
		 * almost all entries are 4 bytes in size. There is one
		 * entry which has 12 bytes which is a regular entry
		 * followed by 8 bytes data. ATM I don't know how
		 * things are organized if we get next to the a
		 * boundary so I worry about that once we try to handle
		 * that.
		 */
		evt->lpos = (evt->lpos + 4) % DWC3_EVENT_BUFFERS_SIZE;
		left -= 4;

		dwc3_writel(dwc->regs, DWC3_GEVNTCOUNT(buf), 4);
	}

	evt->count = 0;
	evt->flags &= ~DWC3_EVENT_PENDING;
	ret = IRQ_HANDLED;

	/* Unmask interrupt */
	reg = dwc3_readl(dwc->regs, DWC3_GEVNTSIZ(buf));
	reg &= ~DWC3_GEVNTSIZ_INTMASK;
	dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(buf), reg);

	return ret;
}

static irqreturn_t dwc3_thread_interrupt(int irq, void *_dwc)
{
	struct dwc3 *dwc = _dwc;
	unsigned long flags;
	irqreturn_t ret = IRQ_NONE;
	int i;
	spin_lock_irqsave(&dwc->lock, flags);

	for (i = 0; i < dwc->num_event_buffers; i++)
		ret |= dwc3_process_event_buf(dwc, i);

	spin_unlock_irqrestore(&dwc->lock, flags);

	return ret;
}

static irqreturn_t dwc3_check_event_buf(struct dwc3 *dwc, u32 buf)
{
	struct dwc3_event_buffer *evt;
	u32 count;
	u32 reg;
	evt = dwc->ev_buffs[buf];

	count = dwc3_readl(dwc->regs, DWC3_GEVNTCOUNT(buf));
	count &= DWC3_GEVNTCOUNT_MASK;
	if (!count)
		return IRQ_NONE;

	evt->count = count;
	evt->flags |= DWC3_EVENT_PENDING;

	/* Mask interrupt */
	reg = dwc3_readl(dwc->regs, DWC3_GEVNTSIZ(buf));
	reg |= DWC3_GEVNTSIZ_INTMASK;
	dwc3_writel(dwc->regs, DWC3_GEVNTSIZ(buf), reg);

	return IRQ_WAKE_THREAD;
}

static irqreturn_t dwc3_interrupt(int irq, void *_dwc)
{
	struct dwc3			*dwc = _dwc;
	int				i;
	irqreturn_t			ret = IRQ_NONE;

	spin_lock(&dwc->lock);

	for (i = 0; i < dwc->num_event_buffers; i++) {
		irqreturn_t status;

		status = dwc3_check_event_buf(dwc, i);
		if (status == IRQ_WAKE_THREAD)
			ret = status;
	}

	spin_unlock(&dwc->lock);

	return ret;
}

void dwc3_gadget_uboot_handle_interrupt(struct dwc3 *dwc)
{
	int ret = dwc3_interrupt(0, dwc);

	if (ret == IRQ_WAKE_THREAD) {
		int i;
		struct dwc3_event_buffer *evt;

		for (i = 0; i < dwc->num_event_buffers; i++) {
			evt = dwc->ev_buffs[i];
			dwc3_flush_cache((long)evt->buf, evt->length);
		}

		dwc3_thread_interrupt(0, dwc);
	}
}

void dwc3_uboot_handle_interrupt(void)
{
	dwc3_gadget_uboot_handle_interrupt(global_dwc);
}

void udc_disconnect(void)
{
}

/* Turn on the USB connection by enabling the pullup resistor */
int rk_dwc3_connect(void)
{
	DWC_PRINT("rk_dwc3_connect in.\n");
	int ret;
	/* usbphy_tunning(); */

	ret = dwc3_uboot_init(&dwc3_device_data);
	if (ret) {
		DWC_ERR("dwc3_uboot_init failed,err=%d!\n", ret);
		return ret;
	}
	DWC_PRINT("dwc3_uboot_init ok.\n");
	ret = dwc3_gadget_start(&global_dwc->gadget, NULL);
	if (ret) {
		DWC_ERR("dwc3_gadget_start failed,err=%d!\n", ret);
		return ret;
	}
	DWC_PRINT("dwc3_gadget_start ok.\n");
	ret = dwc3_gadget_pullup(&global_dwc->gadget, 1);
	if (ret) {
		DWC_ERR("dwc3_gadget_pullup failed,err=%d!\n", ret);
		return ret;
	}
	DWC_PRINT("dwc3 connect ok.\n");
	/* irq_install_handler(IRQ_USB3, (interrupt_handler_t *)dwc3_uboot_handle_interrupt, (void *)NULL);
	irq_handler_enable(IRQ_USB3); */
	return 0;
}

void rk_dwc3_disconnect(void)
{
	dwc3_uboot_exit(0);
}
/**
 * udc_startup - allow udc code to do any additional startup
 */
void rk_dwc3_startup(struct rk_dwc3_udc_instance *device)
{
	DWC_PRINT("rk_dwc3_startup in.\n");
	int i;
	struct usb_bos_descriptor bos_desc;
	struct usb_dev_cap_header cap;
	global_rk_instance.device_desc = device->device_desc;
	global_rk_instance.config_desc = device->config_desc;
	global_rk_instance.interface_desc = device->interface_desc;
	for (i = 0; i < 2; i++) {
		global_rk_instance.endpoint_desc[i] = device->endpoint_desc[i];
		global_rk_instance.rx_buffer[i] = NULL;
		global_rk_instance.tx_buffer[i] = NULL;
	}
	global_rk_instance.rx_buffer_size = 0;
	global_rk_instance.tx_buffer_size = 0;
	for (i = 0; i < 6; i++)
		global_rk_instance.string_desc[i] = device->string_desc[i];
	bos_desc.bLength = sizeof(struct usb_bos_descriptor);
	bos_desc.bDescriptorType = USB_DT_BOS;
	bos_desc.wTotalLength = 8;
	bos_desc.bNumDeviceCaps = 1;
	cap.bLength = sizeof(struct usb_dev_cap_header);
	cap.bDescriptorType = USB_DT_DEVICE_CAPABILITY;
	cap.bDevCapabilityType = 0;
	memcpy(global_rk_instance.bos_desc, &bos_desc, bos_desc.bLength);
	memcpy(global_rk_instance.bos_desc+bos_desc.bLength, &cap, cap.bLength);

	memcpy(global_rk_instance.whole_config_desc, (uint8_t *)global_rk_instance.config_desc, sizeof(struct usb_config_descriptor));
	global_rk_instance.whole_config_size = sizeof(struct usb_config_descriptor);
	memcpy(global_rk_instance.whole_config_desc + global_rk_instance.whole_config_size, global_rk_instance.interface_desc, sizeof(struct usb_interface_descriptor));
	global_rk_instance.whole_config_size += sizeof(struct usb_interface_descriptor);
	memcpy(global_rk_instance.whole_config_desc + global_rk_instance.whole_config_size, global_rk_instance.endpoint_desc[0], 7);
	global_rk_instance.whole_config_size += 7;
	memcpy(global_rk_instance.whole_config_desc + global_rk_instance.whole_config_size, global_rk_instance.endpoint_desc[1], 7);
	global_rk_instance.whole_config_size += 7;
	global_rk_instance.device_event = device->device_event;
	global_rk_instance.rx_handler = device->rx_handler;
	global_rk_instance.tx_handler = device->tx_handler;
	global_rk_instance.connected = 0;
	global_rk_instance.suspended = 0;
	global_rk_instance.rx_current_buffer = 0;
	global_rk_instance.tx_current_buffer = 0;
	DWC_PRINT("rk_dwc3_startup out.\n");
}

int rockusb_clear_feature(struct dwc3 *dwc, u32 wIndex)
{
	int ret = -1;
	if (wIndex==0) {/* in ep */
		ret = dwc3_gadget_ep_disable(private_data.in_ep);
		ret = dwc3_send_gadget_generic_command(dwc, DWC3_DGCMD_SELECTED_FIFO_FLUSH, 1 << 5);
		ret = dwc3_gadget_ep_enable(private_data.in_ep, global_rk_instance.endpoint_desc[1]);
	}
	if (wIndex==0x1) {/* out ep */
		ret = dwc3_gadget_ep_disable(private_data.out_ep);
		ret = dwc3_send_gadget_generic_command(dwc, DWC3_DGCMD_SELECTED_FIFO_FLUSH, 0);
		ret = dwc3_gadget_ep_enable(private_data.out_ep, global_rk_instance.endpoint_desc[0]);
	}
	if (wIndex==0x10) {/*  in and out ep */
		ret = dwc3_gadget_ep_disable(private_data.in_ep);
		ret = dwc3_send_gadget_generic_command(dwc, DWC3_DGCMD_SELECTED_FIFO_FLUSH, 1 << 5);
		ret = dwc3_gadget_ep_enable(private_data.in_ep, global_rk_instance.endpoint_desc[1]);
		
		ret = dwc3_gadget_ep_disable(private_data.out_ep);
		ret = dwc3_send_gadget_generic_command(dwc, DWC3_DGCMD_SELECTED_FIFO_FLUSH, 0);
		ret = dwc3_gadget_ep_enable(private_data.out_ep, global_rk_instance.endpoint_desc[0]);
	}
	return ret;
}

int RK_Dwc3ReadBulkEndpoint(uint32_t nLen, void *buffer)
{
	int ret = -1;
	if (buffer) {
		if (global_rk_instance.rx_current_buffer) {
			private_data.out_req2->length = nLen;
			private_data.out_req2->buf = buffer;
			dwc3_invalidate_cache((long)private_data.out_req2->buf, nLen);
			ret = dwc3_gadget_ep_queue(private_data.out_ep, private_data.out_req2, 0xFF);
			global_rk_instance.rx_current_buffer = 0;
		} else {
			private_data.out_req->length = nLen;
			private_data.out_req->buf = buffer;
			dwc3_invalidate_cache((long)private_data.out_req->buf, nLen);
			ret = dwc3_gadget_ep_queue(private_data.out_ep, private_data.out_req, 0xFF);
			global_rk_instance.rx_current_buffer = 1;
		}
	}
	return ret;
}

int RK_Dwc3WriteBulkEndpoint(uint32_t nLen, void *buffer)
{
	int ret = -1;
	if (buffer) {
		if (global_rk_instance.tx_current_buffer) {
			private_data.in_req2->length = nLen;
			private_data.in_req2->buf = buffer;
			dwc3_flush_cache((long)private_data.in_req2->buf, nLen);
			ret = dwc3_gadget_ep_queue(private_data.in_ep, private_data.in_req2, 0xFF);
			global_rk_instance.tx_current_buffer = 0;
		} else {
			private_data.in_req->length = nLen;
			private_data.in_req->buf = buffer;
			dwc3_flush_cache((long)private_data.in_req->buf, nLen);
			ret = dwc3_gadget_ep_queue(private_data.in_ep, private_data.in_req, 0xFF);
			global_rk_instance.tx_current_buffer = 1;
		}
	}
	return ret;
}

uint32_t GetVbus(void)
{
	uint32_t vbus = 1;
#if defined(CONFIG_RKCHIP_RK3366)
	vbus = grf_readl(GRF_SOC_STATUS7);
	vbus = (vbus >> 8) & 0x1;
#elif defined(CONFIG_RKCHIP_RK3399)
	vbus = grf_readl(GRF_SIG_DETECT_STATUS);
	vbus = (vbus >> 3) & 0x1;
#endif
	return vbus;
}

void ClearVbus(void)
{
#if defined(CONFIG_RKCHIP_RK3399)
	uint32_t vbus ;
    vbus = grf_readl(GRF_SIG_DETECT_CLR);
	vbus |= (1 << 3);
    grf_writel(vbus, GRF_SIG_DETECT_CLR);
#endif
}

