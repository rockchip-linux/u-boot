/*
 * Based on drivers/usb/gadget/omap1510_udc.c
 * TI OMAP1510 USB bus interface driver
 *
 * (C) Copyright 2009
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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


/* usb host base */
#if defined(CONFIG_RKCHIP_RK3288)
	#define RKIO_USBHOST_BASE	RKIO_USBHOST1_PHYS
#elif defined(CONFIG_RKCHIP_RK3036)
	#define RKIO_USBHOST_BASE	RKIO_USBHOST20_PHYS
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	#define RKIO_USBHOST_BASE	RKIO_USBHOST_EHCI_PHYS
#else
	#error "PLS config chiptype for usb otg base!"
#endif


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

// rock-chip dwc_otg controller
#define     TIME_OUT                    5000   //5000ms
#define     HOST_CHN_NUM                 4

#define HCSTAT_SETUP    ((uint32_t)1)
#define HCSTAT_DATA     ((uint32_t)2)
#define HCSTAT_STAT     ((uint32_t)3)
#define HCSTAT_STALL    ((uint32_t)0x0fc)
#define HCSTAT_DONE     ((uint32_t)0x0fd)
#define HCSTAT_REINIT   ((uint32_t)0x0fe)
#define HCSTAT_ERR      ((uint32_t)0x0ff)
typedef volatile struct _HC_INFO
{
	HCTSIZ_DATA hctsiz;
	HCINTMSK_DATA hcintmaskn;
	HCCHAR_DATA hcchar;
	uint32_t pBufAddr;
	uint32_t hcStat;
	uint32_t errCnt;
} HC_INFO, *pHC_INFO;

typedef enum _HOST_RET
{
	HOST_OK = 0,
	HOST_ERR,
	HOST_STALL,
	HOST_NOT_RDY,
	HOST_SPD_UNSP,  //speed not support
	HOST_DEV_UNSP,  //device not support
	HOST_RET_MAX
} HOST_RET;

HC_INFO         g_hcInfo[HOST_CHN_NUM];
static struct dwc_ctrl dwcctl;

int dwc_init_channel(struct usb_device *dev, uint32_t hctsiz, HCCHAR_DATA hcchar, uint32_t buffer)
{
	struct dwc_ctrl *ctrl = dev->controller;
	pUSB_OTG_REG otgReg = ctrl->otgReg;
	uint32_t channel_num = hcchar.b.epnum;

	debug("dwc_init channel hcziz %x, hcdma %x, hcchar %x\n", hctsiz, buffer, hcchar.d32);
	otgReg->Host.haintmsk = 0;
	otgReg->Host.hchn[channel_num].hcintn = 0x7ff;
	otgReg->Host.hchn[channel_num].hcintmaskn = 0;
	otgReg->Host.hchn[channel_num].hctsizn = hctsiz;
	otgReg->Host.hchn[channel_num].hcdman =  buffer;
	otgReg->Host.hchn[channel_num].hccharn = hcchar.d32;
	return 0;
}
int dwc_wait_for_complete(struct usb_device *dev, uint32_t channel_num, uint32_t *hcStat, uint32_t *errCnt)
{
	struct dwc_ctrl *ctrl = dev->controller;
	pUSB_OTG_REG otgReg = ctrl->otgReg;
	HCINT_DATA hcintn;

start:
	hcintn.d32 = otgReg->Host.hchn[channel_num].hcintn;
	debug("dwc_wait_for_complete, hcints %x\n", hcintn.d32);
    
	if(hcintn.b.chhltd)
	{
		if((hcintn.b.stall)||(hcintn.b.xfercomp))
		{
			*errCnt = 0;
			if(hcintn.b.stall)
			{
				*hcStat = HCSTAT_STALL;
				//g_hcInfo[chn].hctsiz.b.dopng = 0;
			}
			else
			{
				*hcStat = HCSTAT_DONE;
				if(hcintn.b.nyet)
				{
					//g_hcInfo[chn].hctsiz.b.dopng = 1;
				}
				else
				{
					//g_hcInfo[chn].hctsiz.b.dopng = 0;
				}
			}
		}
		else if((hcintn.b.xacterr)||(hcintn.b.bblerr))
		{
			if((hcintn.b.ack)||(hcintn.b.nak)||(hcintn.b.nyet))
			{
				*errCnt = 1;
				*hcStat = HCSTAT_REINIT;
			}
			else
			{
				*errCnt++;
				if(*errCnt == 3)
				{
					*hcStat = HCSTAT_ERR;
				}
				else
				{
					*hcStat = HCSTAT_REINIT;
				}
			}
			//g_hcInfo[chn].hctsiz.b.dopng = 0;
		}
		otgReg->Host.hchn[channel_num].hcintn = 0x7ff;
	}

	if(!(*hcStat & 0xf0)){
		udelay(5);
		goto start;
	}

	return 0;
}

