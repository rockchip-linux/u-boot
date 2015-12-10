/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SDC_H_
#define _SDC_H_

#include "../config.h"

#define SDC0_ADDR		SDMMC_BASE_ADDR
#define SDC1_ADDR		SDIO_BASE_ADDR
#define SDC2_ADDR		EMMC_BASE_ADDR

#define	SD_FIFO_OFFSET		0x200
#define	FIFO_DEPTH		0x100       /* FIFO depth = 256 word  */

#define SDC0_FIFO_ADDR		(SDC0_ADDR + SD_FIFO_OFFSET)
#define SDC1_FIFO_ADDR		(SDC1_ADDR + SD_FIFO_OFFSET)
#define SDC2_FIFO_ADDR		(SDC2_ADDR + SD_FIFO_OFFSET)


/***************************************************************/
/* 可配置的参数 */
/***************************************************************/
/* FIFO watermark */
#define RX_WMARK          (FIFO_DEPTH/2-1)  /* RX watermark level set to 127 */
#define TX_WMARK          (FIFO_DEPTH/2)    /* TX watermark level set to  128 */


/***************************************************************/
/* 不可配置的参数 */
/***************************************************************/
/* SDMMC Control Register */
#define ENABLE_DMA        (1 << 5)     /* Enable DMA transfer mode */
#define ENABLE_INT        (1 << 4)     /* Enable interrupt */
#define DMA_RESET         (1 << 2)     /* FIFO reset */
#define FIFO_RESET        (1 << 1)     /* FIFO reset */
#define SDC_RESET         (1 << 0)     /* controller reset */

/* Power Enable Register */
#define POWER_ENABLE      (1 << 0)     /* Power enable */

/* SDMMC Clock source Register */
#define CLK_DIV_SRC_0     (0x0)        /* clock divider 0 selected */
#define CLK_DIV_SRC_1     (0x1)        /* clock divider 1 selected */
#define CLK_DIV_SRC_2     (0x2)        /* clock divider 2 selected */
#define CLK_DIV_SRC_3     (0x3)        /* clock divider 3 selected */

/* Clock Enable Register */
#define CCLK_LOW_POWER    (1 << 16)    /* Low-power control for SD/MMC card clock */
#define NO_CCLK_LOW_POWER (0 << 16)    /* low-power mode disabled */
#define CCLK_ENABLE       (1 << 0)     /* clock enable control for SD/MMC card clock */
#define CCLK_DISABLE      (0 << 0)     /* clock disabled */

/* Card Type Register */
#define BUS_1_BIT         (0x0)
#define BUS_4_BIT         (0x1)
#define BUS_8_BIT         (0x10000)

/* interrupt mask bit */
#define SDIO_INT          (1 << 24)    /* SDIO interrupt */
#define BDONE_INT         (1 << 16)    /* busy Done interrupt */

#define EBE_INT           (1 << 15)    /* End Bit Error(read)/Write no CRC */
#define ACD_INT           (1 << 14)    /* Auto Command Done */
#define SBE_INT           (1 << 13)    /* Start Bit Error */
#define HLE_INT           (1 << 12)    /* Hardware Locked Write Error */
#define FRUN_INT          (1 << 11)    /* FIFO Underrun/Overrun Error */
#define HTO_INT           (1 << 10)    /* Data Starvation by Host Timeout */
#define VSWTCH_INT        (1 << 10)    /* Volt Switch interrupt */
#define DRTO_INT          (1 << 9)     /* Data Read TimeOut */
#define RTO_INT           (1 << 8)     /* Response TimeOut */
#define DCRC_INT          (1 << 7)     /* Data CRC Error */
#define RCRC_INT          (1 << 6)     /* Response CRC Error */
#define RXDR_INT          (1 << 5)     /* Receive FIFO Data Request */
#define TXDR_INT          (1 << 4)     /* Transmit FIFO Data Request */
#define DTO_INT           (1 << 3)     /* Data Transfer Over */
#define CD_INT            (1 << 2)     /* Command Done */
#define RE_INT            (1 << 1)     /* Response Error */
#define CDT_INT           (1 << 0)     /* Card Detect */

