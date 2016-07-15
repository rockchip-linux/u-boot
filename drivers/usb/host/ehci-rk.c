/*
 * (C) Copyright 2014 Kever Yang, Fuzhou Rockchip Electronic Ltd.Co
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <usb.h>
#include <asm/arch/rkplat.h>
#include "ehci.h"

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	if (init == USB_INIT_DEVICE)
		return -ENODEV;
	
	*hccr = (struct ehci_hccr *)(rkusb_active_hcd->regbase);
	*hcor = (struct ehci_hcor *)(rkusb_active_hcd->regbase + 0x10);

	printf("ehci_hcd_init index %d,complete\n", index);
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
