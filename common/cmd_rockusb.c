/********************************************************************************
*********************************************************************************
                        COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
                                --  ALL RIGHTS RESERVED  --

File Name:      FW_Upgrade.C
Author:         XUESHAN LIN
Created:        1st Dec 2008

Modified:	Kever Yang 2014.03.31
Revision:       1.00
********************************************************************************
********************************************************************************/
#define IN_FW_Upgrade
#include <cmd_rockusb.h>
#include <asm/arch/rkplat.h>
DECLARE_GLOBAL_DATA_PTR;
#if defined(CONFIG_RK_UDC)
#include <usb/dwc_otg_udc.h>
#endif

int do_rockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

/* USB specific */
void rkusb_receive_firstcbw(void)
{
    struct usb_endpoint_instance *ep = &endpoint_instance[1];

	// get first CBW
    ep->rcv_urb->buffer = (u8*)&usbcmd.cbw;
    ep->rcv_urb->buffer_length = 31;
    ep->rcv_urb->actual_length = 0;
	resume_usb(ep, 0);
}
static void rkusb_event_handler (struct usb_device_instance *device,
				  usb_device_event_t event, int data)
{
    RKUSBINFO("%x \n", event);
	switch (event) {
	case DEVICE_RESET:
	case DEVICE_BUS_INACTIVE:
		usbcmd.configured = 0;
		break;
	case DEVICE_CONFIGURED:
		usbcmd.configured = 1;

		break;

	case DEVICE_ADDRESS_ASSIGNED:
		rkusb_init_endpoints();
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
	device_instance->device_descriptor = &device_descriptor;
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
	usbcmd.tx_buffer[0] = usbcmd.rx_buffer[1]+(CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH>>1);
	usbcmd.tx_buffer[1] = usbcmd.tx_buffer[0]+(CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH>>1);
#else
	//is this enough?
	int buffer_len = RKUSB_BUFFER_BLOCK_MAX << 10;
	usbcmd.rx_buffer[0] = (u8 *)malloc(buffer_len << 2);
	usbcmd.rx_buffer[1] = usbcmd.rx_buffer[0]+buffer_len;
	usbcmd.tx_buffer[0] = usbcmd.rx_buffer[1]+buffer_len;
	usbcmd.tx_buffer[1] = usbcmd.tx_buffer[0]+buffer_len;
#endif
    RKUSBINFO("%p %p %p %p %x\n",usbcmd.rx_buffer[0], usbcmd.rx_buffer[1], usbcmd.tx_buffer[1], usbcmd.tx_buffer[0], usbcmd.tx_buffer[1]);
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
		if (endpoint_instance[i].endpoint_address & USB_DIR_IN){
			endpoint_instance[i].tx_urb =
				usbd_alloc_urb(device_instance,
					       &endpoint_instance[i]);
			endpoint_instance[i].tx_urb->buffer = usbcmd.tx_buffer[0];
			endpoint_instance[i].tx_urb->status = SEND_FINISHED_OK;
		}
		else{
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
			 endpoint_instance[i].tx_packetSize =0x201;
//				 le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);
			 endpoint_instance[i].rcv_packetSize =
				 le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);
	}
}

void FW_TestUnitReady(void)
{
    RKUSBINFO("%s \n", __func__);
    if(FW_StorageGetValid() == 0)
    {//ÕýÔÚµÍ¸ñ
        uint32_t totleBlock = FW_GetTotleBlk();
        uint32_t currEraseBlk = FW_GetCurEraseBlock();
        usbcmd.csw.Residue = cpu_to_be32((totleBlock<<16)|currEraseBlk);
        usbcmd.csw.Status= CSW_FAIL;
    }
    else if(usbcmd.cbw.CDB[1] == 0xFD)
    {
        uint32_t totleBlock = FW_GetTotleBlk();
        uint32_t currEraseBlk = 0;
        usbcmd.csw.Residue = cpu_to_be32((totleBlock<<16)|currEraseBlk);
        usbcmd.csw.Status= CSW_FAIL;
        FWLowFormatEn = 1;
    }
    else if(usbcmd.cbw.CDB[1] == 0xFA)
    {
        usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
        usbcmd.csw.Status= CSW_GOOD;

        usbcmd.reset_flag = 0x10;
    }
    else if(usbcmd.cbw.CDB[1] == 0xF9)
    {
        usbcmd.csw.Residue = cpu_to_be32(StorageGetCapacity());
        usbcmd.csw.Status= CSW_GOOD;
    }
//    else if(SecureBootLock)
//    {
//        usbcmd.csw.Residue = Swap32(usbcmd.cbw.DataTransferLength);
//        usbcmd.csw.Status= CSW_FAIL;
//    }
    else
    {
        usbcmd.csw.Residue = cpu_to_be32(6);
        usbcmd.csw.Status= CSW_GOOD;
    }
    usbcmd.status = RKUSB_STATUS_CSW;
}

