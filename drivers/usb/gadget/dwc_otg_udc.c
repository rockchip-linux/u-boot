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

#include <usbdevice.h>
#include <usb/dwc_otg_udc.h>
#include <asm/arch/rkplat.h>
#include "ep0.h"
#include "dwc_otg_regs.h"


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


#ifndef RKIO_USBOTG_BASE
	#error "PLS config usb otg base!"
#endif /* RKIO_USBOTG_BASE */


#define	BULK_IN_EP			0x01
#define	BULK_OUT_EP			0x02
#define	EP0_TX_FIFO_SIZE		64
#define	EP0_RX_FIFO_SIZE		64
#define	EP0_PACKET_SIZE_FS		8
#define	EP0_PACKET_SIZE_HS		64
#define EP0_BUF_MAXSIZE			64

#define	EP1_TX_FIFO_SIZE		64
#define	EP1_RX_FIFO_SIZE		64
#define	EP1_PACKET_SIZE			64

#define	FS_BULK_RX_SIZE			64
#define	FS_BULK_TX_SIZE			64
#define	HS_BULK_RX_SIZE			512
#define	HS_BULK_TX_SIZE			512
#define	USB_RECIPIENT            	(uint8_t)0x1F
#define	USB_RECIPIENT_DEVICE     	(uint8_t)0x00
#define	USB_RECIPIENT_INTERFACE  	(uint8_t)0x01
#define	USB_RECIPIENT_ENDPOINT   	(uint8_t)0x02

#define	USB_REQUEST_TYPE_MASK    	(uint8_t)0x60
#define	USB_STANDARD_REQUEST     	(uint8_t)0x00
#define	USB_CLASS_REQUEST        	(uint8_t)0x20
#define	USB_VENDOR_REQUEST       	(uint8_t)0x40

#define	USB_REQUEST_MASK         	(uint8_t)0x0F
#define	DEVICE_ADDRESS_MASK      	0x7F


//厂商请求代码
#define	SETUP_DMA_REQUEST		0x0471
#define	GET_FIRMWARE_VERSION		0x0472
#define	GET_SET_TWAIN_REQUEST		0x0473
#define	GET_BUFFER_SIZE			0x0474

#define UDC_INIT_MDELAY			80	/* Device settle delay */
#define FBT_BULK_IN_EP			2
#define FBT_BULK_OUT_EP			1
#define FBT_USB_XFER_MAX_SIZE		(0x80*512)

#define	STAGE_IDLE			0
#define	STAGE_DATA       		1
#define	STAGE_STATUS        		2


/* Global variable */
//带数据的设备请求结构
typedef struct _control_xfer {
	DEVICE_REQUEST_T DeviceRequest;
	uint16_t 	wLength;
	uint16_t 	wCount;
	uint8_t 	*pData;
} CONTROL_XFER;


static struct urb 		*ep0_urb;
static struct usb_device_instance *udc_device;

static CONTROL_XFER		ControlData;
static volatile uint8_t		*Ep0Buf;

static volatile uint8_t		UsbConnected;
static volatile uint8_t		UsbBusReset;
static volatile uint8_t		ControlStage;
static volatile uint8_t		Ep0PktSize;
static volatile uint16_t	BulkEpSize;


static void udc_stall_ep(uint32_t ep_num);
static void dwc_otg_epn_rx(uint32_t len);
static void dwc_otg_epn_tx(struct usb_endpoint_instance *endpoint);

extern void rkplat_uart2UsbEn(uint32 en);
/* For secure boot from boot.c */
extern uint32_t SecureBootLock;
extern uint32_t SecureBootLock_backup;

extern uint32 FW_StorageGetValid(void);

/**************************************************************************
读取端点数据
***************************************************************************/
static void ReadEndpoint0(uint16_t len, void *buf)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

//	debug("%s: buf = 0x%p, len = %d\n", __func__, buf, len);
	invalidate_dcache_range((unsigned long)buf, (unsigned long)buf + ((len + ARCH_DMA_MINALIGN - 1) & ~(ARCH_DMA_MINALIGN - 1)));
	OtgReg->Device.OutEp[0].DoEpDma = (uint32_t)(unsigned long)buf;
	OtgReg->Device.OutEp[0].DoEpTSiz = Ep0PktSize | (1<<29) | (1<<19);
	/* Active ep, Clr Nak, endpoint enable */
	OtgReg->Device.OutEp[0].DoEpCtl = (1<<15) | (1<<26) | (1<<31);
}


/**************************************************************************
写端点
***************************************************************************/
static void WriteEndpoint0(uint16_t len, void *buf)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;
	const uint32 gnptxsts = OtgReg->Core.gnptxsts;

//	debug("%s: buf = 0x%p, len = %d\n", __func__, buf, len);
	flush_dcache_range((unsigned long)buf, (unsigned long)buf + len);
	if (((gnptxsts & 0xffff) >= (len+3)/4) && (((gnptxsts >> 16) & 0xff) > 0))
	{
		OtgReg->Device.InEp[0].DiEpTSiz = len | (1<<19);
		OtgReg->Device.InEp[0].DiEpDma = (uint32_t)(unsigned long)buf;
		/* Ep enable & clear NAK */
		OtgReg->Device.InEp[0].DiEpCtl = (1<<26) | (1<<31);
	}
}

