/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SDM_H_
#define _SDM_H_

#define SDM_CMD_RESENT_COUNT  (3) /* 当读写出错时，重复发送的次数，用于读写出错处理 */
#define SDM_CMD_ERROR_RETRY_COUNT   (1) /* 命令回复出错时，重复发送的次数 */

#define SDM_INVALID_CARDID    (-1)           /* 无效的cardId */
#define SDM_WIDE_BUS_MODE     (1 << 0)       /* for debug */
#define SDM_HIGH_SPEED_MODE   (1 << 1)       /* for debug */
#define SDM_DDR_SPEED_MODE    (1 << 2)       /* for debug */
#define SDM_HS200_SPEED_MODE  (1 << 3)       /* for debug */

#define SDM_MAX_MANAGER_PORT  (3) /* SDM最大管理多少个端口 */

/* Card Operation Function */
struct SDM_OPERATION_FUN_T {
	int32 (*read)(int32 cardId, uint32 dataAddr, uint32 blockCount, void *pBuf);
	int32 (*write)(int32 cardId, uint32 dataAddr, uint32 blockCount, void *pBuf);
};

/* Card Information */
typedef struct TagSDM_CARD_INFO {
	int32            cardId;           /* cardId，-1被认为是无效id */
	uint32           type;             /* Card type */
	uint16           rca;              /* Relative Card Address */
	uint32           workMode;         /* for debug, record card work mode */
	uint32           tran_speed;       /* 卡的最大数据传输速度，也就是卡的最大工作频率，单位KHz */
	/*************************************************************
			SD/MMC information
	*************************************************************/
	uint32             WriteProt;      /* Mechanical write Protect switch state,
						TRUE:write protected, FALSE: no write protected, can read and write */
	uint32             bPassword;      /* 用于指示卡是否是有密码的卡, TRUE:have password, FALSE:no password */
	/* Card internal detail */
	uint16           year;             /* Card manufacture year */
	uint8            month;            /* Card manufacture month */
	uint32           psn;              /* Product serial number */
	uint8            prv;              /* Product revision, 高4位表示大版本号，低4位表示小版本号，如:0110 0010b表示6.2版本 */
	uint8            pnm[7];           /* Product name,ASCII string */
	uint8            oid[3];           /* OEM/Application ID,ASCII string */
	uint8            mid;              /* Manufacturer ID */
	uint32           capability;       /* Card capability,单位block数，每个block为512字节 */
	uint16           ccc;              /* Card Command Class */
	uint32           taac;
	uint32           nsac;
	uint32           timeout;          /* 这个timeout是用写的timeout算出来的，不管读写都用这个timeout值 */
	uint32           r2w_factor;
	uint32           dsr_imp;
	CARD_SPEC_VER_E  specVer;          /* SD Specification Version */
	/* operation function */
	struct SDM_OPERATION_FUN_T fun;    /* SDM operation functions */
	/*************************************************************
			SDIO information
	*************************************************************/
	uint8            nf;               /* number of functions in the card */
	uint32           smb;              /* is card support multi-block, TRUE:support, FALSE:not support, and fnBS[] is useless */
	uint16           fnBS[8];          /* each function block size, (0 <= function <= 7), (1 <= block size <= 2048) */
	/*************************************************************
			eMMC Boot information
	*************************************************************/
	uint32           bootSize;         /* boot partition size,单位sector(512B) */
} SDM_CARD_INFO_T, *pSDM_CARD_INFO_T;

/* SDM Port Information */
typedef struct TagSDM_PORT_INFO {
	pMUTEX           mutex;        /* 端口互斥量,因为完成一个读写或设置密码等操作，需要在同一个端口上连续发送几条命令
					  而且命令中间不能穿插其他命令，否则执行的效果就不是我们想要的结果，因此为了保证
					  命令之间的连续，需要一个互斥量 */
	uint32           bOpen;        /* Is port opened, TRUE:opened, FALSE:closed */
	SDM_CARD_INFO_T  cardInfo;     /* CardInfo */
#ifdef SDM_PROT_INFO_DEBUG
	uint32           step;         /* for debug, 卡识别走到哪一部出错 */
	int32            error;        /* for debug，出错信息保存在这边 */
#endif
} SDM_PORT_INFO_T, *pSDM_PORT_INFO_T;

#undef EXT
#ifdef SDM_DRIVER
#define EXT
#else
#define EXT extern
#endif

/* 控制SDM驱动的全局变量 */
EXT SDM_PORT_INFO_T gSDMDriver[SDM_MAX_MANAGER_PORT];
EXT uint32 gEmmcBootPart;

/****************************************************************
			供SDM内部使用的接口
****************************************************************/
uint16 _GenerateRCA(void);
uint32 _IsRCAUsable(uint16 rca);
int32 _Identify_SendCmd(int32 cardId,
			uint32 cmd,
			uint32 cmdArg,
			uint32 *responseBuf,
			uint32  blockSize,
			uint32  dataLen,
			void   *pDataBuf);
int32 _Identify_SendAppCmd(int32 cardId,
			uint16 rca,
			uint32 cmd,
			uint32 cmdArg,
			uint32 *responseBuf,
			uint32  blockSize,
			uint32  dataLen,
			void   *pDataBuf);

#endif /* end of #ifndef _SDM_H_ */

#endif /* end of #ifdef DRIVERS_SDMMC */
