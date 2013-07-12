#ifndef __DWC_OTG_REGS_H__
#define __DWC_OTG_REGS_H__

//1芯片相关宏定义

//1寄存器结构定义
//Global Registers
typedef volatile struct tagCORE_REG 
{
    uint32 gotgctl; 
    uint32 gotgint;
    uint32 gahbcfg;
    uint32 gusbcfg; 
    uint32 grstctl; 
    uint32 gintsts; 
    uint32 gintmsk; 
    uint32 grxstsr; 
    uint32 grxstsp; 
    uint32 grxfsiz; 
    uint32 gnptxfsiz; 
    uint32 gnptxsts; 
    uint32 gi2cctl; 
    uint32 gpvndctl;
    uint32 ggpio; 
    uint32 guid; 
    uint32 gsnpsid;
    uint32 ghwcfg1; 
    uint32 ghwcfg2;
    uint32 ghwcfg3;
    uint32 ghwcfg4;
    uint32 RESERVED1[(0x100-0x54)/4];
    uint32 hptxfsiz;
    uint32 dptxfsiz_dieptxf[15]; 
    uint32 RESERVED2[(0x400-0x140)/4];
} CORE_REG, *pCORE_REG;

//Host Mode Register Structures
typedef volatile struct tagHOST_REG 
{
    uint32 hcfg;		  
    uint32 hfir;		  
    uint32 hfnum; 
    uint32 reserved40C;
    uint32 hptxsts;	  
    uint32 haint;	  
    uint32 haintmsk;	  
    uint32 RESERVED[(0x400-0x1c)/4];
} HOST_REG, *pHOST_REG;

//Device IN ep reg
typedef volatile struct tagIN_EP_REG 
{
    uint32 DiEpCtl;
    uint32 reserved04;	
    uint32 DiEpInt; 
    uint32 reserved0C;	
    uint32 DiEpTSiz; 
    uint32 DiEpDma; 
    uint32 DTXFSTS;
    uint32 DiEpDmaB; 
}IN_EP_REG, *pIN_EP_REG;

typedef volatile struct tagOUT_EP_REG 
{
    uint32 DoEpCtl; 
    uint32 DoEpFn; 
    uint32 DoEpInt; 
    uint32 reserved0C;	
    uint32 DoEpTSiz; 
    uint32 DoEpDma; 
    uint32 reserved18;
    uint32 DoEpDmaB0;
}OUT_EP_REG, *pOUT_EP_REG;


//Device Mode Registers Structures
typedef volatile struct tagDEVICE_REG 
{
    uint32 dcfg; 
    uint32 dctl; 
    uint32 dsts; 
    uint32 unused;		
    uint32 diepmsk; 
    uint32 doepmsk;	
    uint32 daint;	
    uint32 daintmsk; 
    uint32 dtknqr1;	
    uint32 dtknqr2;	
    uint32 dvbusdis;		
    uint32 dvbuspulse;
    uint32 dtknqr3_dthrctl;	
    uint32 dtknqr4_fifoemptymsk;
    uint32 RESERVED1[(0x900-0x838)/4];

    //0x900~0xaff:ep in reg
    IN_EP_REG InEp[16];
    OUT_EP_REG OutEp[16];
    //0xb00~0xcff:ep out reg
    uint32 RESERVED8[(0xe00-0xd00)/4];
} DEVICE_REG, *pDEVICE_REG; 

typedef volatile struct tagPOWER_CLOCK_CTRL 
{
    uint32 PCGCR;
    uint32 RESERVED[(0x1000-0xe04)/4];
}POWER_CLOCK_CTRL, *pPOWER_CLOCK_CTRL;

typedef volatile struct tagFIFO
{
    uint32 dataPort;
    uint32 RESERVED[(0x1000-0x004)/4];
}FIFO, *pFIFO;

typedef volatile struct tagUSB_OTG_REG 
{
    CORE_REG Core; 
    HOST_REG Host; 
    DEVICE_REG Device;
    POWER_CLOCK_CTRL ClkGate;
    FIFO Fifo[16];
    uint32 RESERVED[(0x40000-0x11000)/4];
}USB_OTG_REG, *pUSB_OTG_REG;

//1寄存器位结构定义
typedef union tagDESC_STS_DATA
{
	uint32 d32;
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
	uint32 *buf;
}DMA_DESC, *pDMA_DESC;

#endif