/**************************************************************************
读取端点数据
***************************************************************************/
static void ReadBulkEndpoint(uint32_t len, void *buf)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;
	uint32_t regBak;

//	debug("%s: buf = 0x%p, len = %d\n", __func__, buf, len);
	invalidate_dcache_range((unsigned long)buf, (unsigned long)buf + ((len + ARCH_DMA_MINALIGN - 1) & ~(ARCH_DMA_MINALIGN - 1)));
	OtgReg->Device.OutEp[BULK_OUT_EP].DoEpDma = (uint32_t)(unsigned long)buf;
	// OtgReg->Device.OutEp[BULK_OUT_EP].DoEpTSiz = BulkEpSize | (1<<19);
	regBak = 0x20000 | (((len+BulkEpSize-1)/BulkEpSize)<<19);
	OtgReg->Device.OutEp[BULK_OUT_EP].DoEpTSiz = regBak;
	regBak = OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl;
	regBak = (regBak&0xFFFFF800) | (1ul<<15) | (1ul<<19) | (1ul<<26) | (1ul<<31) | BulkEpSize;
	OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl = regBak;//Active ep, Clr Nak, endpoint enable
}

/**************************************************************************
写端点
***************************************************************************/
static void WriteBulkEndpoint(uint32_t len, void* buf)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;
	uint32_t regBak;

//	debug("%s: buf = 0x%p, len = %d\n", __func__, buf, len);
	flush_dcache_range((unsigned long)buf, (unsigned long)buf + len);
	//if ((OtgReg->Device.InEp[BULK_IN_EP].DTXFSTS & 0xffff) >= (len+3)/4)
	{
		OtgReg->Device.InEp[BULK_IN_EP].DiEpTSiz = len | (((len+BulkEpSize-1)/BulkEpSize)<<19);
		OtgReg->Device.InEp[BULK_IN_EP].DiEpDma = (uint32_t)(unsigned long)buf;
		regBak = ((OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl & (1<<16))==0)?(1<<28):(1<<29);
		regBak |= (1<<15)|(2<<18)|(BULK_IN_EP<<22)|BulkEpSize; //endpoint enable
		regBak |= (1ul<<26)|(1ul<<31);
		OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl = regBak;
	}
}

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

static void ControlInPacket(void)
{
	uint16_t length = ControlData.wLength;

	if(STAGE_DATA == ControlStage)
	{
		if (length > Ep0PktSize)
			length = Ep0PktSize;

		WriteEndpoint0(length, (void *)Ep0Buf);
		ControlData.pData += length;
		ControlData.wLength -= length;
		if (ControlData.wLength == 0)
			ControlStage = STAGE_STATUS;
	}
}


uint32_t GetVbus(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;
	uint32_t vbus = 1;

#if defined(CONFIG_RKCHIP_RK3288)
	if (grf_readl(GRF_UOC0_CON2) & (0x01 << 2)) {
		/* exit suspend */
		grf_writel(((0x01 << 2) << 16), GRF_UOC0_CON2);
		/* delay more than 1ms, waiting for usb phy init */
		mdelay(3);
	}
#elif defined(CONFIG_RKCHIP_RK3036)
	if (grf_readl(GRF_UOC0_CON5) & (0x01 << 0)) {
		/* exit suspend */
		grf_writel(((0x01 << 0) << 16), GRF_UOC0_CON5);
		/* delay more than 1ms, waiting for usb phy init */
		mdelay(3);
	}
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128) \
		|| defined(CONFIG_RKCHIP_RK3368) || defined(CONFIG_RKCHIP_RK3366)
	if (grf_readl(GRF_UOC0_CON0) & (0x01 << 0)) {
		/* exit suspend */
		grf_writel(((0x1 << 0) << 16), GRF_UOC0_CON0);
		/* delay more than 1ms, waiting for usb phy init */
		mdelay(3);
	}
#elif defined(CONFIG_RKCHIP_RK322X)
	if (grf_readl(GRF_USBPHY0_CON0) & (0x01 << 1)) {
		/* exit suspend */
		grf_writel(((0x1 << 1) << 16), GRF_USBPHY0_CON0);
		/* delay more than 1ms, waiting for usb phy init */
		mdelay(3);
	}
#elif defined(CONFIG_RKCHIP_RK322XH)
	if (readl(RKIO_USB2PHY_GRF_PHYS + USB2PHY_GRF_CON(0)) & (0x01 << 1)) {
		/* exit suspend */
		writel(((0x1 << 1) << 16), RKIO_USB2PHY_GRF_PHYS + USB2PHY_GRF_CON(0));
		/* delay more than 1ms, waiting for usb phy init */
		mdelay(3);
	}
