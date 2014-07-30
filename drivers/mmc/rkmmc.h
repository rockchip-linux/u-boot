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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef _RKEMMC_H
#define _RKEMMC_H

#include <common.h>
#include <asm/arch/rkplat.h>

#define BIT(nr)		(1UL << (nr))

#define	MMC_CTRL	0x00
#define MMC_PWREN	0X04
#define MMC_CLKDIV	0x08
#define MMC_CLKSRC	0x0c
#define MMC_CLKENA	0x10
#define MMC_TMOUT	0x14
#define MMC_CTYPE	0x18
#define MMC_BLKSIZ	0x1c
#define MMC_BYTCNT	0x20
#define MMC_INTMASK	0x24
#define MMC_CMDARG	0x28
#define MMC_CMD         0x2c
#define MMC_RESP0	0x30
#define MMC_RESP1	0X34
#define MMC_RESP2	0x38
#define MMC_RESP3	0x3c
#define MMC_MINTSTS	0x40
#define MMC_RINTSTS	0x44
#define MMC_STATUS	0x48
#define MMC_FIFOTH	0x4c
#define MMC_CDETECT	0x50
#define MMC_WRTPRT	0x54
#define MMC_TCBCNT	0x5c
#define MMC_TBBCNT	0x60
#define MMC_DEBNCE	0x64
#define MMC_USRID	0x68
#define MMC_VERID	0x6c
#define MMC_UHS_REG	0X74
#define MMC_RST_N	0x78
#define MMC_BMOD	0x80
#define MMC_DBADDR	0x88
#define MMC_IDSTS	0x8c
#define MMC_IDINTEN	0x90

#define MAX_RETRY_COUNT (250000)  //250ms
#define MMC_FIFO_BASE	0x200
#define MMC_DATA	MMC_FIFO_BASE

#define MAX_DESC_NUM_IDMAC	64//1024//128
#define MAX_BUFF_SIZE_IDMAC	8192
#define MAX_DATA_SIZE_IDMAC	(MAX_DESC_NUM_IDMAC*MAX_BUFF_SIZE_IDMAC)

#define CTRL_USE_IDMAC		0x02000000  
#define CTRL_IDMAC_RESET	0x00000004    

/* Bus Mode Register Bit Definitions */
#define  BMOD_SWR 		0x00000001	// Software Reset: Auto cleared after one clock cycle                                0
#define  BMOD_FB 		0x00000002	// Fixed Burst Length: when set SINGLE/INCR/INCR4/INCR8/INCR16 used at the start     1 
#define  BMOD_DE 		0x00000080	// Idmac Enable: When set IDMAC is enabled                                           7
#define  BMOD_DSL_MSK		0x0000007C	// Descriptor Skip length: In Number of Words                                      6:2 
#define  BMOD_DSL_Shift		2	        // Descriptor Skip length Shift value
#define  BMOD_DSL_ZERO          0x00000000	// No Gap between Descriptors
#define  BMOD_DSL_TWO           0x00000008	// 2 Words Gap between Descriptors
#define  BMOD_PBL		0x00000400	// MSIZE in FIFOTH Register 
/* Internal DMAC Status Register IDSTS Bit Definitions
 * Internal DMAC Interrupt Enable Register Bit Definitions */
#define  IDMAC_AI		0x00000200   // Abnormal Interrupt Summary Enable/ Status                                       9
#define  IDMAC_NI    		0x00000100   // Normal Interrupt Summary Enable/ Status                                         8
#define  IDMAC_CES		0x00000020   // Card Error Summary Interrupt Enable/ status                                     5
#define  IDMAC_DU		0x00000010   // Descriptor Unavailabe Interrupt Enable /Status                                  4
#define  IDMAC_FBE		0x00000004   // Fata Bus Error Enable/ Status                                                   2
#define  IDMAC_RI		0x00000002   // Rx Interrupt Enable/ Status                                                     1
#define  IDMAC_TI		0x00000001   // Tx Interrupt Enable/ Status       

#define  IDMAC_EN_INT_ALL   	0x00000337   // Enables all interrupts 


