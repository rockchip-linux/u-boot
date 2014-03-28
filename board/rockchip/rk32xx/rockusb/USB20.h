/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    USB.H
Author:		    XUESHAN LIN
Created:        1st Dec 2008
Modified:
Revision:		1.00
        VID     PID     BULK-IN BULK-OUT
RK26XX  0x071b  0x3202  0x81    0x01
RK27XX  0x071b  0x3201  0x82    0x01
RK28XX  0x071b  0x3228  0x81    0x02
********************************************************************************
********************************************************************************/
#ifndef _USB20_H
#define _USB20_H

//1可配置参数
#define         BULK_IN_EP          0x01
#define         BULK_OUT_EP         0x02

#if(PALTFORM == RK29XX)
#define         USB_VID             0x2207
#define         USB_PID             0x290A
#elif(PALTFORM == RK28XX)
#define         USB_VID             0x071B
#define         USB_PID             0x3228
#elif(PALTFORM == RK30XX)
#define         USB_VID             0x2207
#define         USB_PID             0x300A
#elif(PALTFORM == RK2828)
#define         USB_VID             0x2207
#define         USB_PID             0x282A
#elif(PALTFORM == RK292X || PALTFORM == RK_ALL)
#define         USB_VID             0x2207
#define         USB_PID             0x292A
#endif

#ifndef __GNUC__
#define PACKED1 __packed
#define PACKED2
#define ALIGN(x) __align(x)
#else
#define PACKED1
#define PACKED2 __attribute__((packed))
#define ALIGN(x) __attribute__ ((aligned(x)))
#endif

#define 		iManufactuer		0		/*厂商描述符字符串索引*/
#define 		iProduct			0		/*产品描述符字符串索引*/
#define 		iSerialNumber		0		/*设备序列号字符串索引*/
#define 		iConfiguration		0		/*配置描述符字符串索引*/
#define 		iInterface			0		/*接口描述符字符串索引*/


//1结构定义
//设备描述符结构
typedef PACKED1 struct _USB_DEVICE_DESCRIPTOR
{
    uint8		bLength;
    uint8		bDescriptorType;
    uint16 		bcdUSB;
    uint8 		bDeviceClass;
    uint8 		bDeviceSubClass;
    uint8 		bDeviceProtocol;
    uint8 		bMaxPacketSize0;
    uint16 		idVendor;
    uint16 		idProduct;
    uint16 		bcdDevice;
    uint8 		iiManufacturer;
    uint8 		iiProduct;
    uint8 		iiSerialNumber;
    uint8 		bNumConfigurations;
}PACKED2 USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;

//端点描述符结构
typedef PACKED1 struct _USB_ENDPOINT_DESCRIPTOR
{
    uint8 		bLength;
    uint8 		bDescriptorType;
    uint8 		bEndpointAddress;
    uint8 		bmAttributes;
    uint16 		wMaxPacketSize;
    uint8 		bInterval;
}PACKED2 USB_ENDPOINT_DESCRIPTOR, *PUSB_ENDPOINT_DESCRIPTOR;


//配置描述符结构
typedef PACKED1 struct  _USB_CONFIGURATION_DESCRIPTOR
{
    uint8 		bLength;
    uint8 		bDescriptorType;
    uint16		wTotalLength;
    uint8 		bNumInterfaces;
    uint8 		bConfigurationValue;
    uint8 		iiConfiguration;
    uint8 		bmAttributes;
    uint8 		MaxPower;
}PACKED2 USB_CONFIGURATION_DESCRIPTOR, *PUSB_CONFIGURATION_DESCRIPTOR;

//接口描述符结构
typedef PACKED1 struct _USB_INTERFACE_DESCRIPTOR
{
    uint8 		bLength;
    uint8 		bDescriptorType;
    uint8 		bInterfaceNumber;
    uint8 		bAlternateSetting;
    uint8 		bNumEndpoints;
    uint8 		bInterfaceClass;
    uint8 		bInterfaceSubClass;
    uint8 		bInterfaceProtocol;
    uint8 		iiInterface;
}PACKED2 USB_INTERFACE_DESCRIPTOR, *PUSB_INTERFACE_DESCRIPTOR;

