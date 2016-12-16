/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Kever Yang 2014.03.31
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#define IN_FW_Upgrade
#include <cmd_rockusb.h>
#include <asm/arch/rkplat.h>

#if defined(CONFIG_RK_UDC)
#include <usb/dwc_otg_udc.h>
#endif
#ifdef CONFIG_RK_DWC3_UDC
#include <dwc3-uboot.h>
#include <usb/dwc3_rk_udc.h>
#endif
#include <../board/rockchip/common/config.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_ROCKUSB_TIMEOUT_CHECK
static uint64_t TimeOutBase;
#endif

#ifdef CONFIG_RK_DWC3_UDC
static void rkusb_init_strings(void);

int rkusb_write_bulk_ep(uint32_t nLen, void *buf)
{
	usbcmd.tx_giveback.status = SEND_IN_PROGRESS;
	usbcmd.tx_giveback.actual = 0;
	usbcmd.tx_giveback.buf = NULL;
	return RK_Dwc3WriteBulkEndpoint(nLen, buf);
}

int rkusb_read_bulk_ep(uint32_t nLen, void *buf)
{
	usbcmd.rx_giveback.status = RECV_READY;
	usbcmd.rx_giveback.actual = 0;
	usbcmd.rx_giveback.buf = NULL;
	return RK_Dwc3ReadBulkEndpoint(nLen, buf);
}

void rkusb_event_handler_for_dwc3(int nEvent)
{
	int event = nEvent & 0xffff;
	int param = nEvent>>16;
	RKUSBINFO("@rkusb_event_handler_for_dwc3   %d\n", event);
	switch (event) {
	case 1:/* Device_Reset */
	case 5:/* Device_Disconnect */
		usbcmd.configured = 0;
		break;
	case 2:/* DEVICE_CONFIGURED */
		usbcmd.configured = 1;
		usbcmd.txbuf_num = 0;
		usbcmd.rxbuf_num = 0;
		usbcmd.tx_giveback.status = SEND_FINISHED_OK;
		usbcmd.tx_giveback.actual = 0;
		rkusb_read_bulk_ep(31, usbcmd.rx_buffer[usbcmd.rxbuf_num]);
		usbcmd.rxbuf_num = (usbcmd.rxbuf_num + 1) % 2;
		break;
	case 3:/* DEVICE_ADDRESS_ASSIGNED */
		usbcmd.status = RKUSB_STATUS_IDLE;
		break;
	case 4:/* DEVICE_CLEAR_FEATURE */
		usbcmd.status = RKUSB_STATUS_IDLE;
		/* in ep */
		if (param == 0) {
			usbcmd.txbuf_num = 0;
			usbcmd.tx_giveback.status = SEND_FINISHED_OK;
			usbcmd.tx_giveback.actual = 0;
		}
		/* out ep */
		if (param == 0x1) {
			usbcmd.rxbuf_num = 0;
			rkusb_read_bulk_ep(31, usbcmd.rx_buffer[usbcmd.rxbuf_num]);
			usbcmd.rxbuf_num = (usbcmd.rxbuf_num + 1) % 2;
		}
		/* in and out ep */
		if (param == 0x10) {
			usbcmd.txbuf_num = 0;
			usbcmd.tx_giveback.status = SEND_FINISHED_OK;
			usbcmd.tx_giveback.actual = 0;
			
			usbcmd.rxbuf_num = 0;
			rkusb_read_bulk_ep(31, usbcmd.rx_buffer[usbcmd.rxbuf_num]);
			usbcmd.rxbuf_num = (usbcmd.rxbuf_num + 1) % 2;
		}
	default:
		break;
	}
}

void dwc3_rx_handler(int status, uint32_t actual, void *buf)
{
	if (status) {
		usbcmd.rx_giveback.status = RECV_ERROR;
	} else {
		usbcmd.rx_giveback.status = RECV_OK;
		usbcmd.rx_giveback.actual = actual;
		usbcmd.rx_giveback.buf = buf;
	}
}

void dwc3_tx_handler(int status, uint32_t actual, void *buf)
{
	if (status) {
		usbcmd.tx_giveback.status = SEND_FINISHED_ERROR;
	} else {
		usbcmd.tx_giveback.status = SEND_FINISHED_OK;
		usbcmd.tx_giveback.actual = actual;
		usbcmd.tx_giveback.buf = buf;
	}
}

void init_dwc3_udc_instance(struct rk_dwc3_udc_instance *instance)
{
	int i;
	memset(instance, 0, sizeof(*instance));
	instance->device_desc = &device_descriptor;
	instance->config_desc = (void *)&rkusb_config_desc.configuration_desc;
	instance->interface_desc = &rkusb_config_desc.interface_desc;

	usbcmd.rx_buffer[0] = (u8 *)gd->arch.rk_boot_buf_addr;
	usbcmd.rx_buffer[1] = usbcmd.rx_buffer[0]+(CONFIG_RK_BOOT_BUFFER_SIZE>>2);

	usbcmd.tx_buffer[0] = (u8 *)gd->arch.rk_boot_buf_addr + (CONFIG_RK_BOOT_BUFFER_SIZE>>1);
	usbcmd.tx_buffer[1] = usbcmd.tx_buffer[0]+(CONFIG_RK_BOOT_BUFFER_SIZE>>2);

	for (i = 0; i < 2; i++) {
		instance->endpoint_desc[i] = &rkusb_config_desc.endpoint_desc[i];
		instance->rx_buffer[i] = usbcmd.rx_buffer[i];
		instance->tx_buffer[i] = usbcmd.tx_buffer[i];
	}
	instance->rx_buffer_size = CONFIG_RK_BOOT_BUFFER_SIZE>>2;
	instance->tx_buffer_size = CONFIG_RK_BOOT_BUFFER_SIZE>>2;
	rkusb_init_strings();
	for (i = 0; i < STR_COUNT; i++)
		instance->string_desc[i] = rkusb_string_table[i];
	instance->device_event = rkusb_event_handler_for_dwc3;
	instance->rx_handler = dwc3_rx_handler;
	instance->tx_handler = dwc3_tx_handler;
}
#endif

#ifdef CONFIG_RK_UDC
/* USB specific */
void rkusb_receive_firstcbw(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[1];
	/* get first CBW */
	ep->rcv_urb->buffer = (u8 *)&usbcmd.cbw;
	ep->rcv_urb->buffer_length = 31;
	ep->rcv_urb->actual_length = 0;
	/* make sure endpoint will be re-enabled */
	suspend_usb();
	resume_usb(ep, 0);
}

static void rkusb_event_handler (struct usb_device_instance *device,
				  usb_device_event_t event, int data)
{
	RKUSBINFO("@rkusb_event_handler   %x\n", event);
	switch (event) {
	case DEVICE_RESET:
	case DEVICE_BUS_INACTIVE:
		usbcmd.configured = 0;
		break;
	case DEVICE_CONFIGURED:
		usbcmd.configured = 1;
		break;
	case DEVICE_ADDRESS_ASSIGNED:
		usbcmd.status = RKUSB_STATUS_IDLE;
		rkusb_init_endpoints();
		rkusb_receive_firstcbw();
	case DEVICE_CLEAR_FEATURE:
		usbcmd.status = RKUSB_STATUS_IDLE;
		endpoint_instance[2].tx_urb->status = SEND_FINISHED_OK;
		endpoint_instance[1].rcv_urb->status = RECV_OK;
		rkusb_receive_firstcbw();
	default:
		break;
	}
}
#endif
/* utility function for converting char* to wide string used by USB */
static void str2wide(char *str, u16 *wide)
{
	int i;
	for (i = 0; i < strlen(str) && str[i]; i++) {
		#if defined(__LITTLE_ENDIAN)
			wide[i] = (u16) str[i];
		#elif defined(__BIG_ENDIAN)
			wide[i] = ((u16)(str[i])<<8);
		#else
			#error "__LITTLE_ENDIAN or __BIG_ENDIAN undefined"
		#endif
	}
}

