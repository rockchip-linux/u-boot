/*
 * Copyright 2008,2010 Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Copyright 2013 Google Inc.  All rights reserved.
 *
 * Based (loosely) on the Linux code
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

#ifndef __DRIVERS_STORAGE_MMC_H__
#define __DRIVERS_STORAGE_MMC_H__

#include <linux/list.h>
#include <linux/compiler.h>
#include <part.h>

#include "blockdev.h"
#include "bouncebuf.h"

#ifndef BIT
#define BIT(nr)			(1 << (nr))
#endif

#define SD_VERSION_SD		0x20000
#define SD_VERSION_2		(SD_VERSION_SD | 0x20)
#define SD_VERSION_1_0		(SD_VERSION_SD | 0x10)
#define SD_VERSION_1_10		(SD_VERSION_SD | 0x1a)
#define MMC_VERSION_MMC		0x10000
#define MMC_VERSION_UNKNOWN	(MMC_VERSION_MMC)
#define MMC_VERSION_1_2		(MMC_VERSION_MMC | 0x12)
#define MMC_VERSION_1_4		(MMC_VERSION_MMC | 0x14)
#define MMC_VERSION_2_2		(MMC_VERSION_MMC | 0x22)
#define MMC_VERSION_3		(MMC_VERSION_MMC | 0x30)
#define MMC_VERSION_4		(MMC_VERSION_MMC | 0x40)

#define MMC_MODE_HS		0x001
#define MMC_MODE_HS_52MHz	0x010
#define MMC_MODE_HS_200MHz	0x020
#define MMC_MODE_HS_400MHz	0x040

#define MMC_MODE_1V8_VDD	0x080
#define MMC_MODE_4BIT		0x100
#define MMC_MODE_8BIT		0x200
#define MMC_MODE_SPI		0x400
#define MMC_MODE_HC		0x800
#define MMC_AUTO_CMD12		0x1000

#define SD_DATA_4BIT		0x00040000

#define IS_SD(x)		(x->version & SD_VERSION_SD)

#define MMC_DATA_READ		1
#define MMC_DATA_WRITE		2

#define MMC_NO_CARD_ERR		-16 /* No SD/MMC card inserted */
#define MMC_UNUSABLE_ERR	-17 /* Unusable Card */
#define MMC_COMM_ERR		-18 /* Communications Error */
#define MMC_TIMEOUT		-19
#define MMC_IN_PROGRESS		-20 /* operation is in progress */
#define MMC_INVALID_ERR		-21 /* A catch all case. */

#define MMC_CMD_GO_IDLE_STATE		0
#define MMC_CMD_SEND_OP_COND		1
#define MMC_CMD_ALL_SEND_CID		2
#define MMC_CMD_SET_RELATIVE_ADDR	3
#define MMC_CMD_SET_DSR			4
#define MMC_CMD_SWITCH			6
#define MMC_CMD_SELECT_CARD		7
#define MMC_CMD_SEND_EXT_CSD		8
#define MMC_CMD_SEND_CSD		9
#define MMC_CMD_SEND_CID		10
#define MMC_CMD_STOP_TRANSMISSION	12
#define MMC_CMD_SEND_STATUS		13
#define MMC_CMD_SET_BLOCKLEN		16
#define MMC_CMD_READ_SINGLE_BLOCK	17
#define MMC_CMD_READ_MULTIPLE_BLOCK	18

#define MMC_SEND_TUNING_BLOCK    19   /* adtc                    R1  */
#define MMC_SEND_TUNING_BLOCK_HS200	21	/* adtc R1  */

#define MMC_CMD_WRITE_SINGLE_BLOCK	24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK	25
#define MMC_CMD_ERASE_GROUP_START	35
#define MMC_CMD_ERASE_GROUP_END		36
#define MMC_CMD_ERASE			38
#define MMC_CMD_APP_CMD			55
#define MMC_CMD_SPI_READ_OCR		58
#define MMC_CMD_SPI_CRC_ON_OFF		59

#define MMC_TRIM_ARG			0x1
#define MMC_SECURE_ERASE_ARG		0x80000000

#define SD_CMD_SEND_RELATIVE_ADDR	3
#define SD_CMD_SWITCH_FUNC		6
#define SD_CMD_SEND_IF_COND		8

#define SD_CMD_APP_SET_BUS_WIDTH	6
#define SD_CMD_ERASE_WR_BLK_START	32
#define SD_CMD_ERASE_WR_BLK_END		33
#define SD_CMD_APP_SEND_OP_COND		41
#define SD_CMD_APP_SEND_SCR		51