/* Command Register */
#define START_CMD         (0x1U << 31) /* start command */
#define USE_HOLD_REG      (1 << 29)
#define VOLT_SWITCH       (1 << 28)
#define BOOT_MODE         (1 << 27)
#define DISABLE_BOOT      (1 << 26)
#define EXPECT_BOOT_ACK   (1 << 25)
#define ENABLE_BOOT       (1 << 24)
#define CCS_EXPECTED      (1 << 23)
#define READ_CEATA        (1 << 22)
#define UPDATE_CLOCK      (1 << 21)    /* update clock register only */
#define SEND_INIT         (1 << 15)    /* send initialization sequence */
#define STOP_CMD          (1 << 14)    /* stop abort command */
#define NO_WAIT_PREV      (0 << 13)    /* not wait previous data transfer complete, send command at once */
#define WAIT_PREV         (1 << 13)    /* wait previous data transfer complete */
#define AUTO_STOP         (1 << 12)    /* send auto stop command at end of data transfer */
#define BLOCK_TRANS       (0 << 11)    /* block data transfer command */
#define STREAM_TRANS      (1 << 11)    /* stream data transfer command */
#define READ_CARD         (0 << 10)    /* read from card */
#define WRITE_CARD        (1 << 10)    /* write to card */
#define NOCARE_RW         (0 << 10)    /* not care read or write */
#define NO_DATA_EXPECT    (0 << 9)     /* no data transfer expected */
#define DATA_EXPECT       (1 << 9)     /* data transfer expected */
#define NO_CHECK_R_CRC    (0 << 8)     /* do not check response crc */
#define CHECK_R_CRC       (1 << 8)     /* check response crc */
#define NOCARE_R_CRC      CHECK_R_CRC  /* not care response crc */
#define SHORT_R           (0 << 7)     /* short response expected from card */
#define LONG_R            (1 << 7)     /* long response expected from card */
#define NOCARE_R          SHORT_R      /* not care response length */
#define NO_R_EXPECT       (0 << 6)     /* no response expected from card */
#define R_EXPECT          (1 << 6)     /* response expected from card */

/* SDMMC status Register */
#define STATUS_BUSY       (1 << 10)    /* state-machine is busy */
#define DATA_BUSY         (1 << 9)     /* Card busy */
#define FIFO_FULL         (1 << 3)     /* FIFO is full status */
#define FIFO_EMPTY        (1 << 2)     /* FIFO is empty status */

/* SDMMC FIFO Register */
#define SD_MSIZE_1        (0x0 << 28)  /* DW_DMA_Multiple_Transaction_Size */
#define SD_MSIZE_4        (0x1 << 28)
#define SD_MSIZE_8        (0x1 << 28)
#define SD_MSIZE_16       (0x3 << 28)
#define SD_MSIZE_32       (0x4 << 28)
#define SD_MSIZE_64       (0x5 << 28)
#define SD_MSIZE_128      (0x6 << 28)
#define SD_MSIZE_256      (0x7 << 28)

#define RX_WMARK_SHIFT    (16)
#define TX_WMARK_SHIFT    (0)

/* Card detect Register */
#define NO_CARD_DETECT    (1 << 0)     /* Card detect */

/* Write Protect Register */
#define WRITE_PROTECT     (1 << 0)     /* write protect, 1 represent write protection */

/* SDMMC Host Controller */
typedef enum SDMMC_PORT_Enum {
	SDC0 = 0,
	SDC1,
	SDC2,
	SDC_MAX
} SDMMC_PORT_E;

#define MAX_DESC_NUM_IDMAC     128
#define MAX_BUFF_SIZE_IDMAC    8192
#define MAX_DATA_SIZE_IDMAC    (MAX_DESC_NUM_IDMAC*MAX_BUFF_SIZE_IDMAC)

#define CTRL_USE_IDMAC	       0x02000000
#define CTRL_IDMAC_RESET       0x00000004

/* Bus Mode Register Bit Definitions */
#define  BMOD_SWR		0x00000001 /*  Software Reset: Auto cleared after one clock cycle                                0 */
#define  BMOD_FB		0x00000002 /*  Fixed Burst Length: when set SINGLE/INCR/INCR4/INCR8/INCR16 used at the start     1 */
#define  BMOD_DE		0x00000080 /*  Idmac Enable: When set IDMAC is enabled                                           7 */
#define	 BMOD_DSL_MSK		0x0000007C /*  Descriptor Skip length: In Number of Words                                      6:2 */
#define	 BMOD_DSL_Shift		2	   /*  Descriptor Skip length Shift value */
#define	 BMOD_DSL_ZERO          0x00000000 /*  No Gap between Descriptors */
#define	 BMOD_DSL_TWO           0x00000008 /*  2 Words Gap between Descriptors */
#define  BMOD_PBL		0x00000400 /*  MSIZE in FIFOTH Register */
/* Internal DMAC Status Register IDSTS Bit Definitions
 * Internal DMAC Interrupt Enable Register Bit Definitions */