static void rkusb_init_strings(void)
{
	struct usb_string_descriptor *string;

	rkusb_string_table[STR_LANG] = (struct usb_string_descriptor *)wstr_lang;

	string = (struct usb_string_descriptor *)wstr_manufacturer;
	string->bLength = sizeof(wstr_manufacturer);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(CONFIG_USBD_MANUFACTURER, string->wData);
	rkusb_string_table[STR_MANUFACTURER] = string;

	string = (struct usb_string_descriptor *)wstr_product;
	string->bLength = sizeof(wstr_product);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(CONFIG_USBD_PRODUCT_NAME, string->wData);
	rkusb_string_table[STR_PRODUCT] = string;

	string = (struct usb_string_descriptor *)wstr_serial;
	string->bLength = sizeof(wstr_serial);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(serial_number, string->wData);
	rkusb_string_table[STR_SERIAL] = string;

	string = (struct usb_string_descriptor *)wstr_configuration;
	string->bLength = sizeof(wstr_configuration);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(CONFIG_USBD_CONFIGURATION_STR, string->wData);
	rkusb_string_table[STR_CONFIGURATION] = string;

	string = (struct usb_string_descriptor *)wstr_interface;
	string->bLength = sizeof(wstr_interface);
	string->bDescriptorType = USB_DT_STRING;
	str2wide(CONFIG_USBD_INTERFACE_STR, string->wData);
	rkusb_string_table[STR_INTERFACE] = string;

	/* Now, initialize the string table for ep0 handling */
	usb_strings = rkusb_string_table;
}

/* fastboot_init has to be called before this fn to get correct serial string */
#ifdef CONFIG_RK_UDC
static void rkusb_init_instances(void)
{
	int i;
	/* initialize device instance */
	memset((void *)device_instance, 0, sizeof(struct usb_device_instance));
	device_instance->name = rockusb_name;
	device_instance->device_state = STATE_INIT;
#if defined(CONFIG_RKCHIP_RK3126)
	/* audi-b rockusb product id adjust */
	if (grf_readl(GRF_CHIP_TAG) == 0x3136) {
		device_descriptor.idProduct = cpu_to_le16(0x310D);
	}
#endif
	device_instance->device_descriptor = &device_descriptor;
	device_instance->bos_descriptor = &rkusb_bos_desc;
	device_instance->event = rkusb_event_handler;
	device_instance->cdc_recv_setup = NULL;
	device_instance->bus = bus_instance;
	device_instance->configurations = NUM_CONFIGS;
	device_instance->configuration_instance_array = config_instance;

	/* XXX: what is this bus instance for ?, can't it be removed by moving
	    endpoint_array and serial_number_str is moved to device instance */
	/* initialize bus instance */
	memset(bus_instance, 0, sizeof(struct usb_bus_instance));
	bus_instance->device = device_instance;
	bus_instance->endpoint_array = endpoint_instance;
	/* XXX: what is the relevance of max_endpoints & maxpacketsize ? */
	bus_instance->max_endpoints = 1;
	bus_instance->maxpacketsize = 64;
	bus_instance->serial_number_str = serial_number;

	/* configuration instance */
	memset(config_instance, 0, sizeof(struct usb_configuration_instance));
	config_instance->interfaces = NUM_INTERFACES;
	config_instance->configuration_descriptor =
		(struct usb_configuration_descriptor *)&rkusb_config_desc;
	config_instance->interface_instance_array = interface_instance;

	/* XXX: is alternate instance required in case of no alternate ? */
	/* interface instance */
	memset(interface_instance, 0, sizeof(struct usb_interface_instance));
	interface_instance->alternates = 1;
	interface_instance->alternates_instance_array = alternate_instance;

	/* alternates instance */
	memset(alternate_instance, 0, sizeof(struct usb_alternate_instance));
	alternate_instance->interface_descriptor = interface_descriptors;
	alternate_instance->endpoints = NUM_ENDPOINTS;
	alternate_instance->endpoints_descriptor_array = ep_descriptor_ptrs;

	/* endpoint instances */
	memset(endpoint_instance, 0, sizeof(endpoint_instance));
	endpoint_instance[0].endpoint_address = 0;
	endpoint_instance[0].rcv_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].rcv_attributes = USB_ENDPOINT_XFER_CONTROL;
	endpoint_instance[0].tx_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].tx_attributes = USB_ENDPOINT_XFER_CONTROL;
	/* XXX: following statement to done along with other endpoints
		at another place ? */
#ifdef CONFIG_CMD_FASTBOOT
	usbcmd.rx_buffer[0] = (u8 *)gd->arch.fastboot_buf_addr;
	usbcmd.rx_buffer[1] = usbcmd.rx_buffer[0]+(CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH>>1);

	usbcmd.tx_buffer[0] = (u8 *)gd->arch.fastboot_buf_addr + CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH;
	usbcmd.tx_buffer[1] = usbcmd.tx_buffer[0]+(CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH>>1);
#else
	usbcmd.rx_buffer[0] = (u8 *)gd->arch.rk_boot_buf_addr;
	usbcmd.rx_buffer[1] = usbcmd.rx_buffer[0]+(CONFIG_RK_BOOT_BUFFER_SIZE>>2);

	usbcmd.tx_buffer[0] = (u8 *)gd->arch.rk_boot_buf_addr + (CONFIG_RK_BOOT_BUFFER_SIZE>>1);
	usbcmd.tx_buffer[1] = usbcmd.tx_buffer[0]+(CONFIG_RK_BOOT_BUFFER_SIZE>>2);
#endif
	RKUSBINFO("%p %p %p %p\n",
		usbcmd.rx_buffer[0], usbcmd.rx_buffer[1], usbcmd.tx_buffer[0], usbcmd.tx_buffer[1]);
	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		endpoint_instance[i].endpoint_address =
			ep_descriptor_ptrs[i - 1]->bEndpointAddress;

		endpoint_instance[i].rcv_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].rcv_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].tx_packetSize = 0x201;

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		urb_link_init(&endpoint_instance[i].rcv);
		urb_link_init(&endpoint_instance[i].rdy);
		urb_link_init(&endpoint_instance[i].tx);
		urb_link_init(&endpoint_instance[i].done);
		RKUSBINFO("ENDPOINT %d,addr %x\n", i, endpoint_instance[i].endpoint_address);
		if (endpoint_instance[i].endpoint_address & USB_DIR_IN) {
			endpoint_instance[i].tx_urb =
				usbd_alloc_urb(device_instance,
					       &endpoint_instance[i]);
			endpoint_instance[i].tx_urb->buffer = usbcmd.tx_buffer[0];
			endpoint_instance[i].tx_urb->status = SEND_FINISHED_OK;
		} else {
			endpoint_instance[i].rcv_urb =
				usbd_alloc_urb(device_instance,
					       &endpoint_instance[i]);
			endpoint_instance[i].rcv_urb->buffer = usbcmd.rx_buffer[0];
			endpoint_instance[i].rcv_urb->status = RECV_OK;
		}
	}
}