/* Control register defines */
#define MMC_CTRL_ABORT_READ_DATA	BIT(8)
#define MMC_CTRL_SEND_IRQ_RESPONSE	BIT(7)
#define MMC_CTRL_READ_WAIT		BIT(6)
#define MMC_CTRL_DMA_ENABLE		BIT(5)
#define MMC_CTRL_INT_ENABLE		BIT(4)
#define MMC_CTRL_DMA_RESET		BIT(2)
#define MMC_CTRL_FIFO_RESET		BIT(1)
#define MMC_CTRL_RESET			BIT(0)
/* Hardware reset register defines */
#define MMC_CARD_RESET			BIT(0)
/* Power enable register defines */
#define MMC_PWREN_ON			BIT(0)
/* Clock Enable register defines */
#define MMC_CLKEN_LOW_PWR             	BIT(16)
#define MMC_CLKEN_ENABLE              	BIT(0)
/* time-out register defines */
#define MMC_TMOUT_DATA(n)             	_SBF(8, (n))
#define MMC_TMOUT_DATA_MSK            	0xFFFFFF00
#define MMC_TMOUT_RESP(n)             	((n) & 0xFF)
#define MMC_TMOUT_RESP_MSK            	0xFF
/* card-type register defines */
#define MMC_CTYPE_8BIT                	BIT(16)
#define MMC_CTYPE_4BIT                	BIT(0)
#define MMC_CTYPE_1BIT                	0
/* Interrupt status & mask register defines */
#define MMC_INT_SDIO                  	BIT(16)
#define MMC_INT_EBE                   	BIT(15)
#define MMC_INT_ACD                   	BIT(14)
#define MMC_INT_SBE                   	BIT(13)
#define MMC_INT_HLE                   	BIT(12)
#define MMC_INT_FRUN                  	BIT(11)
#define MMC_INT_HTO                   	BIT(10)
#define MMC_INT_DTO                   	BIT(9)
#define MMC_INT_RTO                   	BIT(8)
#define MMC_INT_DCRC                  	BIT(7)
#define MMC_INT_RCRC                  	BIT(6)
#define MMC_INT_RXDR                  	BIT(5)
#define MMC_INT_TXDR                  	BIT(4)
#define MMC_INT_DATA_OVER             	BIT(3)
#define MMC_INT_CMD_DONE              	BIT(2)
#define MMC_INT_RESP_ERR              	BIT(1)
#define MMC_INT_CD                    	BIT(0)
#define MMC_INT_ERROR                 	0xbfc2
/* Command register defines */
#define MMC_CMD_START                 	BIT(31)
#define MMC_USE_HOLD_REG		BIT(29)
#define MMC_CMD_CCS_EXP               	BIT(23)
#define MMC_CMD_CEATA_RD              	BIT(22)
#define MMC_CMD_UPD_CLK               	BIT(21)
#define MMC_CMD_INIT                  	BIT(15)
#define MMC_CMD_STOP                  	BIT(14)
#define MMC_CMD_PRV_DAT_WAIT          	BIT(13)
#define MMC_CMD_SEND_STOP             	BIT(12)
#define MMC_CMD_STRM_MODE             	BIT(11)
#define MMC_CMD_DAT_WR                	BIT(10)
#define MMC_CMD_DAT_EXP               	BIT(9)
#define MMC_CMD_RESP_CRC              	BIT(8)
#define MMC_CMD_RESP_LONG		BIT(7)
#define MMC_CMD_RESP_EXP		BIT(6)
#define MMC_CMD_INDX(n)		        ((n) & 0x1F)

/* Status register defines */
#define MMC_GET_FCNT(x)		        (((x)>>17) & 0x1FF)
#define MMC_MC_BUSY			BIT(10)
#define MMC_DATA_BUSY			BIT(9)
#define MMC_FIFO_EMPTY                  BIT(2)
#define MMC_BUSY			(MMC_MC_BUSY | MMC_DATA_BUSY)

/* EMMC dma mode config */
#ifdef CONFIG_RK_MMC_DMA

#if (defined (CONFIG_RK_MMC_IDMAC) && defined (CONFIG_RK_MMC_EDMAC))
	#error "PLS check emmc dmac mode!"
#endif

#if defined (CONFIG_RK_MMC_IDMAC)
	#define USE_MMC_IDMA		1
#endif

#if defined (CONFIG_RK_MMC_EDMAC)
	#define USE_MMC_EDMA		1
#endif

#endif /* CONFIG_RK_MMC_DMA */

/* FIFO threshold register defines */
#if USE_MMC_IDMA
#define FIFO_DETH			256
#else
#define FIFO_DETH			512
#endif


/* UHS-1 register defines */
#define MMC_UHS_DDR_MODE		BIT(16)
#define MMC_UHS_VOLT_18			BIT(0)


/* Common flag combinations */
#define MMC_DATA_ERROR_FLAGS (MMC_INT_DTO | MMC_INT_DCRC | \
		                                 MMC_INT_HTO | MMC_INT_SBE  | \
		                                 MMC_INT_EBE)
#define MMC_CMD_RES_TIME_OUT  	MMC_INT_RTO                                 
#define MMC_CMD_ERROR_FLAGS  ( MMC_INT_RCRC | \
		                                 MMC_INT_RESP_ERR)
#define MMC_ERROR_FLAGS      (MMC_DATA_ERROR_FLAGS | \
		                                 MMC_CMD_ERROR_FLAGS  | MMC_INT_HLE)

#define MMC_BUS_CLOCK		96000000


#define Readl(addr)		(*(volatile u32 *)(addr))
#define Writel(addr, v)		(*(volatile u32 *)(addr) = v)

#define BLKSZ			0x200

#define gMmcBaseAddr		RKIO_EMMC_PHYS
#define gCruBaseAddr		RKIO_CRU_PHYS
#define gGrfBaseAddr		RKIO_GRF_PHYS


typedef struct tagSDMMC_DMA_DESC {
	uint32 desc0;   	 /* control and status information of descriptor */
	uint32 desc1;   	 /* buffer sizes                                 */
	uint32 desc2;   	 /* physical address of the buffer 1             */  
	uint32 desc3;    	 /* physical address of the buffer 2             */       
} SDMMC_DMA_DESC, *PSDMMC_DMA_DESC;


