/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    epphal.h
Author:		    XUESHAN LIN
Created:        1st Dec 2008
Modified:
Revision:		1.00
********************************************************************************
********************************************************************************/
#ifndef _RK30REG_H
#define _RK30REG_H
    #define     read_XDATA(address)             (*((uint16 volatile*)(address)))
    #define     read_XDATA32(address)           (*((uint32 volatile*)(address)))
    #define     write_XDATA(address, value)     (*((uint16 volatile*)(address)) = value)
    #define     write_XDATA32(address, value)   (*((uint32 volatile*)(address)) = value)

    #define     USB_OTG_INT_CH                  (16+32)
    #define     NANDC_INT_CH                    (USB_OTG_INT_CH+11)
    #define     NAND_RDY_INT_CH                 (USB_OTG_INT_CH+12)
    
    typedef enum _CLK_GATE
    {   
        /* SCU CLK GATE 0 CON */    
        CLK_GATE_CORE = 0,  
        CLK_GATE_CORE_APB,  
        CLK_GATE_CORE_ATB,  
        CLK_GATE_CPU_AXI,   
        CLK_GATE_CPU_AXI2,  
        CLK_GATE_CPU_AHB,   
        CLK_GATE_CPU_MATRIX1_AHB,   
        CLK_GATE_CPU_APB,   
        CLK_GATE_CPU_ATB,   
        CLK_GATE_DMA0,  
        CLK_GATE_DMA1,  
        CLK_GATE_GIC,   
        CLK_GATE_IMEM,  
        CLK_GATE_EBROM = 14,    
        CLK_GATE_I2S0,  
        CLK_GATE_I2S1,  
        CLK_GATE_SPDIF, 
        CLK_GATE_DDR_PHY,   
        CLK_GATE_DDR_REG,   
        CLK_GATE_DDR_CPU,   
        CLK_GATE_EFUSE, 
        CLK_GATE_TZPC,  
        CLK_GATE_TIMER0,    
        CLK_GATE_GPIO0, 
        CLK_GATE_UART0, 
        CLK_GATE_I2C0,  
        CLK_GATE_DEBUG, 
        CLK_GATE_TPIU,  
        CLK_GATE_RTC,   
        CLK_GATE_PMU,   
        CLK_GATE_GRF,   
    
        /* SCU CLK GATE 1 CON */    
        CLK_GATE_PEIRPH_AXI = 32,   
        CLK_GATE_PEIRPH_AHB,    
        CLK_GATE_PEIRPH_APB,    
        CLK_GATE_EMEM,  
        CLK_GATE_USB,   
        CLK_GATE_DMA2,  
        CLK_GATE_DDR_PERIPH,    
        CLK_GATE_PERIPH,    
    
        /* FIXME */ 
        CLK_GATE_SMC_AXI,   
        CLK_GATE_SMC,   
        CLK_GATE_MAC_AHB = 43,  
        CLK_GATE_MAC_PHY,   
        CLK_GATE_MAC_TX,    
        CLK_GATE_MAC_RX,    
        CLK_GATE_HIF,   
        CLK_GATE_NANDC, 
        CLK_GATE_HSADC_AHB, 
        CLK_GATE_HSADC, 
        CLK_GATE_SDMMC0_AHB,    
        CLK_GATE_SDMMC0,    
        CLK_GATE_SDMMC1_AHB,    
        CLK_GATE_SDMMC1,    
        CLK_GATE_EMMC_AHB,  
        CLK_GATE_EMMC,  
        CLK_GATE_USBOTG0,   
        CLK_GATE_USBPHY0,   
        CLK_GATE_USBOTG1,   
        CLK_GATE_USBPHY1,   
        CLK_GATE_UHOST_AHB, 
        CLK_GATE_UHOST, 
        CLK_GATE_PID_FILTER,    
    
        /* SCU CLK GATE 2 CON */    
        CLK_GATE_UART1 = 64,    
        CLK_GATE_UART2, 
        CLK_GATE_UART3,
        CLK_GATE_TIMER1,    
        CLK_GATE_TIMER2,    
        CLK_GATE_TIMER3,    
        CLK_GATE_GPIO1, 
        CLK_GATE_GPIO2, 
        CLK_GATE_GPIO3,
        CLK_GATE_GPIO4, 
        CLK_GATE_GPIO5, 
        CLK_GATE_GPIO6, 
        CLK_GATE_I2C1,  
        CLK_GATE_I2C2,  
        CLK_GATE_I2C3,  
        CLK_GATE_SPI0,  
        CLK_GATE_SPI1,  
        CLK_GATE_VIP_SLAVE = 82,    
        CLK_GATE_WDT,   
        CLK_GATE_SARADC,    
        CLK_GATE_PWM,   
        CLK_GATE_VIP_BUS,   
        CLK_GATE_VIP_MATRIX,    
        CLK_GATE_VIP,   
        CLK_GATE_VIP_INPUT, 
        CLK_GATE_JTAG,  
    
        /* CRU CLK GATE 3 CON */    
        CLK_GATE_LCDC_AXI = 96, 
        CLK_GATE_DDR_LCDC_AXI,  
        CLK_GATE_LCDC_AHB,  
        CLK_GATE_LCDC,  
        CLK_GATE_IPP_AXI,   
        CLK_GATE_IPP_AHB,   
        CLK_GATE_EBOOK_AHB, 
        CLK_GATE_EBOOK, 
        CLK_GATE_DISPLAY_MATRIX_AXI,    
        CLK_GATE_DISPLAY_MATRIX_AHB,    
        CLK_GAET_VEPU_AXI,  
        CLK_GATE_DDR_VEDU_AXI,  
        CLK_GATE_VDPU_AXI,  
        CLK_GATE_DDR_VDPU_AXI,  
        CLK_GATE_GPU,   
        CLK_GATE_GPU_AXI,   
        CLK_GATE_DDR_GPU_AXI,   
        CLK_GATE_GPU_AHB,   
        CLK_GATE_VEPU_AHB,  
        CLK_GATE_VDPU_AHB,  
        CLK_GATE_CPU_VCODEC_AHB,    
        CLK_GATE_CPU_DISPLAY_AHB,   
        CLK_GATE_MAX
    }eCLK_GATE;