//配置描述符集合描述符结构
typedef PACKED1 struct _USB_CONFIGS_DESCRIPTOR
{
    USB_CONFIGURATION_DESCRIPTOR Config;
    USB_INTERFACE_DESCRIPTOR Interface;
    USB_ENDPOINT_DESCRIPTOR BulkIn;
    USB_ENDPOINT_DESCRIPTOR BulkOut;
}PACKED2 USB_CONFIGS_DESCRIPTOR;

//高速设备的其它速度配置描述符
typedef PACKED1 struct _OTHER_SPEED_CONFIG_DESCRIPTOR
{
    uint8 		bLength;
    uint8 		bDescriptorType;
    uint16		wTotalLength;
    uint8 		bNumInterfaces;
    uint8 		bConfigurationValue;
    uint8 		iiConfiguration;
    uint8 		bmAttributes;
    uint8 		MaxPower;
}PACKED2 OTHER_SPEED_CONFIG_DESCRIPTOR;

//字符串描述符结构
typedef PACKED1 struct _USB_STRING_DESCRIPTOR
{
    uint8 		bLength;
    uint8 		bDescriptorType;
    uint8 		bString[1];
}PACKED2 USB_STRING_DESCRIPTOR, *PUSB_STRING_DESCRIPTOR;


//高速设备限制描述符
typedef PACKED1 struct _HS_DEVICE_QUALIFIER
{
	uint8	bLength;			//length of HS Device Descriptor
	uint8	bQualifier; 			//HS Device Qualifier Type
	uint16	wVersion;			// USB 2.0 version
	uint8	bDeviceClass;		//Device class
	uint8	bDeviceSubClasss;	//Device SubClass
	uint8	bProtocol;			//Device Protocol Code
	uint8	MaxPktSize;			//Maximum Packet SIze for other speed
	uint8	bOther_Config;		//Number of Other speed configurations
	uint8	Reserved;			//Reserved
}PACKED2 HS_DEVICE_QUALIFIER;

//BOS: group of device-level capabilities 
typedef PACKED1 struct _USB_BOS_DESCRIPTOR
{
    uint8   bLength;
    uint8   bDescriptorType;
    uint16  wTotalLength;
    uint8   bNumDeviceCaps;
}PACKED2 USB_BOS_DESCRIPTOR;

//Capability header descriptor 
typedef PACKED1 struct _USB_DEVICE_CAP_HEADER
{
    uint8   bLength;
    uint8   bDescriptorType;
    uint8   bDevCapabilityType;
} PACKED2 USB_DEVICE_CAP_HEADER;

//BOS描述符集合描述符结构
typedef PACKED1 struct _USB_BOS_ALL_DESCRIPTORS
{
    USB_BOS_DESCRIPTOR  BosDescriptor;
    USB_DEVICE_CAP_HEADER  CapHeaderDescriptor;
}PACKED2 USB_BOS_ALL_DESCRIPTORS;


//电源描述符结构
typedef struct _USB_POWER_DESCRIPTOR
{
    uint8 		bLength;
    uint8 		bDescriptorType;
    uint8 		bCapabilitiesFlags;
    uint16 		EventNotification;
    uint16 		D1LatencyTime;
    uint16 		D2LatencyTime;
    uint16		D3LatencyTime;
    uint8 		PowerUnit;
    uint16 		D0PowerConsumption;
    uint16 		D1PowerConsumption;
    uint16 		D2PowerConsumption;
} USB_POWER_DESCRIPTOR, *PUSB_POWER_DESCRIPTOR;


//通用描述符结构
typedef struct _USB_COMMON_DESCRIPTOR 
{
    uint8 		bLength;
    uint8 		bDescriptorType;
} USB_COMMON_DESCRIPTOR, *PUSB_COMMON_DESCRIPTOR;