#else
	#error "PLS config chiptype for usb vbus check!"
#endif

	vbus = (OtgReg->Core.gotgctl >> 19) & 0x01;
	if (vbus == 0) {
		mdelay(1);
		vbus = (OtgReg->Core.gotgctl >> 19) & 0x01;
	}

	return (vbus);     //vbus状态
}


uint8_t UsbConnectStatus(void)
{
	return UsbConnected;
}


void UdcInit(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;
	uint32_t count;

	UsbConnected = 0;
	UsbBusReset = 0;

	/* Wait AHB Master idle */
	for (count = 0; count < 10000; count++)
	{
		if ((OtgReg->Core.grstctl & (1<<31)) != 0)
			break;
	}
	/* Core soft reset */
	OtgReg->Core.grstctl |= 1 << 0;
	for (count=0; count<10000; count++)
	{
		if ((OtgReg->Core.grstctl & (1<<0)) == 0)
			break;
	}

	OtgReg->ClkGate.PCGCR=0x00;             //Restart the Phy Clock
	OtgReg->Device.dcfg &= ~0x03;                   //Enable HS
#ifdef FORCE_FS    
	OtgReg->Device.dcfg |= 0x01;                    //Force FS
#endif    
	OtgReg->Device.dcfg &= ~0x07f0;                 //reset device addr
	OtgReg->Core.grstctl |= (0x10<<6) | (1<<5);     //Flush all Txfifo
	for (count=0; count<10000; count++)
	{
		if ((OtgReg->Core.grstctl & (1<<5)) == 0)
			break;
	}
	OtgReg->Core.grstctl |= 1<<4;                     //Flush all Rxfifo
	for (count=0; count<10000; count++)
	{
		if ((OtgReg->Core.grstctl & (1<<4)) == 0)
			break;
	}
	OtgReg->Core.grstctl |= 1<<3;                     //Flush IN token lenarning queue

	OtgReg->Core.grxfsiz = 0x00000210;
	OtgReg->Core.gnptxfsiz = 0x00100210;
	OtgReg->Core.dptxfsiz_dieptxf[0] = 0x01000220;
	OtgReg->Core.dptxfsiz_dieptxf[1] = 0x00100320;

	OtgReg->Device.InEp[0].DiEpCtl = (1<<27)|(1<<30);        //IN0 SetNAK & endpoint disable
	OtgReg->Device.InEp[0].DiEpTSiz = 0;
	OtgReg->Device.InEp[0].DiEpDma = 0;
	OtgReg->Device.InEp[0].DiEpInt = 0xff;

	OtgReg->Device.OutEp[0].DoEpCtl = (1<<27)|(1<<30);        //OUT0 SetNAK & endpoint disable
	OtgReg->Device.OutEp[0].DoEpTSiz = 0;
	OtgReg->Device.OutEp[0].DoEpDma = 0;
	OtgReg->Device.OutEp[0].DoEpInt = 0xff;

	OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl = 1<<28;
	OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl = (1ul<<31)|(1<<28)|(1<<26)|(2<<22)|(2<<18)|(1<<15)|0x200;
	OtgReg->Device.OutEp[BULK_OUT_EP].DoEpInt = 0xff;

	OtgReg->Device.diepmsk = 0x2f;                   //device IN interrutp mask
	OtgReg->Device.doepmsk = 0x2f;                   //device OUT interrutp mask
	OtgReg->Device.daint = 0xffffffff;               //clear all pending intrrupt
	OtgReg->Device.daintmsk = 0x00010001 | ((1<<BULK_IN_EP) | ((1<<BULK_OUT_EP)<<16));    //device all ep interrtup mask(IN0 & OUT0)
	OtgReg->Core.gintsts = 0xffffffff;
	OtgReg->Core.gotgint = 0xffffffff;
	OtgReg->Core.gintmsk = (1<<4)|/*(1<<5)|*/(1<<10)|(1<<11)|(1<<12)|(1<<13)|(1<<18)|(1<<19)|(1ul<<30)|(1ul<<31);
//	OtgReg->Core.gahbcfg |= 0x01;             //Global interrupt mask
	OtgReg->Core.gahbcfg |= (1<<5)|(7<<1)|1;

	OtgReg->Core.gintmsk &= ~(1<<4);
}

static void ep0in_ack(void)
{
	WriteEndpoint0(0, (void *)NULL);
}

static void set_address(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	OtgReg->Device.dcfg = (OtgReg->Device.dcfg & (~0x07f0)) | (ControlData.DeviceRequest.wValue << 4);  //reset device addr
	ep0in_ack();
}

