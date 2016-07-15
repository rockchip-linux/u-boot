/*
 * Based on drivers/usb/gadget/omap1510_udc.c
 * TI OMAP1510 USB bus interface driver
 *
 * (C) Copyright 2009
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
 *
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <errno.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <usb.h>
#include <asm/io.h>
#include <malloc.h>
#include <watchdog.h>
#include <linux/compiler.h>

#include <asm/arch/rkplat.h>
#include "../gadget/dwc_otg_regs.h"
#include "ehci.h"

#define T_ERROR_RETRY_INTERVAL_MS 10

typedef enum {
	HCSTAT_DONE = 0,
	HCSTAT_XFERERR,
	HCSTAT_BABBLE,
	HCSTAT_STALL,
	HCSTAT_UNKNOW,
	HCSTAT_TIMEOUT,
} hcstat_t;

#define DMA_SIZE (64 * 1024)

static struct descriptor {
	struct usb_hub_descriptor hub;
	struct usb_device_descriptor device;
	struct usb_linux_config_descriptor config;
	struct usb_linux_interface_descriptor interface;
	struct usb_endpoint_descriptor endpoint;
}  __attribute__ ((packed)) descriptor = {
	{
		0x8,		/* bDescLength */
		0x29,		/* bDescriptorType: hub descriptor */
		1,		/* bNrPorts -- runtime modified */
		0,		/* wHubCharacteristics */
		10,		/* bPwrOn2PwrGood */
		0,		/* bHubCntrCurrent */
		{},		/* Device removable */
		{}		/* at most 7 ports! XXX */
	},
	{
		0x12,		/* bLength */
		1,		/* bDescriptorType: UDESC_DEVICE */
		cpu_to_le16(0x0200), /* bcdUSB: v2.0 */
		9,		/* bDeviceClass: UDCLASS_HUB */
		0,		/* bDeviceSubClass: UDSUBCLASS_HUB */
		1,		/* bDeviceProtocol: UDPROTO_HSHUBSTT */
		64,		/* bMaxPacketSize: 64 bytes */
		0x0000,		/* idVendor */
		0x0000,		/* idProduct */
		cpu_to_le16(0x0100), /* bcdDevice */
		1,		/* iManufacturer */
		2,		/* iProduct */
		0,		/* iSerialNumber */
		1		/* bNumConfigurations: 1 */
	},
	{
		0x9,
		2,		/* bDescriptorType: UDESC_CONFIG */
		cpu_to_le16(0x19),
		1,		/* bNumInterface */
		1,		/* bConfigurationValue */
		0,		/* iConfiguration */
		0x40,		/* bmAttributes: UC_SELF_POWER */
		0		/* bMaxPower */
	},
	{
		0x9,		/* bLength */
		4,		/* bDescriptorType: UDESC_INTERFACE */
		0,		/* bInterfaceNumber */
		0,		/* bAlternateSetting */
		1,		/* bNumEndpoints */
		9,		/* bInterfaceClass: UICLASS_HUB */
		0,		/* bInterfaceSubClass: UISUBCLASS_HUB */
		0,		/* bInterfaceProtocol: UIPROTO_HSHUBSTT */
		0		/* iInterface */
	},
	{
		0x7,		/* bLength */
		5,		/* bDescriptorType: UDESC_ENDPOINT */
		0x81,		/* bEndpointAddress:
				 * UE_DIR_IN | EHCI_INTR_ENDPT
				 */
		3,		/* bmAttributes: UE_INTERRUPT */
		8,		/* wMaxPacketSize */
		255		/* bInterval */
	},
};
static int
dwc2_submit_root(struct usb_device *dev, unsigned long pipe, void *buffer,
                 int length, struct devrequest *req)
{
	uint8_t tmpbuf[4];
	u16 typeReq;
	void *srcptr = NULL;
	int len, srclen;
	int port = le16_to_cpu(req->index) & 0xff;
	struct dwc_ctrl *ctrl = dev->controller;
	pUSB_OTG_REG reg = ctrl->otgReg;
	HPRT0_DATA hprt0;

	srclen = 0;

	debug("req=%u (%#x), type=%u (%#x), value=%u, index=%u hprt %x\n",
	      req->request, req->request,
	      req->requesttype, req->requesttype,
	      le16_to_cpu(req->value), le16_to_cpu(req->index), reg->Host.hprt);

	typeReq = req->request | req->requesttype << 8;

	switch (typeReq) {
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		switch (le16_to_cpu(req->value) >> 8) {
		case USB_DT_DEVICE:
			debug("USB_DT_DEVICE request\n");
			srcptr = &descriptor.device;
			srclen = descriptor.device.bLength;
			break;
		case USB_DT_CONFIG:
			debug("USB_DT_CONFIG config\n");
			srcptr = &descriptor.config;
			srclen = descriptor.config.bLength +
				 descriptor.interface.bLength +
				 descriptor.endpoint.bLength;
			break;
		case USB_DT_STRING:
			debug("USB_DT_STRING config\n");
			switch (le16_to_cpu(req->value) & 0xff) {
			case 0:	/* Language */
				srcptr = "\4\3\1\0";
				srclen = 4;
				break;
			case 1:	/* Vendor */
				srcptr = "\16\3R\0o\0c\0k\0c\0h\0i\0p\0";
				srclen = 14;
				break;
			case 2:	/* Product */
				srcptr = "\52\3D\0W\0C\0-\0 "
				         "\0H\0C\0D\0 "
				         "\0C\0o\0n\0t\0r\0o\0l\0l\0e\0r\0";
				srclen = 42;
				break;
			default:
				debug("unknown value DT_STRING %x\n",
				      le16_to_cpu(req->value));
				goto unknown;
			}
			break;
		default:
			debug("unknown value %x\n", le16_to_cpu(req->value));
			goto unknown;
		}
		break;
	case USB_REQ_GET_DESCRIPTOR | ((USB_DIR_IN | USB_RT_HUB) << 8):
		switch (le16_to_cpu(req->value) >> 8) {
		case USB_DT_HUB:
			debug("USB_DT_HUB config\n");
			srcptr = &descriptor.hub;
			srclen = descriptor.hub.bLength;
			break;
		default:
			debug("unknown value %x\n", le16_to_cpu(req->value));
			goto unknown;
		}
		break;
	case USB_REQ_SET_ADDRESS | (USB_RECIP_DEVICE << 8):
		debug("USB_REQ_SET_ADDRESS\n");
		ctrl->rootdev = le16_to_cpu(req->value);
		break;
	case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
		debug("USB_REQ_SET_CONFIGURATION\n");
		/* Nothing to do */
		break;
	case USB_REQ_GET_STATUS | ((USB_DIR_IN | USB_RT_HUB) << 8):
		tmpbuf[0] = 1;	/* USB_STATUS_SELFPOWERED */
		tmpbuf[1] = 0;
		srcptr = tmpbuf;
		srclen = 2;
		break;
	case USB_REQ_GET_STATUS | ((USB_RT_PORT | USB_DIR_IN) << 8):
		memset(tmpbuf, 0, 4);
		hprt0.d32 = reg->Host.hprt;
		if (hprt0.prtconnsts)
			tmpbuf[0] |= USB_PORT_STAT_CONNECTION;
		if (hprt0.prtena)
			tmpbuf[0] |= USB_PORT_STAT_ENABLE;
		if (hprt0.prtsusp)
			tmpbuf[0] |= USB_PORT_STAT_SUSPEND;
		if (hprt0.prtovrcurract)
			tmpbuf[0] |= USB_PORT_STAT_OVERCURRENT;
		if (hprt0.prtrst)
			tmpbuf[0] |= USB_PORT_STAT_RESET;
		if (hprt0.prtpwr)
			tmpbuf[1] |= USB_PORT_STAT_POWER >> 8;

		switch (hprt0.prtspd) {
		case DWC_HPRT0_PRTSPD_LOW_SPEED:
			break;
		case DWC_HPRT0_PRTSPD_FULL_SPEED:
			tmpbuf[1] |= USB_PORT_STAT_LOW_SPEED >> 8;
			break;
		case DWC_HPRT0_PRTSPD_HIGH_SPEED:
		default:
			tmpbuf[1] |= USB_PORT_STAT_HIGH_SPEED >> 8;
			break;
		}

		if (hprt0.prtconndet)
			tmpbuf[2] |= USB_PORT_STAT_C_CONNECTION;
		if (hprt0.prtenchng)
			tmpbuf[2] |= USB_PORT_STAT_C_ENABLE;
		if (hprt0.prtovrcurrchng)
			tmpbuf[2] |= USB_PORT_STAT_C_OVERCURRENT;
		if (ctrl->portreset & (1 << port))
			tmpbuf[2] |= USB_PORT_STAT_C_RESET;

		srcptr = tmpbuf;
		srclen = 4;
		break;
	case USB_REQ_SET_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
		hprt0.d32 = reg->Host.hprt;
		//hprt0.prtconndet = 0;
		//hprt0.prtenchng = 0;
		//hprt0.prtovrcurrchng = 0;

		switch (le16_to_cpu(req->value)) {
		case USB_PORT_FEAT_ENABLE:
			error("USB_PORT_FEAT_ENABLE");
			break;
		case USB_PORT_FEAT_POWER:
			hprt0.prtpwr = 1;
			reg->Host.hprt = hprt0.d32;
			break;
		case USB_PORT_FEAT_RESET:
			hprt0.prtrst = 1;
			reg->Host.hprt = hprt0.d32;
			mdelay(60);
			hprt0.prtrst = 0;
			reg->Host.hprt = hprt0.d32;
			break;
		case USB_PORT_FEAT_TEST:
			break;
		default:
			debug("unknown feature %x\n", le16_to_cpu(req->value));
			goto unknown;
		}
		break;
	case USB_REQ_CLEAR_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
		hprt0.d32 = reg->Host.hprt;

		switch (le16_to_cpu(req->value)) {
		case USB_PORT_FEAT_ENABLE:
			hprt0.prtena = 1;
			reg->Host.hprt = hprt0.d32;
			break;
		case USB_PORT_FEAT_SUSPEND:
			hprt0.prtres = 1;
			reg->Host.hprt = hprt0.d32;
			/* Clear Resume bit */
			mdelay(100);
			hprt0.prtres = 0;
			reg->Host.hprt = hprt0.d32;
			break;
		case USB_PORT_FEAT_POWER:
			hprt0.prtpwr = 0;
			reg->Host.hprt = hprt0.d32;
			break;
		case USB_PORT_FEAT_OVER_CURRENT:
			break;
		case USB_PORT_FEAT_C_CONNECTION:
			break;
		case USB_PORT_FEAT_C_RESET:
			break;
		case USB_PORT_FEAT_C_ENABLE:
			break;
		case USB_PORT_FEAT_C_SUSPEND:
			break;
		case USB_PORT_FEAT_C_OVER_CURRENT:
			break;
		default:
			goto unknown;
		}

		break;
	default:
		debug("Unknown request\n");
		goto unknown;
	}

	mdelay(1);
	len = min3(srclen, le16_to_cpu(req->length), length);
	if (srcptr != NULL && len > 0)
		memcpy(buffer, srcptr, len);
	else
		debug("Len is 0\n");

	dev->act_len = len;
	dev->status = 0;
	return 0;

unknown:
	debug("requesttype=%x, request=%x, value=%x, index=%x, length=%x\n",
	      req->requesttype, req->request, le16_to_cpu(req->value),
	      le16_to_cpu(req->index), le16_to_cpu(req->length));

	dev->act_len = 0;
	dev->status = USB_ST_STALLED;
	return -1;
}

