/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Kever Yang 2014.03.31
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
#define IN_FW_Upgrade
#include <cmd_rockusb.h>
#include <asm/arch/rkplat.h>
DECLARE_GLOBAL_DATA_PTR;
#if defined(CONFIG_RK_UDC)
#include <usb/dwc_otg_udc.h>
#endif
#include <../board/rockchip/common/config.h>


/* USB specific */
void rkusb_receive_firstcbw(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[1];
	// get first CBW
	ep->rcv_urb->buffer = (u8*)&usbcmd.cbw;
	ep->rcv_urb->buffer_length = 31;
	ep->rcv_urb->actual_length = 0;
	// make sure endpoint will be re-enabled
	suspend_usb();
	resume_usb(ep, 0);
}

static void rkusb_event_handler (struct usb_device_instance *device,
				  usb_device_event_t event, int data)
{
	RKUSBINFO("@rkusb_event_handler   %x \n", event);
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

/* fastboot_init has to be called before this fn to get correct serial string */
static void rkusb_init_instances(void)
{
	int i;

	/* initialize device instance */
	memset((void*)device_instance, 0, sizeof(struct usb_device_instance));
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
	//udc_setup_ep(device_instance, 0, &endpoint_instance[0]);//setup ep0 reg in UdcInit()

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
	RKUSBINFO("%p %p %p %p %x\n",
		usbcmd.rx_buffer[0], usbcmd.rx_buffer[1], usbcmd.tx_buffer[1], usbcmd.tx_buffer[0], usbcmd.tx_buffer[1]);
	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		endpoint_instance[i].endpoint_address =
			ep_descriptor_ptrs[i - 1]->bEndpointAddress;

		endpoint_instance[i].rcv_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].rcv_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].tx_packetSize =0x201;
//			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);

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
			RKUSBINFO("setting up HS USB device ep%x\n",
				endpoint_instance[i].endpoint_address);
 			ep_descriptor_ptrs[i - 1]->wMaxPacketSize = 0x200;
		} else {
			RKUSBINFO("setting up FS USB device ep%x\n",
					endpoint_instance[i].endpoint_address);
			ep_descriptor_ptrs[i - 1]->wMaxPacketSize = 0x40;
		}
		// fastboot will send a zero packet if the last data packet is tx_packetSize, but rockusb don't
		endpoint_instance[i].tx_packetSize = 0x201;
//			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);
		endpoint_instance[i].rcv_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);
	}
}


/***************************************************************************
 * 命令:测试准备0x00
 ***************************************************************************/
