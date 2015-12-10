/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SD_COMMON_H_
#define _SD_COMMON_H_

/* Class 0 (basic command) */
#define SD_CMD0    (0)	/* [31:0] stuff bits */
#define SD_CMD1    (1)	/* [31:0] stuff, MMC */
#define SD_CMD2    (2)	/* [31:0] stuff */
#define SD_CMD3    (3)	/* [31:0] stuff */
#define SD_CMD4    (4)	/* [31:16] DSR [15:0] stuff */
#define SD_CMD5    (5)	/* [31:24] stuff [23:0] I/O OCR, SDIO */
#define SD_CMD6    (6)	/* ACMD6:[31:2] stuff [1:0] bus width */
			/* SD2.0 CMD6 */
			/* [31] Mode 0:Check function, 1:Switch function */
			/* [30:24] reserved */
			/* [23:20] reserved for function group 6 */
			/* [19:16] reserved for function group 5 */
			/* [15:12] reserved for function group 4 */
			/* [11:8]  reserved for function group 3 */
			/* [7:4]   function group 2 for command system */
			/* [3:0]   function group 1 for access mode */
			/* MMC4.0 CMD6 */
			/* [31:26] Set to 0 */
			/* [25:24] Access */
			/* [23:16] Index */
			/* [15:8] Value */
			/* [7:3] Set to 0 */
			/* [2:0] Cmd Set */
#define SD_CMD7    (7)	/* [31:16] RCA [15:0] stuff */
#define SD_CMD8    (8)	/* SD 2.0 */
			/* [31:12] reserved [11:8] supply voltage(VHS) [7:0] check pattern */
			/* MMC4.0 */
			/* [31:0] stuff bits */
#define SD_CMD9    (9)	/* [31:16] RCA [15:0] stuff */
#define SD_CMD10   (10)	/* [31:16] RCA [15:0] stuff */
#define SD_CMD11   (11)
#define SD_CMD12   (12)	/* [31:0] stuff */
#define SD_CMD13   (13)	/* [31:16] RCA [15:0] stuff, ACMD13:[31:0] stuff */
#define SD_CMD14   (14)	/* MMC4.0 */
			/* [31:0] stuff bits */
#define SD_CMD15   (15)	/* [31:16] RCA [15:0] stuff */
/* Class 2 */
#define SD_CMD16   (16)	/* [31:0] block length */
#define SD_CMD17   (17)	/* [31:0] data address */
#define SD_CMD18   (18)	/* [31:0] data address */
#define SD_CMD19   (19)	/* MMC4.0 */
			/* [31:0] stuff bits */
#define SD_CMD20   (20)
#define SD_CMD21   (21)
#define SD_CMD22   (22)	/* ACMD22:[31:0] stuff */
#define SD_CMD23   (23)	/* [31:16] stuff [15:0] number of blocks, MMC */
			/* ACMD23:[31:23] stuff [22:0] Number of blocks */
/* Class 4 */
#define SD_CMD24   (24)	/* [31:0] data address */
#define SD_CMD25   (25)	/* [31:0] data address */
#define SD_CMD26   (26)
#define SD_CMD27   (27)	/* [31:0] stuff */
/* Class 6 */
#define SD_CMD28   (28)	/* [31:0] data address */
#define SD_CMD29   (29)	/* [31:0] data address */
#define SD_CMD30   (30)	/* [31:0] write protect data address */
#define SD_CMD31   (31)
/* Class 5 */
#define SD_CMD32   (32)   /* [31:0] data address */
#define SD_CMD33   (33)   /* [31:0] data address */
#define SD_CMD34   (34)
#define SD_CMD35   (35)	/* [31:0] data address, MMC */
#define SD_CMD36   (36)	/* [31:0] data address, MMC */
#define SD_CMD37   (37)
#define SD_CMD38   (38)	/* [31:0] stuff */
#define SD_CMD39   (39)
#define SD_CMD40   (40)
#define SD_CMD41   (41)	/* ACMD41:[31:0] OCR without busy, */
			/* ACMD41:[31] reserved [30] HCS(OCR[30]) [29:24] reserved  */
			/* [23:0] Vdd Voltage window(OCR[23:0]), SD2.0 */