int dwc_wait_for_complete(struct usb_device *dev, uint32_t ch)
{
	struct dwc_ctrl *ctrl = dev->controller;
	pUSB_OTG_REG reg = ctrl->otgReg;
	HCINT_DATA hcint;
	HCTSIZ_DATA hctsiz;
	HCCHAR_DATA hcchar;

	int timeout = 3000000; /* time out after 3000000 * 1us = 3s */
	/*
	 * TODO: We should take care of up to three times of transfer error
	 * retry here, according to the USB 2.0 spec 4.5.2
	 */
	do {
		hcint.d32 = reg->Host.hchn[ch].hcintn;
		hctsiz.d32 = reg->Host.hchn[ch].hctsizn;
		udelay(1);

		if (hcint.chhltd) {
			reg->Host.hchn[ch].hcintn = hcint.d32;

			if (hcint.xfercomp) {
				return hctsiz.xfersize;
			} else if (hcint.xacterr) {
				return -HCSTAT_XFERERR;
			} else if (hcint.bblerr) {
				return -HCSTAT_BABBLE;
			} else if (hcint.stall) {
				return -HCSTAT_STALL;
			} else if (hcint.xacterr) {
				/*Todo if transfer error wait for a whilt then retry*/
				mdelay(T_ERROR_RETRY_INTERVAL_MS);
				printf("#%s# Transfer Error\n", __func__);
				break;
			} else {
				printf("#%s# Unknow Problem :"
				       "hcint 0x%08x - hctsize 0x%08x - hcchar 0x%08x\n",
				       __func__, hcint.d32, hctsiz.d32,
				       reg->Host.hchn[ch].hccharn);
				return -HCSTAT_UNKNOW;
			}
		}
	} while (timeout--);

	/* Release the channel when hit timeout condition */
	hcchar.d32 = reg->Host.hchn[ch].hccharn;
	if (hcchar.chen) {
		/*
		 * Programming the HCCHARn register with the chdis and
		 * chena bits set to 1 at the same time to disable the
		 * channel and the core will generate a channel halted
		 * interrupt.
		 */
		hcchar.chdis = 1;
		reg->Host.hchn[ch].hccharn = hcchar.d32;
		do {
			hcchar.d32 = reg->Host.hchn[ch].hccharn;
		} while (hcchar.chen);
	}

	/* Clear all pending interrupt flags */
	hcint.d32 = ~0;
	reg->Host.hchn[ch].hcintn = hcint.d32;

	return -HCSTAT_TIMEOUT;
}