#define     DWC_EPDIR_OUT          0x0
#define     DWC_EPDIR_IN           0x1

static int
ehci_submit_async(struct usb_device *dev, unsigned long pipe, void *buffer,
		   int length, struct devrequest *req)
{
	struct dwc_ctrl *ctrl = dev->controller;
	pUSB_OTG_REG otgReg = ctrl->otgReg;
	uint32_t channel_num = usb_pipeendpoint(pipe);
	uint32_t eptype;
	HOST_RET ret = HOST_OK;

	HCTSIZ_DATA hctsiz;
	HCCHAR_DATA hcchar;
	uint32_t hcStat;
	uint32_t errCnt;
	uint32_t packet_size;
	uint32_t datatoggle;

	eptype = usb_pipetype(pipe);
	// ep tye definition is different in pipe and dwc hcchar
	if(eptype == 2 )
		eptype = 0;
	else if (eptype == 3)
		eptype = 2;
    
	debug("ehci_submit_async channel pipe %lx req %p, len %x\n", pipe, req, length);
	if(req == NULL)
		packet_size = 0x200;
	else
		packet_size = 0x40;
	if (req != NULL) {  // setup for control
		hctsiz.d32 = 0;
		hctsiz.b.pid =  DWC_HCTSIZ_SETUP;
		hctsiz.b.pktcnt =  1;
		hctsiz.b.xfersize =  8;
		hcchar.d32 = 0;
		hcchar.b.mps = packet_size; 
		hcchar.b.epnum = channel_num; // use the same channel number as endpoint number
		hcchar.b.epdir = DWC_EPDIR_OUT;
		hcchar.b.eptype = eptype;
		hcchar.b.multicnt = 1;
		hcchar.b.devaddr = usb_pipedevice(pipe);
		hcchar.b.chdis = 0;
		hcchar.b.chen = 1;
		hcStat = HCSTAT_SETUP;
		errCnt = 0;
		dwc_init_channel(dev, hctsiz.d32, hcchar, (uint32_t)req);
		if(dwc_wait_for_complete(dev, channel_num, &hcStat, &errCnt)){
			ret = HOST_ERR;
			goto out;
		}
		if(hcStat != HCSTAT_DONE){
			ret = HOST_ERR;
			goto out;
		}
	}
	
	if (length || (req == NULL)) {    // data for bulk & control
		if(req)
			datatoggle = DWC_HCTSIZ_DATA1;
		else
			datatoggle = ctrl->datatoggle[usb_pipein(pipe)];
		debug("dwc_hcd data len %x toggle %x\n", length, datatoggle);
		hctsiz.d32 = 0;
		hctsiz.b.pid =  datatoggle;
		hctsiz.b.pktcnt =  (length+packet_size - 1)/packet_size;
		hctsiz.b.xfersize =  length;
		hcchar.d32 = 0;
		hcchar.b.mps = packet_size; 
		hcchar.b.epnum = channel_num; // use the same channel number as endpoint number
		hcchar.b.epdir = (req == NULL) ? usb_pipein(pipe) : DWC_EPDIR_IN;
		hcchar.b.eptype = eptype;
		hcchar.b.multicnt = 1;
		hcchar.b.devaddr = usb_pipedevice(pipe);
		hcchar.b.chdis = 0;
		hcchar.b.chen = 1;
		hcStat = HCSTAT_DATA;
		errCnt = 0;
        
		if((req == NULL)&&(hctsiz.b.pktcnt&0x01))
		{
			ctrl->datatoggle[usb_pipein(pipe)] ^= 0x02;
		}
		dwc_init_channel(dev, hctsiz.d32, hcchar, (uint32_t)buffer);
		if(dwc_wait_for_complete(dev, channel_num, &hcStat, &errCnt)){
			ret = HOST_ERR;
			goto out;
		}
		if(hcStat == HCSTAT_STALL)
			ctrl->datatoggle[usb_pipein(pipe)] = 0;
	}
	if (req != NULL) {  // status for control
		debug("status len %x\n", length);
		hctsiz.d32 = 0;
		hctsiz.b.dopng = 0;
		hctsiz.b.pid =  DWC_HCTSIZ_DATA1;
		hctsiz.b.pktcnt =  1;
		hctsiz.b.xfersize =  0;
		hcchar.d32 = 0;
		hcchar.b.mps = packet_size; 
		hcchar.b.epnum = channel_num; // use the same channel number as endpoint number
		hcchar.b.epdir = (length) ? DWC_EPDIR_OUT : DWC_EPDIR_IN;
		hcchar.b.eptype = eptype;
		hcchar.b.multicnt = 1;
		hcchar.b.devaddr = usb_pipedevice(pipe);
		hcchar.b.chdis = 0;
		hcchar.b.chen = 1;
		hcStat = HCSTAT_DATA;
		errCnt = 0;
		dwc_init_channel(dev, hctsiz.d32, hcchar, 0);
		if(dwc_wait_for_complete(dev, channel_num, &hcStat, &errCnt)){
			ret = HOST_ERR;
			goto out;
		}
	}
	
out:
	if(ret){
		debug("dwc_init channel hcziz %x, hcdma %x, hcchar %x\n", 
			otgReg->Host.hchn[channel_num].hctsizn, 
			otgReg->Host.hchn[channel_num].hcdman,
			otgReg->Host.hchn[channel_num].hccharn);
	}
	dev->act_len = length;
	dev->status = 0;
	return (ret);
}