void FW_LowFormatSysDisk(void)
{
    usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_CSW;
}

void FW_GetChipVer(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	
    RKUSBINFO("%s \n", __func__);

	current_urb = ep->tx_urb;
	if (!current_urb) {
		RKUSBERR("%s: current_urb NULL", __func__);
		return;
	}
	current_urb->buffer[0] = 0;
	ftl_memcpy(current_urb->buffer, (uint8*)(BOOT_ROM_CHIP_VER_ADDR), 16);

#if(CONFIG_RKCHIPTYPE==CONFIG_RK3288) 
    current_urb->buffer[0] =  0x33323041; // "320A"
#endif
    current_urb->actual_length = 16;
		
    usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_TXDATA;
}

void FW_TestBadBlock(void)
{
	uint16_t i;
	uint32_t TestResult[16];
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	
    RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;

	for (i=0; i<16; i++)
		TestResult[i]=0;
    
    ftl_memcpy(current_urb->buffer, TestResult, 64);
	current_urb->actual_length = 64;
    usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_TXDATA;
}
void FW_Read10(void)
{
    usbcmd.u_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 528;
    usbcmd.u_bytes = 0;
    usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);

    usbcmd.status = RKUSB_STATUS_TXDATA_PREPARE;
    RKUSBINFO("lba %x block num %x \n", usbcmd.lba, usbcmd.u_size);
}
void FW_Write10(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[1];
	struct urb *current_urb = ep->rcv_urb;
	
    usbcmd.d_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 528;
    usbcmd.d_bytes= 0;
    usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);
    
    RKUSBINFO("lba %x len %x\n", usbcmd.lba, usbcmd.d_size);
	current_urb->actual_length = 0;
    usbcmd.status = RKUSB_STATUS_RXDATA_PREPARE;
}

extern uint32_t  SecureBootLock;

void FW_Erase10(void)
{
	bool status = 0;
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	
    RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;

	StorageEraseBlock(get_unaligned_be32(&usbcmd.cbw.CDB[2]), get_unaligned_be32(&usbcmd.cbw.CDB[7]), 0);
	current_urb->actual_length = 13;
    usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= status;
    usbcmd.status = RKUSB_STATUS_CSW;
}
void FW_Erase10Force(void)
{
	bool status = 0;
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	
    RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;
	
    if(gpMemFun->Erase && (SecureBootLock == 0))
        status = gpMemFun->Erase(usbcmd.cbw.Lun, usbcmd.cbw.CDB[2], usbcmd.cbw.CDB[7], 1);
        
	current_urb->actual_length = 13;
    usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= status;
    usbcmd.status = RKUSB_STATUS_CSW;
}

void FW_LBARead10( void )
{
    usbcmd.u_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 512;
    usbcmd.u_bytes = 0;
    usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);
    
    RKUSBINFO("lba %x len %x\n", usbcmd.lba, usbcmd.u_size);

    usbcmd.status = RKUSB_STATUS_TXDATA_PREPARE;
    
}

void FW_LBAWrite10(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[1];
	struct urb *current_urb = NULL;
	
    usbcmd.d_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 512;
    usbcmd.d_bytes= 0;
    usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);
    usbcmd.imgwr_mode = usbcmd.cbw.CDB[1];
    
	current_urb = ep->rcv_urb;
    RKUSBINFO("lba %x, size %x\n", usbcmd.lba, usbcmd.d_size);
	current_urb->actual_length = 0;
    usbcmd.status = RKUSB_STATUS_RXDATA_PREPARE;
}