static void FW_TestUnitReady(void)
{
	if(FW_StorageGetValid() == 0)
	{//正在低格
		uint32_t totleBlock = FW_GetTotleBlk();
		uint32_t currEraseBlk = FW_GetCurEraseBlock();
		usbcmd.csw.Residue = cpu_to_be32((totleBlock<<16)|currEraseBlk);
		usbcmd.csw.Status = CSW_FAIL;
	}
	else if(usbcmd.cbw.CDB[1] == 0xFD)
	{
		uint32_t totleBlock = FW_GetTotleBlk();
		uint32_t currEraseBlk = 0;
		usbcmd.csw.Residue = cpu_to_be32((totleBlock<<16)|currEraseBlk);
		usbcmd.csw.Status = CSW_FAIL;
		FW_SorageLowFormatEn(1);
	}
	else if(usbcmd.cbw.CDB[1] == 0xFA)
	{
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_GOOD;

		usbcmd.reset_flag = 0x10;
	}
	else if(usbcmd.cbw.CDB[1] == 0xF9)
	{
		usbcmd.csw.Residue = cpu_to_be32(StorageGetCapacity());
		usbcmd.csw.Status = CSW_GOOD;
	}
	else if(SecureBootLock)
	{
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_FAIL;
	}
	else
	{
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
void FW_ReadID(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;

	RKUSBINFO("%s \n", __func__);

	current_urb = ep->tx_urb;
	if (!current_urb) {
		RKUSBERR("%s: current_urb NULL", __func__);
		return;
	}

	StorageReadId(current_urb->buffer);
	current_urb->actual_length = 5;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
}

/***************************************************************************
函数描述:固件升级命令:设置FLASH 类型
	Flash 1:
	0:8bit small page;	1:8bit large page 4cyc;		2:8bit large page 5cyc
	3:16bit small page;	4:16bit large page 4cyc;	5:16bit large page 5cyc
	6:MLC 8bit large page 5cyc		7:MLC 8bit large page 5cyc, 4KB/page
入口参数:
出口参数:无
调用函数:无
         1、2009-4-10 ：增加出错返回，PC工具检测到擦除系统盘出错会擦除所有保留块
            后面的块，重启后loader会低格，重新分配空间。            
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
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	unsigned int chip_info[4];

	RKUSBINFO("%s \n", __func__);

	current_urb = ep->tx_urb;
	if (!current_urb) {
		RKUSBERR("%s: current_urb NULL", __func__);
		return;
	}
	/* notice here chip version should the same as rk tools RKBOOT/*.ini config */
	current_urb->buffer[0] = 0;
	memset(chip_info, 0, sizeof(chip_info));
	ftl_memcpy(chip_info, (uint8*)(RKIO_ROM_CHIP_VER_ADDR), 16);
#if defined(CONFIG_RKCHIP_RK3036)
	chip_info[0] = 0x33303341; // 303A
#endif
#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	chip_info[0] = 0x33313241; // 312A
#endif
	ftl_memcpy(current_urb->buffer, chip_info, 16);
	current_urb->actual_length = 16;
		
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
}

/***************************************************************************
函数描述:测试坏块——0:好块; 1:坏块
入口参数:命令块中的指定物理块地址
出口参数:无
调用函数:无
***************************************************************************/
static void FW_TestBadBlock(void)
{
	uint16_t i;
	uint32_t TestResult[16];
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	
	RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;

	for (i=0; i<16; i++)
		TestResult[i] = 0;
    
	ftl_memcpy(current_urb->buffer, TestResult, 64);
	current_urb->actual_length = 64;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
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
	if(!(((usbcmd.lba + rxdata_blocks) < usbcmd.pre_read.pre_lba)
			|| (usbcmd.lba > (usbcmd.pre_read.pre_lba + usbcmd.pre_read.pre_blocks)))) {
		RKUSBINFO("FW_Write10: invalid pre read\n");
		usbcmd.pre_read.pre_blocks = 0;
		usbcmd.pre_read.pre_lba = 0;
	}

	usbcmd.status = RKUSB_STATUS_RXDATA_PREPARE;
}


/***************************************************************************
函数描述:按物理BLOCK擦除——0:好块; 1:坏块
入口参数:命令块中的物理块地址
出口参数:无
调用函数:无
***************************************************************************/
static void FW_Erase10(void)
{
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
}

/***************************************************************************
函数描述:按物理BLOCK擦除——0:好块; 1:坏块
入口参数:命令块中的物理块地址
出口参数:无
调用函数:无
***************************************************************************/
static void FW_Erase10Force(void)
{
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
}

static void FW_LBARead10(void)
{
	usbcmd.u_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 512;
	usbcmd.u_bytes = 0;
	usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);
	usbcmd.status = RKUSB_STATUS_TXDATA_PREPARE;
}

static void FW_LBAWrite10(void)
{
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
	if(!(((usbcmd.lba + rxdata_blocks) < usbcmd.pre_read.pre_lba)
			|| (usbcmd.lba > (usbcmd.pre_read.pre_lba + usbcmd.pre_read.pre_blocks)))) {
		RKUSBINFO("FW_LBAWrite10: invalid pre read\n");
		usbcmd.pre_read.pre_blocks = 0;
		usbcmd.pre_read.pre_lba = 0;
	}

	usbcmd.status = RKUSB_STATUS_RXDATA_PREPARE;
}

static void FW_GetFlashInfo(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;

	RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;

	StorageReadFlashInfo(current_urb->buffer);

	ftl_memcpy(current_urb->buffer, current_urb->buffer, 11);
	current_urb->actual_length = 11;
	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_TXDATA;
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

	if(usbcmd.cbw.CDB[1])
		usbcmd.reset_flag = usbcmd.cbw.CDB[1];
	else
		usbcmd.reset_flag = 0xff;
	// SoftReset in main loop(usbhook)

	usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
	usbcmd.csw.Status = CSW_GOOD;
	usbcmd.status = RKUSB_STATUS_CSW;
}

