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
DECLARE_GLOBAL_DATA_PTR;

int do_rockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

/* USB specific */

static void rkusb_event_handler (struct usb_device_instance *device,
				  usb_device_event_t event, int data)
{
    RKUSBINFO("%s \n", __func__);
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

	default:
		break;
	}
}

/* fastboot_init has to be called before this fn to get correct serial string */
static void rkusb_init_instances(void)
{
	int i;

	/* initialize device instance */
	memset(device_instance, 0, sizeof(struct usb_device_instance));
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

	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		endpoint_instance[i].endpoint_address =
			ep_descriptor_ptrs[i - 1]->bEndpointAddress;

		endpoint_instance[i].rcv_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].rcv_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].tx_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);

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
			endpoint_instance[i].tx_urb->buffer = (u8 *)(u8 *)gd->arch.fastboot_buf_addr;
		}
		else{
			endpoint_instance[i].rcv_urb =
				usbd_alloc_urb(device_instance,
					       &endpoint_instance[i]);
			endpoint_instance[i].rcv_urb->buffer = (u8 *)(u8 *)gd->arch.fastboot_buf_addr
			                                       + CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH;
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
			
			 endpoint_instance[i].tx_packetSize =
				 le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);
			 endpoint_instance[i].rcv_packetSize =
				 le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);
			 //udc_setup_ep(device_instance, i, &endpoint_instance[i]);//setup epi reg in UdcInit()
	}
}

void FW_TestUnitReady(void)
{
    RKUSBINFO("%s \n", __func__);
    if(FW_StorageGetValid() == 0)
    {//ÕýÔÚµÍ¸ñ
        uint32_t totleBlock = FW_GetTotleBlk();
        uint32_t currEraseBlk = FW_GetCurEraseBlock();
        usbcmd.csw.Residue = cpu_to_le32((totleBlock<<16)|currEraseBlk);
        usbcmd.csw.Status= CSW_FAIL;
    }
    else if(usbcmd.cbw.CDB[1] == 0xFD)
    {
        uint32_t totleBlock = FW_GetTotleBlk();
        uint32_t currEraseBlk = 0;
        usbcmd.csw.Residue = cpu_to_le32((totleBlock<<16)|currEraseBlk);
        usbcmd.csw.Status= CSW_FAIL;
        FWLowFormatEn = 1;
    }
    else if(usbcmd.cbw.CDB[1] == 0xFA)
    {
        usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
        usbcmd.csw.Status= CSW_GOOD;
        
        FWSetResetFlag = 0x10;;
    }
    else if(usbcmd.cbw.CDB[1] == 0xF9)
    {
        usbcmd.csw.Residue = cpu_to_le32(StorageGetCapacity());
        usbcmd.csw.Status= CSW_GOOD;
    }
//    else if(SecureBootLock)
//    {
//        usbcmd.csw.Residue = Swap32(usbcmd.cbw.DataTransferLength);
//        usbcmd.csw.Status= CSW_FAIL;
//    }
    else
    {
        usbcmd.csw.Residue = cpu_to_le32(6);
        usbcmd.csw.Status= CSW_GOOD;
        RKUSBINFO("FW_TestUnitReady CMD %8x %8x \n",6, usbcmd.csw.Residue);
    }
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

    #if((PALTFORM==RK30XX)||(PALTFORM==RK_ALL)) 
        extern uint32_t Rk30ChipVerInfo[4];  
        ftl_memcpy(current_urb->buffer, Rk30ChipVerInfo, 16);
    #else
        ftl_memcpy(current_urb->buffer, (uint8*)(BOOT_ROM_CHIP_VER_ADDR), 16);
    #endif
    
#if(PALTFORM==RK292X)
    if(current_urb->buffer[0]==0x32393241)//"292A"
    {
        current_urb->buffer[0] = 0x32393258; // "292X"
    }
#endif
    
	current_urb->actual_length = 16;
		
    usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
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
    usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_TXDATA;
}
void FW_Read10(void)
{
    RKUSBINFO("%x \n", get_unaligned_be16(&usbcmd.cbw.CDB[7]));
    usbcmd.data_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 528;;
    usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);

    usbcmd.status = RKUSB_STATUS_TXDATA;
}
void FW_Write10(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[1];
	struct urb *current_urb = NULL;
	
    FW_DataLenCnt=0;
    FW_WR_Mode = FW_WR_MODE_PBA;
    
    usbcmd.data_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 528;
    usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);;
    
	current_urb = ep->rcv_urb;
    RKUSBINFO("%p\n", current_urb);
	current_urb->actual_length = 0;
	current_urb->buffer_length = usbcmd.data_size;//CONFIG_FASTBOOT_TRANSFER_BUFFER_SIZE_EACH;
	resume_usb(ep, 0);
    usbcmd.status = RKUSB_STATUS_RXDATA;
}