void HaltBulkEndpoint(uint8 ep_num)
{
	uint32 count;
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	switch(ep_num) {
	case BULK_IN_EP:
		if(OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl & (1<<31)) {
			/* Enable the Global IN NP NAK */
			OtgReg->Device.dctl |= (1<<7);
			/* Wait for the GINTSTS.Global IN NP NAK Effective */
			for (count = 0; count < 10000; count++) {
				if (OtgReg->Core.gintsts & (1<<6))
					break;
				udelay(1);
			}

			/* Disable non-periodic Endpoints */
			OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl |= (1<<30 | 1<<27);
			/* Wait for the DIEPINTn.EPDisabled interrupt */
			for (count=0; count < 10000; count++) {
				if (OtgReg->Device.InEp[BULK_IN_EP].DiEpInt & (1 << 1)) {
					OtgReg->Device.InEp[BULK_IN_EP].DiEpInt |= (1 << 1);
					break;
				}
				udelay(1);
			}
			/* Clear the Global IN NP NAK */
			OtgReg->Device.dctl |= (1<<8);

			//Flush all Tx Fifo
			OtgReg->Core.grstctl |= (0x10<<6) | (1<<5);
			for (count=0; count < 10000; count++) {
				if (!(OtgReg->Core.grstctl & (1<<5)))
					break;
			}
		}
		/* Set DATA0 Toggle */
		printf("SET IN TOGGLE 0 \n");
		OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl |= 1<<28;
		break;

	case BULK_OUT_EP:
		if(OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl & (1<<31)) {
			/* Setting Global OUT NAK */
			OtgReg->Device.dctl |= (1<<9);
			/* Wait for the GINTSTS.GOUTNakEff interrupt*/
			for (count=0; count<10000; count++) {
				if (OtgReg->Core.gintsts & (1<<7))
					break;
				udelay(1);
			}

			/* Disable the required OUT endpoint */
			OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl |= (1<<30 | 1<<27);
			/* Wait for the DOEPINTn.EPDisabled interrupt */
			for (count=0; count<10000; count++) {
				if (OtgReg->Device.OutEp[BULK_OUT_EP].DoEpInt & (1<<1)) {
					OtgReg->Device.OutEp[BULK_OUT_EP].DoEpInt |= (1<<1);
					break;
				}
				udelay(1);
			}

			/* Flush Rxfifo */
			OtgReg->Core.grstctl |= 1<<4;
			for (count=0; count<10000; count++) {
				if (!(OtgReg->Core.grstctl & (1<<4)))
					break;
			}

			/* Clear the Global OUT NAK */
			OtgReg->Device.dctl = (1<<10);
			mdelay(5);
		}

		//Set DATA0 Toggle
		OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl |= 1<<28;
		break;

	default:
		printf("Invalid bulk ep num!\n");
	}
}

void clear_feature(uint8 ep_num)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	ep_num &= 0x0f;
	printf("USB CMD Clear_feature\n");
	if(ep_num == BULK_IN_EP) {
		HaltBulkEndpoint(BULK_IN_EP);
		printf("##0x%08x DiEpCtl \n",
		       OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl);
		printf("##0x%08x DiEpInt \n",
		       OtgReg->Device.InEp[BULK_IN_EP].DiEpInt);
	}
	if(ep_num == BULK_OUT_EP) {
		HaltBulkEndpoint(BULK_OUT_EP);
		printf("##0x%08x DoEpCtl \n",
		       OtgReg->Device.OutEp[BULK_OUT_EP].DoEpCtl);
		printf("##0x%08x DoEpInt \n",
		       OtgReg->Device.OutEp[BULK_OUT_EP].DoEpInt);
		usbd_device_event_irq(udc_device, DEVICE_CLEAR_FEATURE, 0);
	}

	ep0in_ack();
}

/***************************************************************************
返回stall应答
***************************************************************************/
#if 0
static void stall_ep0(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	OtgReg->Device.OutEp[0].DoEpCtl |= 1<<21;  //send OUT0 stall handshack
	OtgReg->Device.InEp[0].DiEpCtl |= 1<<21;   //send IN0 stall handshack
}
#endif

static void set_configuration(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl = (1<<28)|(BULK_IN_EP<<22)|(2<<18)|(1<<15)|0x200;//|(1<<27)
	ep0in_ack();
}


static void dwc_otg_epn_in_ack(void)
{
	usberr("write 0");
	WriteBulkEndpoint(0, NULL);
}


volatile int suspend = 1;
void suspend_usb(void)
{
	suspend = true;
}