static int rkusb_send_csw(void)
{
	struct usb_endpoint_instance *ep1 = &endpoint_instance[2];
	struct usb_endpoint_instance *ep2 = &endpoint_instance[1];
	struct urb *current_urb = ep1->tx_urb;
	struct bulk_cs_wrap	*csw = &usbcmd.csw;
    
	RKUSBINFO("%s tag %x\n", __func__, usbcmd.cbw.Tag);
	if (!current_urb) {
		RKUSBERR("%s: current_urb NULL", __func__);
		return -1;
	}
	while(current_urb->status != SEND_FINISHED_OK)
		udelay(10);
//	RKUSBERR("current_urb status %x\n", current_urb->status);
	
	csw->Signature = cpu_to_le32(USB_BULK_CS_SIG);
	csw->Tag = usbcmd.cbw.Tag;
	current_urb->actual_length = 13;
	current_urb->buffer = (u8*)&usbcmd.csw;
	ep2->rcv_urb->buffer = (u8*)&usbcmd.cbw;
	ep2->rcv_urb->buffer_length = 31;
	ep2->rcv_urb->actual_length = 0;
	resume_usb(ep2, 0);

	// wait for last tx complete
	udc_endpoint_write(ep1);

	usbcmd.status = RKUSB_STATUS_IDLE;

	return 0;
}

void do_rockusb_cmd(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = ep->tx_urb;
	current_urb->buffer = usbcmd.tx_buffer[usbcmd.txbuf_num&1];
	usbcmd.cmnd = usbcmd.cbw.CDB[0];
	RKUSBINFO("CBW %x %x %x\n",usbcmd.cbw.Tag, usbcmd.cbw.Flags, usbcmd.cbw.Length);
    
//	RKUSBINFO("do_rockusb_cmd %x\n", usbcmd.cbw.CDB[0]);
	switch (usbcmd.cmnd)
	{
		case K_FW_TEST_UNIT_READY:      //0x00
			FW_TestUnitReady();
			break;

		case K_FW_READ_FLASH_ID:	//0x01
			FW_ReadID();
			break;

		case K_FW_TEST_BAD_BLOCK:	//0x03
			FW_TestBadBlock();
			break;
	
		case K_FW_READ_10:		//0x04
			FW_Read10();
			break;

		case K_FW_WRITE_10:		//0x05
			FW_Write10();
			break;
		case K_FW_ERASE_10:		//0x06
			FW_Erase10();
			break;
		case K_FW_ERASE_10_FORCE:	//0x0b
			FW_Erase10Force();
			break;
		case K_FW_LBA_READ_10:		//0x14
			FW_LBARead10();
			break;

		case K_FW_LBA_WRITE_10:		//0x15
			FW_LBAWrite10();
			break;
		case K_FW_ERASE_SYS_DISK:	//0x16
			FW_LowFormatSysDisk();
			break;

		case K_FW_READ_FLASH_INFO:	//0x1A
			FW_GetFlashInfo();
			break;
		case K_FW_GET_CHIP_VER:         //0x1B
			FW_GetChipVer();
			break;
		case K_FW_LOW_FORMAT:		//0x1C
			FW_LowFormat();
			break;
		case K_FW_SET_RESET_FLAG:       //0x1e
			FW_SetResetFlag();
			break;
		case K_FW_RESET:		//0xff
			FW_Reset();
			break;

		default:
			break;
	}
}