/* Class 7 */
#define SD_CMD42   (42)	/* [31:0] stuff, ACMD42:[31:1] stuff [0] set_cd */
#define SD_CMD43   (43)
#define SD_CMD44   (44)
#define SD_CMD45   (45)
#define SD_CMD46   (46)
#define SD_CMD47   (47)
#define SD_CMD48   (48)
#define SD_CMD49   (49)
#define SD_CMD50   (50)
#define SD_CMD51   (51)	/* ACMD51:[31:0] stuff */
#define SD_CMD52   (52)	/* [31] R/W flag [30:28] Function Number [27] RAW flag [26] stuff  */
			/* [25:9] Register Address [8] stuff [7:0] Write Data or stuff, SDIO */
#define SD_CMD53   (53)	/* [31] R/W flag [30:28] Function Number [27] Block Mode [26] OP Code */
			/* [25:9] Register Address [8:0] Byte/Block Count */
#define SD_CMD54   (54)
/* Class 8 */
#define SD_CMD55   (55)	/* [31:16] RCA [15:0] stuff */
#define SD_CMD56   (56)	/* [31:1] stuff [0] RD/WRn */
#define SD_CMD57   (57)
#define SD_CMD58   (58)
#define SD_CMD59   (59)	/* [31:1] stuff [0] CRC option, MMC */
#define SD_CMD60   (60)
#define SD_CMD61   (61)
#define SD_CMD62   (62)
#define SD_CMD63   (63)

/* SD 1.0 command */
/* Class 0 */
#define SD_GO_IDLE_STATE        SD_CMD0
#define SD_ALL_SEND_CID         SD_CMD2
#define SD_SEND_RELATIVE_ADDR   SD_CMD3
#define SD_SET_DSR              SD_CMD4
#define SD_SELECT_DESELECT_CARD SD_CMD7
#define SD_SEND_CSD             SD_CMD9
#define SD_SEND_CID             SD_CMD10
#define SD_STOP_TRANSMISSION    SD_CMD12
#define SD_SEND_STATUS          SD_CMD13
#define SD_GO_INACTIVE_STATE    SD_CMD15
/* Class 2 */
#define SD_SET_BLOCKLEN         SD_CMD16
#define SD_READ_SINGLE_BLOCK    SD_CMD17
#define SD_READ_MULTIPLE_BLOCK  SD_CMD18

#define SD_SET_BLOCK_COUNT       SD_CMD23
/* Class 4 */
#define SD_WRITE_BLOCK          SD_CMD24
#define SD_WRITE_MULTIPLE_BLOCK SD_CMD25
#define SD_PROGRAM_CSD          SD_CMD27
/* Class 6 */
#define SD_SET_WRITE_PROT       SD_CMD28
#define SD_CLR_WRITE_PROT       SD_CMD29
#define SD_SEND_WRITE_PROT      SD_CMD30
/* Class 5 */
#define SD_ERASE_WR_BLK_START   SD_CMD32
#define SD_ERASE_WR_BLK_END     SD_CMD33
#define SD_ERASE                SD_CMD38
/* Class 7 */
#define SD_LOCK_UNLOCK          SD_CMD42
/* Class 8 */
#define SD_APP_CMD              SD_CMD55
#define SD_GEN_CMD              SD_CMD56
/* Application cmd */
#define SDA_SET_BUS_WIDTH       SD_CMD6
#define SDA_SD_STATUS           SD_CMD13
#define SDA_SEND_NUM_WR_BLOCKS  SD_CMD22
#define SDA_SET_WR_BLK_ERASE_COUNT SD_CMD23
#define SDA_SD_APP_OP_COND      SD_CMD41
#define SDA_SET_CLR_CARD_DETECT SD_CMD42
#define SDA_SEND_SCR            SD_CMD51

/* SD 2.0 addition command */
#define SD2_SEND_IF_COND        SD_CMD8
#define SD2_SWITCH_FUNC         SD_CMD6

/* SDIO addition command */
#define SDIO_IO_SEND_OP_COND    SD_CMD5
#define SDIO_IO_RW_DIRECT       SD_CMD52
#define SDIO_IO_RW_EXTENDED     SD_CMD53

/* MMC addition command */
#define MMC_SEND_OP_COND        SD_CMD1
#define MMC_SET_RELATIVE_ADDR   SD_CMD3