/* SCR definitions in different words */
#define SD_HIGHSPEED_BUSY	0x00020000
#define SD_HIGHSPEED_SUPPORTED	0x00020000

#define MMC_HS_TIMING		0x00000100
#define MMC_HS_52MHZ		0x2
#define MMC_HS_200MHZ		0x10
#define MMC_HS_400MHZ		0x40
#define MMC_HS_HS400ES	    (1<<8)	/* Card can run at HS400ES */


#define OCR_BUSY		0x80000000
#define OCR_HCS			0x40000000
#define OCR_VOLTAGE_MASK	0x007FFF80
#define OCR_ACCESS_MODE		0x60000000

#define SECURE_ERASE		0x80000000

#define MMC_STATUS_MASK		(~0x0206BF7F)
#define MMC_STATUS_RDY_FOR_DATA (1 << 8)
#define MMC_STATUS_CURR_STATE	(0xf << 9)
#define MMC_STATUS_ERROR	(1 << 19)

#define MMC_VDD_165_195		0x00000080	/* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21		0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22		0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23		0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24		0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25		0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26		0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27		0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28		0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29		0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30		0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31		0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32		0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33		0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34		0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35		0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36		0x00800000	/* VDD voltage 3.5 ~ 3.6 */

#define MMC_VDD_165_195_SHIFT   7

#define MMC_SWITCH_MODE_CMD_SET		0x00 /* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS	0x01 /* Set bits in EXT_CSD byte
						addressed by index which are
						1 in value field */
#define MMC_SWITCH_MODE_CLEAR_BITS	0x02 /* Clear bits in EXT_CSD byte
						addressed by index, which are
						1 in value field */
#define MMC_SWITCH_MODE_WRITE_BYTE	0x03 /* Set target byte to value */

#define SD_SWITCH_CHECK		0
#define SD_SWITCH_SWITCH	1

/*
 * EXT_CSD fields
 */
#define EXT_CSD_PARTITIONING_SUPPORT	160	/* RO */
#define EXT_CSD_ERASE_GROUP_DEF		175	/* R/W */
#define EXT_CSD_PART_CONF		179	/* R/W */
#define EXT_CSD_BUS_WIDTH		183	/* R/W */
#define EXT_CSD_STROBE_SUPPORT		184	/* RO */
#define EXT_CSD_HS_TIMING		185	/* R/W */
#define EXT_CSD_REV			192	/* RO */
#define EXT_CSD_CARD_TYPE		196	/* RO */
#define EXT_CSD_SEC_CNT			212	/* RO, 4 bytes */
#define EXT_CSD_HC_ERASE_GRP_SIZE	224	/* RO */
#define EXT_CSD_TRIM_MULT		232     /* RO */

/*
 * EXT_CSD field definitions
 */

#define EXT_CSD_CMD_SET_NORMAL		(1 << 0)
#define EXT_CSD_CMD_SET_SECURE		(1 << 1)
#define EXT_CSD_CMD_SET_CPSECURE	(1 << 2)

#define EXT_CSD_CARD_TYPE_26	(1 << 0)	/* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52	(1 << 1)	/* Card can run at 52MHz */

#define EXT_CSD_BUS_WIDTH_1	0	/* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4	1	/* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8	2	/* Card is in 8 bit mode */
#define EXT_CSD_DDR_BUS_WIDTH_4	5	/* Card is in 4 bit DDR mode */
#define EXT_CSD_DDR_BUS_WIDTH_8	6	/* Card is in 8 bit DDR mode */

#define EXT_CSD_TIMING_BC	0	/* Backwards compatility */
#define EXT_CSD_TIMING_HS	1	/* High speed */
#define EXT_CSD_TIMING_HS200	2	/* HS200 */
#define EXT_CSD_TIMING_HS400	3	/* HS400 */
#define EXT_CSD_DRV_STR_SHIFT	4	/* Driver Strength shift */
#define EXT_CSD_BUS_WIDTH_STROBE BIT(7)	/* Enhanced strobe mode */

#define R1_ILLEGAL_COMMAND		(1 << 22)
#define R1_APP_CMD			(1 << 5)

#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136	(1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC	(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE| \
			MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_PRESENT)
#define MMC_RSP_R4	(MMC_RSP_PRESENT)
#define MMC_RSP_R5	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define MMC_INIT_TIMEOUT_US	(1000 * 1000)
#define MMC_IO_RETRIES	(1000)
#define MMC_CLOCK_20MHZ (20000000)
#define MMC_CLOCK_25MHZ (25000000)
#define MMC_CLOCK_26MHZ (26000000)
#define MMC_CLOCK_50MHZ (50000000)
#define MMC_CLOCK_52MHZ (52000000)
#define MMC_CLOCK_200MHZ (200000000)
#define MMC_CLOCK_DEFAULT_MHZ	(MMC_CLOCK_20MHZ)