void FW_GetFlashInfo(void)
{
    extern void ReadFlashInfo(void *buf);
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	
    RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;
    
    StorageReadFlashInfo(current_urb->buffer);

    ftl_memcpy(current_urb->buffer, current_urb->buffer,11);
	current_urb->actual_length = 11;
    usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_TXDATA;
}
void FW_LowFormat(void)
{	
    RKUSBINFO("%s \n", __func__);
    usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_CSW;
    
    FWLowFormatEn = 1;
}

void FW_SetResetFlag(void)
{
    RKUSBINFO("%s \n", __func__);
    usbcmd.reset_flag = 1;
    
    usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_CSW;
}

void FW_Reset(void)
{
    RKUSBINFO("%x \n", usbcmd.cbw.CDB[1]);
    
    if(usbcmd.cbw.CDB[1])
        usbcmd.reset_flag = usbcmd.cbw.CDB[1];
    else
        usbcmd.reset_flag = 0xff;
    // SoftReset in main loop(usbhook)
    
    usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_CSW;
}

static int rkusb_send_csw(void)
{
	struct usb_endpoint_instance *ep1 = &endpoint_instance[2];
	struct usb_endpoint_instance *ep2 = &endpoint_instance[1];
	struct urb *current_urb = ep1->tx_urb;
	struct bulk_cs_wrap	*csw=&usbcmd.csw;
    
    RKUSBINFO("%s \n", __func__);
	if (!current_urb) {
		RKUSBERR("%s: current_urb NULL", __func__);
		return -1;
	}
    while(current_urb->status != SEND_FINISHED_OK)
        udelay(10);
//            RKUSBERR("current_urb status %x\n", current_urb->status);
	
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
    
//	RKUSBINFO("do_rockusb_cmd %x\n", usbcmd.cbw.CDB[0]);
    switch (usbcmd.cmnd)
    {
        case K_FW_TEST_UNIT_READY:      //0x00
            FW_TestUnitReady();
            break;
            
        case K_FW_TEST_BAD_BLOCK:		//0x03
            FW_TestBadBlock();
            break;
			
        case K_FW_READ_10:				//0x04
            FW_Read10();
            break;

        case K_FW_WRITE_10:				//0x05
            FW_Write10();
            break;
	case K_FW_ERASE_10:				//0x06
	    FW_Erase10();
	    break;
	case K_FW_ERASE_10_FORCE:		//0x0b
	    FW_Erase10Force();
	    break;
	case K_FW_LBA_READ_10:				//0x14
	    FW_LBARead10();
	    break;

	case K_FW_LBA_WRITE_10:				//0x15
			FW_LBAWrite10();
			break;
        case K_FW_ERASE_SYS_DISK:           //0x16
            FW_LowFormatSysDisk();
            break;
	case K_FW_READ_FLASH_INFO:        //0x1A
            FW_GetFlashInfo();
            break;
        case K_FW_GET_CHIP_VER:         //0x1B
            FW_GetChipVer();
            break;
        case K_FW_LOW_FORMAT:			//0x1C
            FW_LowFormat();
            break;
        case K_FW_SET_RESET_FLAG:       //0x1e
            FW_SetResetFlag();
            break;
		case K_FW_RESET:				//0xff
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
	if(current_urb->actual_length){ // data received
	    usbcmd.d_bytes += current_urb->actual_length;
	    rxdata_buf = current_urb->buffer;
	    rxdata_blocks = current_urb->actual_length/block_length;

        current_urb->actual_length = 0;
	    usbcmd.rxbuf_num ++;
	    current_urb->buffer = usbcmd.rx_buffer[usbcmd.rxbuf_num & 1];
 //       RKUSBINFO("data received %x %p\n", rxdata_buf, current_urb->buffer);
    }
    
    // read next packet from usb
    if(usbcmd.d_bytes < usbcmd.d_size){
            
        transfer_length = usbcmd.d_size - usbcmd.d_bytes;
        rx_blocks = transfer_length/block_length;
        if(rx_blocks > RKUSB_BUFFER_BLOCK_MAX)
            rx_blocks = RKUSB_BUFFER_BLOCK_MAX;
         transfer_length = rx_blocks*block_length;
        
 //       RKUSBINFO("read next packet %x\n", transfer_length);
        current_urb->buffer_length = transfer_length;
        resume_usb(ep, 0);
        usbcmd.status = RKUSB_STATUS_RXDATA;
    }
    else{   // data receive complete
 //       RKUSBINFO("data receive complete\n");
        usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
        usbcmd.csw.Status= CSW_GOOD;
//        usbcmd.status = RKUSB_STATUS_CSW;
        rkusb_send_csw();
    }

    // write to media
    if(rxdata_blocks){
        RKUSBINFO("write to media %x, lba %x, buf %p\n", rxdata_blocks, usbcmd.lba, rxdata_buf);
        if(usbcmd.cmnd==K_FW_WRITE_10){
            ISetLoaderFlag(SYS_LOADER_ERR_FLAG);
            if(SecureBootLock == 0){
        	    StorageWritePba(usbcmd.lba,rxdata_buf,rxdata_blocks);
        	    usbcmd.lba += rxdata_blocks;
        	}
        }
        else if(usbcmd.cmnd==K_FW_LBA_WRITE_10){
            if(usbcmd.lba >= 0xFFFFFF00){
                UsbStorageSysDataStore(usbcmd.lba -0xFFFFFF00,rxdata_blocks,(uint32_t *)rxdata_buf);
        	    usbcmd.lba += rxdata_blocks;
        	}
            else if(usbcmd.lba == 0xFFFFF000)
                SecureBootUnlock(current_urb->buffer);
            else if(SecureBootLock == 0){
                StorageWriteLba(usbcmd.lba,rxdata_buf,rxdata_blocks,usbcmd.imgwr_mode);
        	    usbcmd.lba += rxdata_blocks;
        	}
        }
    }
}


void rkusb_handle_datatx(void)
{
	struct usb_endpoint_instance *ep=&endpoint_instance[2];
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
    if(tx_blocks >= RKUSB_BUFFER_BLOCK_MAX){
        tx_blocks = RKUSB_BUFFER_BLOCK_MAX;
        txdata_size = tx_blocks*block_length;
    }
    
    // send data to usb
    if(tx_blocks
       &&(usbcmd.pre_read.pre_blocks >= tx_blocks)
       &&(usbcmd.pre_read.pre_lba == usbcmd.lba)){
        
        usbcmd.u_bytes += txdata_size;
        usbcmd.lba += tx_blocks;
        usbcmd.pre_read.pre_blocks -= tx_blocks;
        current_urb->actual_length = txdata_size;
        current_urb->buffer = usbcmd.pre_read.pre_buffer;
        udc_endpoint_write(ep);
        RKUSBINFO("udc_endpoint_write buffer %p, len %x\n", current_urb->buffer, current_urb->actual_length);
    }

    if((usbcmd.u_bytes == 0) || (tx_blocks == RKUSB_BUFFER_BLOCK_MAX)){
        RKUSBINFO("read u_bytes %x, tx_blocks %x\n", usbcmd.u_bytes, tx_blocks);
        pre_blocks = tx_blocks;
    	usbcmd.pre_read.pre_buffer = usbcmd.tx_buffer[usbcmd.txbuf_num++ & 1];
    	usbcmd.pre_read.pre_lba = usbcmd.lba;
    	usbcmd.pre_read.pre_blocks = tx_blocks;
    }
    // read data from media
    if(pre_blocks){
        RKUSBINFO("read lba %x, buffer %p block %x\n", usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer, pre_blocks);
        if(usbcmd.cmnd ==K_FW_READ_10){
        	StorageReadPba(usbcmd.pre_read.pre_lba, usbcmd.pre_read.pre_buffer,pre_blocks);
        }
        else if(usbcmd.cmnd == K_FW_LBA_READ_10){
              if(usbcmd.lba >= 0xFFFFFF00)
                  UsbStorageSysDataLoad(usbcmd.pre_read.pre_lba -0xFFFFFF00,pre_blocks, (uint32_t *)usbcmd.pre_read.pre_buffer);
              else if(usbcmd.lba == 0xFFFFF000)
                  SecureBootUnlockCheck(usbcmd.pre_read.pre_buffer);
              else
                  StorageReadLba(usbcmd.pre_read.pre_lba,usbcmd.pre_read.pre_buffer,pre_blocks);
        }
    }
    if(usbcmd.u_bytes == 0)
        goto start;
        
    if(usbcmd.u_size == usbcmd.u_bytes){
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
    if(RKUSB_STATUS_TXDATA == usbcmd.status){
        ep = &endpoint_instance[2];
        current_urb = ep->tx_urb;
        udc_endpoint_write(ep);
        usbcmd.data_size = 0;
        RKUSBINFO("udc_endpoint_write %x\n", current_urb->actual_length);
        
        if(usbcmd.data_size == 0){
            usbcmd.csw.Residue = cpu_to_be32(usbcmd.cbw.DataTransferLength);
            usbcmd.csw.Status= CSW_GOOD;
        	usbcmd.status = RKUSB_STATUS_CSW;
    	}
    }
    else if(RKUSB_STATUS_RXDATA == usbcmd.status){
        ep = &endpoint_instance[1];
        current_urb = ep->rcv_urb;
        actural_length = current_urb->actual_length;

        if(actural_length)
            rkusb_handle_datarx();
    }
    else if(RKUSB_STATUS_RXDATA_PREPARE == usbcmd.status){
        rkusb_handle_datarx();
    }
    else if(RKUSB_STATUS_TXDATA_PREPARE == usbcmd.status){
        ep = &endpoint_instance[2];
        current_urb = ep->tx_urb;
        if((usbcmd.u_bytes == 0)||(current_urb->status == SEND_FINISHED_OK))
            rkusb_handle_datatx();
    }
    
}


int do_rockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
    // init usbd controller for rockusb
    //
    
    //powerOn();
    RKUSBINFO("do_rockusb\n");
	rkusb_init_endpoint_ptrs();
	memset(&usbcmd, 0, sizeof(struct cmd_rockusb_interface));

	ret = udc_init();
	if (ret < 0) {
		RKUSBERR("%s: MUSB UDC init failure\n", __func__);
		goto out;
	}

//	rkusb_init_strings();
	rkusb_init_instances();

	udc_startup_events(device_instance);
	udc_connect();
	
	while(1)
	{
	    /*if(usbcmd.configured)*/{

	        if(usbcmd.status == RKUSB_STATUS_IDLE){
	            struct usb_endpoint_instance *ep = &endpoint_instance[1];
	            if(ep->rcv_urb->actual_length){
    	            usbcmd.status = RKUSB_STATUS_CMD;
    	            ep->rcv_urb->buffer = usbcmd.rx_buffer[usbcmd.rxbuf_num&1];
    	            do_rockusb_cmd();
	            }
	        }
	        if(usbcmd.status == RKUSB_STATUS_RXDATA
	           ||usbcmd.status == RKUSB_STATUS_TXDATA
	           ||usbcmd.status == RKUSB_STATUS_RXDATA_PREPARE
	           ||usbcmd.status == RKUSB_STATUS_TXDATA_PREPARE){
	            rkusb_handle_response();
	        }
	        if(usbcmd.status == RKUSB_STATUS_CSW){
	            rkusb_send_csw();
	        }
            if(usbcmd.reset_flag==0xFF){
                usbcmd.reset_flag = 0;
	       reset_cpu(0);
            }
	        SysLowFormatCheck();
		}
	}
out:
    return ret;
}

U_BOOT_CMD(rockusb, CONFIG_SYS_MAXARGS, 1, do_rockusb,
	"Use the UMS [User Mass Storage]",
	"ums <USB_controller> <mmc_dev>  e.g. ums 0 0"
);