int
ehci_submit_root(struct usb_device *dev, unsigned long pipe, void *buffer,
		 int length, struct devrequest *req)
{
	uint8_t tmpbuf[4];
	u16 typeReq;
	void *srcptr = NULL;
	int len, srclen;
	int port = le16_to_cpu(req->index) & 0xff;
	struct dwc_ctrl *ctrl = dev->controller;
	pUSB_OTG_REG otgReg = ctrl->otgReg;
	HPRT0_DATA hprt0;

	srclen = 0;

	debug("req=%u (%#x), type=%u (%#x), value=%u, index=%u hprt %x\n",
	      req->request, req->request,
	      req->requesttype, req->requesttype,
	      le16_to_cpu(req->value), le16_to_cpu(req->index), otgReg->Host.hprt);

	typeReq = req->request | req->requesttype << 8;
#if 0
	switch (typeReq) {
	case USB_REQ_GET_STATUS | ((USB_RT_PORT | USB_DIR_IN) << 8):
	case USB_REQ_SET_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
	case USB_REQ_CLEAR_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
		status_reg = otgReg->Host.hprt;
		if (!status_reg)
			return -1;
		break;
	default:
		status_reg = NULL;
		break;
	}
	#endif

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
		hprt0.d32 = otgReg->Host.hprt;
		if (hprt0.b.prtconnsts)
			tmpbuf[0] |= USB_PORT_STAT_CONNECTION;
		if (hprt0.b.prtena)
			tmpbuf[0] |= USB_PORT_STAT_ENABLE;
		if (hprt0.b.prtsusp)
			tmpbuf[0] |= USB_PORT_STAT_SUSPEND;
		if (hprt0.b.prtovrcurract)
			tmpbuf[0] |= USB_PORT_STAT_OVERCURRENT;
		if (hprt0.b.prtrst)
			tmpbuf[0] |= USB_PORT_STAT_RESET;
		if (hprt0.b.prtpwr)
			tmpbuf[1] |= USB_PORT_STAT_POWER >> 8;

		switch (hprt0.b.prtspd) {
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

		if (hprt0.b.prtconndet)
			tmpbuf[2] |= USB_PORT_STAT_C_CONNECTION;
		if (hprt0.b.prtenchng)
			tmpbuf[2] |= USB_PORT_STAT_C_ENABLE;
		if (hprt0.b.prtovrcurrchng)
			tmpbuf[2] |= USB_PORT_STAT_C_OVERCURRENT;
		if (ctrl->portreset & (1 << port))
			tmpbuf[2] |= USB_PORT_STAT_C_RESET;

		srcptr = tmpbuf;
		srclen = 4;
		break;
	case USB_REQ_SET_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
		hprt0.d32 = otgReg->Host.hprt;
		//hprt0.b.prtconndet = 0;
		//hprt0.b.prtenchng = 0;
		//hprt0.b.prtovrcurrchng = 0;
		
		switch (le16_to_cpu(req->value)) {
		case USB_PORT_FEAT_ENABLE:
		    error("USB_PORT_FEAT_ENABLE");
			break;
		case USB_PORT_FEAT_POWER:
		    hprt0.b.prtpwr = 1;
		    otgReg->Host.hprt = hprt0.d32;
            // 5V power enable
        	gpio_direction_output(GPIO_BANK0 | GPIO_B6, 1); //gpio0_B6  output high
			break;
		case USB_PORT_FEAT_RESET:
    		hprt0.b.prtrst = 1;
    		otgReg->Host.hprt = hprt0.d32;
    		mdelay(60);
    		hprt0.b.prtrst = 0;
    		otgReg->Host.hprt = hprt0.d32;
			break;
		case USB_PORT_FEAT_TEST:
			break;
		default:
			debug("unknown feature %x\n", le16_to_cpu(req->value));
			goto unknown;
		}
		break;
	case USB_REQ_CLEAR_FEATURE | ((USB_DIR_OUT | USB_RT_PORT) << 8):
		    hprt0.d32 = otgReg->Host.hprt;
	
			switch (le16_to_cpu(req->value)) {		
			case USB_PORT_FEAT_ENABLE:		
    			hprt0.b.prtena = 1;		
    			otgReg->Host.hprt = hprt0.d32;
    			break;		
			case USB_PORT_FEAT_SUSPEND:	
    			hprt0.b.prtres = 1;			
    			otgReg->Host.hprt = hprt0.d32;			
    			/* Clear Resume bit */            	
    			mdelay (100);            		
    			hprt0.b.prtres = 0;	
    			otgReg->Host.hprt = hprt0.d32;	
    			break;
			case USB_PORT_FEAT_POWER:			
    			hprt0.b.prtpwr = 0;			
    			otgReg->Host.hprt = hprt0.d32;		
                // 5V power enable
            	gpio_direction_output(GPIO_BANK0 | GPIO_B6, 0); //gpio0_B6  output low
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

int usb_lowlevel_stop(int index)
{
	return 0;
}


int usb_lowlevel_init(int index, enum usb_init_type init, void **controller)
{
	pUSB_OTG_REG otgReg = (pUSB_OTG_REG)RKIO_USBHOST_BASE;
	GINTMSK_DATA gintmsk;
	uint32_t count;

	dwcctl.otgReg = otgReg;
	for (count=0; count<10000; count++)
	{
		if ((otgReg->Core.grstctl & (1ul<<31))!=0)
			break;
	}
	otgReg->ClkGate.PCGCR=0x00;               //Restart the Phy Clock
	//core soft reset
	otgReg->Core.grstctl|=1<<0;               //Core soft reset
	for (count=0; count<10000; count++)
	{
		if ((otgReg->Core.grstctl & (1<<0))==0)
			break;
	}
	// 16bit phy if,force host mode
	otgReg->Core.gusbcfg |= ((0x01u<<3)|(0x01<<29));
	udelay(20);

	otgReg->Core.grxfsiz= 0x0208;
	otgReg->Core.gnptxfsiz = 0x00800208;//0x04000208
	otgReg->Core.gnptxfsiz = 0x00800208;//0x04000208
	otgReg->Host.hprt |= (0x01<<12);          //power on the port
	otgReg->Core.gintsts=0xffffffff;
	otgReg->Core.gotgint=0xffffffff;
	gintmsk.d32 = 0;
	gintmsk.b.disconnint = 1;
	gintmsk.b.conidstschng = 1;
	gintmsk.b.hchint = 1;
	gintmsk.b.prtint = 1;
	otgReg->Core.gintmsk = gintmsk.d32;
	otgReg->Core.gahbcfg = 0x2f;      //unmask int, internal dma

	dwcctl.rootdev = 0;
	dwcctl.datatoggle[0] = 0;
	dwcctl.datatoggle[1] = 0;

	*controller = &dwcctl;
	return 0;
}

int
submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int length)
{

	if (usb_pipetype(pipe) != PIPE_BULK) {
		debug("non-bulk pipe (type=%lu)", usb_pipetype(pipe));
		return -1;
	}
	return ehci_submit_async(dev, pipe, buffer, length, NULL);
}

int
submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		   int length, struct devrequest *setup)
{
	struct dwc_ctrl *ctrl = dev->controller;

	if (usb_pipetype(pipe) != PIPE_CONTROL) {
		debug("non-control pipe (type=%lu)", usb_pipetype(pipe));
		return -1;
	}

	if (usb_pipedevice(pipe) == ctrl->rootdev) {
		if (!ctrl->rootdev)
			dev->speed = USB_SPEED_HIGH;
		return ehci_submit_root(dev, pipe, buffer, length, setup);
	}
	return ehci_submit_async(dev, pipe, buffer, length, setup);
}
int
submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
	       int length, int interval)
{
	return 0;
}