//标准HUB描述符结构
typedef struct _USB_HUB_DESCRIPTOR 
{
    uint8        	bDescriptorLength;      	// Length of this descriptor
    uint8        	bDescriptorType;        	// Hub configuration type
    uint8        	bNumberOfPorts;         	// number of ports on this hub
    uint16	    	wHubCharacteristics;    	// Hub Charateristics
    uint8       	bPowerOnToPowerGood;        // port power on till power good in 2ms
    uint8       	bHubControlCurrent;     	// max current in mA
    // room for 255 ports power control and removable bitmask
    uint8        	bRemoveAndPowerMask[64];
} USB_HUB_DESCRIPTOR, *PUSB_HUB_DESCRIPTOR;

//设备请求结构
typedef struct _device_request
{
	uint8 	bmRequestType;
	uint8 	bRequest;
	uint16	wValue;
	uint16	wIndex;
	uint16	wLength;
} DEVICE_REQUEST;

//I/O请求结构(用于DMA)
typedef struct _IO_REQUEST 
{
	uint16	uAddressL;
	uint8	bAddressH;
	uint16	uSize;
	uint8	bCommand;
} IO_REQUEST, *PIO_REQUEST;


#define MAX_CONTROLDATA_SIZE	8		//控制管道最大包长
//带数据的设备请求结构
typedef struct _control_xfer
{
	DEVICE_REQUEST DeviceRequest;
	uint16 	wLength;
	uint16 	wCount;
	uint8 	*pData;
//	uint8 	dataBuffer[MAX_CONTROLDATA_SIZE];
} CONTROL_XFER;



typedef struct _TWAIN_FILEINFO 
{
	uint8	bPage;		// bPage bit 7 - 5 map to uSize bit 18 - 16
	uint8	uSizeH;		// uSize bit 15 - 8
	uint8	uSizeL;		// uSize bit 7 - 0
} TWAIN_FILEINFO, *PTWAIN_FILEINFO;



//1常量定义
#ifndef 		FALSE
#define 		FALSE  	 		0
#endif

#ifndef 		TRUE
#define 		TRUE    			(!FALSE)
#endif

#define			USB_CAP_HEADER_SIZE             3
#define			USB_DEVICE_CAPABILITY           0x10


#define 		NUM_ENDPOINTS		2		//端点数除0外
#define 		CONFIG_DESCRIPTOR_LENGTH    sizeof(USB_CONFIGURATION_DESCRIPTOR) \
											+ sizeof(USB_INTERFACE_DESCRIPTOR) \
											+ (NUM_ENDPOINTS * sizeof(USB_ENDPOINT_DESCRIPTOR))
#define			BOS_DESCRIPTOR_LENGTH       sizeof(USB_BOS_DESCRIPTOR) \
											+ USB_CAP_HEADER_SIZE

#define 		MAX_ENDPOINTS      			(uint8)0x3
#define 		EP0_TX_FIFO_SIZE   			64
#define 		EP0_RX_FIFO_SIZE   			64
#define 		EP0_PACKET_SIZE_FS 		    8
#define 		EP0_PACKET_SIZE_HS 		    64

#define 		EP1_TX_FIFO_SIZE   			64
#define 		EP1_RX_FIFO_SIZE   			64
#define 		EP1_PACKET_SIZE    			64

#define 		FS_BULK_RX_SIZE    			64
#define 		FS_BULK_TX_SIZE    			64
#define 		HS_BULK_RX_SIZE    			512
#define 		HS_BULK_TX_SIZE    			512


#define 		STAGE_IDLE           			0
#define 		STAGE_DATA       			1
#define 		STAGE_STATUS        			2


#define 		USB_RECIPIENT            	(uint8)0x1F
#define 		USB_RECIPIENT_DEVICE     	(uint8)0x00
#define 		USB_RECIPIENT_INTERFACE  	(uint8)0x01
#define 		USB_RECIPIENT_ENDPOINT   	(uint8)0x02

#define 		USB_REQUEST_TYPE_MASK    	(uint8)0x60
#define 		USB_STANDARD_REQUEST     	(uint8)0x00
#define 		USB_CLASS_REQUEST        	(uint8)0x20
#define 		USB_VENDOR_REQUEST       	(uint8)0x40

