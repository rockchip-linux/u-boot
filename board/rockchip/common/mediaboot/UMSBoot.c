/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2015 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:  
Author:     
Created:    
Modified:
Revision:   1.00
********************************************************************************
********************************************************************************/
#include "../config.h"
#include <usb.h>
#include <asm/arch/usbhost.h>
#include "UMSBoot.h"

struct rkusb_hcd_cfg *rkusb_active_hcd = NULL;

#define UMS_BOOT_PART_SIZE	1024
#define UMS_BOOT_PART_OFFSET	64
#define UMS_FW_PART_OFFSET	8192
#define UMS_SYS_PART_OFFSET	8064

static int usb_stor_curr_dev = -1; /* current device */
static uint32 g_umsboot_mode = 0;
extern unsigned long gIdDataBuf[512];

static struct rkusb_hcd_cfg rkusb_hcd[] = {
#if defined(CONFIG_RKCHIP_RK3288)
#if defined(RKUSB_UMS_BOOT_FROM_HOST1)
	{
		.name = "ehci-host",
		.enable = true,
		.regbase = (void *)RKIO_USBHOST0_EHCI_PHYS,
		.gpio_vbus = GPIO_BANK0 | GPIO_B6,
	},
#elif defined(RKUSB_UMS_BOOT_FROM_HSIC)
	{
		.name = "ehci-hsic",
		.enable = true,
		.regbase = (void *)RKIO_HSIC_PHYS,
	},
#elif defined(RKUSB_UMS_BOOT_FROM_HOST2)
	{
		.name = "dwc2-host",
		.enable = true,
		.regbase = (void *)RKIO_USBHOST1_PHYS,
	},
#elif defined(RKUSB_UMS_BOOT_FROM_OTG)
	{
		.name = "dwc2-otg",
		.enable = true,
		.regbase = (void *)RKIO_USBOTG_PHYS,
	},
#endif
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
#if defined(RKUSB_UMS_BOOT_FROM_HOST1)
	{
		.name = "ehci-host",
		.enable = true,
		.regbase = (void *)RKIO_USBHOST_EHCI_PHYS,
	},
#elif defined(RKUSB_UMS_BOOT_FROM_OTG)
	{
		.name = "dwc2-otg",
		.enable = true,
		.regbase = (void *)RKIO_USBOTG20_PHYS,
	},
#endif
#elif defined(CONFIG_RKCHIP_RK3036)
#if defined(RKUSB_UMS_BOOT_FROM_HOST1)

	{
		.name = "dwc2-host",
		.enable = true,
		.regbase = (void *)RKIO_USBHOST20_PHYS,
	},
#elif defined(RKUSB_UMS_BOOT_FROM_OTG)
	{
		.name = "dwc2-otg",
		.enable = true,
		.regbase = (void *)RKIO_USBOTG20_PHYS,
	},
#endif
#endif
};

inline int rk_usb_host_lookup() {
	int n = ARRAY_SIZE(rkusb_hcd);
	const char *name = NULL;

	printf("%d USB controller selected\n", n);

	if (!n) {
		printf("No USB controller selected\n");
		return -1;
	}

	if (!rkusb_hcd[0].regbase || !rkusb_hcd[0].enable || !rkusb_hcd[0].name) {
		printf("Controller not enabled regbase addr %p, parameter err\n",
		       rkusb_hcd[0].regbase);
		return -1;
	}

	rkusb_active_hcd = &rkusb_hcd[0];
	return 0;
}

/* 
 * Low level USB mass storage ops
 * caller should guarantee parameters valid
 */
static int __UMSReadLBA(uint8 index, uint32 LBA, void *pbuf, uint32 nSec)
{
	unsigned long blk  = LBA;
	unsigned long cnt  = nSec;
	unsigned long n;
	block_dev_desc_t *stor_dev;

	stor_dev = usb_stor_get_dev(index);
	n = stor_dev->block_read(index, blk, cnt, pbuf);

	if (n == cnt)
		return 0;

	return ERROR;
}