//1寄存器结构定义
    //INTC Registers
    typedef volatile struct tagGICD_REG
    {
        uint32 ICDDCR 	      ;      //0x000 
        uint32 ICDICTR 	      ;    //0x004   
        uint32 ICDIIDR 	      ;    //0x008
        uint32 RESERVED0[29]  ; 
        uint32 ICDISR[4] 	    ;   //   0x080  
        uint32 RESERVED1[28]  ;
        uint32 ICDISER[4]   	;     // 0x100 
        uint32 RESERVED2[28]  ;   
        uint32 ICDICER[4] 	  ;        //0x180   
        uint32 RESERVED3[28]  ;
        uint32 ICDISPR[4]   	;      //0x200  
        uint32 RESERVED4[28]  ;
        uint32 ICDICPR[4] 	  ;      //0x280   
        uint32 RESERVED5[28]  ;
        uint32 ICDIABR[4] 	  ;        //0x300
        uint32 RESERVED6[60]  ;
        uint32 ICDIPR_SGI[4] 	;       // 0x400
        uint32 ICDIPR_PPI[4] 	;       // 0x410 
        uint32 ICDIPR_SPI[18] ;	        //0x420
        uint32 RESERVED7[230];
        uint32 ITARGETSR[255];        //0x800
        uint32 RESERVED9[1] ;
        uint32 ICDICFR[7]     ;        //0xc00
        uint32 RESERVED8[185] ;
        uint32 ICDSGIR 	      ;        //0xf00 
    }GICD_REG, *pGICD_REG;	
    typedef volatile struct tagGICC_REG
    {
        uint32 ICCICR 	     ;         //0x00 
        uint32 ICCPMR 	     ;         //0x04 
        uint32 ICCBPR 	     ;         //0x08 
        uint32 ICCIAR 	     ;         //0x0c 
        uint32 ICCEOIR      ;         //0x10 
        uint32 ICCRPR 	     ;         //0x14 
        uint32 ICCHPIR      ;         //0x18 
        uint32 ICCABPR      ;         //0x1c 
        uint32 RESERVED0[55];
        uint32 ICCIIDR      ;         //0xfc  
    }GICC_REG, *pGICC_REG;
    
    //CRU Registers
    typedef volatile struct tagCRU_STRUCT 
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

