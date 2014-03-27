/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
#ifndef __RK_REG_H
#define __RK_REG_H
typedef volatile struct tagGICD_REG
{
	uint32 ICDDCR		  ; 	 //0x000 
	uint32 ICDICTR		  ;    //0x004	 
	uint32 ICDIIDR		  ;    //0x008
	uint32 RESERVED0[29]  ; 
	uint32 ICDISR[4]		;	//	 0x080	
	uint32 RESERVED1[28]  ;
	uint32 ICDISER[4]		;	  // 0x100 
	uint32 RESERVED2[28]  ;   
	uint32 ICDICER[4]	  ; 	   //0x180	 
	uint32 RESERVED3[28]  ;
	uint32 ICDISPR[4]		;	   //0x200	
	uint32 RESERVED4[28]  ;
	uint32 ICDICPR[4]	  ; 	 //0x280   
	uint32 RESERVED5[28]  ;
	uint32 ICDIABR[4]	  ; 	   //0x300
	uint32 RESERVED6[60]  ;
	uint32 ICDIPR_SGI[4]	;		// 0x400
	uint32 ICDIPR_PPI[4]	;		// 0x410 
	uint32 ICDIPR_SPI[18] ; 		//0x420
	uint32 RESERVED7[230];
	uint32 ITARGETSR[255];		  //0x800
	uint32 RESERVED9[1] ;
	uint32 ICDICFR[7]	  ; 	   //0xc00
	uint32 RESERVED8[185] ;
	uint32 ICDSGIR		  ; 	   //0xf00 
}GICD_REG, *pGICD_REG;	
typedef volatile struct tagGICC_REG
{
	uint32 ICCICR		 ;		   //0x00 
	uint32 ICCPMR		 ;		   //0x04 
	uint32 ICCBPR		 ;		   //0x08 
	uint32 ICCIAR		 ;		   //0x0c 
	uint32 ICCEOIR		;		  //0x10 
	uint32 ICCRPR		 ;		   //0x14 
	uint32 ICCHPIR		;		  //0x18 
	uint32 ICCABPR		;		  //0x1c 
	uint32 RESERVED0[55];
	uint32 ICCIIDR		;		  //0xfc  
}GICC_REG, *pGICC_REG;


typedef enum _IRQ_NUM
{
	INT_DMAC1_0=32	   ,
	INT_DMAC1_1 	,
	INT_DMAC2_0 	,
	INT_DMAC2_1 	,
	INT_USB_OTG = 55	,
	INT_USB_Host = 56	,
	INT_USB_Host1 = 57	 ,
	INT_SDMMC = 64		,
	INT_SDIO = 65		,
	INT_SDIO1 = 66		 ,
	INT_eMMC = 67		,
	INT_SARADC = 68 	,
	INT_TSADC = 69	   ,
	INT_NandC = 70		,
INT_USB_Host0_OHCI	,
	INT_SPI0 = 70		,
	INT_SPI1		,
	INT_SPI2		,
	INT_UART0  = 87 	,
	INT_UART1		,
	INT_UART2		,
	INT_UART3		,
INT_UART4		,

	INT_I2C0 = 92		,
	INT_I2C1		,
	INT_I2C2		,
	INT_I2C3		,
	INT_I2C4		,
	INT_I2C5		,
	INT_GPIO0 = 113 ,
	INT_GPIO1		,
	INT_GPIO2		,
	INT_GPIO3		,
	INT_GPIO4		,
	INT_GPIO5		,
	INT_GPIO6		,
	INT_GPIO7		,
	INT_GPIO8		,
	INT_MAXNUM		
}eINT_NUM;