static uint32 __UMSWriteLBA(uint8 index, uint32 LBA, void *pbuf, uint32 nSec)
{
	unsigned long blk  = LBA;
	unsigned long cnt  = nSec;
	unsigned long n;
	block_dev_desc_t *stor_dev;

	stor_dev = usb_stor_get_dev(index);
	n = stor_dev->block_write(index, blk, cnt, pbuf);

	return n;
}

static uint32 __UMSGetCapacity(uint8 index)
{
	block_dev_desc_t *stor_dev;

	stor_dev = usb_stor_get_dev(usb_stor_curr_dev);

	return stor_dev->lba;
}
/************************************************************/

uint32 UMSInit(uint32 ChipSel)
{
	uint ret = -1;

	/* Select active USB controller */
	if (rk_usb_host_lookup()) {
		printf("Could not find USB controller\n");
		return ret;
	}

	/* USB hardware init */
	if (rkusb_active_hcd->hw_init)
		rkusb_active_hcd->hw_init();
	/* Enable VBus */
	if (rkusb_active_hcd->gpio_vbus)
		gpio_direction_output(rkusb_active_hcd->gpio_vbus, 1);

	printf("Boot from usb device %d @ %p \n", rkusb_active_hcd->name,
	       rkusb_active_hcd->regbase);

	if (usb_init() >= 0) {
		/* Try to recognize storage devices immediately */
		usb_stor_curr_dev = usb_stor_scan(1);
		if (usb_stor_curr_dev >= 0) {
			__UMSReadLBA(usb_stor_curr_dev, UMS_BOOT_PART_OFFSET, gIdDataBuf, 4);
			if(gIdDataBuf[0] == 0xFCDC8C3B)
			{
				if(0 == gIdDataBuf[128+104/4]) // ums…˝º∂
				{
					g_umsboot_mode = UMS_UPDATE;
					printf("UMS Update.\n");
				}
				else if(1 == gIdDataBuf[128+104/4])// ums‘À––
				{
					g_umsboot_mode = UMS_BOOT;
					printf("UMS Boot.\n");
				}
				ret = 0;
			} else {
				ret = -1;
			}
		}
			
	}

	return ret;
}

uint32 UMSDeInit(uint32 ChipSel)
{
	return 0;
}

uint32 UMSReadPBA(uint8 ChipSel, uint32 PBA, void *pbuf, uint32 nSec)
{
	/* Currently, support one UMS device */
	if ((usb_stor_curr_dev < 0) && (ChipSel != usb_stor_curr_dev))
		return ERROR;

	if (PBA + nSec >= UMS_BOOT_PART_SIZE * 5)
		PBA &= (UMS_BOOT_PART_SIZE - 1);
	PBA = PBA + UMS_BOOT_PART_OFFSET;
        
	if(__UMSReadLBA(usb_stor_curr_dev, PBA, pbuf, nSec) != 0);
		return ERROR;

	return 0;
}

uint32 UMSReadLBA(uint8 ChipSel, uint32 LBA, void *pbuf, uint32 nSec)
{
	/* Currently, support one UMS device */
	if ((usb_stor_curr_dev < 0) && (ChipSel != usb_stor_curr_dev))
		return ERROR;

	LBA += UMS_FW_PART_OFFSET;

	if(__UMSReadLBA(usb_stor_curr_dev, LBA, pbuf, nSec) != 0)
		return ERROR;

	return 0;
}

void UMSReadID(uint8 ChipSel, void *buf)
{
	uint8 * pbuf = buf;

	pbuf[0] = 'U';
	pbuf[1] = 'M';
	pbuf[2] = 'S';
	pbuf[3] = ' ';
}

void UMSReadFlashInfo(void *buf)
{
	;
}

uint32 UMSGetCapacity(uint8 ChipSel)
{
	if (usb_stor_curr_dev >= 0)
		return __UMSGetCapacity(usb_stor_curr_dev);

	return ERROR;
}

uint32 UMSGetBootMode(void)
{
	return g_umsboot_mode;
}