void rkusb_handle_datarx(void)
{
	struct usb_endpoint_instance *ep=&endpoint_instance[1];
	struct urb *current_urb = ep->rcv_urb;
	uint8_t *rxdata_buf = NULL;
	uint32_t rxdata_blocks = 0;
	uint32_t transfer_length;
	uint32_t rx_blocks = 0;
	uint32_t block_length;
    
	if(usbcmd.cmnd == K_FW_WRITE_10)
		block_length = 528;
	else if(usbcmd.cmnd == K_FW_LBA_WRITE_10)
		block_length = 512;

	// updatea varilables
	if(current_urb->actual_length) { // data received
		usbcmd.d_bytes += current_urb->actual_length;
		rxdata_buf = current_urb->buffer;
		rxdata_blocks = current_urb->actual_length/block_length;

		current_urb->actual_length = 0;
		usbcmd.rxbuf_num ++;
		current_urb->buffer = usbcmd.rx_buffer[usbcmd.rxbuf_num & 1];
//		RKUSBINFO("data received %x %p\n", rxdata_buf, current_urb->buffer);
    }
    
	// read next packet from usb
	if(usbcmd.d_bytes < usbcmd.d_size) {
		transfer_length = usbcmd.d_size - usbcmd.d_bytes;
		rx_blocks = transfer_length/block_length;
		if(rx_blocks > RKUSB_BUFFER_BLOCK_MAX)
			rx_blocks = RKUSB_BUFFER_BLOCK_MAX;
		transfer_length = rx_blocks*block_length;
        
//		RKUSBINFO("read next packet %x\n", transfer_length);
		current_urb->buffer_length = transfer_length;
		resume_usb(ep, 0);
		usbcmd.status = RKUSB_STATUS_RXDATA;
	}
	else {   // data receive complete
//		RKUSBINFO("data receive complete\n");
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status = CSW_GOOD;
//		usbcmd.status = RKUSB_STATUS_CSW;
		rkusb_send_csw();
	}

	/* Write to media */
	if(rxdata_blocks) {
		RKUSBINFO("write to media %x, lba %x, buf %p\n", rxdata_blocks, usbcmd.lba, rxdata_buf);
		if(usbcmd.cmnd == K_FW_WRITE_10) {
			ISetLoaderFlag(SYS_LOADER_ERR_FLAG);
			if(SecureBootLock == 0) {
				StorageWritePba(usbcmd.lba, rxdata_buf, rxdata_blocks);
				usbcmd.lba += rxdata_blocks;
			}
		}
		else if(usbcmd.cmnd == K_FW_LBA_WRITE_10) {
			if(usbcmd.lba >= 0xFFFFFF00) {
				UsbStorageSysDataStore(usbcmd.lba - 0xFFFFFF00, rxdata_blocks, (uint32_t *)rxdata_buf);
				usbcmd.lba += rxdata_blocks;
			}
			else if(usbcmd.lba == 0xFFFFF000) {
				SecureBootUnlock(rxdata_buf);
			}
			else if(SecureBootLock == 0) {
				StorageWriteLba(usbcmd.lba, rxdata_buf, rxdata_blocks, usbcmd.imgwr_mode);
				usbcmd.lba += rxdata_blocks;
			}
		}
	}
}


