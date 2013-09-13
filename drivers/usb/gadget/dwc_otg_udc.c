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
#include <asm/io.h>

#include <usbdevice.h>
#include <fastboot.h>
#include "ep0.h"
#include <usb/dwc_otg_udc.h>
#include <asm/arch/rk30_drivers.h>
#include "../../../board/rockchip/common/common/rockusb/dwc_otg_regs.h"
#include "../../../board/rockchip/common/common/rockusb/USB20.h"

#define UDC_INIT_MDELAY		80	/* Device settle delay */
#define FBT_BULK_IN_EP              2
#define FBT_BULK_OUT_EP             1
#define FBT_USB_XFER_BUF_SIZE       (1024*512)
#define FBT_USB_XFER_MAX_SIZE       (0x80*512)

#define DWCERR
#undef DWCWARN
#undef DWCINFO
#undef DWCDEBUG

/* Some kind of debugging output... */
#ifdef DWCERR
#define DWC_ERR(fmt, args...)\
    printf("ERROR: [%s]: "fmt, __func__, ##args)
#else
#define DWC_ERR(fmt, args...) do {} while (0)
#endif

#ifdef DWCWARN
#define DWC_WARN(fmt, args...)\
    printf("WARNING: [%s]: "fmt, __func__, ##args)
#else
#define DWC_WARN(fmt, args...) do {} while (0)
#endif

#ifdef DWCINFO
#define DWC_PRINT(fmt, args...)\
    printf("INFO: [%s]: "fmt, __func__, ##args)
#else
#define DWC_PRINT(fmt, args...) do {} while (0)
#endif

#ifdef DWCDEBUG
#define DWC_DBG(fmt, args...)\
    printf("DEBUG: [%s]: %d:\n"fmt, __func__, __LINE__, ##args)
#else
#define DWC_DBG(fmt, args...) do {} while (0)
#endif

static struct urb *ep0_urb;
static struct usb_device_instance *udc_device;
static pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)USB_OTG_BASE_ADDR;

static void udc_stall_ep(uint32 ep_num);
static void dwc_otg_epn_rx(uint32);
static void dwc_otg_epn_tx(struct usb_endpoint_instance *endpoint);

extern uint32 RockusbEn;
ALIGN(8) uint8 FbtBulkInBuf[512];
ALIGN(8) uint8 FbtBulkOutBuf[512];
ALIGN(64) uint32 FbtXferBuf[FBT_USB_XFER_BUF_SIZE];
/*
 * udc_state_transition - Write the next packet to TxFIFO.
 * @initial:	Initial state.
 * @final:	Final state.
 *
 * Helper function to implement device state changes. The device states and
 * the events that transition between them are:
 *
 *				STATE_ATTACHED
 *				||	/\
 *				\/	||
 *	DEVICE_HUB_CONFIGURED			DEVICE_HUB_RESET
 *				||	/\
 *				\/	||
 *				STATE_POWERED
 *				||	/\
 *				\/	||
 *	DEVICE_RESET				DEVICE_POWER_INTERRUPTION
 *				||	/\
 *				\/	||
 *				STATE_DEFAULT
 *				||	/\
 *				\/	||
 *	DEVICE_ADDRESS_ASSIGNED			DEVICE_RESET
 *				||	/\
 *				\/	||
 *				STATE_ADDRESSED
 *				||	/\
 *				\/	||
 *	DEVICE_CONFIGURED			DEVICE_DE_CONFIGURED
 *				||	/\
 *				\/	||
 *				STATE_CONFIGURED
 *
 * udc_state_transition transitions up (in the direction from STATE_ATTACHED
 * to STATE_CONFIGURED) from the specified initial state to the specified final
 * state, passing through each intermediate state on the way. If the initial
 * state is at or above (i.e. nearer to STATE_CONFIGURED) the final state, then
 * no state transitions will take place.
 *
 * udc_state_transition also transitions down (in the direction from
 * STATE_CONFIGURED to STATE_ATTACHED) from the specified initial state to the
 * specified final state, passing through each intermediate state on the way.
 * If the initial state is at or below (i.e. nearer to STATE_ATTACHED) the final
 * state, then no state transitions will take place.
 *
 * This function must only be called with interrupts disabled.
 */
static void udc_state_transition(usb_device_state_t initial,
                                 usb_device_state_t final)
{
        if (initial < final) {
                switch (initial) {
                case STATE_ATTACHED:
                        usbd_device_event_irq(udc_device,
                                              DEVICE_HUB_CONFIGURED, 0);
                        if (final == STATE_POWERED)
                                break;
                case STATE_POWERED:
                        usbd_device_event_irq(udc_device, DEVICE_RESET, 0);
                        if (final == STATE_DEFAULT)
                                break;
                case STATE_DEFAULT:
                        usbd_device_event_irq(udc_device,
                                              DEVICE_ADDRESS_ASSIGNED, 0);
                        if (final == STATE_ADDRESSED)
                                break;
                case STATE_ADDRESSED:
                        usbd_device_event_irq(udc_device, DEVICE_CONFIGURED, 0);
                case STATE_CONFIGURED:
                        break;
                default:
                        break;
                }
        } else if (initial > final) {
                switch (initial) {
                case STATE_CONFIGURED:
                        usbd_device_event_irq(udc_device,
                                              DEVICE_DE_CONFIGURED, 0);
                        if (final == STATE_ADDRESSED)
                                break;
                case STATE_ADDRESSED:
                        usbd_device_event_irq(udc_device, DEVICE_RESET, 0);
                        if (final == STATE_DEFAULT)
                                break;
                case STATE_DEFAULT:
                        usbd_device_event_irq(udc_device,
                                              DEVICE_POWER_INTERRUPTION, 0);
			if (final == STATE_POWERED)
                                break;
                case STATE_POWERED:
                        usbd_device_event_irq(udc_device, DEVICE_HUB_RESET, 0);
                case STATE_ATTACHED:
                        break;
                default:
                        break;
                }
        }
}

void ControlInPacket(void)
{
	uint16 length = ControlData.wLength;
	if(STAGE_DATA == ControlStage)
	{
		if (length > Ep0PktSize)
			length=Ep0PktSize;

		WriteEndpoint0(length, Ep0Buf);
		ControlData.pData += length;
		ControlData.wLength -= length;
		if (ControlData.wLength == 0)
			ControlStage=STAGE_STATUS;
	}
}

static dwc_otg_epn_in_ack(void)
{
	WriteBulkEndpoint(0, NULL);
}

volatile int suspend = 0;
void suspend_usb() {
    suspend = true;
}
void resume_usb(struct usb_endpoint_instance *endpoint, int max_size) {
    if (suspend) {
        suspend = false;
        if (endpoint && endpoint->rcv_urb) {
            struct urb* urb = endpoint->rcv_urb;
            //get available size for next xfer.
            int remaining_space = urb->buffer_length - urb->actual_length;
            if (remaining_space > 0) {
                remaining_space = remaining_space > FBT_USB_XFER_MAX_SIZE? FBT_USB_XFER_MAX_SIZE : remaining_space;

                if (max_size && remaining_space > max_size)
                    remaining_space = max_size;

                usbdbg("next request:%d\n", remaining_space);
                //schedule next xfer.
                ReadBulkEndpoint(remaining_space, FbtXferBuf);
            }
        }
    }
}

/* Flow control */
void udc_set_nak(int epid)
{
	/* noop */
}

void udc_unset_nak(int epid)
{
	/* noop */
}


static void dwc_otg_setup(struct usb_endpoint_instance *endpoint)
{
	DWC_DBG("-> Entering device setup\n");
	ftl_memcpy(&ep0_urb->device_request, Ep0Buf, 8);
	ftl_memcpy(&ControlData.DeviceRequest, Ep0Buf, 8);

	/* Try to process setup packet */
	if (ep0_recv_setup(ep0_urb)) {
		/* Not a setup packet, stall next EP0 transaction */
		udc_stall_ep(0);
		DWC_ERR("can't parse setup packet, still waiting for setup\n");
		return;
	}

	/* Check direction */
	if ((ep0_urb->device_request.bmRequestType & USB_REQ_DIRECTION_MASK)
	    == USB_REQ_HOST2DEVICE) {
		DWC_DBG("control write on EP0\n");
		if (le16_to_cpu(ep0_urb->device_request.wLength)) {
			/* Stall this request */
			DWC_DBG("Stalling unsupported EP0 control write data "
			       "stage.\n");
			udc_stall_ep(0);
		}
		if((ep0_urb->device_request.bmRequestType & USB_REQ_TYPE_MASK)
		== USB_STANDARD_REQUEST)
		{
			switch (ep0_urb->device_request.bRequest & USB_REQUEST_MASK)
			{
				case 5:
					DWC_DBG("set address\n");
					set_address();
					udc_state_transition(udc_device->device_state, STATE_ADDRESSED);
					break;
				case 9:
					DWC_DBG("set configuration\n");
					udc_state_transition(udc_device->device_state,STATE_CONFIGURED);
					set_configuration();
					break;
				default:
					udc_stall_ep(0);
					break;
			}
		}
	} else {

		DWC_DBG("control read on EP0\n");
		/*
		 * The ep0_recv_setup function has already placed our response
		 * packet data in ep0_urb->buffer and the packet length in
		 * ep0_urb->actual_length.
		 */
		endpoint->tx_urb = ep0_urb;
		endpoint->sent = 0;
		
		DWC_DBG("urb->buffer %p, buffer_length %d, actual_length %d\n",
			ep0_urb->buffer,ep0_urb->buffer_length, ep0_urb->actual_length);
		ftl_memcpy(Ep0Buf, ep0_urb->buffer, ep0_urb->actual_length);
		
		//WriteEndpoint0(ep0_urb->actual_length, Ep0Buf);
		ControlData.pData=(uint8*)&Ep0Buf[0];
		ControlData.wLength=ep0_urb->actual_length;
		ControlStage=STAGE_DATA;
		
		ControlInPacket();
	}
	DWC_DBG("<- Leaving device setup\n");
}

static void dwc_otg_enum_done_intr(void)
{
	BulkEpSize=FS_BULK_TX_SIZE;
	switch ((OtgReg->Device.dsts>>1) & 0x03)
	{
		case 0:         //High speed, PHY clock @30MHz or 60MHz
			BulkEpSize=HS_BULK_TX_SIZE;
		case 1:         //Full speed, PHY clock @30MHz or 60MHz
		case 3:         //Full speed, PHY clock @48MHz
			OtgReg->Device.InEp[0].DiEpCtl &= ~0x03;   //64bytes MPS
			Ep0PktSize=EP0_PACKET_SIZE_HS;
			break;
 		case 2:
		default:
			OtgReg->Device.InEp[0].DiEpCtl |= 0x03;   //8bytes MPS
			Ep0PktSize=EP0_PACKET_SIZE_FS;
			break;
	}
	OtgReg->Device.dctl |= 1<<8;               //clear global IN NAK
	ReadEndpoint0(Ep0PktSize, Ep0Buf);
	//ReadBulkEndpoint(31, (uint8*)&gCBW);
    ReadBulkEndpoint(FASTBOOT_COMMAND_SIZE, FbtXferBuf);
	OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl = (1ul<<28) | (1<<15)|(2<<18)|(BULK_IN_EP<<22);
}

static void dwc_otg_in_intr(void)
{
	uint32 i;
	uint32 ch;
	uint32 event;

	ch=(OtgReg->Device.daint & OtgReg->Device.daintmsk) & 0xffff;   //在ROM里只有EP0中断
	for (i=0; i<3; i++)
	{
		if ((1<<i) & ch)
		{
			event=OtgReg->Device.diepmsk | ((OtgReg->Device.dtknqr4_fifoemptymsk & 0x01)<<7);    //<<7是因为msk是保留的
			event &= OtgReg->Device.InEp[i].DiEpInt;
			if ((event & 0x01) != 0)        //Transfer complete
			{
				OtgReg->Device.InEp[i].DiEpInt=0x01;
				if(i == 0)
					ControlInPacket();
				else
				{
					struct usb_endpoint_instance *endpoint;
					endpoint = &udc_device->bus->endpoint_array[2];

					dwc_otg_epn_tx(endpoint);
				}
			}
			if ((event & 0x02) != 0)        //Endpoint disable
			{
				OtgReg->Device.InEp[i].DiEpInt=0x02;
			}
			if ((event & 0x04) != 0)        //AHB Error
			{
				OtgReg->Device.InEp[i].DiEpInt=0x04;
			}
			if ((event & 0x08) != 0)        //TimeOUT Handshake (non-ISOC IN EPs)
			{
				OtgReg->Device.InEp[i].DiEpInt=0x08;
			}
			if ((event & 0x20) != 0)        //IN Token Received with EP mismatch
			{
				OtgReg->Device.InEp[i].DiEpInt=0x20;
			}
			if ((event & 0x80) != 0)        //Transmit FIFO empty
			{
				OtgReg->Device.InEp[i].DiEpInt=0x10;
			}
			if ((event & 0x100) != 0)       //Buffer Not Available
			{
				OtgReg->Device.InEp[i].DiEpInt=0x100;
			}
		}
	}
}

static void dwc_otg_out_intr(void)
{
	uint32 i;
	uint32 ch;
	uint32 event;

	ch=(OtgReg->Device.daint & OtgReg->Device.daintmsk) >> 16;   //在ROM里只有EP0中断
	for (i=0; i<3; i++)
	{
		if ((1<<i) & ch)
		{
			event=OtgReg->Device.OutEp[i].DoEpInt & OtgReg->Device.doepmsk;
			if ((event & 0x01) != 0)        //Transfer complete
			{
				OtgReg->Device.OutEp[i].DoEpInt=0x01;
				if (i==0)
				{
					uint32 len;
					len=Ep0PktSize-(OtgReg->Device.OutEp[0].DoEpTSiz&0x7f);
					if (len>0)
						Ep0OutPacket(len);
					ReadEndpoint0(Ep0PktSize, Ep0Buf);
				}
				else
				{
					uint32 len;
					len=0x20000-(OtgReg->Device.OutEp[BULK_OUT_EP].DoEpTSiz&0x1ffff);
					if (len>0)
					{
						dwc_otg_epn_rx(len);
					}
				}
			}
			if ((event & 0x02) != 0)        //Endpoint disable
			{
				OtgReg->Device.OutEp[i].DoEpInt=0x02;
			}
			if ((event & 0x04) != 0)        //AHB Error
			{
				OtgReg->Device.OutEp[i].DoEpInt=0x04;
			}
			if ((event & 0x08) != 0)        //Setup Phase Done (contorl EPs)
			{
				OtgReg->Device.OutEp[i].DoEpInt=0x08;
				//Setup();
				dwc_otg_setup(udc_device->bus->endpoint_array);
				ReadEndpoint0(Ep0PktSize, Ep0Buf);
			}
		}
	}
}

/* Stall endpoint */
static void udc_stall_ep(uint32 ep_num)
{
	OtgReg->Device.OutEp[ep_num].DoEpCtl |= 1<<21;  //send OUT0 stall handshack
	OtgReg->Device.InEp[ep_num].DiEpCtl |= 1<<21;   //send IN0 stall handshack	
}

static void dwc_otg_write_data(struct usb_endpoint_instance *endpoint)
{
	struct urb *urb = endpoint->tx_urb;

	if(urb){
		uint32 last;

		DWC_DBG("urb->buffer %p, buffer_length %d, actual_length %d \n",
			urb->buffer, urb->buffer_length, urb->actual_length);

		last = MIN(urb->actual_length - endpoint->sent,
			    endpoint->tx_packetSize);

		if(last){
			uint8 *cp = urb->buffer + endpoint->sent;

			DWC_DBG("endpoint->sent %d, tx_packetSize %d, last %d \n",
				endpoint->sent, endpoint->tx_packetSize, last);

			ftl_memcpy(FbtBulkOutBuf, cp, last);
			WriteBulkEndpoint(last, FbtBulkOutBuf);
		}
		endpoint->last = last;
	}

}
/* Called to start packet transmission. */
int udc_endpoint_write(struct usb_endpoint_instance *endpoint)
{
	dwc_otg_epn_tx(endpoint);
	return 0;
}

static void dwc_otg_epn_rx(uint32 len)
{
	struct urb *urb;
	struct usb_endpoint_instance *endpoint;
    int remaining_space = 0;

	endpoint = &udc_device->bus->endpoint_array[1];

	if(endpoint){
        urb = endpoint->rcv_urb;
        remaining_space = FBT_USB_XFER_MAX_SIZE;
        if (urb) {
            uint8 *cp = urb->buffer + urb->actual_length;

            //get available size for next xfer.
            remaining_space = urb->buffer_length - urb->actual_length;
            usbdbg("buffer_length:%d, actual_length:%d, len:%d\n", urb->buffer_length, urb->actual_length, len);
            len = len <= remaining_space ? len : remaining_space;
            if (len > 0)
                ftl_memcpy(cp, FbtXferBuf, len);
        }
        usbd_rcv_complete(endpoint, len, 0);
        remaining_space -= len;
        if (remaining_space <= 0) {
            //buffer is full, so we not do another xfer here. 
            suspend_usb();
        } else {
            //schedule next xfer.
            remaining_space = remaining_space > FBT_USB_XFER_MAX_SIZE? FBT_USB_XFER_MAX_SIZE : remaining_space;
            usbdbg("next request:%d\n", remaining_space);
            ReadBulkEndpoint(remaining_space, FbtXferBuf);
        }
    }
}

static void dwc_otg_epn_tx(struct usb_endpoint_instance *endpoint)
{
	/*
	 * We need to transmit a terminating zero-length packet now if
	 * we have sent all of the data in this URB and the transfer
	 * size was an exact multiple of the packet size.
	 */
	if (endpoint->tx_urb &&
	    (endpoint->last == endpoint->tx_packetSize) &&
	    (endpoint->tx_urb->actual_length - endpoint->sent -
	     endpoint->last == 0)) {
		/* handle zero length packet here */
		dwc_otg_epn_in_ack();
	}

	if (endpoint->tx_urb && endpoint->tx_urb->actual_length) {
		/* retire the data that was just sent */
		usbd_tx_complete(endpoint);
		/*
		 * Check to see if we have more data ready to transmit
		 * now.
		 */
		if (endpoint->tx_urb && endpoint->tx_urb->actual_length) {
			/* write data */
			dwc_otg_write_data(endpoint);

		} else if (endpoint->tx_urb
			   && (endpoint->tx_urb->actual_length == 0)) {
			/* udc_set_nak(ep); */
		}
	}
}

/* Start to initialize h/w stuff */
int udc_init(void)
{
	udc_device = NULL;
	DWC_PRINT("starting \n");
	
	return 0;
}

int is_usbd_high_speed(void)
{
	return ((OtgReg->Device.dsts>>1) & 0x03) ? 0 : 1;
}

/* Turn on the USB connection by enabling the pullup resistor */
void udc_connect(void)
{
	RockusbEn = 0;
	UsbBoot();
	DWC_DBG("OtgReg->Core.grstctl = 0x%08x\n",OtgReg->Core.grstctl);
	DWC_DBG("OtgReg->Device.dcfg = 0x%08x\n",OtgReg->Device.dcfg);
	DWC_DBG("OtgReg->Core.grxfsiz = 0x%08x\n",OtgReg->Core.grxfsiz);
	DWC_DBG("OtgReg->Core.gnptxfsiz = 0x%08x\n",OtgReg->Core.gnptxfsiz);
	DWC_DBG("OtgReg->Core.dptxfsiz_dieptxf[0] = 0x%08x\n",OtgReg->Core.dptxfsiz_dieptxf[0] );
	DWC_DBG("OtgReg->Core.dptxfsiz_dieptxf[1]%08x\n",OtgReg->Core.dptxfsiz_dieptxf[1]);
	DWC_DBG("OtgReg->Device.InEp[0].DiEpCtl = 0x%08x\n",OtgReg->Device.InEp[0].DiEpCtl);
	DWC_DBG("OtgReg->Device.InEp[0].DiEpTSiz = 0x%08x\n",OtgReg->Device.InEp[0].DiEpTSiz);
	DWC_DBG("OtgReg->Device.InEp[0].DiEpDma = 0x%08x\n",OtgReg->Device.InEp[0].DiEpDma);
	DWC_DBG("OtgReg->Device.InEp[0].DiEpInt = 0x%08x\n",OtgReg->Device.InEp[0].DiEpInt);
	DWC_DBG("OtgReg->Device.OutEp[0].DoEpCtl = 0x%08x\n",OtgReg->Device.OutEp[0].DoEpCtl);
	DWC_DBG("OtgReg->Device.OutEp[0].DoEpTSiz = 0x%08x\n",OtgReg->Device.OutEp[0].DoEpTSiz);
	DWC_DBG("OtgReg->Device.OutEp[0].DoEpDma = 0x%08x\n",OtgReg->Device.OutEp[0].DoEpDma);
	DWC_DBG("OtgReg->Device.OutEp[0].DoEpInt = 0x%08x\n",OtgReg->Device.OutEp[0].DoEpInt);
	DWC_DBG("OtgReg->Device.diepmsk = 0x%08x\n",OtgReg->Device.diepmsk);
	DWC_DBG("OtgReg->Device.doepmsk = 0x%08x\n",OtgReg->Device.doepmsk);
	DWC_DBG("OtgReg->Core.gahbcfg = 0x%08x\n",OtgReg->Core.gahbcfg);
}

/* Turn off the USB connection by disabling the pullup resistor */
void udc_disconnect(void)
{

}

/* Switch on the UDC */
void udc_enable(struct usb_device_instance *device)
{
	DWC_PRINT("enable device %p, status %d ,device_state %d\n", device, device->status, device->device_state);

	/* Save the device structure pointer */
	udc_device = device;
	DWC_PRINT("enable device %p, status %d ,device_state %d\n", udc_device, udc_device->status, udc_device->device_state);
	/* Setup ep0 urb */
	if (!ep0_urb) {
		ep0_urb =
			usbd_alloc_urb(udc_device, udc_device->bus->endpoint_array);
	} else {
		DWC_WARN("udc_enable: ep0_urb already allocated %p\n", ep0_urb);
	}

}

/**
 * udc_startup - allow udc code to do any additional startup
 */
void udc_startup_events(struct usb_device_instance *device)
{
	/* The DEVICE_INIT event puts the USB device in the state STATE_INIT. */
	usbd_device_event_irq(device, DEVICE_INIT, 0);

	/*
	* The DEVICE_CREATE event puts the USB device in the state
	* STATE_ATTACHED.
	*/
	usbd_device_event_irq(device, DEVICE_CREATE, 0);

	/*
	* Some USB controller driver implementations signal
	* DEVICE_HUB_CONFIGURED and DEVICE_RESET events here.
	* DEVICE_HUB_CONFIGURED causes a transition to the state STATE_POWERED,
	* and DEVICE_RESET causes a transition to the state STATE_DEFAULT.
	* The DW USB client controller has the capability to detect when the
	* USB cable is connected to a powered USB bus, so we will defer the
	* DEVICE_HUB_CONFIGURED and DEVICE_RESET events until later.
	*/

	udc_enable(device);
}

/*
 * UDC interrupts
 */
void udc_irq(void)
{
	uint32 IntFlag;

	IntFlag=OtgReg->Core.gintsts & OtgReg->Core.gintmsk;
	if (IntFlag == 0)
		return;

	if(IntFlag & (1<<4))       //receive FIFO non-enpty
	{
		OtgReg->Core.gintmsk &= ~(1<<4);
		RxFifoNonEmpty();
		OtgReg->Core.gintmsk |= 1<<4;
	}
	if(IntFlag & (1<<5))    //xfer FIFO enpty
	{
		OtgReg->Core.gintmsk &= ~(1<<5);
		OtgReg->Device.dtknqr4_fifoemptymsk=0;
	}
	if(IntFlag & (1<<10))       //early suspend
	{
		OtgReg->Core.gintsts=1<<10;
	}
	if(IntFlag & (1<<11))       //suspend
	{
//		usbd_device_event_irq(udc_device, DEVICE_BUS_INACTIVE, 0);
		OtgReg->Core.gintsts=1<<11;
	}
	if(IntFlag & (1<<12))  //USB总线复位
	{
		DWC_PRINT("device attached and powered\n");
		BusReset();
		OtgReg->Core.gintsts=1<<12;
		udc_state_transition(udc_device->device_state, STATE_POWERED);
	}
	if(IntFlag & (1<<13))  //Enumeration done
	{	
		dwc_otg_enum_done_intr();
		OtgReg->Core.gintsts=1<<13;
		udc_state_transition(udc_device->device_state, STATE_DEFAULT);
	}
	if(IntFlag & (1<<18))       //IN中断
	{
		dwc_otg_in_intr();
		//InIntr();
	}
	if(IntFlag & (1<<19))       //OUT中断
	{
		dwc_otg_out_intr();
		//OutIntr();
	}
    
	if(IntFlag & (1<<30))  //USB VBUS中断
	{
		OtgReg->Core.gintsts=1<<30;
	}
	if(IntFlag & (1ul<<31))     //resume
	{
//		usbd_device_event_irq(udc_device, DEVICE_BUS_ACTIVITY, 0);
		OtgReg->Core.gintsts=1ul<<31;
	}
	if(IntFlag & ((1<<22)|(1<<6)|(1<<7)|(1<<17)))
	{
		OtgReg->Core.gintsts=IntFlag & ((1<<22)|(1<<6)|(1<<7)|(1<<17));
	}
}

