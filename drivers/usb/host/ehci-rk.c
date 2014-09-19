/*
 * (C) Copyright 2014 Kever Yang , Fuzhou Rockchip Electronic Ltd.Co
 *
 */

#include <common.h>
#include <usb.h>

#include "ehci.h"
#include <asm/arch/gpio.h>

#define RK32_EHCI_BASE (0xff500000)
/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	// TODO: POWER enable for ehci hcd 1.2V, external 5V power
	gpio_direction_output(GPIO_BANK0 | GPIO_B6, 1); //gpio0_B6  output high
	
	// TODO: enable clock
	
	*hccr = (struct ehci_hccr *)(RK32_EHCI_BASE);
	*hcor = (struct ehci_hcor *)(RK32_EHCI_BASE + 0x10);
	
	printf("ehci_hcd_init,complete\n");
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