/* PMU registers */
typedef volatile struct tagPMU_REG
{
    uint32 PMU_WAKEUP_CFG[2];
    uint32 PMU_PWRDN_CON;
    uint32 PMU_PWRDN_ST;
    uint32 PMU_INT_CON;
    uint32 PMU_INT_ST;
    uint32 PMU_MISC_CON;
    uint32 PMU_OSC_CNT;
    uint32 PMU_PLL_CNT;
    uint32 PMU_PMU_CNT;
    uint32 PMU_DDRIO_PWRON_CNT;
    uint32 PMU_WAKEUP_RST_CLR_CNT;
    uint32 PMU_SCU_PWRDWN_CNT;
    uint32 PMU_SCU_PWRUP_CNT;
    uint32 PMU_MISC_CON1;
    uint32 PMU_GPIO6_CON;
    uint32 PMU_PMU_SYS_REG[4];
} PMU_REG, *pPMU_REG;
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
#define g_pmuReg ((pPMU_REG)RK3288_PMU_PHYS)
#else
#define g_pmuReg ((pPMU_REG)RK3188_PMU_PHYS)
#endif

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
//CRU Registers
    typedef volatile struct tagCRU_STRUCT 
    {
		uint32 CRU_PLL_CON[4][5]; 
		uint32 CRU_MODE_CON;
		uint32 reserved1[3];
		uint32 CRU_CLKSEL_CON[42];
		uint32 reserved2[22];
		uint32 CRU_CLKGATE_CON[18];
		uint32 reserved3[1];
		uint32 CRU_GLB_SRST_FST_VALUE;
		uint32 CRU_GLB_SRST_SND_VALUE;
		uint32 CRU_SOFTRST_CON[11];
		uint32 CRU_MISC_CON;
		uint32 CRU_GLB_CNT_TH;
    } CRU_REG, *pCRU_REG;

#else

typedef volatile struct tagCRU_REG
{
    uint32 CRU_PLL_CON[4][4];
    uint32 CRU_MODE_CON;
    uint32 CRU_CLKSEL_CON[35];
    uint32 CRU_CLKGATE_CON[10];
    uint32 reserved1[2];
    uint32 CRU_GLB_SRST_FST_VALUE;
    uint32 CRU_GLB_SRST_SND_VALUE;
    uint32 reserved2[2];
    uint32 CRU_SOFTRST_CON[9];
    uint32 CRU_MISC_CON;
    uint32 reserved3[2];
    uint32 CRU_GLB_CNT_TH;
} CRU_REG, *pCRU_REG;
#endif

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
#define g_cruReg ((pCRU_REG)RK3288_CRU_PHYS)
#else
#define g_cruReg ((pCRU_REG)RK3188_CRU_PHYS)
#endif

typedef struct tagGPIO_LH
{
    uint32 GPIOL;
    uint32 GPIOH;
} GPIO_LH_T;


typedef struct tagGPIO_IOMUX
{
    uint32 GPIOA_IOMUX;
    uint32 GPIOB_IOMUX;
    uint32 GPIOC_IOMUX;
    uint32 GPIOD_IOMUX;
} GPIO_IOMUX_T;

typedef struct tagGPIO_PE
{
    uint32 GPIOA;
    uint32 GPIOB;
    uint32 GPIOC;
    uint32 GPIOD;
} GPIO_PE;


