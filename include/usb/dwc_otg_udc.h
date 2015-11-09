/*
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