#define  IDMAC_AI		0x00000200  /*  Abnormal Interrupt Summary Enable/ Status                                        9 */
#define  IDMAC_NI		0x00000100  /*  Normal Interrupt Summary Enable/ Status                                          8 */
#define  IDMAC_CES		0x00000020  /*  Card Error Summary Interrupt Enable/ status                                      5 */
#define  IDMAC_DU		0x00000010  /*  Descriptor Unavailabe Interrupt Enable /Status                                   4 */
#define  IDMAC_FBE		0x00000004  /*  Fata Bus Error Enable/ Status                                                    2 */
#define  IDMAC_RI		0x00000002  /*  Rx Interrupt Enable/ Status                                                      1 */
#define  IDMAC_TI		0x00000001  /*  Tx Interrupt Enable/ Status                                                        */

#define  IDMAC_EN_INT_ALL	0x00000337   /*  Enables all interrupts */

/*----------------------------------- Typedefs -------------------------------*/
/*
 DBADDR  = 0x88 : Descriptor List Base Address Register
 The DBADDR is the pointer to the first Descriptor
 The Descriptor format in Little endian with a 32 bit Data bus is as shown below
     --------------------------------------------------------------------------
     DES0 | OWN (31) | Control and Status                                     |
     --------------------------------------------------------------------------
     DES1 | Reserved |         Buffer 2 Size        |        Buffer 1 Size    |
     --------------------------------------------------------------------------
     DES2 |  Buffer Address Pointer 1                                         |
     --------------------------------------------------------------------------
     DES3 |  Buffer Address Pointer 2 / Next Descriptor Address Pointer       |
     --------------------------------------------------------------------------
*/
enum DmaDescriptorDES0 { /*  Control and status word of DMA descriptor DES0  */
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

enum DmaDescriptorDES1 { /*  Buffer's size field of Descriptor */
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

typedef struct tagSDMMC_DMA_DESC {
	uint32 desc0;	 /* control and status information of descriptor */
	uint32 desc1;	 /* buffer sizes                                 */
	uint32 desc2;	 /* physical address of the buffer 1             */
	uint32 desc3;	 /* physical address of the buffer 2             */
} SDMMC_DMA_DESC, *PSDMMC_DMA_DESC;


/* SDMMC Host Controller register struct */
typedef volatile struct TagSDC_REG {
	volatile uint32 SDMMC_CTRL;        /* SDMMC Control register */
	volatile uint32 SDMMC_PWREN;       /* Power enable register */
	volatile uint32 SDMMC_CLKDIV;      /* Clock divider register */
	volatile uint32 SDMMC_CLKSRC;      /* Clock source register */
	volatile uint32 SDMMC_CLKENA;      /* Clock enable register */
	volatile uint32 SDMMC_TMOUT;       /* Time out register */
	volatile uint32 SDMMC_CTYPE;       /* Card type register */
	volatile uint32 SDMMC_BLKSIZ;      /* Block size register */
	volatile uint32 SDMMC_BYTCNT;      /* Byte count register */
	volatile uint32 SDMMC_INTMASK;     /* Interrupt mask register */
	volatile uint32 SDMMC_CMDARG;      /* Command argument register */
	volatile uint32 SDMMC_CMD;         /* Command register */
	volatile uint32 SDMMC_RESP0;       /* Response 0 register */
	volatile uint32 SDMMC_RESP1;       /* Response 1 register */
	volatile uint32 SDMMC_RESP2;       /* Response 2 register */
	volatile uint32 SDMMC_RESP3;       /* Response 3 register */
	volatile uint32 SDMMC_MINTSTS;     /* Masked interrupt status register */
	volatile uint32 SDMMC_RINISTS;     /* Raw interrupt status register */
	volatile uint32 SDMMC_STATUS;      /* Status register */
	volatile uint32 SDMMC_FIFOTH;      /* FIFO threshold register */
	volatile uint32 SDMMC_CDETECT;     /* Card detect register */
	volatile uint32 SDMMC_WRTPRT;      /* Write protect register */
	volatile uint32 SDMMC_GPIO;        /* GPIO register */
	volatile uint32 SDMMC_TCBCNT;      /* Transferred CIU card byte count */
	volatile uint32 SDMMC_TBBCNT;      /* Transferred host/DMA to/from BIU_FIFO byte count */
	volatile uint32 SDMMC_DEBNCE;      /* Card detect debounce register */
	volatile uint32 SDMMC_USRID;       /* User ID register */
	volatile uint32 SDMMC_VERID;       /* Synopsys version ID register */
	volatile uint32 SDMMC_HCON;        /* Hardware configuration register */
	volatile uint32 SDMMC_UHS_REG;     /* UHS-1 register */
	volatile uint32 SDMMC_RST_n;       /* Hardware reset register */
	volatile uint32 SDMMC_RESERVED0;
	volatile uint32 SDMMC_BMOD;        /* HBus Mode Register */
	volatile uint32 SDMMC_PLDMND;      /* Poll Demand Register */
	volatile uint32 SDMMC_DBADDR;      /* Descriptor List Base Address Register */
	volatile uint32 SDMMC_IDSTS;       /* Internal DMAC Status Register */
	volatile uint32 SDMMC_IDINTEN;     /*  Internal DMAC Interrupt Enable Register */
	volatile uint32 SDMMC_DSCADDR;     /* Current Host Descriptor Address Register */
	volatile uint32 SDMMC_BUFADDR;     /* Current Buffer Descriptor Address Register */
	volatile uint32 SDMMC_RESERVED1[(0x100-0x9C)/4];
	volatile uint32 SDMMC_CARDTHRCTL;       /* Card Threshold Control Register */
	volatile uint32 SDMMC_BACK_END_POWER;   /* Back-end Power Register */
	volatile uint32 SDMMC_UHS_REG_EXT;      /* UHS Register */
	volatile uint32 SDMMC_EMMC_DDR_REG;     /* eMMC 4.5 DDR START Bit Detection Control Register */
	volatile uint32 SDMMC_ENABLE_SHIFT;     /* Enable Phase Shift Register */
} SDC_REG_T, *pSDC_REG_T;

/* Interrupt Information */
typedef struct TagSDC_INT_INFO {
	uint32     transLen;	/* 已经发送或接收的数据长度 */
	uint32     desLen;	/* 需要传输的数据长度 */
	uint32    *pBuf;	/* 中断数据接收或发送数据用到的buf地址 */
				/* 指针用32 bit的uint32指针，就可以满足SDMMC FIFO要求的32 bits对齐的要求了， */
				/* 这样就算你的数据没有4字节对齐，也会因为用了uint32指针，每次向FIFO操作是4字节对齐的。 */
} SDC_INT_INFO_T;

/* SDMMC Host Controller Information */
typedef struct TagSDC_INFO {
	pSDC_REG_T        pReg;			/* SDMMC Host Controller Register, only for debug */
	HOST_BUS_WIDTH_E  busWidth;		/* SDMMC Host Controller Bus Width， */
						/* 控制器支持的总线宽度是由硬件设计的IO共用情况决定的， */
						/* 插在该控制器上的卡设置总线宽度时受到该宽度的限制 */
	SDC_INT_INFO_T    intInfo;		/* 控制器中断要用到的信息 */
	pEVENT            event;		/* SDMMC Host Controller对应的事件 */
	uint32            cardFreq;		/* 卡的工作频率,单位KHz，这个频率只有SDM层有权修改 */
	uint32            updateCardFreq;	/* 表示是否需要更新卡的频率，用于由于AHB频率改变而导致需要更新卡的频率,TRUE:要更新，FALSE:不用更新 */
	uint32            bSdioEn;		/* 是否使能SDIO中断,TRUE:使能，FALSE:禁止中断 */
	pFunc             pSdioCb;		/* SDIO中断的回调函数 */
#if (EN_SDC_INTERAL_DMA)
	uint32            ErrorStat;
	SDMMC_DMA_DESC    IDMADesc[MAX_DESC_NUM_IDMAC];
#endif
} SDC_INFO_T, *pSDC_INFO_T;


#undef EXT
#ifdef SDC_DRIVER
#define EXT
#else
#define EXT extern
#endif

/* 控制SDC驱动的全局变量 */
EXT SDC_INFO_T    gSDCInfo[SDC_MAX];

void eMMC_SetDataHigh(void);

#endif /* end of #ifndef _SDC_H_ */

#endif /* end of #ifdef DRIVERS_SDMMC */