#define g_cruReg ((pCRU_REG)CRU_BASE_ADDR)

    typedef struct tagGPIO_LH
    {
        uint32 GPIOL;
        uint32 GPIOH;
    }GPIO_LH_T;
    
    typedef struct tagGPIO_IOMUX
    {
        uint32 GPIOA_IOMUX;
        uint32 GPIOB_IOMUX;
        uint32 GPIOC_IOMUX;
        uint32 GPIOD_IOMUX;
    }GPIO_IOMUX_T;

    //REG FILE registers
    typedef volatile struct tagGRF_REG
    {
        GPIO_LH_T GRF_GPIO_DIR[7];
        GPIO_LH_T GRF_GPIO_DO[7];
        GPIO_LH_T GRF_GPIO_EN[7];
        GPIO_IOMUX_T GRF_GPIO_IOMUX[7];
        GPIO_LH_T GRF_GPIO_PULL[7];
        uint32 GRF_SOC_CON[3];
        uint32 GRF_SOC_STATUS0;
        uint32 GRF_DMAC1_CON[3];
        uint32 GRF_DMAC2_CON[4];
        uint32 GRF_UOC0_CON[3];
        uint32 GRF_UOC1_CON[4];
        uint32 GRF_DDRC_CON0;
        uint32 GRF_DDRC_STAT;
        uint32 reserved[(0x1c8-0x1a0)/4];
        uint32 GRF_OS_REG[4];
    } GRF_REG, *pGRF_REG;
    #define g_grfReg ((pGRF_REG)GRF_BASE)

    typedef volatile struct tagRK3066B_GRF_REG
    {
        GPIO_LH_T GRF_GPIO_DIR[4];
        GPIO_LH_T GRF_GPIO_DO[4];
        GPIO_LH_T GRF_GPIO_EN[4];
        GPIO_IOMUX_T GRF_GPIO_IOMUX[4];
        uint32 GRF_SOC_CON[3];
        uint32 GRF_SOC_STATUS0;
        uint32 GRF_DMAC1_CON[3];
        uint32 GRF_DMAC2_CON[4];
        uint32 GRF_CPU_CON[6]; //  no use
        uint32 GRF_CPU_STAT[2]; //  no use
        uint32 GRF_DDRC_CON0;
        uint32 GRF_DDRC_STAT;
        uint32 GRF_IO_CON[5];
        uint32 reserved0;
        uint32 GRF_UOC0_CON[4];
        uint32 GRF_UOC1_CON[4];
        uint32 GRF_UOC2_CON[3];
        uint32 GRF_UOC3_CON[2];
        uint32 GRF_HSIC_STAT;
        uint32 GRF_OS_REG[8];
    } RK3066B_GRF_REG, *pRK3066B_GRF_REG;
   
    #define g_3066B_grfReg ((pRK3066B_GRF_REG)GRF_BASE)
    
    //SDMMC0
