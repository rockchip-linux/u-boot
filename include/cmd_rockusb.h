/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Kever Yang 2014.03.31
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef ROCKUSB_H
#define ROCKUSB_H

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <usbdevice.h>
#include <linux/sizes.h>
#include <asm/unaligned.h>
#include <usb/udc.h>
#include <usb_defs.h>

#if defined(CONFIG_RK_UDC)
#include <usb/dwc_otg_udc.h>
#endif
#ifdef CONFIG_RK_DWC3_UDC
#include <usb/dwc3_rk_udc.h>
#endif


/*******************************************************************
固件升级命令集
*******************************************************************/
#define	K_FW_TEST_UNIT_READY		0x00
#define	K_FW_READ_FLASH_ID		0x01
#define	K_FW_SET_DEVICE_ID		0x02
#define	K_FW_TEST_BAD_BLOCK		0x03
#define	K_FW_READ_10			0x04
#define	K_FW_WRITE_10			0x05
#define	K_FW_ERASE_10			0x06
#define	K_FW_WRITE_SPARE		0x07
#define	K_FW_READ_SPARE			0x08

#define	K_FW_ERASE_10_FORCE		0x0b
#define	K_FW_GET_VERSION		0x0c

#define	K_FW_LBA_READ_10		0x14
#define	K_FW_LBA_WRITE_10		0x15
#define	K_FW_ERASE_SYS_DISK		0x16
#define	K_FW_SDRAM_READ_10		0x17
#define	K_FW_SDRAM_WRITE_10		0x18
#define	K_FW_SDRAM_EXECUTE		0x19
#define	K_FW_READ_FLASH_INFO		0x1A
#define	K_FW_GET_CHIP_VER		0x1B
#define	K_FW_LOW_FORMAT			0x1C
#define	K_FW_SET_RESET_FLAG		0x1E
#define	K_FW_SPI_READ_10		0x21
#define	K_FW_SPI_WRITE_10		0x22

#define	K_FW_SESSION			0X30
#define	K_FW_RESET			0xff
/* Bulk-only data structures */


#define RKUSB_ERR
#undef  RKUSB_WARN
#undef  RKUSB_INFO
#undef  RKUSB_DEBUG