#define EXT_CSD_SIZE	(512)

#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2
#define MMC_TIMING_UHS_SDR12	3
#define MMC_TIMING_UHS_SDR25	4
#define MMC_TIMING_UHS_SDR50	5
#define MMC_TIMING_UHS_SDR104	6
#define MMC_TIMING_UHS_DDR50	7
#define MMC_TIMING_MMC_DDR52	8
#define MMC_TIMING_MMC_HS200	9
#define MMC_TIMING_MMC_HS400	10


typedef struct MmcCommand {
	uint16_t cmdidx;
	uint32_t resp_type;
	uint32_t cmdarg;
	uint32_t response[4];
	uint32_t flags;
} MmcCommand;

typedef struct MmcData {
	union {
		char *dest;
		const char *src;
	};
	uint32_t flags;
	uint32_t blocks;
	uint32_t blocksize;
} MmcData;

struct MmcMedia;
typedef struct MmcMedia MmcMedia;

typedef struct MmcCtrlr {
	BlockDevCtrlr ctrlr;

	MmcMedia *media;

	uint32_t voltages;
	uint32_t f_min;
	uint32_t f_max;
	uint32_t bus_width;
	uint32_t bus_hz;
	uint32_t caps;
	uint32_t b_max;
    uint32_t timing;
    
    int		 can_retune;	/* re-tuning can be used */
    int      need_retune;    /* re-tuning is needed */
    int      enhanced_strobe;
	/*
	 * Some eMMC devices do not support iterative OCR setting, they need
	 * to be programmed with the required/expected value. This field is
	 * used to set the value for those devices.
	 */
	uint32_t hardcoded_voltage;

	int (*send_cmd)(struct MmcCtrlr *me, MmcCommand *cmd, MmcData *data);
	void (*set_ios)(struct MmcCtrlr *me);

    int	(*execute_tuning)(struct MmcCtrlr *me, u32 opcode);
    
    void	(*hs400_enhanced_strobe)(struct MmcCtrlr *me, u32 enhanced_strobe);
} MmcCtrlr;

typedef struct MmcMedia {
	BlockDev dev;

	MmcCtrlr *ctrlr;

	uint32_t caps;
	uint32_t version;
	uint32_t read_bl_len;
	uint32_t write_bl_len;
	uint64_t capacity;
	int high_capacity;
	uint32_t tran_speed;
	/* Erase size in terms of block length. */
	uint32_t erase_size;
	/* Trim operation multiplier for determining timeout. */
	uint32_t trim_mult;

	uint32_t ocr;
	uint16_t rca;
    uint16_t strobe_support;
	uint32_t scr[2];
	uint32_t csd[4];
	uint32_t cid[4];
    unsigned int        mmc_avail_type; /* supported device type by both host and card */
    
	uint32_t op_cond_response; // The response byte from the last op_cond
} MmcMedia;

int mmc_busy_wait_io(volatile uint32_t *address, uint32_t *output,
		     uint32_t io_mask, uint32_t timeout_ms);
int mmc_busy_wait_io_until(volatile uint32_t *address, uint32_t *output,
			   uint32_t io_mask, uint32_t timeout_ms);

int mmc_setup_media(MmcCtrlr *ctrlr);

int32 mmc_trim(uint32_t start_addr, uint32_t end_addr);
lba_t block_mmc_read(BlockDevOps *me, lba_t start, lba_t count, void *buffer);
lba_t block_mmc_write(BlockDevOps *me, lba_t start, lba_t count,
		      const void *buffer);
lba_t block_mmc_erase(BlockDevOps *me, lba_t start, lba_t count);
lba_t block_mmc_fill_write(BlockDevOps *me, lba_t start, lba_t count,
			   uint32_t fill_pattern);

int mmc_retune(MmcCtrlr *ctrlr);

int mmc_init(void);

uint32_t mmc_get_capacity(void);

// Debug functions.
extern int __mmc_debug, __mmc_trace;
#define mmc_debug(format...) \
		while (__mmc_debug) { printf("mmc: " format); break; }
#define mmc_trace(format...) \
		while (__mmc_trace) { printf(format); break; }
#define mmc_error(format...) printf("mmc: ERROR: " format)

#endif /* __DRIVERS_STORAGE_MMC_H__ */