static int
dwc2_transfer(struct usb_device *dev, unsigned long pipe, int size,
	      int pid, ep_dir_t dir, uint32_t ch_num, u8 *data_buf)
{
	struct dwc_ctrl *ctrl = dev->controller;
	pUSB_OTG_REG reg = ctrl->otgReg;
	uint32_t do_copy;
	int ret;
	uint32_t packet_cnt;
	uint32_t packet_size;
	uint32_t transferred = 0;
	uint32_t inpkt_length;
	uint32_t eptype;
	HCTSIZ_DATA hctsiz = { .d32 = 0 };
	HCCHAR_DATA hcchar = { .d32 = 0 };
	void *aligned_buf;

	debug("# %s #dev %p, size %d, pid %d, dir %d, buf %p\n",
	      __func__, dev, size, pid, dir, data_buf);

	if (dev->speed != USB_SPEED_HIGH) {
		printf("Support high-speed only\n");
		return -1;
	}

	if (size > DMA_SIZE) {
		printf("Transfer too large: %d\n", size);
		return -1;
	}
	packet_size = usb_maxpacket(dev, pipe);
	packet_cnt = DIV_ROUND_UP(size, packet_size);
	inpkt_length = roundup(size, packet_size);

	/* At least 1 packet should be programed */
	packet_cnt = (packet_cnt == 0) ? 1 : packet_cnt;

	/*
	 * For an IN, this field is the buffer size that the application has
	 * reserved for the transfer. The application should program this field
	 * as integer multiple of the maximum packet size for IN transactions.
	 */
	hctsiz.xfersize = (dir == EPDIR_OUT) ? size : inpkt_length;
	hctsiz.pktcnt = packet_cnt;
	hctsiz.pid = pid;

	hcchar.mps = packet_size;
	hcchar.epnum = usb_pipeendpoint(pipe);
	hcchar.epdir = dir;

	switch (usb_pipetype(pipe)) {
	case PIPE_CONTROL:
		eptype = 0;
		break;
	case PIPE_BULK:
		eptype = 2;
		break;
	default:
		printf("Un-supported type\n");
		return -EOPNOTSUPP;
	}
	hcchar.eptype = eptype;
	hcchar.multicnt = 1;
	hcchar.devaddr = usb_pipedevice(pipe);
	hcchar.chdis = 0;
	hcchar.chen = 1;

	/*
	 * Check the buffer address which should be 4-byte aligned and DMA
	 * coherent
	 */
	//do_copy = !dma_coherent(data_buf) || ((uintptr_t)data_buf & 0x3);
	do_copy = 1;//(uintptr_t)data_buf & 0x3;
	aligned_buf = do_copy ? ctrl->align_buf : data_buf;

	if (do_copy && (dir == EPDIR_OUT))
		memcpy(aligned_buf, data_buf, size);

	if (dir == EPDIR_OUT)
		flush_dcache_range(aligned_buf, aligned_buf +
				   roundup(size, ARCH_DMA_MINALIGN));

	writel(hctsiz.d32, &reg->Host.hchn[ch_num].hctsizn);
	writel((uint32_t)aligned_buf, &reg->Host.hchn[ch_num].hcdman);
	writel(hcchar.d32, &reg->Host.hchn[ch_num].hccharn);

	ret = dwc_wait_for_complete(dev, ch_num);

	if (ret >= 0) {
		/* Calculate actual transferred length */
		transferred = (dir == EPDIR_IN) ? inpkt_length - ret : ret;

		if (dir == EPDIR_IN)
			invalidate_dcache_range(aligned_buf, aligned_buf +
				roundup(transferred, ARCH_DMA_MINALIGN));

		if (do_copy && (dir == EPDIR_IN))
			memcpy(data_buf, aligned_buf, transferred);
	}

	/* Save data toggle */
	hctsiz.d32 = readl(&reg->Host.hchn[ch_num].hctsizn);
	usb_settoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe),
	              (hctsiz.pid >> 1));

	if (ret < 0) {
		printf("%s Transfer stop code: %d\n", __func__, ret);
		return ret;
	}

	return transferred;
}