void rkusb_handle_datatx(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = ep->tx_urb;
	uint32_t txdata_size = 0;
	uint32_t tx_blocks;
	uint32_t block_length = 0;
	uint32_t pre_blocks;

start:
	pre_blocks = 0;
	// check if data available
	txdata_size = usbcmd.u_size - usbcmd.u_bytes;
	if(usbcmd.cmnd == K_FW_READ_10)
		block_length = 528;
	else if(usbcmd.cmnd == K_FW_LBA_READ_10)
		block_length = 512;
	tx_blocks = txdata_size / block_length;
	if(tx_blocks >= RKUSB_BUFFER_BLOCK_MAX) {
		tx_blocks = RKUSB_BUFFER_BLOCK_MAX;
		txdata_size = tx_blocks * block_length;
	}
    
	// send data to usb
	if(tx_blocks
			&&(usbcmd.pre_read.pre_blocks >= tx_blocks)
			&&(usbcmd.pre_read.pre_lba == usbcmd.lba)) {
        
		usbcmd.u_bytes += txdata_size;
		usbcmd.lba += tx_blocks;
		usbcmd.pre_read.pre_blocks -= tx_blocks;
		current_urb->actual_length = txdata_size;
		current_urb->buffer = usbcmd.pre_read.pre_buffer;
		udc_endpoint_write(ep);
		RKUSBINFO("udc_endpoint_write buffer %p, len %x\n",
			current_urb->buffer, current_urb->actual_length);
	}

	if((usbcmd.u_bytes == 0) || (tx_blocks == RKUSB_BUFFER_BLOCK_MAX)) {
		RKUSBINFO("read u_bytes %x, tx_blocks %x\n", usbcmd.u_bytes, tx_blocks);
		pre_blocks = tx_blocks;
		usbcmd.pre_read.pre_buffer = usbcmd.tx_buffer[usbcmd.txbuf_num++ & 1];
		usbcmd.pre_read.pre_lba = usbcmd.lba;
		usbcmd.pre_read.pre_blocks = tx_blocks;
	}
	// read data from media
	if(pre_blocks) {
		RKUSBINFO("read lba %x, buffer %p block %x\n", usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
		if(usbcmd.cmnd == K_FW_READ_10) {
			StorageReadPba(usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
		}
		else if(usbcmd.cmnd == K_FW_LBA_READ_10) {
			if(usbcmd.lba >= 0xFFFFFF00)
				UsbStorageSysDataLoad(usbcmd.pre_read.pre_lba - 0xFFFFFF00, pre_blocks, (uint32_t *)usbcmd.pre_read.pre_buffer);
			else if(usbcmd.lba == 0xFFFFF000)
				SecureBootUnlockCheck(usbcmd.pre_read.pre_buffer);
			else
				StorageReadLba(usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
		}
	}
	if(usbcmd.u_bytes == 0)
		goto start;
        
	if(usbcmd.u_size == usbcmd.u_bytes) {
		usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
		usbcmd.csw.Status= CSW_GOOD;
		usbcmd.status = RKUSB_STATUS_CSW;
	}
}

void rkusb_handle_response(void)
{
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
        
		if(usbcmd.data_size == 0) {
			usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
			usbcmd.csw.Status= CSW_GOOD;
			usbcmd.status = RKUSB_STATUS_CSW;
		}
		break;

	case RKUSB_STATUS_RXDATA:
		ep = &endpoint_instance[1];
		current_urb = ep->rcv_urb;
		actural_length = current_urb->actual_length;

		if(actural_length)
			rkusb_handle_datarx();
		break;

	case RKUSB_STATUS_RXDATA_PREPARE:
		rkusb_handle_datarx();
		break;

	case RKUSB_STATUS_TXDATA_PREPARE:
		ep = &endpoint_instance[2];
		current_urb = ep->tx_urb;
		if((usbcmd.u_bytes == 0)||(current_urb->status == SEND_FINISHED_OK))
			rkusb_handle_datatx();
		break;

	default :
		break;
	}
}


#ifdef CONFIG_ROCKUSB_TIMEOUT_CHECK
static uint64_t TimeOutBase = 0;
static inline int rkusb_timeout_check(int flag)
{
	/* TV Box: usb default as host, so Vbus always is high,
	 * if recovery key pressed and not connect to pc,
	 * 10s timeout enter recovery.
	 */
	if (flag) { // if recovery key pressed
		if (GetVbus()) { // if Vbus is high
			if (!UsbConnectStatus()) { // if usb no connect
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
}
#endif /* CONFIG_ROCKUSB_TIMEOUT_CHECK */


static inline void rkusb_reset_check(void)
{
	if (usbcmd.reset_flag == 0x03) { // reboot to maskrom
		usbcmd.reset_flag = 0;
		ISetLoaderFlag(0xEF08A53C);
		mdelay(10);
		reset_cpu(0);
	} else if (usbcmd.reset_flag == 0x10) { // lock loader
		usbcmd.reset_flag = 0;
		SecureBootLockLoader();

	} else if (usbcmd.reset_flag == 0xFF) { // reboot
		usbcmd.reset_flag = 0;
		mdelay(10);
		reset_cpu(0);
	}
}

static inline void rkusb_lowformat_check(void)
{
	FW_SorageLowFormat();
}


int do_rockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
	//init usbd controller for rockusb
    
	RKUSBINFO("do_rockusb\n");
	rkusb_init_endpoint_ptrs();
	memset(&usbcmd, 0, sizeof(struct cmd_rockusb_interface));

	ret = udc_init();
	if (ret < 0) {
		RKUSBERR("%s: UDC init failure\n", __func__);
		return ret;
	}

	rkusb_init_instances();

	udc_startup_events(device_instance);
	udc_connect();

#ifdef CONFIG_ROCKUSB_TIMEOUT_CHECK
	TimeOutBase = get_ticks();
#endif
	while(1)
	{
		if(usbcmd.status == RKUSB_STATUS_IDLE) {
			struct usb_endpoint_instance *ep = &endpoint_instance[1];
			if(ep->rcv_urb->actual_length) {
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
		if(rkusb_timeout_check(flag) == 1) {
			/* if timeout, return 1 for enter recovery */
			return 1;
		}
#endif
	}
}

U_BOOT_CMD(rockusb, CONFIG_SYS_MAXARGS, 1, do_rockusb,
	"Use the UMS [User Mass Storage]",
	"ums <USB_controller> <mmc_dev>  e.g. ums 0 0"
);

