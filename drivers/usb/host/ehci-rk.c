/*
 * (C) Copyright 2014 Kever Yang , Fuzhou Rockchip Electronic Ltd.Co
 *
 */

#include <common.h>
#include <usb.h>

#include "ehci.h"
#include <asm/arch/gpio.h>
#include <asm/arch/usbhost.h>

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	if (!rkusb_active_hcd->enable)
		return -ENODEV;

	if (init == USB_INIT_DEVICE)
		return -ENODEV;

	/* USB hardware init */
	if (rkusb_active_hcd->hw_init)
		rkusb_active_hcd->hw_init();
	/* TODO: Power enable for ehci hcd 1.2V, external 5V power */
	/* Enable VBus */
	if (rkusb_active_hcd->gpio_vbus)
		gpio_direction_output(rkusb_active_hcd->gpio_vbus, 1);
	
	*hccr = (struct ehci_hccr *)(rkusb_active_hcd->regbase);
	*hcor = (struct ehci_hcor *)(rkusb_active_hcd->regbase + 0x10);

	printf("%d index %d,complete\n", index);
	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int ehci_hcd_stop(int index)
{
	printf("ehci_hcd_stop,complete\n");
	return 0;
}