#ifdef RKUSB_DEBUG
#define RKUSBDBG(fmt, args...)\
	printf("DEBUG: [%s]: %d:\n"fmt, __func__, __LINE__, ##args)
#else
	#define RKUSBDBG(fmt, args...) do {} while (0)
#endif

#ifdef RKUSB_INFO
#define RKUSBINFO(fmt, args...)\
	printf("INFO: [%s]: "fmt, __func__, ##args)
#else
	#define RKUSBINFO(fmt, args...) do {} while (0)
#endif

#ifdef RKUSB_WARN
#define RKUSBWARN(fmt, args...)\
	printf("WARNING: [%s]: "fmt, __func__, ##args)
#else
	#define RKUSBWARN(fmt, args...) do {} while (0)
#endif

#ifdef RKUSB_ERR
#define RKUSBERR(fmt, args...)\
	printf("ERROR: [%s]: "fmt, __func__, ##args)
#else
	#define RKUSBERR(fmt, args...) do {} while (0)
#endif

/*
 * dwc otg controller can handle max 0x20000 bytes data for the XferSize in DoEpSiz
 * is 18 bit length, we cut the transfer into smaller pieces
 * block size = 0x200/0x210
 */
#define RKUSB_BUFFER_BLOCK_MAX 0x80

#define	USB_DEVICE_CLASS_VENDOR_SPECIFIC	0xFF
#define	USB_SUBCLASS_CODE_SCSI			0x06

#define STR_LANG		0x00
#define STR_MANUFACTURER	0x01
#define STR_PRODUCT		0x02
#define STR_SERIAL		0x03
#define STR_CONFIGURATION	0x04
#define STR_INTERFACE		0x05
#define STR_COUNT		0x06

#define CONFIG_USBD_CONFIGURATION_STR	"Rockchip RockUsb Configuration"
#define CONFIG_USBD_INTERFACE_STR	"Rockchip RockUsb Interface"

#define RKUSB_BCD_DEVICE	0x0100
#define	RKUSB_MAXPOWER		200

#define USB_FLUSH_DELAY_MICROSECS 1000

#define	NUM_CONFIGS	1
#define	NUM_INTERFACES	1
#define	NUM_ENDPOINTS	2
#define RKUSB_ENDPOINT_BULKIN 1
#ifdef CONFIG_RK_DWC3_UDC
	#define RKUSB_ENDPOINT_BULKOUT 1
#else
	#define RKUSB_ENDPOINT_BULKOUT 2
#endif
#define	RX_EP_INDEX	2
#define	TX_EP_INDEX	1

#define FW_WR_MODE_PBA       0
#define FW_WR_MODE_LBA       1
#define FW_WR_MODE_SDRAM     2
#define FW_WR_MODE_SPI       3
#define FW_WR_MODE_SESSION   4

#define SYS_LOADER_ERR_FLAG      0X1888AAFF

struct _rkusb_config_desc {
	struct usb_configuration_descriptor configuration_desc;
	struct usb_interface_descriptor interface_desc;
	struct usb_endpoint_descriptor endpoint_desc[NUM_ENDPOINTS];
};
static struct cmd_rockusb_interface usbcmd __attribute__((aligned(ARCH_DMA_MINALIGN)));



/* USB Descriptor Strings */
static char serial_number[] = "123456789abcdef"; /* what should be the length ?, 33 ? */
static __attribute__ ((aligned(4))) u8 wstr_lang[4] = {4, USB_DT_STRING, 0x9, 0x4};
static __attribute__ ((aligned(4))) u8 wstr_manufacturer[2 + 2*(sizeof(CONFIG_USBD_MANUFACTURER)-1)];
static __attribute__ ((aligned(4))) u8 wstr_product[2 + 2*(sizeof(CONFIG_USBD_PRODUCT_NAME)-1)];
static __attribute__ ((aligned(4))) u8 wstr_serial[2 + 2*(sizeof(serial_number) - 1)];
static __attribute__ ((aligned(4))) u8 wstr_configuration[2 + 2*(sizeof(CONFIG_USBD_CONFIGURATION_STR)-1)];
static __attribute__ ((aligned(4))) u8 wstr_interface[2 + 2*(sizeof(CONFIG_USBD_INTERFACE_STR)-1)];


/* defined and used by gadget/ep0.c */
extern struct usb_string_descriptor **usb_strings;

/* USB descriptors */
static struct usb_device_descriptor device_descriptor = {
    .bLength = sizeof(struct usb_device_descriptor),
    .bDescriptorType =  USB_DT_DEVICE,
    .bcdUSB =       cpu_to_le16(0x0201),
    .bDeviceClass =     0x00,
    .bDeviceSubClass =  0x00,
    .bDeviceProtocol =  0x00,
    .bMaxPacketSize0 =  EP0_MAX_PACKET_SIZE,
    .idVendor =     cpu_to_le16(CONFIG_USBD_VENDORID),
    .idProduct =        cpu_to_le16(CONFIG_USBD_PRODUCTID_ROCKUSB),
    .bcdDevice =        cpu_to_le16(RKUSB_BCD_DEVICE),
    .iManufacturer =    0,
    .iProduct =     0,
    .iSerialNumber =    0,
    .bNumConfigurations =   NUM_CONFIGS
};

static struct _rkusb_config_desc rkusb_config_desc = {
	.configuration_desc = {
		.bLength = sizeof(struct usb_configuration_descriptor),
		.bDescriptorType = USB_DT_CONFIG,
		.wTotalLength = cpu_to_le16(sizeof(struct _rkusb_config_desc)),
		.bNumInterfaces = NUM_INTERFACES,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = BMATTRIBUTE_RESERVED,
		.bMaxPower = RKUSB_MAXPOWER,
    },
	.interface_desc = {
		.bLength  = sizeof(struct usb_interface_descriptor),
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = NUM_ENDPOINTS,
		.bInterfaceClass = USB_DEVICE_CLASS_VENDOR_SPECIFIC,
		.bInterfaceSubClass = USB_SUBCLASS_CODE_SCSI,
		.bInterfaceProtocol = 0x05,
		.iInterface = 0,
    },
	.endpoint_desc = {
		{
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			/* XXX: can't the address start from 0x1, currently
			    seeing problem with "epinfo" */
			.bEndpointAddress = RKUSB_ENDPOINT_BULKOUT | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = 0x200,
			.bInterval = 0x00,
		},
		{
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			/* XXX: can't the address start from 0x1, currently
			    seeing problem with "epinfo" */
			.bEndpointAddress = RKUSB_ENDPOINT_BULKIN | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = 0x200,
			.bInterval = 0x00,
		},
	},
};
#ifdef CONFIG_RK_UDC
	static struct usb_bos_descriptor rkusb_bos_desc = {
		.bLength = (sizeof (struct usb_bos_descriptor) - 3),
		.bDescriptorType = USB_DESCRIPTOR_TYPE_BOS,
		.wTotalLength = sizeof (struct usb_bos_descriptor),
		.bNumDeviceCaps = 0x01,
		.bCapHeaderLength = (sizeof (struct usb_bos_descriptor) - 5),
		.bCapabilityType = USB_DT_DEVICE_CAPABILITY,
		.bDevCapabilityType = 0
	};
#endif


static struct usb_string_descriptor *rkusb_string_table[STR_COUNT];
#ifdef CONFIG_RK_UDC
	static char rockusb_name[] = "rockchip_rockusb";
	static struct usb_interface_descriptor interface_descriptors[NUM_INTERFACES];
	static struct usb_endpoint_descriptor *ep_descriptor_ptrs[NUM_ENDPOINTS];
	static struct usb_device_instance device_instance[1];
	static struct usb_bus_instance bus_instance[1];
	static struct usb_configuration_instance config_instance[NUM_CONFIGS];
	static struct usb_interface_instance interface_instance[NUM_INTERFACES];
	static struct usb_alternate_instance alternate_instance[NUM_INTERFACES];
	static struct usb_endpoint_instance endpoint_instance[NUM_ENDPOINTS + 1];
#endif
/* USB specific */

/* Command Block Wrapper */
struct fsg_bulk_cb_wrap {
	__le32	Signature;		/* Contains 'USBC' */
	u32	Tag;			/* Unique per command id */
	__le32	DataTransferLength;	/* Size of the data */
	u8	Flags;			/* Direction in bit 7 */
	u8	Lun;			/* LUN (normally 0) */
	u8	Length;			/* Of the CDB, <= MAX_COMMAND_SIZE */
	u8	CDB[16];		/* Command Data Block */
};

#define USB_BULK_CB_WRAP_LEN	31
#define USB_BULK_CB_SIG		0x43425355	/* Spells out USBC */
#define USB_BULK_IN_FLAG	0x80

/* Command Status Wrapper */
struct bulk_cs_wrap {
	__le32	Signature;		/* Should = 'USBS' */
	u32	Tag;			/* Same as original command */
	__le32	Residue;		/* Amount not transferred */
	u8	Status;			/* See below */
};

#define USB_BULK_CS_WRAP_LEN	13
#define USB_BULK_CS_SIG		0x53425355	/* Spells out 'USBS' */
#define USB_STATUS_PASS		0
#define USB_STATUS_FAIL		1
#define USB_STATUS_PHASE_ERROR	2

#define RKUSB_STATUS_IDLE 0
#define RKUSB_STATUS_CMD  1
#define RKUSB_STATUS_RXDATA 2
#define RKUSB_STATUS_TXDATA 3
#define RKUSB_STATUS_CSW 4
#define RKUSB_STATUS_RXDATA_PREPARE 5
#define RKUSB_STATUS_TXDATA_PREPARE 6
/*******************************************************************
CSW返回状态值
*******************************************************************/
#define	CSW_GOOD		0x00
#define	CSW_FAIL		0x01

struct cmd_rockusb_preread {
	uint8_t *pre_buffer;
	uint32_t pre_lba;
	uint32_t pre_blocks;
};

struct cmd_rockusb_interface {
	uint8_t cmd;
	uint8_t status;
	unsigned int configured;
	uint8_t *rx_buffer[2];
	uint8_t *tx_buffer[2];
	uint8_t rxbuf_num;
	uint8_t txbuf_num;
	/*
	 * Download size, if download has to be done. This can be checked to find
	 * whether next packet is a command or a data
	 */
	uint32_t d_size;

	/* Data downloaded so far */
	uint32_t d_bytes;

	/* Download status, < 0 when error, > 0 when complete */
	uint32_t d_status;

	/* Upload size, if download has to be done */
	uint32_t u_size;

	/* Data uploaded so far */
	uint32_t u_bytes;

	uint32_t lba;
	uint32_t cmnd;
	uint32_t imgwr_mode;
	uint16_t data_size;
	uint32_t data_size_from_cmnd;
	uint32_t tag;
	uint32_t residue;
	uint32_t usb_amount_left;
	uint32_t transfer_size;
	int 	 transfer_status;
	uint32_t receive_size;
	int 	 receive_status;
	uint32_t reset_flag;
#ifdef CONFIG_RK_DWC3_UDC
	struct dwc3_giveback_data rx_giveback;
	struct dwc3_giveback_data tx_giveback;
#endif
	struct bulk_cs_wrap csw __attribute__((aligned(ARCH_DMA_MINALIGN)));
	struct fsg_bulk_cb_wrap cbw __attribute__((aligned(ARCH_DMA_MINALIGN)));
	struct cmd_rockusb_preread pre_read __attribute__((aligned(ARCH_DMA_MINALIGN)));
};

/* Declare functions */
extern void FW_SorageLowFormatEn(int en);

#ifdef CONFIG_RK_UDC
static void rkusb_init_endpoints(void);
#endif
int do_rockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

#endif /* ROCKUSB_H */
