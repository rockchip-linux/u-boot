
#include <common.h>
#include <asm/arch/drivers.h>

#define BIT(nr)			(1UL << (nr))

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

#define MAX_RETRY_COUNT (250000)  //250ms
#define MMC_FIFO_BASE	0x200
#define MMC_DATA	MMC_FIFO_BASE
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
#define MMC_FIFO_EMPTY                     BIT(2)
#define MMC_BUSY			(MMC_MC_BUSY | MMC_DATA_BUSY)
/* FIFO threshold register defines */
#define FIFO_DETH			512

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

#define MMC_BUS_CLOCK  96000000

#define Readl(addr)                     (*(volatile u32 *)(addr))
#define Writel(addr, v)              	(*(volatile u32 *)(addr) = v)

#define BLKSZ 0x200
#define gMmcBaseAddr  EMMC_BASE_ADDR
#define gCruBaseAddr    CRU_BASE_ADDR
#define gGrfBaseAddr	 REG_FILE_BASE_ADDR

#define emmc_dbg(x...)           printk(BIOS_DEBUG, ## x)
#define emmc_info(x...)          printk(BIOS_INFO, ## x)
#define emmc_err(x...)           printk(BIOS_ERR, ## x)


typedef enum {
  READ = 0,
  WRITE,
} OPERATION_TYPE;