extern uint32_t  SecureBootLock;

void FW_Erase10(void)
{
	bool status = 0;
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	
    RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;

    
    if(gpMemFun->Erase && (SecureBootLock == 0))
        status = gpMemFun->Erase(usbcmd.cbw.Lun, usbcmd.cbw.CDB[2], usbcmd.cbw.CDB[7], 0);
        
    //ftl_memcpy(current_urb->buffer, TestResult, 64);
	current_urb->actual_length = 13;
    usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
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
    usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= status;
    usbcmd.status = RKUSB_STATUS_CSW;
}

void FW_LBARead10( void )
{
    RKUSBINFO("%x \n", get_unaligned_be16(&usbcmd.cbw.CDB[7]));
    usbcmd.data_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 512;;
    usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);

    usbcmd.status = RKUSB_STATUS_TXDATA;
    
}

void FW_LBAWrite10(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[1];
	struct urb *current_urb = NULL;
	
    FW_DataLenCnt=0;
    FW_WR_Mode = FW_WR_MODE_LBA;
    FW_IMG_WR_Mode = usbcmd.cbw.CDB[1];
    
    usbcmd.data_size = get_unaligned_be16(&usbcmd.cbw.CDB[7]) * 512;
    usbcmd.lba = get_unaligned_be32(&usbcmd.cbw.CDB[2]);;
    
	current_urb = ep->rcv_urb;
    RKUSBINFO("%p\n", current_urb);
	current_urb->actual_length = 0;
	current_urb->buffer_length = usbcmd.data_size;
	resume_usb(ep, 0);
    usbcmd.status = RKUSB_STATUS_RXDATA;
}

void FW_GetFlashInfo(void)
{
    extern void ReadFlashInfo(void *buf);
	struct usb_endpoint_instance *ep = &endpoint_instance[2];
	struct urb *current_urb = NULL;
	
    RKUSBINFO("%s \n", __func__);
	current_urb = ep->tx_urb;
    
    
    if(gpMemFun->ReadInfo)
        gpMemFun->ReadInfo(current_urb->buffer);
    FW_DataLenCnt=0;

    //ftl_memcpy(current_urb->buffer, &DataBuf[FW_DataLenCnt],11);
    ftl_memcpy(current_urb->buffer, current_urb->buffer+(FW_DataLenCnt<<2),11);
	current_urb->actual_length = 11;
    usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_TXDATA;
}
void FW_LowFormat(void)
{	
    RKUSBINFO("%s \n", __func__);
    usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_CSW;
    
    FWLowFormatEn = 1;
}

void FW_SetResetFlag(void)
{
    RKUSBINFO("%s \n", __func__);
    FWSetResetFlag = 1;
    
    usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_CSW;
}

void FW_Reset(void)
{
    RKUSBINFO("%x \n", usbcmd.cbw.CDB[1]);
    
    if(usbcmd.cbw.CDB[1])
        FWSetResetFlag = usbcmd.cbw.CDB[1];
    else
        FWSetResetFlag = 0xff;
    // SoftReset in main loop(usbhook)
    
    usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
    usbcmd.csw.Status= CSW_GOOD;
    usbcmd.status = RKUSB_STATUS_CSW;
}

