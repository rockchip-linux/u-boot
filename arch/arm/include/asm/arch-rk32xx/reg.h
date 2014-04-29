/*
 * (C) Copyright 2008-2014 Rockchip Electronics
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
#ifndef __RK_REG_H
#define __RK_REG_H




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
	INT_SPI0 = 76		,
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
	INT_TIMER6CH0 = 98 ,
	INT_TIMER6CH1	,
	INT_TIMER6CH2	,
	INT_TIMER6CH3	,
	INT_TIMER6CH4	,
	INT_TIMER6CH5	,
	INT_TIMER2CH0	,
	INT_TIMER2CH1	,
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
} eINT_NUM;

/* PMU registers */
typedef volatile struct tagPMU_REG
{
	uint32 pmu_wakeup_cfg[2];
	uint32 pmu_pwrdn_con;
	uint32 pmu_pwrdn_st;
	uint32 pmu_idle_req;
	uint32 pmu_idle_st;
	uint32 pmu_pwrmode_con;
	uint32 pmu_pwr_state;
	uint32 pmu_osc_cnt;
	uint32 pmu_pll_cnt;
	uint32 pmu_stabl_cnt;
	uint32 pmu_ddr0io_pwron_cnt;
	uint32 pmu_ddr1io_pwron_cnt;
	uint32 pmu_core_pwrdwn_cnt;
	uint32 pmu_core_pwrup_cnt;
	uint32 pmu_gpu_pwrdwn_cnt;
	uint32 pmu_gpu_pwrup_cnt;
	uint32 pmu_wakeup_rst_clr_cnt;
	uint32 pmu_sft_con;
	uint32 pmu_ddr_sref_st;
	uint32 pmu_int_con;
	uint32 pmu_int_st;
	uint32 pmu_boot_addr_sel;
	uint32 pmu_grf_con;
	uint32 pmu_gpio_sr;
	uint32 pmu_gpio0_a_pull;
	uint32 pmu_gpio0_b_pull;
	uint32 pmu_gpio0_c_pull;
	uint32 pmu_gpio0_a_drv;
	uint32 pmu_gpio0_b_drv;
	uint32 pmu_gpio0_c_drv;
	uint32 pmu_gpio_op;
	uint32 pmu_gpio0_sel18;
	uint32 pmu_gpio0_a_iomux;
	uint32 pmu_gpio0_b_iomux;
	uint32 pmu_gpio0_c_iomux;
	uint32 pmu_gpio0_d_iomux;
	uint32 pmu_pmu_sys_reg[4];
} PMU_REG, *pPMU_REG;

//CRU Registers
typedef volatile struct tagCRU_STRUCT 
{
	uint32 cru_pll_con[5][4]; 
	uint32 cru_mode_con;
	uint32 reserved1[3];
	uint32 cru_clksel_con[43];
	uint32 reserved2[21];
	uint32 cru_clkgate_con[19];
	uint32 reserved3[1];
	uint32 cru_glb_srst_fst_value;
	uint32 cru_glb_srst_snd_value;
	uint32 cru_softrst_con[12];
	uint32 cru_misc_con;
	uint32 cru_glb_cnt_th;
	uint32 cru_glb_rst_con;
	uint32 reserved4[1];
	uint32 cru_glb_rst_st;
	uint32 reserved5[1];
	uint32 cru_sdmmc_con[2];
	uint32 cru_sdio0_con[2];
	uint32 cru_sdio1_con[2];
	uint32 cru_emmc_con[2];
} CRU_REG, *pCRU_REG;



