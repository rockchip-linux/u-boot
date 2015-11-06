/*
 * (C) Copyright 2009
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
 *
 * (C) Copyright 2008-2014 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __DWC_UDC_H
#define __DWC_UDC_H

 
#define  EP0_MAX_PACKET_SIZE     64
/*
 * Function declarations
 */

void udc_irq(void);

void udc_set_nak(int epid);
void udc_unset_nak(int epid);
int udc_endpoint_write(struct usb_endpoint_instance *endpoint);
int udc_init(void);
void udc_enable(struct usb_device_instance *device);
void udc_disable(void);
void udc_connect(void);
void udc_disconnect(void);
void udc_startup_events(struct usb_device_instance *device);
void resume_usb(struct usb_endpoint_instance *endpoint, int max_size);
int is_usbd_high_speed(void);

uint32_t GetVbus(void);
uint8_t UsbConnectStatus(void);
void suspend_usb(void);
int dwc_otg_check_dpdm(void);

#endif /* __DWC_UDC_H */