void do_rockusb_cmd(unsigned char *buffer)
{
    memcpy(&usbcmd.cbw, buffer,sizeof(struct fsg_bulk_cb_wrap));
//	RKUSBINFO("do_rockusb_cmd %x\n", usbcmd.cbw.CDB[0]);
    switch (usbcmd.cbw.CDB[0])
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



void rkusb_handle_response(void)
{
	struct usb_endpoint_instance *ep;
	struct urb *current_urb = NULL;
	uint32_t actural_length;
    if(RKUSB_STATUS_TXDATA == usbcmd.status){
        ep = &endpoint_instance[2];
        current_urb = ep->tx_urb;
        if(usbcmd.data_size){ //read10
            //RKUSBINFO("handle read len %x\n", usbcmd.data_size);
            if(usbcmd.cbw.CDB[0]==K_FW_READ_10){
            	StorageReadPba(usbcmd.lba, current_urb->buffer,usbcmd.data_size/528);
            }
            else if(usbcmd.cbw.CDB[0]==K_FW_LBA_READ_10){
                  if(usbcmd.lba >= 0xFFFFFF00)
                      UsbStorageSysDataLoad(usbcmd.lba -0xFFFFFF00,usbcmd.data_size>>9,current_urb->buffer);
                  else if(usbcmd.lba == 0xFFFFF000)
                      SecureBootUnlockCheck(current_urb->buffer);
                  else
                      StorageReadLba(usbcmd.lba,current_urb->buffer,usbcmd.data_size>>9);
            }
        	current_urb->actual_length = usbcmd.data_size;
        	usbcmd.data_size = 0;
        }
        udc_endpoint_write(ep);
        //RKUSBINFO("udc_endpoint_write %x\n", current_urb->actual_length);
        while(current_urb->actual_length != 0);
    }
    else if(RKUSB_STATUS_RXDATA == usbcmd.status){
        ep = &endpoint_instance[1];
        current_urb = ep->rcv_urb;
        actural_length = current_urb->actual_length;
//        RKUSBINFO("write10 %p %x %x\n", current_urb, actural_length, usbcmd.data_size);
        if((actural_length)&&(usbcmd.data_size)){ //write10
            //RKUSBINFO("write10 %x %x\n", actural_length, usbcmd.data_size);
            if(usbcmd.cbw.CDB[0]==K_FW_WRITE_10){
                ISetLoaderFlag(SYS_LOADER_ERR_FLAG);
                if(SecureBootLock == 0)
            	StorageWritePba(usbcmd.lba,current_urb->buffer,(usbcmd.data_size/528));
            }
            else if(usbcmd.cbw.CDB[0]==K_FW_LBA_WRITE_10){
                if(usbcmd.lba >= 0xFFFFFF00)
                    UsbStorageSysDataStore(usbcmd.lba -0xFFFFFF00,(usbcmd.data_size/512),current_urb->buffer);
                else if(usbcmd.lba == 0xFFFFF000)
                    SecureBootUnlock(current_urb->buffer);
                else if(SecureBootLock == 0)
                    StorageWriteLba(usbcmd.lba,current_urb->buffer,(usbcmd.data_size/512),FW_IMG_WR_Mode);

            }
        	usbcmd.data_size = 0;
        }
    }
    
    if(usbcmd.data_size == 0){
        usbcmd.csw.Residue = cpu_to_le32(usbcmd.cbw.DataTransferLength);
        usbcmd.csw.Status= CSW_GOOD;
    	usbcmd.status = RKUSB_STATUS_CSW;
	}
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
		return;
	}
	
	csw->Signature = cpu_to_le32(USB_BULK_CS_SIG);
	csw->Tag = usbcmd.cbw.Tag;
	current_urb->actual_length = 13;
	memcpy(current_urb->buffer, &usbcmd.csw, 13);

    usbcmd.status = RKUSB_STATUS_IDLE;
    
    ep2->rcv_urb->buffer_length = 31;
    ep2->rcv_urb->actual_length = 0;
    resume_usb(ep2, 0);
    
	udc_endpoint_write(ep1);
	return 0;
}


int do_rockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
    // init usbd controller for rockusb
    //
    
    powerOn();
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
    	            do_rockusb_cmd(ep->rcv_urb->buffer);
	            }
	        }
	        if(usbcmd.status == RKUSB_STATUS_RXDATA
	           ||usbcmd.status == RKUSB_STATUS_TXDATA){
	            rkusb_handle_response();
	        }
	        if(usbcmd.status == RKUSB_STATUS_CSW){
	            rkusb_send_csw();
	        }
            if(FWSetResetFlag==0xFF){
                FWSetResetFlag = 0;
                SoftReset();
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