/* MMC4.0 addition command */
#define MMC4_SWITCH_FUNC        SD_CMD6
#define MMC4_SEND_EXT_CSD       SD_CMD8
#define MMC4_BUSTEST_R          SD_CMD14
#define MMC4_BUSTEST_W          SD_CMD19

#define COMMAND_CLASS_7         (0x1 << 7)	/* Command Class 7:lock card */

#define CARD_IS_LOCKED          (0x1 << 25)
#define LOCK_UNLOCK_FAILED      (0x1 << 24)
#define COM_CRC_ERROR           (0x1 << 23)
#define CARD_ECC_FAILED         (0x1 << 21)
#define CC_ERROR                (0x1 << 20)	/* internal card controller error */
#define CARD_UNKNOWN_ERROR      (0x1 << 19)	/* a general or an unknown error occurred during the operation */
#define READY_FOR_DATA          (0x1 << 8)
#define APP_CMD                 (0x1 << 5)

#define R7_VOLTAGE_27_36        (0x1)		/* R7 voltage accepted 2.7-3.6V */
#define R7_LOW_VOLTAGE          (0x2)		/* reserved for low voltage ranger */

#if 0
#define MAX_FOD_FREQ            (400)		/* 卡识别阶段使用的最大频率,单位KHz,协议规定最大400KHz */
#define MAX_SD_FPP_FREQ         (25000)		/* 标准SD卡正常工作最大频率，单位KHz，协议规定最大25MHz */
#define MAX_SDHC_FPP_FREQ       (50000)		/* SDHC卡在高速模式下的最大工作频率，单位KHz，协议规定最大50MHz */
#define MAX_MMC_FPP_FREQ        (20000)		/* 标准MMC卡正常工作最大频率，单位KHz，协议规定最大20MHz */
#define MAX_MMCHS_26_FPP_FREQ   (26000)		/* 高速模式只支持最大26M的HS-MMC卡，在高速模式下的工作最大频率，单位KHz，协议规定最大26MHz */
#define MAX_MMCHS_52_FPP_FREQ   (52000)		/* 高速模式能支持最大52M的HS-MMC卡，在高速模式下的工作最大频率，单位KHz，协议规定最大52MHz */
#endif

/* Card type */
#define UNKNOW_CARD             (0)		/* 无法识别或不支持的卡，不可用 */
#define SDIO                    (0x1 << 1)
#define SDHC                    (0x1 << 2)	/* Ver2.00 High Capacity SD Memory Card */
#define SD20                    (0x1 << 3)	/* Ver2.00 Standard Capacity SD Memory Card */
#define SD1X                    (0x1 << 4)	/* Ver1.xx Standard Capacity SD Memory Card */
#define MMC4                    (0x1 << 5)	/* Ver4.xx MMC */
#define MMC                     (0x1 << 6)	/* Ver3.xx MMC */
#define eMMC2G                  (0x1 << 7)	/* Ver4.2 larter eMMC and densities greater than 2GB */

/* Command Registe bit */
/* 直接把Command Register的位定义拿过来当command flag，
   这样控制器就不用再分析flag，而可以直接把flag设到寄存器中 */
#define BLOCK_TRANS             (0 << 11)	/* block data transfer command */
#define STREAM_TRANS            (1 << 11)	/* stream data transfer command */
#define READ_CARD               (0 << 10)	/* read from card */
#define WRITE_CARD              (1 << 10)	/* write to card */
#define NOCARE_RW               (0 << 10)	/* not care read or write */
#define NO_DATA_EXPECT          (0 << 9)	/* no data transfer expected */
#define DATA_EXPECT             (1 << 9)	/* data transfer expected */
#define NO_CHECK_R_CRC          (0 << 8)	/* do not check response crc */
#define CHECK_R_CRC             (1 << 8)	/* check response crc */
#define NOCARE_R_CRC            CHECK_R_CRC	/* not care response crc */
#define SHORT_R                 (0 << 7)	/* short response expected from card */
#define LONG_R                  (1 << 7)	/* long response expected from card */
#define NOCARE_R                SHORT_R		/* not care response length */
#define NO_R_EXPECT             (0 << 6)	/* no response expected from card */
#define R_EXPECT                (1 << 6)	/* response expected from card */
#define RSP_BUSY                (1 << 16)	/* card may send busy，这是我们自己加的一位，用于R1b的， */
						/* 寄存器并没有这位，因此硬件控制器使用时必须把这位清掉 */