typedef struct tagGPIO_LH
{
	uint32 GPIOL;
	uint32 GPIOH;
} GPIO_LH_T;

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
	uint32 grf_gpio1d_iomux;   //0x0c
	uint32 grf_gpio2a_iomux;   //0x10
	uint32 grf_gpio2b_iomux;
	uint32 grf_gpio2c_iomux;
	uint32 reserved2;
	uint32 grf_gpio3a_iomux;   //0x20
	uint32 grf_gpio3b_iomux;
	uint32 grf_gpio3c_iomux;
	uint32 grf_gpio3dl_iomux;
	uint32 grf_gpio3dh_iomux; //0x30
	uint32 grf_gpio4al_iomux;
	uint32 grf_gpio4ah_iomux;
	uint32 grf_gpio4bl_iomux;
	uint32 reserved3;           //0x40
	uint32 grf_gpio4c_iomux;
	uint32 grf_gpio4d_iomux;
	uint32 reserved4;
	uint32 grf_gpio5b_iomux;   //0x50
	uint32 grf_gpio5c_iomux;
	uint32 grf_gpio6a_iomux;
	uint32 reserved5;
	uint32 grf_gpio6b_iomux;   //0x60
	uint32 grf_gpio6c_iomux;
	uint32 reserved6;
	uint32 grf_gpio7a_iomux;
	uint32 grf_gpio7b_iomux;  //0x70
	uint32 grf_gpio7cl_iomu;
	uint32 grf_gpio7ch_iomu;
	uint32 reserved7;
	uint32 grf_gpio8a_iomux;  //0x80
	uint32 grf_gpio8b_iomux;
	uint32 reserved8[30];
	GPIO_LH_T grf_gpio_sr[8];
	GPIO_PE   grf_gpio_p[8];
	GPIO_PE   grf_gpio_e[8];
	uint32 grf_gpio_smt;     //0x240
	uint32 grf_soc_con[15];
	uint32 grf_soc_status[22];  //0x0280
	uint32 grf_peridmac_con[4];
	uint32 grf_ddrc0_con0;       //0x02e0
	uint32 grf_ddrc1_con0;
	uint32 grf_cpu_con[5];
	uint32 reserved9[3];
	uint32 grf_cpu_status0;
	uint32 reserved10;
	uint32 grf_uoc0_con[5];  //0x320
	uint32 grf_uoc1_con[5];
	uint32 grf_uoc2_con[4];
	uint32 grf_uoc3_con[2];
	uint32 grf_uoc4_con[2];
	uint32 grf_pvtm_con[3];
	uint32 grf_pvtm_status[3];
	uint32 grf_io_vsel;
	uint32 grf_saradc_testbit;
	uint32 grf_tsadc_testbit_l;
	uint32 grf_tsadc_testbit_h;
	uint32 grf_os_reg[4];
	uint32 reserved11;    //0x3a0
	uint32 grf_soc_con15;
	uint32 grf_soc_con16;
} GRF_REG, *pGRF_REG;


typedef volatile struct tagTIMER_STRUCT
{
	uint32 TIMER_LOAD_COUNT0;
	uint32 TIMER_LOAD_COUNT1;
	uint32 TIMER_CURR_VALUE0;
	uint32 TIMER_CURR_VALUE1;
	uint32 TIMER_CTRL_REG;
	uint32 TIMER_INT_STATUS;
} TIMER_REG, *pTIMER_REG;


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



#define PMU_BASE_ADDR           RKIO_PMU_PHYS
#define CRU_BASE_ADDR           RKIO_CRU_PHYS
#define REG_FILE_BASE_ADDR      RKIO_GRF_PHYS
#define UART2_BASE_ADDR         RKIO_UART2_DBG_PHYS
#define USB_OTG_BASE_ADDR       RKIO_USBOTG_PHYS
#define USB_OTG_BASE_ADDR_VA    RKIO_USBOTG_PHYS
#define NANDC_BASE_ADDR         RKIO_NANDC0_PHYS
#define EMMC_BASE_ADDR          RKIO_EMMC_PHYS
#define SARADC_BASE             RKIO_SARADC_PHYS
#define TIMER0_BASE_ADDR        RKIO_TIMER_6CH_PHYS
#define BOOT_ROM_CHIP_VER_ADDR  RKIO_ROM_CHIP_VER_ADDR



#define I2C2_BASE_ADDR          0xFF650000
#define I2C0_BASE_ADDR          0xFF650000
#define I2C1_BASE_ADDR          0xFF140000
#define I2C3_BASE_ADDR          0xFF150000
#define I2C4_BASE_ADDR          0xFF160000
#define I2C5_BASE_ADDR          0xFF170000


#define g_cruReg 	((pCRU_REG)RKIO_CRU_PHYS)
#define g_pmuReg 	((pPMU_REG)RKIO_PMU_PHYS)
#define g_grfReg 	((pGRF_REG)RKIO_GRF_PHYS)
#define g_Time0Reg      ((pTIMER_REG)RKIO_TIMER_6CH_PHYS)
#define g_EMMCReg       ((pSDC_REG_T2)RKIO_EMMC_PHYS)


#define read_XDATA(address) 		(*((uint16 volatile*)(address)))
#define read_XDATA32(address)		(*((uint32 volatile*)(address)))
#define write_XDATA(address, value) 	(*((uint16 volatile*)(address)) = value)
#define write_XDATA32(address, value)	(*((uint32 volatile*)(address)) = value)

#endif /* __RK_REG_H */

