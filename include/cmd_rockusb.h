
#ifndef ROCKUSB_H
#define ROCKUSB_H

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <usbdevice.h>
#include <asm/sizes.h>
#include <asm/unaligned.h>
#include <asm/arch-rk32xx/typedef.h>

#include "../board/rockchip/rk32xx/storage.h"

#include <usb_defs.h>

#if defined(CONFIG_RK_UDC)
#include <usb/dwc_otg_udc.h>
#endif

/*******************************************************************
¹Ì¼þÉý¼¶ÃüÁî¼¯
*******************************************************************/
#define 	K_FW_TEST_UNIT_READY		0x00
#define 	K_FW_READ_FLASH_ID		    0x01
#define 	K_FW_SET_DEVICE_ID		    0x02
#define 	K_FW_TEST_BAD_BLOCK		    0x03
#define 	K_FW_READ_10				0x04
#define 	K_FW_WRITE_10				0x05
#define 	K_FW_ERASE_10				0x06
#define 	K_FW_WRITE_SPARE			0x07
#define 	K_FW_READ_SPARE			    0x08

#define 	K_FW_ERASE_10_FORCE		    0x0b
#define 	K_FW_GET_VERSION			0x0c

#define 	K_FW_LBA_READ_10            0x14
#define 	K_FW_LBA_WRITE_10           0x15
#define 	K_FW_ERASE_SYS_DISK         0x16
#define 	K_FW_SDRAM_READ_10          0x17
#define 	K_FW_SDRAM_WRITE_10         0x18
#define 	K_FW_SDRAM_EXECUTE          0x19
#define 	K_FW_READ_FLASH_INFO	    0x1A
#define     K_FW_GET_CHIP_VER           0x1B
#define     K_FW_LOW_FORMAT             0x1C
#define     K_FW_SET_RESET_FLAG         0x1E
#define     K_FW_SPI_READ_10            0x21  
#define     K_FW_SPI_WRITE_10           0x22  

#define     K_FW_SESSION				0X30 // ADD BY HSL.
#define 	K_FW_RESET				    0xff
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

#define 		USB_DEVICE_CLASS_VENDOR_SPECIFIC    		    0xFF	//
#define		    USB_SUBCLASS_CODE_SCSI					    0x06

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
#define RKUSB_ENDPOINT_BULKOUT 2
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
static struct cmd_rockusb_interface usbcmd;

static void rkusb_handle_response(void);
static void rkusb_init_endpoints(void);

static u8 rockusb_name[] = "rockchip_rockusb";
/* defined and used by gadget/ep0.c */
extern struct usb_string_descriptor **usb_strings;
static char serial_number[]="123456789abcdef"; /* what should be the length ?, 33 ? */
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
    .idProduct =        cpu_to_le16(0x320A),
    .bcdDevice =        cpu_to_le16(RKUSB_BCD_DEVICE),
    .iManufacturer =    0,//STR_MANUFACTURER,
    .iProduct =     0,//STR_PRODUCT,
    .iSerialNumber =    0,//STR_SERIAL,
    .bNumConfigurations =   NUM_CONFIGS
};

static struct _rkusb_config_desc rkusb_config_desc = {
    .configuration_desc = {
        .bLength = sizeof(struct usb_configuration_descriptor),
        .bDescriptorType = USB_DT_CONFIG,
        .wTotalLength = cpu_to_le16(sizeof(struct _rkusb_config_desc)),
        .bNumInterfaces = NUM_INTERFACES,
        .bConfigurationValue = 1,
        .iConfiguration = 0,//STR_CONFIGURATION,
        .bmAttributes = BMATTRIBUTE_RESERVED,//BMATTRIBUTE_SELF_POWERED | 
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
        .iInterface = 0,//STR_INTERFACE,
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
static struct usb_interface_descriptor interface_descriptors[NUM_INTERFACES];
static struct usb_endpoint_descriptor *ep_descriptor_ptrs[NUM_ENDPOINTS];

static struct usb_device_instance device_instance[1];
static struct usb_bus_instance bus_instance[1];
static struct usb_configuration_instance config_instance[NUM_CONFIGS];
static struct usb_interface_instance interface_instance[NUM_INTERFACES];
static struct usb_alternate_instance alternate_instance[NUM_INTERFACES];
static struct usb_endpoint_instance endpoint_instance[NUM_ENDPOINTS + 1];
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
/*******************************************************************
CSW·µ»Ø×´Ì¬Öµ
*******************************************************************/
#define	    CSW_GOOD					0x00		//ÃüÁîÍ¨¹ý
#define	    CSW_FAIL					0x01		//ÃüÁîÊ§°Ü


struct cmd_rockusb_interface {
   uint8_t cmd;
   uint8_t status;
	unsigned int configured;
   uint8_t *buffer;
   
	
	uint16_t			data_size;
	uint32_t			data_size_from_cmnd;
	uint32_t			lba;
	uint32_t			tag;
	uint32_t			residue;
	uint32_t			usb_amount_left;
	
	struct fsg_bulk_cb_wrap cbw;
	struct bulk_cs_wrap csw;
};

#ifndef __GNUC__
#define PACKED1 __packed
#define PACKED2
#define ALIGN(x) __align(x)
#else
#define PACKED1
#define PACKED2 __attribute__((packed))
#define ALIGN(x) __attribute__ ((aligned(x)))
#endif

#undef	EXT
#ifdef	IN_FW_Upgrade
		#define	EXT
#else
		#define	EXT		extern
#endif		
    
#define USB_XFER_BUF_SIZE (2048*512/4) //1MB
#if 1
//	EXT		ALIGN(4) uint8_t 	FWCmdPhase;			//ÃüÁî½×¶Î×´Ì¬×Ö
	extern   int  FWLowFormatEn;
	EXT     ALIGN(4) uint8_t  FWSetResetFlag;
	EXT		uint32_t 			FW_DataLenCnt;
    EXT		uint32_t 			FW_SDRAM_ExcuteAddr;    
//	EXT		uint32_t 			FW_Write10PBA;
//    EXT		int32_t           dCSWDataResidueVal;
    
//	EXT		ALIGN(4) uint16_t FWLBA_DataLenCnt;
//	EXT		uint32_t 			FWLBA_Write10PBA;
//    EXT		uint32_t 			FW_SDRAM_Parameter;
    EXT     uint32_t          FW_WR_Mode;
    EXT     uint32_t          FW_IMG_WR_Mode;//img Ð´»¹ÊÇlbaÐ´£¬0Îªimg£¬1Îªlba
#endif
//    EXT     USB_XFER        usbCmd;
//    EXT     uint32_t 			*bulkBuf[2];

//	EXT		ALIGN(64)CSW  	      gCSW;
//    EXT		ALIGN(64)CBW           gCBW;
//    EXT		ALIGN(64) uint8_t          BulkInBuf[512];

//    EXT    ALIGN(64) uint32_t          FWLBAWriteSrcBuf[512*32/4];
//    EXT    ALIGN(64) uint32_t          FWLBAReadSrcBuf[512*32/4];

    //rkloader.c
    EXT       ALIGN(64)uint32_t          DataBuf[528*128/4];

    // sdmmcboot.c
    EXT		ALIGN(64)uint32_t          Data[(1024*8*4/4)];
    EXT		ALIGN(64)uint32_t          SpareBuf[(32*8*4/4)];
    EXT     ALIGN( 64 ) uint32_t 			usbXferBuf[2*USB_XFER_BUF_SIZE];
#endif