typedef volatile struct tagGRF_REG
{
    uint32 reserved[3];       
    uint32 GRF_GPIO1D_IOMUX;   //0X0C
    uint32 GRF_GPIO2A_IOMUX;   //0X10
    uint32 GRF_GPIO2B_IOMUX;
    uint32 GRF_GPIO2C_IOMUX;
    uint32 reserved2; 
    uint32 GRF_GPIO3A_IOMUX;   //0X20
    uint32 GRF_GPIO3B_IOMUX;
    uint32 GRF_GPIO3C_IOMUX;
    uint32 GRF_GPIO3DL_IOMUX;
    uint32 GRF_GPIO3DH_IOMUX; //0X30
    uint32 GRF_GPIO4AL_IOMUX;
    uint32 GRF_GPIO4AH_IOMUX;
    uint32 GRF_GPIO4BL_IOMUX;
    uint32 reserved3;           //0X40
    uint32 GRF_GPIO4C_IOMUX;
    uint32 GRF_GPIO4D_IOMUX;
    uint32 reserved4;          
    uint32 GRF_GPIO5B_IOMUX;   //0X50
    uint32 GRF_GPIO5C_IOMUX;
    uint32 GRF_GPIO6A_IOMUX;
    uint32 reserved5;  
    uint32 GRF_GPIO6B_IOMUX;   //0X60
    uint32 GRF_GPIO6C_IOMUX; 
    uint32 reserved6;
    uint32 GRF_GPIO7A_IOMUX;
    uint32 GRF_GPIO7B_IOMUX;  //0X70
    uint32 GRF_GPIO7CL_IOMU;
    uint32 GRF_GPIO7CH_IOMU;
    uint32 reserved7;
    uint32 GRF_GPIO8A_IOMUX;  //0X80
    uint32 GRF_GPIO8B_IOMUX;   
    uint32 reserved8[30];
    GPIO_LH_T GRF_GPIO_SR[8];
    GPIO_PE   GRF_GPIO_P[8];
    GPIO_PE   GRF_GPIO_E[8];
    uint32 GRF_GPIO_SMT;     //0x240
    uint32 GRF_SOC_CON[15];
    uint32 GRF_SOC_STATUS[22];  //0x0280
    uint32 GRF_PERIDMAC_CON[4];
    uint32 GRF_DDRC0_CON0;       //0x02e0
    uint32 GRF_DDRC1_CON0;
    uint32 GRF_CPU_CON[5];
    uint32 reserved9[3];          
    uint32 GRF_CPU_STATUS0;
    uint32 reserved10;
    uint32 GRF_UOC0_CON[5];  //0x320
    uint32 GRF_UOC1_CON[5];
    uint32 GRF_UOC2_CON[4];
    uint32 GRF_UOC3_CON[2];
    uint32 GRF_UOC4_CON[2];
    uint32 GRF_PVTM_CON[3];
    uint32 GRF_PVTM_STATUS[3];
    uint32 GRF_IO_VSEL;
    uint32 GRF_SARADC_TESTBIT;  
    uint32 GRF_TSADC_TESTBIT_L; 
    uint32 GRF_TSADC_TESTBIT_H;
    uint32 GRF_OS_REG[4];
    uint32 reserved11;    //0x3a0
    uint32 GRF_SOC_CON15;       
    uint32 GRF_SOC_CON16;        
}GRF_REG, *pGRF_REG;


typedef volatile struct tagTIMER_STRUCT
{
	uint32 TIMER_LOAD_COUNT0;
	uint32 TIMER_LOAD_COUNT1;
	uint32 TIMER_CURR_VALUE0;
	uint32 TIMER_CURR_VALUE1;
	uint32 TIMER_CTRL_REG;
	uint32 TIMER_INT_STATUS;
}TIMER_REG,*pTIMER_REG;