/* XXX: ep_descriptor_ptrs can be removed by making better use of
	RKUSB_config_desc.endpoint_desc */
static void rkusb_init_endpoint_ptrs(void)
{
	ep_descriptor_ptrs[0] = &rkusb_config_desc.endpoint_desc[0];
	ep_descriptor_ptrs[1] = &rkusb_config_desc.endpoint_desc[1];
}

static void rkusb_init_endpoints(void)
{
	int i;

	/* XXX: should it be moved to some other function ? */
	bus_instance->max_endpoints = NUM_ENDPOINTS + 1;

	/* XXX: is this for loop required ?, yes for MUSB it is */
	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		/* configure packetsize based on HS negotiation status */
		if (is_usbd_high_speed()) {
			RKUSBINFO("setting up HS USB device ep%x\n", endpoint_instance[i].endpoint_address);
			ep_descriptor_ptrs[i - 1]->wMaxPacketSize = 0x200;
		} else {
			RKUSBINFO("setting up FS USB device ep%x\n", endpoint_instance[i].endpoint_address);
			ep_descriptor_ptrs[i - 1]->wMaxPacketSize = 0x40;
		}
		/* fastboot will send a zero packet if the last data packet is tx_packetSize, but rockusb don't */
		endpoint_instance[i].tx_packetSize = 0x201;
		endpoint_instance[i].rcv_packetSize = le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);
	}
}
#endif

/***************************************************************************
 * 命令:测试准备0x00
 ***************************************************************************/
static void FW_TestUnitReady(void)
{
	if (FW_StorageGetValid() == 0) {
		uint32_t totleBlock = FW_GetTotleBlk();
		uint32_t currEraseBlk = FW_GetCurEraseBlock();
		usbcmd.csw.Residue = cpu_to_be32((totleBlock<<16)|currEraseBlk);
		usbcmd.csw.Status = CSW_FAIL;
	} else if (usbcmd.cbw.CDB[1] == 0xFD) {
		uint32_t totleBlock = FW_GetTotleBlk();
		uint32_t currEraseBlk = 0;
		usbcmd.csw.Residue = cpu_to_be32((totleBlock<<16)|currEraseBlk);
		usbcmd.csw.Status = CSW_FAIL;
		FW_SorageLowFormatEn(1);
	} else if (usbcmd.cbw.CDB[1] == 0xFA) {
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_GOOD;
		usbcmd.reset_flag = 0x10;
	} else if (usbcmd.cbw.CDB[1] == 0xF9) {
		usbcmd.csw.Residue = cpu_to_be32(StorageGetCapacity());
		usbcmd.csw.Status = CSW_GOOD;
	} else if (SecureBootLock) {
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_FAIL;
	} else {
		usbcmd.csw.Residue = cpu_to_be32(6);
		usbcmd.csw.Status = CSW_GOOD;
	}
	usbcmd.status = RKUSB_STATUS_CSW;
}

/***************************************************************************
函数描述:固件升级命令:读FLASH ID
入口参数:无
出口参数:无
调用函数:无
***************************************************************************/
static void FW_ReadID(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	RKUSBINFO("%s \n", __func__);
	StorageReadId(usbcmd.tx_buffer[usbcmd.txbuf_num]);
	usbcmd.transfer_size = 5;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
#else
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;

	RKUSBINFO("%s \n", __func__);

	current_urb = ep->tx_urb;
	if (!current_urb) {
		RKUSBERR("%s: current_urb NULL", __func__);
		return;
	}

	StorageReadId((void *)current_urb->buffer);
	current_urb->actual_length = 5;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
#endif
}

/***************************************************************************
函数描述:固件升级命令:设置FLASH 类型
	Flash 1:
	0:8bit small page;	1:8bit large page 4cyc;		2:8bit large page 5cyc
	3:16bit small page;	4:16bit large page 4cyc;	5:16bit large page 5cyc
	6:MLC 8bit large page 5cyc		7:MLC 8bit large page 5cyc, 4KB/page
入口参数:
出口参数:
调用函数:
 1	2009-4-10 ：增加出错返回，PC工具检测到擦除系统盘出错会擦除所有保留块
 后面的块，重启后loader会低格，重新分配空间
***************************************************************************/
static void FW_LowFormatSysDisk(void)
{
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_CSW;
}

/***************************************************************************
函数描述:获取中间件版权信息
入口参数:无
出口参数:无
调用函数:无
***************************************************************************/
static void FW_GetChipVer(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	unsigned int chip_info[4];
	RKUSBINFO("%s \n", __func__);

	memset(chip_info, 0, sizeof(chip_info));
	rk_get_bootrom_chip_version(chip_info);

#if defined(CONFIG_RKCHIP_RK3399)
	chip_info[0] = 0x33333043;
#elif defined(CONFIG_RKCHIP_RK3366)
	chip_info[0] = 0x33333042;
#endif
	memcpy((uint8_t *)usbcmd.tx_buffer[usbcmd.txbuf_num], (uint8_t *)chip_info, 16);
	usbcmd.transfer_size = 16;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
#else
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	unsigned int chip_info[4];

	RKUSBINFO("%s \n", __func__);

	current_urb = ep->tx_urb;
	if (!current_urb) {
		RKUSBERR("%s: current_urb NULL", __func__);
		return;
	}
	/* notice here chip version should the same as rk tools *.ini config of RKBOOT */
	current_urb->buffer[0] = 0;
	memset(chip_info, 0, sizeof(chip_info));
	rk_get_bootrom_chip_version(chip_info);

#if defined(CONFIG_RKCHIP_RK3036)
	chip_info[0] = 0x33303341;
#elif defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	chip_info[0] = 0x33313241;
#elif defined(CONFIG_RKCHIP_RK322X)
	chip_info[0] = 0x33323241;
#elif defined(CONFIG_RKCHIP_RK3366)
	chip_info[0] = 0x33333042;
#elif defined(CONFIG_RKCHIP_RK322XH)
	chip_info[0] = 0x33323248;
#endif
	memcpy((void *)current_urb->buffer, (void *)chip_info, 16);
	current_urb->actual_length = 16;

	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
#endif
}

/***************************************************************************
函数描述:测试坏块——0:好块; 1:坏块
入口参数:命令块中的指定物理块地址
出口参数:无
调用函数:无
***************************************************************************/
static void FW_TestBadBlock(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	RKUSBINFO("%s \n", __func__);
	memset((uint8_t *)usbcmd.tx_buffer[usbcmd.txbuf_num], 0, 64);
	usbcmd.transfer_size = 64;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
#else
	uint16_t i;
	uint32_t TestResult[16];
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;

	RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;
	for (i = 0; i < 16; i++)
		TestResult[i] = 0;
	memcpy((void *)current_urb->buffer, (void *)TestResult, 64);
	current_urb->actual_length = 64;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
#endif
}