/* command flag */
#define SD_RSP_NONE             (NOCARE_R_CRC | NOCARE_R | NO_R_EXPECT)
#define SD_RSP_R1               (CHECK_R_CRC | SHORT_R | R_EXPECT)
#define SD_RSP_R1B              (CHECK_R_CRC | SHORT_R | R_EXPECT | RSP_BUSY)
#define SD_RSP_R2               (CHECK_R_CRC | LONG_R | R_EXPECT)
#define SD_RSP_R3               (NO_CHECK_R_CRC | SHORT_R | R_EXPECT)
#define SD_RSP_R4               (NO_CHECK_R_CRC | SHORT_R | R_EXPECT)
#define SD_RSP_R5               (CHECK_R_CRC | SHORT_R | R_EXPECT)
#define SD_RSP_R6               (CHECK_R_CRC | SHORT_R | R_EXPECT)
#define SD_RSP_R6M              (CHECK_R_CRC | SHORT_R | R_EXPECT)
#define SD_RSP_R7               (CHECK_R_CRC | SHORT_R | R_EXPECT)

/* 读写操作的flag */
#define SD_OP_MASK              ((1<<11)|(1<<10)|(1<<9))
#define SD_NODATA_OP            (NOCARE_RW | NO_DATA_EXPECT | BLOCK_TRANS)
#define SD_READ_OP              (READ_CARD | DATA_EXPECT | BLOCK_TRANS)
#define SD_WRITE_OP             (WRITE_CARD | DATA_EXPECT | BLOCK_TRANS)

/* 其他flag */
#define SEND_INIT               (1 << 15)    /* send initialization sequence */
#define STOP_CMD                (1 << 14)    /* stop abort command */
#define NO_WAIT_PREV            (0 << 13)    /* not wait previous data transfer complete, send command at once */
#define WAIT_PREV               (1 << 13)    /* wait previous data transfer complete */

#define SD_CMD_MASK             (0x3F)       /* 低6bit是命令 */

/* response type */
typedef enum RESPONSE_Enum {
	R1_TYPE = 0,    /* normal response command (48 bit) */
	R1b_TYPE,       /* identical to R1 with an optional busy signal (48 bit) */
	R2_TYPE,        /* CID, CSD register (136 bit) */
	R3_TYPE,        /* OCR register (48 bit) */
	R4_TYPE,        /* SDIO additional, IO_SEND_OP_COND response (48 bit) */
	R5_TYPE,        /* SDIO additional, IO_RW_DIRECT response (48 bit) */
	R6_TYPE,        /* Published RCA response (48 bit) */
	R6m_TYPE,       /* SDIO modified R6, Published RCA response (48 bit) */
	R7_TYPE,        /* SD2.0 additional, Card interface condition (48 bit) */
	NO_RESP
} RESPONSE_E;

/* SD Specification Version */
typedef enum CARD_SPEC_VER_Enum {
	/* SD Specification Version */
	SPEC_VER_INVALID = 0,
	SD_SPEC_VER_10,             /* SD Specification Version 1.0-1.01 */
	SD_SPEC_VER_11,             /* SD Specification Version 1.1 */
	SD_SPEC_VER_20,             /* SD Specification Version 2.0 */
	SD_SPEC_VER_MAX = 100,

	/* MMC Specification Version*/
	MMC_SPEC_VER_10,            /* MMC Specification Version 1.0 - 1.2 */
	MMC_SPEC_VER_14,            /* MMC Specification Version 1.4 */
	MMC_SPEC_VER_20,            /* MMC Specification Version 2.0 - 2.2 */
	MMC_SPEC_VER_31,            /* MMC Specification Version 3.1 - 3.2 - 3.31 */
	MMC_SPEC_VER_40,            /* MMC Specification Version 4.0 - 4.1 - 4.2 - 4.3 */
	SPEC_VER_MAX
} CARD_SPEC_VER_E;

/* boot partition  */
typedef enum BOOTPARTITION_Enum {
	NOACCESS,
	PART1,
	PART2,
	PART3,
	PART4,
	PART5,
	PART6,
	PART7
} BOOTPARTITION_E;


#endif /* end of #ifndef _SD_COMMON_H_  */

#endif /* end of #ifdef DRIVERS_SDMMC */