/* SDMMC Host Controller register struct */
typedef volatile struct TagSDC_REG2
{
    volatile uint32 SDMMC_CTRL;        //SDMMC Control register
    volatile uint32 SDMMC_PWREN;       //Power enable register
    volatile uint32 SDMMC_CLKDIV;      //Clock divider register
    volatile uint32 SDMMC_CLKSRC;      //Clock source register
    volatile uint32 SDMMC_CLKENA;      //Clock enable register
    volatile uint32 SDMMC_TMOUT;       //Time out register
    volatile uint32 SDMMC_CTYPE;       //Card type register
    volatile uint32 SDMMC_BLKSIZ;      //Block size register
    volatile uint32 SDMMC_BYTCNT;      //Byte count register
    volatile uint32 SDMMC_INTMASK;     //Interrupt mask register
    volatile uint32 SDMMC_CMDARG;      //Command argument register
    volatile uint32 SDMMC_CMD;         //Command register
    volatile uint32 SDMMC_RESP0;       //Response 0 register
    volatile uint32 SDMMC_RESP1;       //Response 1 register
    volatile uint32 SDMMC_RESP2;       //Response 2 register
    volatile uint32 SDMMC_RESP3;       //Response 3 register
    volatile uint32 SDMMC_MINTSTS;     //Masked interrupt status register
    volatile uint32 SDMMC_RINISTS;     //Raw interrupt status register
    volatile uint32 SDMMC_STATUS;      //Status register
    volatile uint32 SDMMC_FIFOTH;      //FIFO threshold register
    volatile uint32 SDMMC_CDETECT;     //Card detect register
    volatile uint32 SDMMC_WRTPRT;      //Write protect register
    volatile uint32 SDMMC_GPIO;        //GPIO register
    volatile uint32 SDMMC_TCBCNT;      //Transferred CIU card byte count
    volatile uint32 SDMMC_TBBCNT;      //Transferred host/DMA to/from BIU_FIFO byte count
    volatile uint32 SDMMC_DEBNCE;      //Card detect debounce register
    volatile uint32 SDMMC_USRID;        //User ID register        
    volatile uint32 SDMMC_VERID;        //Synopsys version ID register
    volatile uint32 SDMMC_HCON;         //Hardware configuration register          
    volatile uint32 SDMMC_UHS_REG;      //UHS-1 register  
    volatile uint32 SDMMC_RST_n;        //Hardware reset register
    volatile uint32 SDMMC_CARDTHRCTL;   //Card Read Threshold Enable
    volatile uint32 SDMMC_BACK_END_POWER; //Back-end Power
}SDC_REG_T2,*pSDC_REG_T2;



#define PMU_BASE_ADDR           RK3288_PMU_PHYS
#define CRU_BASE_ADDR           RK3288_CRU_PHYS
#define GRF_BASE                RK3288_GRF_PHYS
#define UART2_BASE_ADDR         RK3288_UART_DBG_PHYS
#define USB_OTG_BASE_ADDR       RK3288_USB_OTG_PHYS
#define USB_OTG_BASE_ADDR_VA    RK3288_USB_OTG_PHYS
#define NANDC_BASE_ADDR         RK3288_NANDC0_PHYS
#define EMMC_BASE_ADDR          RK3288_EMMC_PHY
#define SARADC_BASE             RK3288_SAR_ADC_PHY
#define TIMER0_BASE_ADDR        RK3288_TIMER0_PHYS
#define BOOT_ROM_CHIP_VER_ADDR  RK3288_BOOTROM_VERSION_ADDR
#define REG_FILE_BASE_ADDR      GRF_BASE



#define I2C2_BASE_ADDR          0xFF650000
#define I2C0_BASE_ADDR          0xFF650000
#define I2C1_BASE_ADDR          0xFF140000
#define I2C3_BASE_ADDR          0xFF150000
#define I2C4_BASE_ADDR          0xFF160000
#define I2C5_BASE_ADDR          0xFF170000

#define g_Time0Reg      ((pTIMER_REG)TIMER0_BASE_ADDR)
#define g_EMMCReg       ((pSDC_REG_T2)EMMC_BASE_ADDR)
#define g_grfReg 		((pGRF_REG)RK3288_GRF_PHYS)
#define g_gicdReg       ((pGICD_REG)RK3288_GIC_DIST_PHYS)
#define g_giccReg       ((pGICC_REG)RK3288_GIC_CPU_PHYS)
#define 	read_XDATA(address) 			(*((uint16 volatile*)(address)))
#define 	read_XDATA32(address)			(*((uint32 volatile*)(address)))
#define 	write_XDATA(address, value) 	(*((uint16 volatile*)(address)) = value)
#define 	write_XDATA32(address, value)	(*((uint32 volatile*)(address)) = value)

#endif /* __RK_REG_H */