typedef enum
{
    IOMUX_SDMMC_1BIT = 0,
    IOMUX_SDMMC_4BIT,  //default
    IOMUX_SDMMC_8BIT,
    IOMUX_SDMMC_OTHER
}eIOMUX_SDMMC;
    typedef enum _IRQ_NUM
    {
        INT_SGI0        ,
        INT_SGI1        ,
        INT_SGI2        ,
        INT_SGI3        ,
        INT_SGI4        ,
        INT_SGI5        ,
        INT_SGI6        ,
        INT_SGI7        ,
        INT_SGI8        ,
        INT_SGI9        ,
        INT_SGI10       ,
        INT_SGI11       ,
        INT_SGI12       ,
        INT_SGI13       ,
        INT_SGI14       ,
        INT_SGI15       ,
        INT_PPI0        ,
        INT_PPI1        ,
        INT_PPI2        ,
        INT_PPI3        ,
        INT_PPI4        ,
        INT_PPI5        ,
        INT_PPI6        ,
        INT_PPI7        ,
        INT_PPI8        ,
        INT_PPI9        ,
        INT_PPI10       ,
        INT_PPI11       ,
        INT_PPI12       ,
        INT_PPI13       ,
        INT_PPI14       ,
        INT_PPI15       ,
        
        INT_DMAC0_0     ,
        INT_DMAC0_1     ,
        INT_DMAC0_2     ,
        INT_DMAC0_3     ,
        INT_DMAC2_0     ,
        INT_DMAC2_1     ,
        INT_DMAC2_2     ,
        INT_DMAC2_3     ,
        INT_DMAC2_4     ,
        INT_GPU         ,
        INT_VEPU        ,
        INT_VDPU        ,
        INT_VIP         ,
        INT_LCDC        ,
        INT_IPP         ,
        INT_EBC         ,
        INT_USB_OTG0    ,
        INT_USB_OTG1    ,
        INT_USB_Host    ,
        INT_MAC         ,
        INT_HIF0        ,
        INT_HIF1        ,
        INT_HSADC_TSI   ,
        INT_SDMMC       ,
        INT_SDIO        ,
        INT_eMMC        ,
        INT_SARADC      ,
        INT_NandC       ,
        INT_NandCRDY    ,
        INT_SMC         ,
        INT_PID_FILTER  ,
        INT_I2S_PCM_8CH ,
        INT_I2S_PCM_2CH ,
        INT_SPDIF       ,
        INT_UART0       ,
        INT_UART1       ,
        INT_UART2       ,
        INT_UART3       ,
        INT_SPI0        ,
        INT_SPI1        ,
        INT_I2C0        ,
        INT_I2C1        ,
        INT_I2C2        ,
        INT_I2C3        ,
        INT_TIMER0      ,
        INT_TIMER1      ,
        INT_TIMER2      ,
        INT_TIMER3      ,
        INT_PWM0        ,
        INT_PWM1        ,
        INT_PWM2        ,
        INT_PWM3        ,
        INT_WDT         ,
        INT_RTC         ,
        INT_PMU         ,
        INT_GPIO0       ,
        INT_GPIO1       ,
        INT_GPIO2       ,
        INT_GPIO3       ,
        INT_GPIO4       ,
        INT_GPIO5       ,
        INT_GPIO6       ,
        INT_USB_AHB_ARB ,
        INT_PERI_AHB_ARB,
        INT_A8IRQ0      ,
        INT_A8IRQ1      ,
        INT_A8IRQ2      ,
        INT_A8IRQ3      ,
        INT_MAXNUM      
    }eINT_NUM;

    typedef enum _CRU_RST
    {
    //cru_rst_con0
        CRU_RST_ARMCORE = 0,
        CRU_RST_CPUSUBSYS_INT1AXI,
        CRU_RST_CPUSUBSYS_INT1AHB,
        CRU_RST_CPUSUBSYS_INT1APB,
        CRU_RST_CPUSUBSYS_INT1ATB,
        CRU_RST_CPUSUBSYS_INT2,
        CRU_RST_DMA0,
        CRU_RST_DMA1,
        CRU_RST_GIC,
        CRU_RST_INMEM,
        CRU_RST_TZPC=11,
        CRU_RST_ROM,
        CRU_RST_I2S0,
        CRU_RST_I2S1,
        CRU_RST_SPDIF,
        CRU_RST_UART0,
        CRU_RST_RTC,
        CRU_RST_DDRPHY,
        CRU_RST_DDRDLL_B0,
        CRU_RST_DDRDLL_B1,
        CRU_RST_DDRDLL_B2,
        CRU_RST_DDRDLL_B3,
        CRU_RST_DDRDLL_CMD,
        CRU_RST_DDR_CON,
        CRU_RST_ARMCORE_DEBUG,
        CRU_RST_DAP_DBG,
        CRU_RST_CPU_VODEC_A2A,
        CRU_RST_CPU_DISPLAY_A2A,
        CRU_RST_DAP_SYS,
    
    // cru_softrst1_con
        CRU_RST_PERIPH_INT1_AXI=32,
        CRU_RST_PERIPH_INT1_AHB,
        CRU_RST_PERIPH_INT1_APB,
        CRU_RST_PERIPH_EMEM=36,
        CRU_RST_PERIPH_USB,
        CRU_RST_DMA2,
        CRU_RST_MAC,
        CRU_RST_HIF,
        CRU_RST_NANDC,
        CRU_RST_SMC,
        CRU_RST_RESERVED1,
        CRU_RST_LSADC,
        CRU_RST_SDMMC0,
        CRU_RST_SDMMC1,
        CRU_RST_EMMC,
        CRU_RST_USBOTG0_AHB,
        CRU_RST_USBPHY0,
        CRU_RST_USBOTG0_CON,
        CRU_RST_USBOTG1_AHB,
        CRU_RST_USBPHY1,
        CRU_RST_USBOTG1_CON,
        CRU_RST_UHOST,
        CRU_RST_VIP,
        CRU_RST_VIP_AHB,
        CRU_RST_SPI0,
        CRU_RST_SPI1,
        CRU_RST_SARADC,
        CRU_RST_UART1,
        CRU_RST_UART2,
        CRU_RST_UART3,
        CRU_RST_PWM,
    
    //CRU_SOFTRST2_CON
        CRU_RST_DISPLAY_AXI=64,
        CRU_RST_DISPLAY_AHB,
        CRU_RST_LCDC,
        CRU_RST_IPP,
        CRU_RST_EBOOK,
        CRU_RST_RESERVED2,
        CRU_RST_RESERVED3,
        CRU_RST_GPU,
        CRU_RST_DDR_REG,
        CRU_RST_DDR_CPU,
        CRU_RST_PERIPH_CPU_AXI,
        CRU_RST_DDR_PERIPH,
        CRU_RST_DDR_LCDC,
        CRU_RST_RESERVED4,
        CRU_RST_RESERVED5,
        CRU_RST_DDR_VCODEC,
        CRU_RST_DDR_GPU,
        CRU_RST_PID_FILTER,
        CRU_RST_VCODEC_AXI,
        CRU_RST_VCODEC_AHB,
        CRU_RST_TIMER0,
        CRU_RST_TIMER1,
        CRU_RST_TIMER2,
        CRU_RST_TIMER3,
        
        CRU_RST_MAX
    }eCRU_RST;

    //GPIO Registers
    typedef volatile struct tagGPIO_STRUCT
    {
        uint32 GPIO_SWPORTA_DR;
        uint32 GPIO_SWPORTA_DDR;
        uint32 RESERVED1;
        uint32 GPIO_SWPORTB_DR;
        uint32 GPIO_SWPORTB_DDR;
        uint32 RESERVED2;
        uint32 GPIO_SWPORTC_DR;
        uint32 GPIO_SWPORTC_DDR;
        uint32 RESERVED3;
        uint32 GPIO_SWPORTD_DR;
        uint32 GPIO_SWPORTD_DDR;
        uint32 RESERVED4;
        uint32 GPIO_INTEN;
        uint32 GPIO_INTMASK;
        uint32 GPIO_INTTYPE_LEVEL;
        uint32 GPIO_INT_POLARITY;
        uint32 GPIO_INT_STATUS;
        uint32 GPIO_INT_RAWSTATUS;
        uint32 GPIO_DEBOUNCE;
        uint32 GPIO_PORTS_EOI;
        uint32 GPIO_EXT_PORTA;
        uint32 GPIO_EXT_PORTB;
        uint32 GPIO_EXT_PORTC;
        uint32 GPIO_EXT_PORTD;
        uint32 GPIO_LS_SYNC;
    }GPIO_REG,*pGPIO_REG;

    //TIMER Registers
    typedef volatile struct tagTIMER_STRUCT
    {
        uint32 TIMER_LOAD_COUNT;
        uint32 TIMER_CURR_VALUE;
        uint32 TIMER_CTRL_REG;
        uint32 TIMER_EOI;
        uint32 TIMER_INT_STATUS;
    }TIMER_REG,*pTIMER_REG;
    
    typedef volatile struct tagRK3188TIMER_STRUCT
    {
        uint32 TIMER_LOAD_COUNT0;
        uint32 TIMER_LOAD_COUNT1;
        uint32 TIMER_CURR_VALUE0;
        uint32 TIMER_CURR_VALUE1;
        uint32 TIMER_CTRL_REG;
        uint32 TIMER_INT_STATUS;
    }RK3188TIMER_REG,*pRK3188TIMER_REG;