int usb_lowlevel_stop(int index)
{
	pUSB_OTG_REG reg = (pUSB_OTG_REG)rkusb_active_hcd->regbase;
	HPRT0_DATA hprt0 = { .d32 = 0 };
	GUSBCFG_DATA gusbcfg = { .d32 = 0 };

	/* Stop connect and put everything of port state in reset. */
	hprt0.prtena = 1;
	hprt0.prtenchng = 1;
	hprt0.prtconndet = 1;
	writel(hprt0.d32, &reg->Host.hprt);

	gusbcfg.d32 = 0x1400;
	writel(gusbcfg.d32, &reg->Core.gusbcfg);

	return 0;
}

static void dwc2_reinit(pUSB_OTG_REG regbase)
{
	pUSB_OTG_REG reg = regbase;
	GUSBCFG_DATA gusbcfg = { .d32 = 0 };
	GRSTCTL_DATA grstctl = { .d32 = 0 };
	GINTSTS_DATA gintsts = { .d32 = 0 };
	GAHBCFG_DATA gahbcfg = { .d32 = 0 };
	RXFIFOSIZE_DATA grxfsiz = { .d32 = 0 };
	HCINTMSK_DATA hcintmsk = { .d32 = 0 };
	TXFIFOSIZE_DATA gnptxfsiz = { .d32 = 0 };

	const int timeout = 10000;
	int i;
	/* Wait for AHB idle */
	for (i = 0; i < timeout; i++) {
		udelay(1);
		grstctl.d32 = readl(&reg->Core.grstctl);
		if (grstctl.ahbidle)
			break;
	}
	if (i == timeout) {
		printf("DWC2 Init error AHB Idle\n");
		return;
	}

	/* Restart the Phy Clock */
	writel(0x0, &reg->ClkGate.PCGCR);
	/* Core soft reset */
	grstctl.csftrst = 1;
	writel(grstctl.d32, &reg->Core.grstctl);
	for (i = 0; i < timeout; i++) {
		udelay(1);
		grstctl.d32 = readl(&reg->Core.grstctl);
		if (!grstctl.csftrst)
			break;
	}
	if (i == timeout) {
		printf("DWC2 Init error reset fail\n");
		return;
	}

	/* Set 16bit PHY if & Force host mode */
	gusbcfg.d32 = readl(&reg->Core.gusbcfg);
	gusbcfg.phyif = 1;
	gusbcfg.forcehstmode = 1;
	gusbcfg.forcedevmode = 0;
	writel(gusbcfg.d32, &reg->Core.gusbcfg);
	/* Wait for force host mode effect, it may takes 100ms */
	for (i = 0; i < timeout; i++) {
		udelay(10);
		gintsts.d32 = readl(&reg->Core.gintsts);
		if (gintsts.curmod)
			break;
	}
	if (i == timeout) {
		printf("DWC2 Init error force host mode fail\n");
		return;
	}

	/*
	 * Config FIFO
	 * The non-periodic tx fifo and rx fifo share one continuous
	 * piece of IP-internal SRAM.
	 */
	grxfsiz.rxfdep = DWC2_RXFIFO_DEPTH;
	writel(grxfsiz.d32, &reg->Core.grxfsiz);
	gnptxfsiz.nptxfstaddr = DWC2_RXFIFO_DEPTH;
	gnptxfsiz.nptxfdep = DWC2_NPTXFIFO_DEPTH;
	writel(gnptxfsiz.d32, &reg->Core.gnptxfsiz);

	/* Init host channels */
	hcintmsk.xfercomp = 1;
	hcintmsk.xacterr = 1;
	hcintmsk.stall = 1;
	hcintmsk.chhltd = 1;
	hcintmsk.bblerr = 1;
	for (i = 0; i < MAX_EPS_CHANNELS; i++)
		writel(hcintmsk.d32, &reg->Host.hchn[i].hcintmaskn);

	/* Unmask interrupt & Use configure dma mode */
	gahbcfg.glblintrmsk = 1;
	gahbcfg.hbstlen = DWC_GAHBCFG_INT_DMA_BURST_INCR8;
	gahbcfg.dmaen = 1;
	writel(gahbcfg.d32, &reg->Core.gahbcfg);

	printf("DWC2@0x%p init finished!\n", regbase);
}

