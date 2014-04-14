#ifndef __DWC_OTG_REGS_H__
#define __DWC_OTG_REGS_H__

typedef volatile struct tagCORE_REG 
{
    uint32_t gotgctl; 
    uint32_t gotgint;
    uint32_t gahbcfg;
    uint32_t gusbcfg; 
    uint32_t grstctl; 
    uint32_t gintsts; 
    uint32_t gintmsk; 
    uint32_t grxstsr; 
    uint32_t grxstsp; 
    uint32_t grxfsiz; 
    uint32_t gnptxfsiz; 
    uint32_t gnptxsts; 
    uint32_t gi2cctl; 
    uint32_t gpvndctl;
    uint32_t ggpio; 
    uint32_t guid; 
    uint32_t gsnpsid;
    uint32_t ghwcfg1; 
    uint32_t ghwcfg2;
    uint32_t ghwcfg3;
    uint32_t ghwcfg4;
    uint32_t RESERVED1[(0x100-0x54)/4];
    uint32_t hptxfsiz;
    uint32_t dptxfsiz_dieptxf[15]; 
    uint32_t RESERVED2[(0x400-0x140)/4];
} CORE_REG, *pCORE_REG;

//Host Mode Register Structures
typedef volatile struct tagHOST_REG 
{
    uint32_t hcfg;		  
    uint32_t hfir;		  
    uint32_t hfnum; 
    uint32_t reserved40C;
    uint32_t hptxsts;	  
    uint32_t haint;	  
    uint32_t haintmsk;	  
    uint32_t RESERVED[(0x400-0x1c)/4];
} HOST_REG, *pHOST_REG;

//Device IN ep reg
typedef volatile struct tagIN_EP_REG 
{
    uint32_t DiEpCtl;
    uint32_t reserved04;	
    uint32_t DiEpInt; 
    uint32_t reserved0C;	
    uint32_t DiEpTSiz; 
    uint32_t DiEpDma; 
    uint32_t DTXFSTS;
    uint32_t DiEpDmaB; 
}IN_EP_REG, *pIN_EP_REG;

typedef volatile struct tagOUT_EP_REG 
{
    uint32_t DoEpCtl; 
    uint32_t DoEpFn; 
    uint32_t DoEpInt; 
    uint32_t reserved0C;	
    uint32_t DoEpTSiz; 
    uint32_t DoEpDma; 
    uint32_t reserved18;
    uint32_t DoEpDmaB0;
}OUT_EP_REG, *pOUT_EP_REG;


//Device Mode Registers Structures
typedef volatile struct tagDEVICE_REG 
{
    uint32_t dcfg; 
    uint32_t dctl; 
    uint32_t dsts; 
    uint32_t unused;		
    uint32_t diepmsk; 
    uint32_t doepmsk;	
    uint32_t daint;	
    uint32_t daintmsk; 
    uint32_t dtknqr1;	
    uint32_t dtknqr2;	
    uint32_t dvbusdis;		
    uint32_t dvbuspulse;
    uint32_t dtknqr3_dthrctl;	
    uint32_t dtknqr4_fifoemptymsk;
    uint32_t RESERVED1[(0x900-0x838)/4];

    //0x900~0xaff:ep in reg
    IN_EP_REG InEp[16];
    OUT_EP_REG OutEp[16];
    //0xb00~0xcff:ep out reg
    uint32_t RESERVED8[(0xe00-0xd00)/4];
} DEVICE_REG, *pDEVICE_REG; 

typedef volatile struct tagPOWER_CLOCK_CTRL 
{
    uint32_t PCGCR;
    uint32_t RESERVED[(0x1000-0xe04)/4];
}POWER_CLOCK_CTRL, *pPOWER_CLOCK_CTRL;

typedef volatile struct tagFIFO
{
    uint32_t dataPort;
    uint32_t RESERVED[(0x1000-0x004)/4];
}FIFO, *pFIFO;

typedef volatile struct tagUSB_OTG_REG 
{
    CORE_REG Core; 
    HOST_REG Host; 
    DEVICE_REG Device;
    POWER_CLOCK_CTRL ClkGate;
    FIFO Fifo[16];
    uint32_t RESERVED[(0x40000-0x11000)/4];
}USB_OTG_REG, *pUSB_OTG_REG;

//1寄存器位结构定义
typedef union tagDESC_STS_DATA
{
	uint32_t d32;
	struct 
	{
		unsigned byte : 16;
		unsigned reserved16_22 : 7;
		unsigned mtrf : 1;              //Multiple Transfer
		unsigned sr : 1;                //Setup packet Received
		unsigned ioc : 1;               //Interrupt On Complete
		unsigned sp : 1;                //short Packet
		#define LAST        0x01
		unsigned l : 1;                 //Last
		#define SUCCESS     0x00
		#define BUFERR      0x03
		unsigned sts : 2;               //received Status
		#define HOST_READY  0x00
		#define DMA_BUSY    0x01
		#define DMA_DONE    0x02
		#define HOST_BUSY   0x03
		unsigned bs : 2;                //Buffer Status
	}b;
}DESC_STS_DATA, *pDESC_STS_DATA;

//基于描述符DMA的数据结构
typedef struct tagDWC_OTG_DMA_DESC
{
	DESC_STS_DATA status;
	uint32_t *buf;
}DMA_DESC, *pDMA_DESC;

//设备请求结构
typedef struct _device_request
{
	uint8_t 	bmRequestType;
	uint8_t 	bRequest;
	uint16_t	wValue;
	uint16_t	wIndex;
	uint16_t	wLength;
} DEVICE_REQUEST_T;
#endif