#define g_rk30Time0Reg ((pTIMER_REG)TIMER0_BASE_ADDR)
#define g_rk3188Time0Reg ((pRK3188TIMER_REG)TIMER0_BASE_ADDR)
    
    typedef volatile struct tagPMU_REG
    {
        uint32 PMU_WAKEUP_EN0;
        uint32 PMU_WAKEUP_EN1;
        uint32 PMU_WAKEUP_EN2;
        uint32 reserved1;
        uint32 PMU_PG_CON;
        uint32 PMU_MISC_CON;
        uint32 PMU_PLL_CNT;
        uint32 PMU_PD_ST;
        uint32 PMU_INT_ST;
    }PMU_REG,*pPMU_REG;

    typedef struct tagSCU_CLK_INFO
    {
        uint32 armFreq;     //ARM PLL FREQ
        uint32 dspFreq;     //DSP PLL FREQ
        uint32 AuxFreq;   //AUX PLL FREQ
        uint32 ahbDiv;
        uint32 apbDiv;
        uint32 armFreqLast;
    }SCU_CLK_INFO,*pSCU_CLK_INFO;

#define g_gicdReg ((pGICD_REG)GIC_PERI_BASE_ADDR)
#define g_giccReg ((pGICC_REG)GIC_CPU_BASE_ADDR)

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

#define g_EMMCReg ((pSDC_REG_T2)EMMC_BASE_ADDR)

//1全局变量
#undef	EXT
#ifdef	IN_EPPHAL
		#define	EXT
#else
		#define	EXT		extern
#endif		

//1函数原型声明

    extern	void    EnableIRQ(void);
    extern	void    DisableIRQ(void);
    extern	void    EnableOtgIntr(void);
    extern	void    DisableOtgIntr(void);
    extern	void    InterruptInit(void);
#endif
	