int usb_lowlevel_init(int index, enum usb_init_type init, void **controller)
{
	pUSB_OTG_REG reg = (pUSB_OTG_REG)rkusb_active_hcd->regbase;
	struct dwc_ctrl *dwcctl;

	dwcctl = malloc(sizeof(struct dwc_ctrl));
	if (!dwcctl)
		return -ENOMEM;

	dwcctl->otgReg = reg;
	dwcctl->rootdev = 0;
	dwcctl->align_buf = memalign(USB_DMA_MINALIGN, DMA_SIZE);
	if (!dwcctl->align_buf)
		return -ENOMEM;

	dwc2_reinit(reg);
	*controller = dwcctl;

	return 0;
}

int
submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
                int length)
{
	ep_dir_t data_dir;
	int pid;
	int ret = 0;

	if (usb_pipetype(pipe) != PIPE_BULK) {
		debug("non-bulk pipe (type=%lu)", usb_pipetype(pipe));
		return -1;
	}

	if (usb_pipein(pipe))
		data_dir = EPDIR_IN;
	else if (usb_pipeout(pipe))
		data_dir = EPDIR_OUT;
	else
		return -1;

	pid = usb_gettoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe));
	if (pid)
		pid = DWC_HCTSIZ_DATA1;
	else
		pid = DWC_HCTSIZ_DATA0;

	ret = dwc2_transfer(dev, pipe, length, pid, data_dir, 0, buffer);

	if (ret < 0)
		return -1;

	dev->act_len = ret;
	dev->status = 0;
	return 0;
}