#define 		USB_REQUEST_MASK         	(uint8)0x0F
#define 		DEVICE_ADDRESS_MASK      	0x7F


//厂商请求代码
#define 		SETUP_DMA_REQUEST 		    0x0471
#define 		GET_FIRMWARE_VERSION    	0x0472
#define 		GET_SET_TWAIN_REQUEST   	0x0473
#define 		GET_BUFFER_SIZE		    	0x0474


//usb100.h
#define 		MAXIMUM_USB_STRING_LENGTH 				255

// values for the bits returned by the USB GET_STATUS command
#define 		USB_GETSTATUS_SELF_POWERED              0x01
#define 		USB_GETSTATUS_REMOTE_WAKEUP_ENABLED	    0x02


#define 		USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define 		USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define 		USB_STRING_DESCRIPTOR_TYPE              0x03
#define 		USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define 		USB_ENDPOINT_DESCRIPTOR_TYPE            0x05
#define 		USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE 	0x06
#define 		USB_OTHER_SPEED_CONF_DESCRIPTOR_TYPE 	0x07
#define 		USB_INTERFACE_POWER_DESCRIPTOR_TYPE 	0x08
#define 		USB_OTG_DESCRIPTOR_TYPE 				0x09
#define 		USB_DEBUG_DESCRIPTOR_TYPE 				0x0A
#define 		USB_IF_ASSOCIATION_DESCRIPTOR_TYPE 		0x0B
#define			USB_BOS_DESCRIPTOR_TYPE                 0x0F

// Values for bmAttributes field of an
// endpoint descriptor

#define 		USB_ENDPOINT_TYPE_MASK                    		0x03
#define 		USB_ENDPOINT_TYPE_CONTROL                 		0x00
#define 		USB_ENDPOINT_TYPE_ISOCHRONOUS               	0x01
#define 		USB_ENDPOINT_TYPE_BULK                    		0x02
#define 		USB_ENDPOINT_TYPE_INTERRUPT               		0x03


// definitions for bits in the bmAttributes field of a 
// configuration descriptor.
#define 		USB_CONFIG_POWERED_MASK                   		0xc0
#define 		USB_CONFIG_BUS_POWERED                    		0x80
#define 		USB_CONFIG_SELF_POWERED                   		0x40
#define 		USB_CONFIG_REMOTE_WAKEUP                  		0x20

// Endpoint direction bit, stored in address
#define 		USB_ENDPOINT_DIRECTION_MASK               	    0x80

// test direction bit in the bEndpointAddress field of
// an endpoint descriptor.
#define 		USB_ENDPOINT_DIRECTION_OUT(addr)          	(!((addr) & USB_ENDPOINT_DIRECTION_MASK))
#define 		USB_ENDPOINT_DIRECTION_IN(addr)           	((addr) & USB_ENDPOINT_DIRECTION_MASK)

// USB defined request codes
#define 		USB_REQUEST_GET_STATUS                    		0x00
#define 		USB_REQUEST_CLEAR_FEATURE                 		0x01
#define 		USB_REQUEST_SET_FEATURE                   		0x03
#define 		USB_REQUEST_SET_ADDRESS                   		0x05
#define 		USB_REQUEST_GET_DESCRIPTOR                		0x06
#define 		USB_REQUEST_SET_DESCRIPTOR                		0x07
#define 		USB_REQUEST_GET_CONFIGURATION               	0x08
#define 		USB_REQUEST_SET_CONFIGURATION               	0x09
#define 		USB_REQUEST_GET_INTERFACE                 		0x0A
#define 		USB_REQUEST_SET_INTERFACE                 		0x0B
#define 		USB_REQUEST_SYNC_FRAME                    		0x0C