typedef enum {
	READ = 0,
	WRITE,
} OPERATION_TYPE;


/*----------------------------------- Typedefs -------------------------------*/
/*
 DBADDR  = 0x88 : Descriptor List Base Address Register
 The DBADDR is the pointer to the first Descriptor
 The Descriptor format in Little endian with a 32 bit Data bus is as shown below 
           --------------------------------------------------------------------------
     DES0 | OWN (31)| Control and Status                                             |
           --------------------------------------------------------------------------
     DES1 | Reserved |         Buffer 2 Size        |        Buffer 1 Size           |
           --------------------------------------------------------------------------
     DES2 |  Buffer Address Pointer 1                                                |
           --------------------------------------------------------------------------
     DES3 |  Buffer Address Pointer 2 / Next Descriptor Address Pointer              |
           --------------------------------------------------------------------------
*/
enum DmaDescriptorDES0    // Control and status word of DMA descriptor DES0 
{
	DescOwnByDma          = 0x80000000,   /* (OWN)Descriptor is owned by DMA engine              31   */
	DescCardErrSummary    = 0x40000000,   /* Indicates EBE/RTO/RCRC/SBE/DRTO/DCRC/RE             30   */
	DescEndOfRing         = 0x00000020,   /* A "1" indicates End of Ring for Ring Mode           05   */
	DescSecAddrChained    = 0x00000010,   /* A "1" indicates DES3 contains Next Desc Address     04   */
	DescFirstDesc         = 0x00000008,   /* A "1" indicates this Desc contains first            03
		                                 buffer of the data                                       */
	DescLastDesc          = 0x00000004,   /* A "1" indicates buffer pointed to by this this      02
		                                 Desc contains last buffer of Data                        */
	DescDisInt            = 0x00000002,   /* A "1" in this field disables the RI/TI of IDSTS     01
		                                 for data that ends in the buffer pointed to by 
		                                 this descriptor                                          */     
};
enum DmaDescriptorDES1    // Buffer's size field of Descriptor
{
	DescBuf2SizMsk       = 0x03FFE000,    /* Mask for Buffer2 Size                            25:13   */
	DescBuf2SizeShift    = 13,            /* Shift value for Buffer2 Size                             */
	DescBuf1SizMsk       = 0x00001FFF,    /* Mask for Buffer1 Size                            12:0    */
	DescBuf1SizeShift    = 0,             /* Shift value for Buffer2 Size                             */
};
enum DescMode {
	RINGMODE    = 0x00000001,
	CHAINMODE   = 0x00000002,
};
enum BufferMode {
	SINGLEBUF   = 0x00000001,
	DUALBUF     = 0x00000002,
};
enum DmaAccessType {
	TO_DEVICE       = 0x00000001,
	FROM_DEVICE     = 0x00000002,
	BIDIRECTIONAL   = 0x00000003,
};
/* EMMC Host Controller register struct */
typedef volatile struct TagSDC_REG {
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
	volatile uint32 reserved;          //reserved
	volatile uint32 SDMMC_TCBCNT;      //Transferred CIU card byte count
	volatile uint32 SDMMC_TBBCNT;      //Transferred host/DMA to/from BIU_FIFO byte count
	volatile uint32 SDMMC_DEBNCE;      //Card detect debounce register
	volatile uint32 SDMMC_USRID;        //User ID register        
	volatile uint32 SDMMC_VERID;        //Synopsys version ID register
	volatile uint32 SDMMC_HCON;         //Hardware configuration register          
	volatile uint32 SDMMC_UHS_REG;      //UHS-1 register  
	volatile uint32 SDMMC_RST_n;        //Hardware reset register
	volatile uint32 SDMMC_RESERVED0;
	volatile uint32 SDMMC_BMOD;        //HBus Mode Register
	volatile uint32 SDMMC_PLDMND;      //Poll Demand Register
	volatile uint32 SDMMC_DBADDR;      //Descriptor List Base Address Register
	volatile uint32 SDMMC_IDSTS;        //Internal DMAC Status Register
	volatile uint32 SDMMC_IDINTEN;      // Internal DMAC Interrupt Enable Register
	volatile uint32 SDMMC_DSCADDR;      //Current Host Descriptor Address Register
	volatile uint32 SDMMC_BUFADDR;      //Current Buffer Descriptor Address Register
	volatile uint32 SDMMC_RESERVED1[(0x100-0x9C)/4];
	volatile uint32 SDMMC_CARDTHRCTL;   //Card Threshold Control Register
	volatile uint32 SDMMC_BACK_END_POWER; //Back-end Power Register
	volatile uint32 SDMMC_UHS_REG_EXT;      //UHS Register
	volatile uint32 SDMMC_EMMC_DDR_REG;     //eMMC 4.5 DDR START Bit Detection Control Register
	volatile uint32 SDMMC_ENABLE_SHIFT;     //Enable Phase Shift Register
} *pSDC_REG_T;


int rk_mmc_init(void);

#endif /* _RKEMMC_H */