int
submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		   int length, struct devrequest *setup)
{
	int ret = 0;
	struct dwc_ctrl *ctrl = dev->controller;
	ep_dir_t data_dir;

	if (usb_pipetype(pipe) != PIPE_CONTROL) {
		debug("non-control pipe (type=%lu)", usb_pipetype(pipe));
		return -1;
	}

	if (usb_pipedevice(pipe) == ctrl->rootdev) {
		if (!ctrl->rootdev)
			dev->speed = USB_SPEED_HIGH;
		return dwc2_submit_root(dev, pipe, buffer, length, setup);
	}

	if (usb_pipein(pipe))
		data_dir = EPDIR_IN;
	else if (usb_pipeout(pipe))
		data_dir = EPDIR_OUT;
	else
		return -1;

	/* Setup Phase */
	if (dwc2_transfer(dev, pipe, 8, PID_SETUP, EPDIR_OUT, 0,
			  (u8 *)setup) < 0)
		return -1;
	/* Data Phase */
	if (length > 0) {
		ret = dwc2_transfer(dev, pipe, length, PID_DATA1, data_dir,
				    0, buffer);
		if (ret < 0)
			return -1;
	}
	/* Status Phase */
	if (dwc2_transfer(dev, pipe, 0, PID_DATA1, !data_dir, 0, NULL) < 0)
		return -1;

	dev->act_len = ret;
	dev->status = 0;
	return 0;
}
int
submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
	       int length, int interval)
{
	return 0;
}