// defined USB device classes
#define 		USB_DEVICE_CLASS_RESERVED           			0x00
#define 		USB_DEVICE_CLASS_AUDIO              			0x01	//音频设备
#define 		USB_DEVICE_CLASS_COMMUNICATIONS     		    0x02	//通讯设备
#define 		USB_DEVICE_CLASS_HUMAN_INTERFACE    		    0x03	//人机接口
#define 		USB_DEVICE_CLASS_MONITOR            			0x04	//显示器
#define 		USB_DEVICE_CLASS_PHYSICAL_INTERFACE 	        0x05	//物理接口
#define 		USB_DEVICE_CLASS_POWER              			0x06	//电源
#define 		USB_DEVICE_CLASS_PRINTER            			0x07	//打印机
#define 		USB_DEVICE_CLASS_STORAGE            			0x08	//海量存储
#define 		USB_DEVICE_CLASS_HUB                			0x09	//HUB
#define 		USB_DEVICE_CLASS_VENDOR_SPECIFIC    		    0xFF	//

//define	USB subclass
#define		    USB_SUBCLASS_CODE_RBC					    0x01
#define		    USB_SUBCLASS_CODE_SFF8020I				    0x02
#define		    USB_SUBCLASS_CODE_QIC157				    0x03
#define		    USB_SUBCLASS_CODE_UFI					    0x04
#define		    USB_SUBCLASS_CODE_SFF8070I				    0x05
#define		    USB_SUBCLASS_CODE_SCSI					    0x06

//define	USB protocol
#define		    USB_PROTOCOL_CODE_CBI0					    0x00
#define		    USB_PROTOCOL_CODE_CBI1					    0x01
#define		    USB_PROTOCOL_CODE_BULK				    	0x50


// USB defined Feature selectors
#define 		USB_FEATURE_ENDPOINT_STALL          		0x0000
#define 		USB_FEATURE_REMOTE_WAKEUP           		0x0001
#define 		USB_FEATURE_POWER_D0                		0x0002
#define 		USB_FEATURE_POWER_D1                		0x0003
#define 		USB_FEATURE_POWER_D2                		0x0004
#define 		USB_FEATURE_POWER_D3                		0x0005

//values for bmAttributes Field in USB_CONFIGURATION_DESCRIPTOR
#define 		BUS_POWERED                           		0x80
#define 		SELF_POWERED                          		0x40
#define 		REMOTE_WAKEUP                         		0x20

// USB power descriptor added to core specification
#define 		USB_SUPPORT_D0_COMMAND      				0x01
#define 		USB_SUPPORT_D1_COMMAND      				0x02
#define 		USB_SUPPORT_D2_COMMAND      				0x04
#define 		USB_SUPPORT_D3_COMMAND      				0x08
#define 		USB_SUPPORT_D1_WAKEUP       				0x10
#define 		USB_SUPPORT_D2_WAKEUP       				0x20



extern volatile uint8           UsbBootFlag;
extern volatile uint8           UsbConnected;
extern volatile uint8           UsbBusReset;
extern volatile uint8           ControlStage;
extern volatile uint8           Ep0PktSize;
extern volatile uint16          BulkEpSize;

extern CONTROL_XFER    ControlData;
extern ALIGN(8)        uint8 Ep0Buf[64];



//1函数原型声明
//extern 	void 	Delay100cyc(uint16 count);
//d12ci.c
extern	void 	UdcInit(void);
extern	void 	ReadEndpoint0(uint16 len, void *buf);
extern	void 	WriteEndpoint0(uint16 len, void* buf);
extern	void 	ReadBulkEndpoint(uint32 len, void *buf);
extern	void 	WriteBulkEndpoint(uint32 len, void* buf);


//usbCtrl.c
extern	void	UsbBoot(void);


//chap_9.c
extern	void 	set_address(void);
extern	void 	get_descriptor(void);
extern	void 	set_configuration(void);
extern	void 	ep0in_ack(void);
extern	void 	stall_ep0(void);
extern  bool    UsbPhyReset(void);
//isr.c
extern	void 	IrqHandler(void);
extern	void 	UsbIsr(void);
extern	void 	BusReset(void);
extern	void 	EnumDone(void);
extern	void	RxFifoNonEmpty(void);
extern	void 	Setup(void);
extern	void 	ControlHandler(void);
extern	void	Ep0OutPacket(uint16 len);
extern	void 	OutIntr(void);
extern	void 	InIntr(void);