/***************************************************************************
函数描述:按528 PAGE大小读
入口参数:命令块中的物理行地址及传输扇区数
出口参数:无
调用函数:无
***************************************************************************/
static void FW_Read10(void)
{
	usbcmd.u_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 528;
	usbcmd.u_bytes = 0;
	usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);
	usbcmd.status = RKUSB_STATUS_TXDATA_PREPARE;
}

/***************************************************************************
函数描述:按528 PAGE大小写
入口参数:命令块中的物理行地址及传输扇区数
出口参数:无
调用函数:无
***************************************************************************/
static void FW_Write10(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	uint32_t rxdata_blocks = 0;

	usbcmd.d_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 528;
	usbcmd.d_bytes = 0;
	usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);

	RKUSBINFO("WRITE10 %x len %x\n", usbcmd.lba, usbcmd.d_size);

	/* check current lba buffer not include in pre lba buffer */
	rxdata_blocks = usbcmd.d_size/528;
	if (!(((usbcmd.lba + rxdata_blocks) < usbcmd.pre_read.pre_lba)
			|| (usbcmd.lba > (usbcmd.pre_read.pre_lba + usbcmd.pre_read.pre_blocks)))) {
		RKUSBINFO("FW_Write10: invalid pre read\n");
		usbcmd.pre_read.pre_blocks = 0;
		usbcmd.pre_read.pre_lba = 0;
	}
	usbcmd.rx_giveback.actual = 0;
	usbcmd.status = RKUSB_STATUS_RXDATA_PREPARE;
#else
	struct usb_endpoint_instance *ep = &endpoint_instance[1];
	struct urb *current_urb = ep->rcv_urb;
	uint32_t rxdata_blocks = 0;

	usbcmd.d_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 528;
	usbcmd.d_bytes = 0;
	usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);

	RKUSBINFO("WRITE10 %x len %x\n", usbcmd.lba, usbcmd.d_size);
	current_urb->actual_length = 0;

	/* check current lba buffer not include in pre lba buffer */
	rxdata_blocks = usbcmd.d_size/528;
	if (!(((usbcmd.lba + rxdata_blocks) < usbcmd.pre_read.pre_lba)
			|| (usbcmd.lba > (usbcmd.pre_read.pre_lba + usbcmd.pre_read.pre_blocks)))) {
		RKUSBINFO("FW_Write10: invalid pre read\n");
		usbcmd.pre_read.pre_blocks = 0;
		usbcmd.pre_read.pre_lba = 0;
	}

	usbcmd.status = RKUSB_STATUS_RXDATA_PREPARE;
#endif
}


/***************************************************************************
函数描述:按物理BLOCK擦除——0:好块; 1:坏块
入口参数:命令块中的物理块地址
出口参数:无
调用函数:无
***************************************************************************/
static void FW_Erase10(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	RKUSBINFO("%s \n", __func__);
	int status = 0;
	if (SecureBootLock == 0) {
		status = StorageEraseBlock(get_unaligned_be32(&usbcmd.cbw.CDB[2]), get_unaligned_be16(&usbcmd.cbw.CDB[7]), 0);
	}
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = (status == 0) ? 0 : 1;
	usbcmd.status = RKUSB_STATUS_CSW;
#else
	bool status = 0;
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;

	RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;

	if (SecureBootLock == 0) {
		StorageEraseBlock(get_unaligned_be32(&usbcmd.cbw.CDB[2]), get_unaligned_be16(&usbcmd.cbw.CDB[7]), 0);
	}
	current_urb->actual_length = 13;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = status;
	usbcmd.status = RKUSB_STATUS_CSW;
#endif
}

/***************************************************************************
函数描述:按物理BLOCK擦除——0:好块; 1:坏块
入口参数:命令块中的物理块地址
出口参数:无
调用函数:无
***************************************************************************/
static void FW_Erase10Force(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	RKUSBINFO("%s \n", __func__);
	int status = 0;
	if (SecureBootLock == 0) {
		status = StorageEraseBlock(get_unaligned_be32(&usbcmd.cbw.CDB[2]), get_unaligned_be16(&usbcmd.cbw.CDB[7]), 1);
	}
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = (status == 0) ? 0 : 1;
	usbcmd.status = RKUSB_STATUS_CSW;
#else
	bool status = 0;
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;
	if (SecureBootLock == 0) {
		StorageEraseBlock(get_unaligned_be32(&usbcmd.cbw.CDB[2]), get_unaligned_be16(&usbcmd.cbw.CDB[7]), 1);
	}
	current_urb->actual_length = 13;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = status;
	usbcmd.status = RKUSB_STATUS_CSW;
#endif
}

static void FW_LBARead10(void)
{
	usbcmd.u_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 512;
	usbcmd.u_bytes = 0;
	usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);
	usbcmd.status = RKUSB_STATUS_TXDATA_PREPARE;
	RKUSBINFO("LBA_READ %x len %x\n", usbcmd.lba, usbcmd.u_size);
}

static void FW_LBAWrite10(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	uint32_t rxdata_blocks = 0;

	usbcmd.d_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 512;
	usbcmd.d_bytes = 0;
	usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);
	usbcmd.imgwr_mode = usbcmd.cbw.CDB[1];
	RKUSBINFO("LBA_WRITE %x len %x\n", usbcmd.lba, usbcmd.d_size);
	/* check current lba buffer not include in pre lba buffer */
	rxdata_blocks = usbcmd.d_size/512;
	if (!(((usbcmd.lba + rxdata_blocks) < usbcmd.pre_read.pre_lba)
			|| (usbcmd.lba > (usbcmd.pre_read.pre_lba + usbcmd.pre_read.pre_blocks)))) {
		RKUSBINFO("FW_LBAWrite10: invalid pre read\n");
		usbcmd.pre_read.pre_blocks = 0;
		usbcmd.pre_read.pre_lba = 0;
	}
	usbcmd.rx_giveback.actual = 0;
	usbcmd.status = RKUSB_STATUS_RXDATA_PREPARE;
#else
	struct usb_endpoint_instance *ep = &endpoint_instance[1];
	struct urb *current_urb = NULL;
	uint32_t rxdata_blocks = 0;

	usbcmd.d_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 512;
	usbcmd.d_bytes = 0;
	usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);
	usbcmd.imgwr_mode = usbcmd.cbw.CDB[1];
	current_urb = ep->rcv_urb;
	current_urb->actual_length = 0;
	/* check current lba buffer not include in pre lba buffer */
	rxdata_blocks = usbcmd.d_size/512;
	if (!(((usbcmd.lba + rxdata_blocks) < usbcmd.pre_read.pre_lba)
			|| (usbcmd.lba > (usbcmd.pre_read.pre_lba + usbcmd.pre_read.pre_blocks)))) {
		RKUSBINFO("FW_LBAWrite10: invalid pre read\n");
		usbcmd.pre_read.pre_blocks = 0;
		usbcmd.pre_read.pre_lba = 0;
	}

	usbcmd.status = RKUSB_STATUS_RXDATA_PREPARE;
#endif
}

static void FW_GetFlashInfo(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	RKUSBINFO("%s \n", __func__);

	StorageReadFlashInfo(usbcmd.tx_buffer[usbcmd.txbuf_num]);

	usbcmd.transfer_size = 11;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
#else
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;

	RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;

	StorageReadFlashInfo((void *)current_urb->buffer);

	memcpy((void *)current_urb->buffer, (void *)current_urb->buffer, 11);
	current_urb->actual_length = 11;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
#endif
}