void resume_usb(struct usb_endpoint_instance *endpoint, int max_size)
{
	if (suspend) {
		suspend = false;
		if (endpoint && endpoint->rcv_urb) {
			struct urb* urb = endpoint->rcv_urb;

			/* Get available size for next xfer */
			int remaining_space = urb->buffer_length - urb->actual_length;
			if (remaining_space > 0) {
				urb->status = RECV_READY;
				usbdbg("next request:%d\n", remaining_space);
				ReadBulkEndpoint(remaining_space, (void *)urb->buffer);
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

char fixed_csw[13] __attribute__((aligned(ARCH_DMA_MINALIGN))) =
	{'U','S','B','S', 0, 0, 0, 0, 0, 0, 0, 0, 1};

static bool dwc_otg_fix_test_ready(int len, void *buf)
{
	int i;
	bool ret = FW_StorageGetValid();

	if(ret == 0) {//low formating... & received TUR command
		printf("...\n");
		for (i = 4; i < 8; i++)
			fixed_csw[i] = *((char *)buf + i);
		WriteBulkEndpoint(13, &fixed_csw);
		ReadBulkEndpoint(31, buf);
	}

	return ret;
}

static void dwc_otg_setup(struct usb_endpoint_instance *endpoint)
{
	usbdbg("-> Entering device setup\n");
	memcpy((void *)&ep0_urb->device_request, (void *)Ep0Buf, 8);
	memcpy((void *)&ControlData.DeviceRequest, (void *)Ep0Buf, 8);

	/* Try to process setup packet */
	if (ep0_recv_setup(ep0_urb)) {
		/* Not a setup packet, stall next EP0 transaction */
		udc_stall_ep(0);
		//usberr("can't parse setup packet, still waiting for setup\n");
		return;
	}

	/* Check direction */
	if ((ep0_urb->device_request.bmRequestType & USB_REQ_DIRECTION_MASK)
			== USB_REQ_HOST2DEVICE) {
		usbdbg("control write on EP0\n");
		if (le16_to_cpu(ep0_urb->device_request.wLength)) {
			/* Stall this request */
			usbdbg("Stalling unsupported EP0 control write data "
			       "stage.\n");
			udc_stall_ep(0);
		}
		if((ep0_urb->device_request.bmRequestType & USB_REQ_TYPE_MASK)
			== USB_STANDARD_REQUEST)
		{
			switch (ep0_urb->device_request.bRequest & USB_REQUEST_MASK)
			{
				case 1:
					clear_feature(ep0_urb->device_request.wIndex);
					break;
				case 5:
					usbdbg("set address\n");
					set_address();
					udc_state_transition(udc_device->device_state, STATE_ADDRESSED);
					break;
				case 9:
					usbdbg("set configuration\n");
					UsbConnected = 1;
					udc_state_transition(udc_device->device_state,STATE_CONFIGURED);
					set_configuration();
					break;
				default:
					udc_stall_ep(0);
					break;
			}
		}
	} else {
		usbdbg("control read on EP0\n");
		/*
		 * The ep0_recv_setup function has already placed our response
		 * packet data in ep0_urb->buffer and the packet length in
		 * ep0_urb->actual_length.
		 */
		endpoint->tx_urb = ep0_urb;
		endpoint->sent = 0;
		
		usbdbg("urb->buffer %p, buffer_length %d, actual_length %d\n",
			ep0_urb->buffer,ep0_urb->buffer_length, ep0_urb->actual_length);
		memcpy((void *)Ep0Buf, (void *)ep0_urb->buffer, ep0_urb->actual_length);
		
		//WriteEndpoint0(ep0_urb->actual_length, Ep0Buf);
		ControlData.pData = (uint8_t*)&Ep0Buf[0];
		ControlData.wLength = ep0_urb->actual_length;
		ControlStage = STAGE_DATA;
		
		ControlInPacket();
	}
	usbdbg("<- Leaving device setup\n");
}

static void dwc_otg_enum_done_intr(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	BulkEpSize = FS_BULK_TX_SIZE;
	switch ((OtgReg->Device.dsts>>1) & 0x03)
	{
		case 0:         //High speed, PHY clock @30MHz or 60MHz
			BulkEpSize = HS_BULK_TX_SIZE;
		case 1:         //Full speed, PHY clock @30MHz or 60MHz
		case 3:         //Full speed, PHY clock @48MHz
			OtgReg->Device.InEp[0].DiEpCtl &= ~0x03;   //64bytes MPS
			Ep0PktSize = EP0_PACKET_SIZE_HS;
			break;
 		case 2:
		default:
			OtgReg->Device.InEp[0].DiEpCtl |= 0x03;   //8bytes MPS
			Ep0PktSize = EP0_PACKET_SIZE_FS;
			break;
	}
	OtgReg->Device.dctl |= 1<<8;               //clear global IN NAK
	ReadEndpoint0(Ep0PktSize, (void *)Ep0Buf);
	OtgReg->Device.InEp[BULK_IN_EP].DiEpCtl = (1<<28) | (1<<15) | (2<<18) | (BULK_IN_EP<<22);
}

static void dwc_otg_in_intr(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;
	uint32_t i;
	uint32_t ch;
	uint32_t event;

	ch = (OtgReg->Device.daint & OtgReg->Device.daintmsk) & 0xffff;   //鍦≧OM閲屽彧鏈塃P0涓柇
	for (i=0; i<3; i++)
	{
		if ((1<<i) & ch)
		{
			event=OtgReg->Device.diepmsk | ((OtgReg->Device.dtknqr4_fifoemptymsk & 0x01)<<7);    //<<7鏄洜涓簃sk鏄繚鐣欑殑
			event &= OtgReg->Device.InEp[i].DiEpInt;
			if ((event & 0x01) != 0)        //Transfer complete
			{
				OtgReg->Device.InEp[i].DiEpInt = 0x01;
				if(i == 0)
				{
					ControlInPacket();
				}
				else
				{
					struct usb_endpoint_instance *endpoint;
					endpoint = &udc_device->bus->endpoint_array[2];

					dwc_otg_epn_tx(endpoint);
				}
			}
			if ((event & 0x02) != 0)        //Endpoint disable
			{
				OtgReg->Device.InEp[i].DiEpInt = 0x02;
			}
			if ((event & 0x04) != 0)        //AHB Error
			{
				OtgReg->Device.InEp[i].DiEpInt = 0x04;
			}
			if ((event & 0x08) != 0)        //TimeOUT Handshake (non-ISOC IN EPs)
			{
				OtgReg->Device.InEp[i].DiEpInt = 0x08;
			}
			if ((event & 0x20) != 0)        //IN Token Received with EP mismatch
			{
				OtgReg->Device.InEp[i].DiEpInt = 0x20;
			}
			if ((event & 0x80) != 0)        //Transmit FIFO empty
			{
				OtgReg->Device.InEp[i].DiEpInt = 0x10;
			}
			if ((event & 0x100) != 0)       //Buffer Not Available
			{
				OtgReg->Device.InEp[i].DiEpInt = 0x100;
			}
		}
	}
}

static void dwc_otg_out_intr(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;
	uint32_t i;
	uint32_t ch;
	uint32_t event;
	
	ch = (OtgReg->Device.daint & OtgReg->Device.daintmsk) >> 16;   //鍦≧OM閲屽彧鏈塃P0涓柇
	for (i=0; i<3; i++)
	{
		if ((1<<i) & ch)
		{
			event = OtgReg->Device.OutEp[i].DoEpInt & OtgReg->Device.doepmsk;
			if ((event & 0x01) != 0)        //Transfer complete
			{
				OtgReg->Device.OutEp[i].DoEpInt = 0x01;
				if (i == 0)
				{
					uint32_t len;

					len = Ep0PktSize-(OtgReg->Device.OutEp[0].DoEpTSiz&0x7f);
					if (len>0) {
					//	Ep0OutPacket(len);
					//usberr("ep0 out packet receive");
					}
					ReadEndpoint0(Ep0PktSize, (void *)Ep0Buf);
				}
				else
				{
					uint32_t len;

					len = 0x20000-(OtgReg->Device.OutEp[BULK_OUT_EP].DoEpTSiz&0x1ffff);
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
				dwc_otg_setup(udc_device->bus->endpoint_array);
				ReadEndpoint0(Ep0PktSize, (void *)Ep0Buf);
			}
			if ((event & 0x20) != 0)        //StsPhseRcvd
			{
				usbdbg("<< StsPhseRcvd >> \n");
				OtgReg->Device.OutEp[i].DoEpInt=0x20;
			}
		}
	}
}

/* Stall endpoint */
static void udc_stall_ep(uint32_t ep_num)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	OtgReg->Device.OutEp[ep_num].DoEpCtl |= 1<<21;  //send OUT0 stall handshack
	OtgReg->Device.InEp[ep_num].DiEpCtl |= 1<<21;   //send IN0 stall handshack	
}

static void dwc_otg_write_data(struct usb_endpoint_instance *endpoint)
{
	struct urb *urb = endpoint->tx_urb;

	if(urb) {
		uint32_t last;

		usbdbg("urb->buffer %p, buffer_length %d, actual_length %d \n",
			urb->buffer, urb->buffer_length, urb->actual_length);
		last = urb->actual_length;
		//last = MIN(urb->actual_length - endpoint->sent,
		//	    endpoint->tx_packetSize);

		if(last) {
			usbdbg("endpoint->sent %d, tx_packetSize %d, last %d \n",
				endpoint->sent, endpoint->tx_packetSize, last);

			WriteBulkEndpoint(last, (void *)urb->buffer);
		}
		endpoint->last = last;
	}

}
/* Called to start packet transmission. */
int udc_endpoint_write(struct usb_endpoint_instance *endpoint)
{
	//struct urb *urb = endpoint->tx_urb;
	//dwc_otg_epn_tx(endpoint);
	//dwc_otg_write_data(endpoint);
	//WriteBulkEndpoint(urb->actual_length, urb->buffer);
	
	usbdbg("%p %x", endpoint->tx_urb, endpoint->tx_urb->actual_length);
	if (endpoint->tx_urb &&
	    (endpoint->last == endpoint->tx_packetSize) &&
	    (endpoint->tx_urb->actual_length - endpoint->sent -
	     endpoint->last == 0)) {
		/* handle zero length packet here */
		dwc_otg_epn_in_ack();
	}
	endpoint->tx_urb->status = SEND_IN_PROGRESS;
	if (endpoint->tx_urb && endpoint->tx_urb->actual_length) {
			/* write data */
			dwc_otg_write_data(endpoint);

	}
	return 0;
}

static void dwc_otg_epn_rx(uint32_t len)
{
	struct urb *urb;
	struct usb_endpoint_instance *endpoint;
	int remaining_space = 0;

	endpoint = &udc_device->bus->endpoint_array[1];

	if(endpoint) {
		urb = endpoint->rcv_urb;
		remaining_space = FBT_USB_XFER_MAX_SIZE;
		if (urb) {
			//get available size for next xfer.
			remaining_space = urb->buffer_length - urb->actual_length;
			usbdbg("buffer_length:%d, actual_length:%x, len:%x\n", urb->buffer_length, urb->actual_length, len);
			if (!dwc_otg_fix_test_ready(len, (void *)urb->buffer))
				return;
			len = (len <= remaining_space) ? len : remaining_space;
		}
		urb->status = RECV_OK;
		usbd_rcv_complete(endpoint, len, 0);
		remaining_space -= len;
		//usberr("buffer_length:%x, actual_length:%x, len:%x\n", urb->buffer_length, urb->actual_length, len);
		if (1/*remaining_space <= 0*/) {
			//buffer is full, so we not do another xfer here. 
			suspend_usb();
		} else {
			urb->status = RECV_READY;
			//schedule next xfer.
			remaining_space = remaining_space > FBT_USB_XFER_MAX_SIZE? FBT_USB_XFER_MAX_SIZE : remaining_space;
			ReadBulkEndpoint(remaining_space, (void *)urb->buffer);
		}
	}
}

static void dwc_otg_epn_tx(struct usb_endpoint_instance *endpoint)
{
	usbdbg("%p %x", endpoint->tx_urb, endpoint->tx_urb->actual_length);
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
	endpoint->tx_urb->status = SEND_FINISHED_OK;

	if (endpoint->tx_urb && endpoint->tx_urb->actual_length) {
		/* retire the data that was just sent */
		usbd_tx_complete(endpoint);
		/*
		 * Check to see if we have more data ready to transmit
		 * now.
		 */
		if (endpoint->tx_urb && endpoint->tx_urb->actual_length) {
		    usberr("send again");
			//endpoint->tx_urb->status = SEND_IN_PROGRESS;
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
	suspend = true;
	Ep0Buf = memalign(ARCH_DMA_MINALIGN, EP0_BUF_MAXSIZE);
	usbdbg("starting \n");
	
	return 0;
}

int is_usbd_high_speed(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	return ((OtgReg->Device.dsts>>1) & 0x03) ? 0 : 1;
}

void usbphy_tunning(void)
{
#if defined(CONFIG_RKCHIP_RK3126) || \
    defined(CONFIG_RKCHIP_RK3128)
	/* Phy PLL recovering */
	grf_writel(0x00030001, GRF_UOC0_CON0);
	mdelay(10);
	grf_writel(0x00030002, GRF_UOC0_CON0);
#endif
#if defined(CONFIG_RKCHIP_RK3036)
	/* Phy PLL recovering */
	grf_writel(0x00030001, GRF_UOC0_CON5);
	mdelay(10);
	grf_writel(0x00030002, GRF_UOC0_CON5);
#endif
#if defined(CONFIG_RKCHIP_RK3366)
	/* open HS pre-emphasize to increase HS slew rate for each port */
	grf_writel(0xffff851f, GRF_USBPHY_CON0);
	grf_writel(0xffff68c8, GRF_USBPHY_CON7);
	grf_writel(0xffff851f, GRF_USBPHY_CON12);
	grf_writel(0xffff68c8, GRF_USBPHY_CON19);

	/* compensation current tuning reference relate to ODT and etc */
	grf_writel(0xffff026e, GRF_USBPHY_CON3);
#endif
}
/* Turn on the USB connection by enabling the pullup resistor */
void udc_connect(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	/* Disable usb-uart bypass */
	rkplat_uart2UsbEn(0);

	OtgReg->Device.dctl |= 0x02;	//soft disconnect
	usbphy_tunning();
	mdelay(500);
	UdcInit();
	OtgReg->Device.dctl &= ~0x02;	//soft connect

	irq_install_handler(IRQ_USB_OTG, (interrupt_handler_t *)udc_irq, (void *)NULL);
	irq_handler_enable(IRQ_USB_OTG);

	usbdbg("OtgReg->Core.grstctl = 0x%08x\n", OtgReg->Core.grstctl);
	usbdbg("OtgReg->Device.dcfg = 0x%08x\n", OtgReg->Device.dcfg);
	usbdbg("OtgReg->Core.grxfsiz = 0x%08x\n", OtgReg->Core.grxfsiz);
	usbdbg("OtgReg->Core.gnptxfsiz = 0x%08x\n", OtgReg->Core.gnptxfsiz);
	usbdbg("OtgReg->Core.dptxfsiz_dieptxf[0] = 0x%08x\n", OtgReg->Core.dptxfsiz_dieptxf[0] );
	usbdbg("OtgReg->Core.dptxfsiz_dieptxf[1]%08x\n", OtgReg->Core.dptxfsiz_dieptxf[1]);
	usbdbg("OtgReg->Device.InEp[0].DiEpCtl = 0x%08x\n", OtgReg->Device.InEp[0].DiEpCtl);
	usbdbg("OtgReg->Device.InEp[0].DiEpTSiz = 0x%08x\n", OtgReg->Device.InEp[0].DiEpTSiz);
	usbdbg("OtgReg->Device.InEp[0].DiEpDma = 0x%08x\n", OtgReg->Device.InEp[0].DiEpDma);
	usbdbg("OtgReg->Device.InEp[0].DiEpInt = 0x%08x\n", OtgReg->Device.InEp[0].DiEpInt);
	usbdbg("OtgReg->Device.OutEp[0].DoEpCtl = 0x%08x\n", OtgReg->Device.OutEp[0].DoEpCtl);
	usbdbg("OtgReg->Device.OutEp[0].DoEpTSiz = 0x%08x\n", OtgReg->Device.OutEp[0].DoEpTSiz);
	usbdbg("OtgReg->Device.OutEp[0].DoEpDma = 0x%08x\n", OtgReg->Device.OutEp[0].DoEpDma);
	usbdbg("OtgReg->Device.OutEp[0].DoEpInt = 0x%08x\n", OtgReg->Device.OutEp[0].DoEpInt);
	usbdbg("OtgReg->Device.diepmsk = 0x%08x\n", OtgReg->Device.diepmsk);
	usbdbg("OtgReg->Device.doepmsk = 0x%08x\n", OtgReg->Device.doepmsk);
	usbdbg("OtgReg->Core.gahbcfg = 0x%08x\n", OtgReg->Core.gahbcfg);
}

/* Turn off the USB connection by disabling the pullup resistor */
void udc_disconnect(void)
{
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;

	OtgReg->Device.dctl |= 0x02;	//soft disconnect
	UsbConnected = 0;
}

/* Switch on the UDC */
void udc_enable(struct usb_device_instance *device)
{
	usbinfo("enable device %p, status %d, device_state %d\n",
		device, device->status, device->device_state);

	/* Save the device structure pointer */
	udc_device = device;
	usbinfo("enable device %p, status %d, device_state %d\n", udc_device,
		udc_device->status, udc_device->device_state);

	/* Setup ep0 urb */
	if (!ep0_urb) {
		ep0_urb =
			usbd_alloc_urb(udc_device, udc_device->bus->endpoint_array);
	} else {
		usbinfo("udc_enable: ep0_urb already allocated %p\n", ep0_urb);
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
	pUSB_OTG_REG OtgReg = (pUSB_OTG_REG)RKIO_USBOTG_BASE;
	uint32_t IntFlag;

	IntFlag = OtgReg->Core.gintsts & OtgReg->Core.gintmsk;
	if (IntFlag == 0)
		return;

	if(IntFlag & (1<<4))       //receive FIFO non-enpty
	{
		OtgReg->Core.gintmsk &= ~(1<<4);
		OtgReg->Core.gintmsk |= 1<<4;
	}
	if(IntFlag & (1<<5))    //xfer FIFO enpty
	{
		OtgReg->Core.gintmsk &= ~(1<<5);
		OtgReg->Device.dtknqr4_fifoemptymsk = 0;
	}
	if(IntFlag & (1<<10))       //early suspend
	{
		OtgReg->Core.gintsts = 1<<10;
	}
	if(IntFlag & (1<<11))       //suspend
	{
		OtgReg->Core.gintsts = 1<<11;
	}
	if(IntFlag & (1<<12))  //USB reset
	{
		usbinfo("device attached and powered\n");
		
		SecureBootLock = SecureBootLock_backup; //恢复lock
		
		UsbBusReset++;
		OtgReg->Device.dcfg &= ~0x07f0;                 //reset device addr
		ControlStage = STAGE_IDLE;
		OtgReg->Device.dctl &= ~0x01;      //Clear the Remote Wakeup Signalling
	    
		OtgReg->Core.gintsts = 1<<12;
		udc_state_transition(udc_device->device_state, STATE_POWERED);
	}
	if(IntFlag & (1<<13))  //Enumeration done
	{	
		dwc_otg_enum_done_intr();
		OtgReg->Core.gintsts = 1<<13;
		udc_state_transition(udc_device->device_state, STATE_DEFAULT);
	}
	if(IntFlag & (1<<18))       //IN涓柇
	{
		dwc_otg_in_intr();
	}
	if(IntFlag & (1<<19))       //OUT涓柇
	{
		dwc_otg_out_intr();
	}
    
	if(IntFlag & (1<<30))  //USB VBUS涓柇
	{
		OtgReg->Core.gintsts = 1<<30;
	}
	if(IntFlag & (1ul<<31))     //resume
	{
		OtgReg->Core.gintsts = 1ul<<31;
	}
	if(IntFlag & ((1<<22)|(1<<6)|(1<<7)|(1<<17)))
	{
		OtgReg->Core.gintsts = IntFlag & ((1<<22)|(1<<6)|(1<<7)|(1<<17));
	}
}