//1表格定义
#ifdef	IN_CHAP9
const USB_DEVICE_DESCRIPTOR HSDeviceDescr =
{
	sizeof(USB_DEVICE_DESCRIPTOR),				//描述符的大小18(1B)
	USB_DEVICE_DESCRIPTOR_TYPE,				    //描述符的类型01(1B)
	0x0201,										//USB规划分布号(2B)
	0,											//1类型代码(由USB指定)(1B),0x00
	0, 0,										//子类型和协议(由USB分配)(2B)
	EP0_PACKET_SIZE_HS,							//端点0最大包长(1B)
	USB_VID,										//VID
	USB_PID,										//PID, rk28xx use 0x3228
	0x0100,
	iManufactuer, iProduct, iSerialNumber,		//厂商,产品,设备序列号字符串索引(3B)
	1											//可能的配置数(1B)
};

const USB_CONFIGS_DESCRIPTOR HSConfigDescr =
{
//配置描述符
	sizeof(USB_CONFIGURATION_DESCRIPTOR),		//描述符的大小9(1B)
	USB_CONFIGURATION_DESCRIPTOR_TYPE,		    //描述符的类型02(1B)
	CONFIG_DESCRIPTOR_LENGTH,
	1,											//配置所支持的接口数(1B)
	1,											//作为Set configuration的一个参数选择配置值(1B)
	0,											//用于描述配置字符串的索引(1B)
	0x80,										//位图,总线供电&远程唤醒(1B)
	200,										//最大消耗电流*2mA(1B)

//接口描述符
	sizeof(USB_INTERFACE_DESCRIPTOR),			//描述符的大小9(1B)
	USB_INTERFACE_DESCRIPTOR_TYPE,				//描述符的类型04(1B)
	0,											//接口的编号(1B)
	0,											//用于为上一个字段可供替换的设置(1B)
	NUM_ENDPOINTS,								//使用的端点数(端点0除外)(1B)
	USB_DEVICE_CLASS_VENDOR_SPECIFIC,			//1类型代码(由USB分配)(1B),USB_DEVICE_CLASS_STORAGE=Mass Storage
	USB_SUBCLASS_CODE_SCSI,				    	//1子类型代码(由USB分配)(1B),"0x06=Reduced Block Commands(RBC)"
	0x05,										//1协议代码(由USB分配)(1B),"0X50=Mass Storage Class Bulk-Only Transport"
	0,											//字符串描述的索引(1B)

//BULK-IN端点描述符
	sizeof(USB_ENDPOINT_DESCRIPTOR),
	USB_ENDPOINT_DESCRIPTOR_TYPE,
	BULK_IN_EP|0x80,
	USB_ENDPOINT_TYPE_BULK,
	HS_BULK_TX_SIZE,
	0,		//bulk trans invailed

//BULK-OUT端点描述符
	sizeof(USB_ENDPOINT_DESCRIPTOR),
	USB_ENDPOINT_DESCRIPTOR_TYPE,
	BULK_OUT_EP,
	USB_ENDPOINT_TYPE_BULK,
	HS_BULK_TX_SIZE,
	0		//bulk trans invailed
};

const USB_BOS_ALL_DESCRIPTORS HSBosDescr =
{
    sizeof(USB_BOS_DESCRIPTOR),                 //Size of descriptor 0x05(1B)
    USB_BOS_DESCRIPTOR_TYPE,                    //BOS descriptor type 0x0F(1B)
    BOS_DESCRIPTOR_LENGTH,                      //Length of this descriptor and all of its sub descriptor
    0x01,                                       //the number of separate device capability descriptors in the BOS
                                                //First device capability
    USB_CAP_HEADER_SIZE,                        //Length of cap header
    USB_DEVICE_CAPABILITY,                      //Device Capability type
    0                                           //DevCapability Type,reserved
};

#else
extern	 	const USB_DEVICE_DESCRIPTOR 		HSDeviceDescr;
extern	 	const USB_CONFIGS_DESCRIPTOR 		HSConfigDescr;
extern		const uint8 MicrosoftOSDesc[18];
#endif
#endif