/***************************************************************************
函数描述:按物理BLOCK擦除——0:好块; 1:坏块
入口参数:命令块中的物理块地址
出口参数:无
调用函数:无
***************************************************************************/
static void FW_LowFormat(void)
{
	RKUSBINFO("%s \n", __func__);
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_CSW;

	FW_SorageLowFormatEn(1);
}

static void FW_SetResetFlag(void)
{
	RKUSBINFO("%s \n", __func__);
	usbcmd.reset_flag = 1;

	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_CSW;
}

/***************************************************************************
函数描述:系统复位
入口参数:无
出口参数:无
调用函数:无
LOG:
 20100209,HSL@RK,ADD LUN for deffirent reboot.
 0: normal  reboot, 1: loader reboot.
***************************************************************************/
static void FW_Reset(void)
{
	RKUSBINFO("%x \n", usbcmd.cbw.CDB[1]);

	if (usbcmd.cbw.CDB[1])
		usbcmd.reset_flag = usbcmd.cbw.CDB[1];
	else
		usbcmd.reset_flag = 0xff;

	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_CSW;
}

static int rkusb_send_csw(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	struct bulk_cs_wrap	*csw = &usbcmd.csw;
	RKUSBINFO("%s tag %x\n", __func__, usbcmd.cbw.Tag);

	if (usbcmd.tx_giveback.status != SEND_FINISHED_OK)
		return -EBUSY;
	csw->Signature = cpu_to_le32(USB_BULK_CS_SIG);
	csw->Tag = usbcmd.cbw.Tag;
	memcpy(usbcmd.tx_buffer[usbcmd.txbuf_num], (u8 *)&usbcmd.csw, 13);
	rkusb_write_bulk_ep(13, usbcmd.tx_buffer[usbcmd.txbuf_num]);
	usbcmd.txbuf_num = (usbcmd.txbuf_num + 1) % 2;
	rkusb_read_bulk_ep(31, usbcmd.rx_buffer[usbcmd.rxbuf_num]);
	usbcmd.rxbuf_num = (usbcmd.rxbuf_num + 1) % 2;

	usbcmd.status = RKUSB_STATUS_IDLE;

	return 0;
#else
	struct usb_endpoint_instance *ep1 = &endpoint_instance[2];
	struct usb_endpoint_instance *ep2 = &endpoint_instance[1];
	struct urb *current_urb = ep1->tx_urb;
	struct bulk_cs_wrap	*csw = &usbcmd.csw;

	RKUSBINFO("%s tag %x\n", __func__, usbcmd.cbw.Tag);
	if (!current_urb) {
		RKUSBERR("%s: current_urb NULL", __func__);
		return -1;
	}
	while (current_urb->status != SEND_FINISHED_OK)
		udelay(10);

	csw->Signature = cpu_to_le32(USB_BULK_CS_SIG);
	csw->Tag = usbcmd.cbw.Tag;
	current_urb->actual_length = 13;
	current_urb->buffer = (u8 *)&usbcmd.csw;
	ep2->rcv_urb->buffer = (u8 *)&usbcmd.cbw;
	ep2->rcv_urb->buffer_length = 31;
	ep2->rcv_urb->actual_length = 0;
	resume_usb(ep2, 0);

	/* wait for last tx complete */
	udc_endpoint_write(ep1);

	usbcmd.status = RKUSB_STATUS_IDLE;

	return 0;
#endif
}

void do_rockusb_cmd(void)
{
#ifdef CONFIG_RK_UDC
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = ep->tx_urb;
	current_urb->buffer = usbcmd.tx_buffer[usbcmd.txbuf_num&1];
	usbcmd.cmnd = usbcmd.cbw.CDB[0];
	RKUSBINFO("CBW %x %x %x\n", usbcmd.cbw.Tag, usbcmd.cbw.Flags, usbcmd.cbw.Length);
#else
	memcpy(&usbcmd.cbw, usbcmd.rx_giveback.buf, sizeof(usbcmd.cbw));
	usbcmd.cmnd = usbcmd.cbw.CDB[0];
	RKUSBINFO("CBW %x %x %x %x\n", usbcmd.cbw.Tag, usbcmd.cbw.Flags, usbcmd.cbw.Length, usbcmd.cmnd);
#endif
	switch (usbcmd.cmnd) {
	case K_FW_TEST_UNIT_READY:
		FW_TestUnitReady();
		break;
	case K_FW_READ_FLASH_ID:
		FW_ReadID();
		break;
	case K_FW_TEST_BAD_BLOCK:
		FW_TestBadBlock();
		break;
	case K_FW_READ_10:
		FW_Read10();
		break;
	case K_FW_WRITE_10:
		FW_Write10();
		break;
	case K_FW_ERASE_10:
		FW_Erase10();
		break;
	case K_FW_ERASE_10_FORCE:
		FW_Erase10Force();
		break;
	case K_FW_LBA_READ_10:
		FW_LBARead10();
		break;
	case K_FW_LBA_WRITE_10:
		FW_LBAWrite10();
		break;
	case K_FW_ERASE_SYS_DISK:
		FW_LowFormatSysDisk();
		break;
	case K_FW_READ_FLASH_INFO:
		FW_GetFlashInfo();
		break;
	case K_FW_GET_CHIP_VER:
		FW_GetChipVer();
		break;
	case K_FW_LOW_FORMAT:
		FW_LowFormat();
		break;
	case K_FW_SET_RESET_FLAG:
		FW_SetResetFlag();
		break;
	case K_FW_RESET:
		FW_Reset();
		break;
	default:
		break;
	}
}

void rkusb_handle_datarx(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	uint8_t *rxdata_buf = NULL;
	uint32_t rxdata_blocks = 0;
	uint32_t transfer_length;
	uint32_t rx_blocks = 0;
	uint32_t block_length ;
	int iRet;

	if (usbcmd.cmnd == K_FW_WRITE_10)
		block_length = 528;
	else if (usbcmd.cmnd == K_FW_LBA_WRITE_10)
		block_length = 512;
	else
		block_length = 512;
	if (usbcmd.rx_giveback.actual) {
		usbcmd.d_bytes += usbcmd.rx_giveback.actual;
		rxdata_buf = (uint8_t *)usbcmd.rx_giveback.buf;
		rxdata_blocks = usbcmd.rx_giveback.actual / block_length;
    }

	/* read next packet from usb */
	if (usbcmd.d_bytes < usbcmd.d_size) {
		transfer_length = usbcmd.d_size - usbcmd.d_bytes;
		rx_blocks = transfer_length / block_length;
		if (rx_blocks > RKUSB_BUFFER_BLOCK_MAX)
			rx_blocks = RKUSB_BUFFER_BLOCK_MAX;
		transfer_length = rx_blocks * block_length;

		rkusb_read_bulk_ep(transfer_length, usbcmd.rx_buffer[usbcmd.rxbuf_num]);
		usbcmd.rxbuf_num = (usbcmd.rxbuf_num + 1) % 2;
		usbcmd.status = RKUSB_STATUS_RXDATA;
	} else {
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_GOOD;
		rkusb_send_csw();
	}

	/* Write to media */
	if (rxdata_blocks) {
		RKUSBINFO("write to media %x, lba %x, buf %p\n", rxdata_blocks, usbcmd.lba, rxdata_buf);
		if (usbcmd.cmnd == K_FW_WRITE_10) {
			ISetLoaderFlag(SYS_LOADER_ERR_FLAG);
			if (SecureBootLock == 0) {
				iRet = StorageWritePba(usbcmd.lba, rxdata_buf, rxdata_blocks);
				usbcmd.lba += rxdata_blocks;
				if (iRet != FTL_OK) {
					RKUSBERR("StorageWritePba failed,err=%d\n", iRet);
				}
			}
		} else if (usbcmd.cmnd == K_FW_LBA_WRITE_10) {
			if (usbcmd.lba >= 0xFFFFFF00) {
				StorageVendorSysDataStore(usbcmd.lba - 0xFFFFFF00, rxdata_blocks, (uint32 *)rxdata_buf);
				usbcmd.lba += rxdata_blocks;
			} else if (usbcmd.lba == 0xFFFFF000) {
				SecureBootUnlock(rxdata_buf);
			} else if ((usbcmd.lba & 0xFFFF0000) == 0xFFF00000) {
				iRet = vendor_storage_write(*(uint16 *)rxdata_buf, rxdata_buf + 8,
					       		    *(uint16 *)(rxdata_buf + 4));
				if (iRet == -1) {
					RKUSBERR("vendor_storage_write failed,err=%d\n", iRet);
				}
				usbcmd.lba += rxdata_blocks;
			} else if (SecureBootLock == 0) {
				iRet = StorageWriteLba(usbcmd.lba, rxdata_buf, rxdata_blocks, usbcmd.imgwr_mode);
				if (iRet != FTL_OK) {
					RKUSBERR("StorageWriteLba failed,err=%d\n", iRet);
				}
				usbcmd.lba += rxdata_blocks;
			}
		}
	}
#else
	struct usb_endpoint_instance *ep = &endpoint_instance[1];
	struct urb *current_urb = ep->rcv_urb;
	uint8_t *rxdata_buf = NULL;
	uint32_t rxdata_blocks = 0;
	uint32_t transfer_length;
	uint32_t rx_blocks = 0;
	uint32_t block_length ;

	if (usbcmd.cmnd == K_FW_WRITE_10)
		block_length = 528;
	else if (usbcmd.cmnd == K_FW_LBA_WRITE_10)
		block_length = 512;
	else
		block_length = 512;

	/* updatea varilables */
	if (current_urb->actual_length) {
		usbcmd.d_bytes += current_urb->actual_length;
		rxdata_buf = (uint8_t *)current_urb->buffer;
		rxdata_blocks = current_urb->actual_length / block_length;

		current_urb->actual_length = 0;
		usbcmd.rxbuf_num++;
		current_urb->buffer = usbcmd.rx_buffer[usbcmd.rxbuf_num & 1];
    }

	/* read next packet from usb */
	if (usbcmd.d_bytes < usbcmd.d_size) {
		transfer_length = usbcmd.d_size - usbcmd.d_bytes;
		rx_blocks = transfer_length / block_length;
		if (rx_blocks > RKUSB_BUFFER_BLOCK_MAX)
			rx_blocks = RKUSB_BUFFER_BLOCK_MAX;
		transfer_length = rx_blocks * block_length;

		current_urb->buffer_length = transfer_length;
		resume_usb(ep, 0);
		usbcmd.status = RKUSB_STATUS_RXDATA;
	} else {
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_GOOD;
		rkusb_send_csw();
	}

	/* Write to media */
	if (rxdata_blocks) {
		RKUSBINFO("write to media %x, lba %x, buf %p\n", rxdata_blocks, usbcmd.lba, rxdata_buf);
		if (usbcmd.cmnd == K_FW_WRITE_10) {
			ISetLoaderFlag(SYS_LOADER_ERR_FLAG);
			if (SecureBootLock == 0) {
				StorageWritePba(usbcmd.lba, rxdata_buf, rxdata_blocks);
				usbcmd.lba += rxdata_blocks;
			}
		} else if (usbcmd.cmnd == K_FW_LBA_WRITE_10) {
			if (usbcmd.lba >= 0xFFFFFF00) {
				StorageVendorSysDataStore(usbcmd.lba - 0xFFFFFF00, rxdata_blocks, (uint32 *)rxdata_buf);
				usbcmd.lba += rxdata_blocks;
			} else if (usbcmd.lba == 0xFFFFF000) {
				SecureBootUnlock(rxdata_buf);
			} else if ((usbcmd.lba & 0xFFFF0000) == 0xFFF00000) {
				int iRet = vendor_storage_write(*(uint16 *)rxdata_buf, rxdata_buf+8, *(uint16 *)(rxdata_buf+4));
				if (iRet == -1) {
					RKUSBERR("vendor_storage_write failed,err=%d\n", iRet);
				}
				usbcmd.lba += rxdata_blocks;
			} else if (SecureBootLock == 0) {
				StorageWriteLba(usbcmd.lba, rxdata_buf, rxdata_blocks, usbcmd.imgwr_mode);
				usbcmd.lba += rxdata_blocks;
			}
		}
	}
#endif
}


void rkusb_handle_datatx(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	uint32_t txdata_size = 0;
	uint32_t tx_blocks = 0;
	uint32_t block_length = 0;
	uint32_t pre_blocks = 0;
	int iRet;
start:
	pre_blocks = 0;
	txdata_size = usbcmd.u_size - usbcmd.u_bytes;
	if (usbcmd.cmnd == K_FW_READ_10)
		block_length = 528;
	else if (usbcmd.cmnd == K_FW_LBA_READ_10)
		block_length = 512;
	tx_blocks = txdata_size / block_length;
	if (tx_blocks >= RKUSB_BUFFER_BLOCK_MAX) {
		tx_blocks = RKUSB_BUFFER_BLOCK_MAX;
		txdata_size = tx_blocks * block_length;
	}

	if (tx_blocks
			&& (usbcmd.pre_read.pre_blocks >= tx_blocks)
			&& (usbcmd.pre_read.pre_lba == usbcmd.lba)) {

		usbcmd.u_bytes += txdata_size;
		usbcmd.lba += tx_blocks;
		usbcmd.pre_read.pre_blocks -= tx_blocks;
		RKUSBINFO("rkusb_write_bulk_ep buffer %p, len %x\n", usbcmd.tx_buffer[usbcmd.txbuf_num], txdata_size);
		rkusb_write_bulk_ep(txdata_size, usbcmd.tx_buffer[usbcmd.txbuf_num]);
		usbcmd.txbuf_num = (usbcmd.txbuf_num + 1) % 2;
	}

	if (usbcmd.u_size == usbcmd.u_bytes) {
		pre_blocks = 0;
		/* clear pre_read data */
		usbcmd.pre_read.pre_lba = 0;
		usbcmd.pre_read.pre_blocks = 0;
	} else if ((usbcmd.u_bytes == 0) || (tx_blocks == RKUSB_BUFFER_BLOCK_MAX)) {
		RKUSBINFO("read u_bytes %x, tx_blocks %x\n", usbcmd.u_bytes, tx_blocks);
		pre_blocks = tx_blocks;
		usbcmd.pre_read.pre_buffer = usbcmd.tx_buffer[usbcmd.txbuf_num];
		usbcmd.pre_read.pre_lba = usbcmd.lba;
		usbcmd.pre_read.pre_blocks = tx_blocks;
	}
	/* read data from media */
	if (pre_blocks) {
		RKUSBINFO("read lba %x, buffer %p block %x\n", usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
		memset(usbcmd.pre_read.pre_buffer, 0, pre_blocks * block_length);
		if (usbcmd.cmnd == K_FW_READ_10) {
			iRet = StorageReadPba(usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
			if (iRet != FTL_OK) {
				RKUSBERR("StorageReadPba failed,err=%d\n", iRet);
			}
		} else if (usbcmd.cmnd == K_FW_LBA_READ_10) {
			if (usbcmd.lba >= 0xFFFFFF00)
				StorageVendorSysDataLoad(usbcmd.pre_read.pre_lba - 0xFFFFFF00, pre_blocks, (uint32 *)usbcmd.pre_read.pre_buffer);
			else if (usbcmd.lba == 0xFFFFF000)
				SecureBootUnlockCheck(usbcmd.pre_read.pre_buffer);
			else if ((usbcmd.lba & 0xFFFF0000) == 0xFFF00000) {
				uint16_t req_id = (usbcmd.lba & 0xFFFF);
				uint8 *p_buf = usbcmd.pre_read.pre_buffer;
				iRet = vendor_storage_read(req_id, p_buf + 8, usbcmd.u_size - 8);
				if (iRet == -1) {
					RKUSBERR("vendor_storage_read failed,err=%d\n", iRet);
				} else {
					p_buf[0] = (uint32_t)(req_id & 0xFF);
					p_buf[1] = (uint32_t)((req_id >> 8) & 0xFF);
					p_buf[4] = (uint32_t)(iRet & 0xFF);
					p_buf[5] = (uint32_t)((iRet >> 8) & 0xFF);
				}
			} else {
				iRet = StorageReadLba(usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
				if (iRet != FTL_OK) {
					RKUSBERR("StorageReadLba failed,err=%d\n", iRet);
				}
			}
		}
	}
	if (usbcmd.u_bytes == 0)
		goto start;

	if (usbcmd.u_size == usbcmd.u_bytes) {
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_GOOD;
		usbcmd.status = RKUSB_STATUS_CSW;
	}
#else
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = ep->tx_urb;
	uint32_t txdata_size = 0;
	uint32_t tx_blocks;
	uint32_t block_length = 0;
	uint32_t pre_blocks;

start:
	pre_blocks = 0;
	txdata_size = usbcmd.u_size - usbcmd.u_bytes;
	if (usbcmd.cmnd == K_FW_READ_10)
		block_length = 528;
	else if (usbcmd.cmnd == K_FW_LBA_READ_10)
		block_length = 512;
	tx_blocks = txdata_size / block_length;
	if (tx_blocks >= RKUSB_BUFFER_BLOCK_MAX) {
		tx_blocks = RKUSB_BUFFER_BLOCK_MAX;
		txdata_size = tx_blocks * block_length;
	}

	/* send data to usb */
	if (tx_blocks
			&& (usbcmd.pre_read.pre_blocks >= tx_blocks)
			&& (usbcmd.pre_read.pre_lba == usbcmd.lba)) {
		usbcmd.u_bytes += txdata_size;
		usbcmd.lba += tx_blocks;
		usbcmd.pre_read.pre_blocks -= tx_blocks;
		current_urb->actual_length = txdata_size;
		current_urb->buffer = usbcmd.pre_read.pre_buffer;
		udc_endpoint_write(ep);
		RKUSBINFO("udc_endpoint_write buffer %p, len %x\n",
			current_urb->buffer, current_urb->actual_length);
	}

	if ((usbcmd.u_bytes == 0) || (tx_blocks == RKUSB_BUFFER_BLOCK_MAX)) {
		RKUSBINFO("read u_bytes %x, tx_blocks %x\n", usbcmd.u_bytes, tx_blocks);
		pre_blocks = tx_blocks;
		usbcmd.pre_read.pre_buffer = usbcmd.tx_buffer[usbcmd.txbuf_num++ & 1];
		usbcmd.pre_read.pre_lba = usbcmd.lba;
		usbcmd.pre_read.pre_blocks = tx_blocks;
	}
	/* read data from media */
	if (pre_blocks) {
		RKUSBINFO("read lba %x, buffer %p block %x\n", usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
		if (usbcmd.cmnd == K_FW_READ_10) {
			StorageReadPba(usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
		} else if (usbcmd.cmnd == K_FW_LBA_READ_10) {
			if (usbcmd.lba >= 0xFFFFFF00) {
				StorageVendorSysDataLoad(usbcmd.pre_read.pre_lba - 0xFFFFFF00, pre_blocks, (uint32 *)usbcmd.pre_read.pre_buffer);
			} else if (usbcmd.lba == 0xFFFFF000) {
				SecureBootUnlockCheck(usbcmd.pre_read.pre_buffer);
			} else if ((usbcmd.lba & 0xFFFF0000) == 0xFFF00000) {
				uint16_t req_id = (usbcmd.lba & 0xFFFF);
				uint8 *p_buf = usbcmd.pre_read.pre_buffer;
				int iRet = vendor_storage_read(req_id, p_buf + 8, usbcmd.u_size - 8);
				if (iRet == -1) {
					RKUSBERR("vendor_storage_read failed,err=%d\n", iRet);
				} else {
					p_buf[0] = (uint32_t)(req_id & 0xFF);
					p_buf[1] = (uint32_t)((req_id >> 8) & 0xFF);
					p_buf[4] = (uint32_t)(iRet & 0xFF);
					p_buf[5] = (uint32_t)((iRet >> 8) & 0xFF);
				}
			} else {
				StorageReadLba(usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
			}
		}
	}
	if (usbcmd.u_bytes == 0)
		goto start;

	if (usbcmd.u_size == usbcmd.u_bytes) {
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_GOOD;
		usbcmd.status = RKUSB_STATUS_CSW;
	}
#endif
}

static void rkusb_handle_response(void)
{
#ifdef CONFIG_RK_DWC3_UDC
	switch (usbcmd.status) {
	case RKUSB_STATUS_TXDATA:
		RKUSBINFO("rkusb_write_bulk_ep %x\n", usbcmd.transfer_size);
		rkusb_write_bulk_ep(usbcmd.transfer_size, usbcmd.tx_buffer[usbcmd.txbuf_num]);
		usbcmd.txbuf_num = (usbcmd.txbuf_num + 1) % 2;
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_GOOD;
		usbcmd.status = RKUSB_STATUS_CSW;
		break;
	case RKUSB_STATUS_RXDATA:
		if (usbcmd.rx_giveback.status == RECV_OK)
			rkusb_handle_datarx();
		break;

	case RKUSB_STATUS_RXDATA_PREPARE:
			rkusb_handle_datarx();
		break;

	case RKUSB_STATUS_TXDATA_PREPARE:
		if ((usbcmd.u_bytes == 0) || (usbcmd.tx_giveback.status == SEND_FINISHED_OK))
			rkusb_handle_datatx();
		break;

	default:
		break;
	}
#else
	struct usb_endpoint_instance *ep;
	struct urb *current_urb = NULL;
	uint32_t actural_length;

	switch (usbcmd.status) {
	case RKUSB_STATUS_TXDATA:
		ep = &endpoint_instance[2];
		current_urb = ep->tx_urb;
		udc_endpoint_write(ep);
		usbcmd.data_size = 0;
		RKUSBINFO("udc_endpoint_write %x\n", current_urb->actual_length);
		if (usbcmd.data_size == 0) {
			usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
			usbcmd.csw.Status = CSW_GOOD;
			usbcmd.status = RKUSB_STATUS_CSW;
		}
		break;

	case RKUSB_STATUS_RXDATA:
		ep = &endpoint_instance[1];
		current_urb = ep->rcv_urb;
		actural_length = current_urb->actual_length;
		if (actural_length)
			rkusb_handle_datarx();
		break;

	case RKUSB_STATUS_RXDATA_PREPARE:
		rkusb_handle_datarx();
		break;
	case RKUSB_STATUS_TXDATA_PREPARE:
		ep = &endpoint_instance[2];
		current_urb = ep->tx_urb;
		if ((usbcmd.u_bytes == 0) || (current_urb->status == SEND_FINISHED_OK))
			rkusb_handle_datatx();
		break;
	default:
		break;
	}
#endif
}
static int rkusb_timeout_check(int flag)
{
#ifdef CONFIG_RK_DWC3_UDC
	if (flag) {
		if (GetVbus()) {
			if (!usbcmd.configured) {
				if (get_timer(TimeOutBase) > (10*1000)) {
					printf("Usb Timeout, Return for boot recovery!\n");
					return 1;
				}
			}
		} else {
			TimeOutBase = get_ticks();
		}
	}
	return 0;
#else
	/* TV Box: usb default as host, so Vbus always is high,
	 * if recovery key pressed and not connect to pc,
	 * 10s timeout enter recovery.
	 */
	if (flag) {
		if (GetVbus()) {
			if (!UsbConnectStatus()) {
				if (get_timer(TimeOutBase) > (10*1000)) {
					printf("Usb Timeout, Return for boot recovery!\n");
					return 1;
				}
			}
		} else {
			TimeOutBase = get_ticks();
		}
	}

	return 0;
#endif
}


static void rkusb_reset_check(void)
{
	if (usbcmd.reset_flag == 0x03) {
		/* reboot to maskrom */
		usbcmd.reset_flag = 0;
		ISetLoaderFlag(0xEF08A53C);
		mdelay(10);
		reset_cpu(0);
	} else if (usbcmd.reset_flag == 0x10) {
	/* lock loader */
		usbcmd.reset_flag = 0;
		SecureBootLockLoader();

	} else if (usbcmd.reset_flag == 0xFF) {
	/* reboot */
		usbcmd.reset_flag = 0;
		mdelay(10);
		reset_cpu(0);
	} else if (usbcmd.reset_flag == 0x01) {
		/* force to reboot to system, no check loader mode */
		usbcmd.reset_flag = 0;
		ISetLoaderFlag(SYS_LOADER_REBOOT_FLAG | BOOT_NORECOVER);
		mdelay(10);
		reset_cpu(0);
	}
}

static void rkusb_lowformat_check(void)
{
	FW_SorageLowFormat();
}


int do_rockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_RK_UDC
	int ret;
	RKUSBINFO("do_rockusb\n");
	rkusb_init_endpoint_ptrs();
	memset(&usbcmd, 0, sizeof(struct cmd_rockusb_interface));

	ret = udc_init();
	if (ret < 0) {
		RKUSBERR("%s: UDC init failure\n", __func__);
		return ret;
	}

	rkusb_init_strings();
	rkusb_init_instances();

	udc_startup_events(device_instance);
	udc_connect();

#ifdef CONFIG_ROCKUSB_TIMEOUT_CHECK
	TimeOutBase = get_ticks();
#endif
	while (1) {
		if (usbcmd.status == RKUSB_STATUS_IDLE) {
			struct usb_endpoint_instance *ep = &endpoint_instance[1];
			if (ep->rcv_urb->actual_length) {
				usbcmd.status = RKUSB_STATUS_CMD;
				ep->rcv_urb->buffer = usbcmd.rx_buffer[usbcmd.rxbuf_num&1];
				do_rockusb_cmd();
			}
		}
		if (usbcmd.status == RKUSB_STATUS_RXDATA ||
		    usbcmd.status == RKUSB_STATUS_TXDATA ||
		    usbcmd.status == RKUSB_STATUS_RXDATA_PREPARE ||
		    usbcmd.status == RKUSB_STATUS_TXDATA_PREPARE) {
			rkusb_handle_response();
		}
		if (usbcmd.status == RKUSB_STATUS_CSW) {
			rkusb_send_csw();
		}
		rkusb_reset_check();
		rkusb_lowformat_check();
#ifdef CONFIG_ROCKUSB_TIMEOUT_CHECK
		/* if press key enter rockusb, flag = 1 */
		if (rkusb_timeout_check(flag) == 1) {
			/* if timeout, return 1 for enter recovery */
			return 1;
		}
#endif
	}
#else
	struct rk_dwc3_udc_instance device;
	int ret;
	RKUSBINFO("<<Dwc3>> do_rockusb come in.\n");
	memset(&usbcmd, 0, sizeof(struct cmd_rockusb_interface));
	init_dwc3_udc_instance(&device);

	rk_dwc3_startup(&device);
	ret = rk_dwc3_connect();
	if (ret) {
		RKUSBINFO("<<Dwc3>> rk_dwc3_connect failed.\n");
		goto Exit_DoRockusb;
	}
#ifdef CONFIG_ROCKUSB_TIMEOUT_CHECK
	TimeOutBase = get_ticks();
#endif
	while (1) {
		dwc3_uboot_handle_interrupt();
		if (usbcmd.status == RKUSB_STATUS_IDLE) {
			if (usbcmd.rx_giveback.actual > 0) {
				usbcmd.status = RKUSB_STATUS_CMD;
				do_rockusb_cmd();
			}
		}
		if (usbcmd.status == RKUSB_STATUS_RXDATA ||
		    usbcmd.status == RKUSB_STATUS_TXDATA ||
		    usbcmd.status == RKUSB_STATUS_RXDATA_PREPARE ||
		    usbcmd.status == RKUSB_STATUS_TXDATA_PREPARE) {
			rkusb_handle_response();
		}
		if (usbcmd.status == RKUSB_STATUS_CSW) {
			rkusb_send_csw();
		}
		rkusb_reset_check();
		rkusb_lowformat_check();
#ifdef CONFIG_ROCKUSB_TIMEOUT_CHECK
		/* if press key enter rockusb, flag = 1 */
		if (rkusb_timeout_check(flag) == 1) {
			/* if timeout, return 1 for enter recovery */
			goto Exit_DoRockusb;
		}
#endif
	}
Exit_DoRockusb:
	rk_dwc3_disconnect();
	return 1;
#endif
}
U_BOOT_CMD(rockusb, CONFIG_SYS_MAXARGS, 1, do_rockusb,
	"Use the UMS [User Mass Storage]",
	"ums <USB_controller> <mmc_dev>  e.g. ums 0 0"
);


