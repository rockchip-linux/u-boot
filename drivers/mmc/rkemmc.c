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

#include <config.h>
#include <common.h>
#include <command.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>
#include <linux/list.h>
#include <div64.h>
#include "rkmmc.h"


/* Set block count limit because of 16 bit register limit on some hardware*/
#ifndef CONFIG_SYS_MMC_MAX_BLK_COUNT
#define CONFIG_SYS_MMC_MAX_BLK_COUNT 65535
#endif

static int mmcwaitbusy(void)
{
	int count;

	/* Wait max 100 ms */
	count = MAX_RETRY_COUNT;
	/* before reset ciu, it should check DATA0. if when DATA0 is low and
	it resets ciu, it might make a problem */
	while ((Readl ((gMmcBaseAddr + MMC_STATUS)) & MMC_BUSY)) {
		if(count == 0) {
			return -1;
		}
		count--;
		udelay(1);
	}

	return 0;
}

static int reset_fifo(void)
{
	int timeout;
	int status;

	status = Readl (gMmcBaseAddr + MMC_STATUS);
	if (!(status&MMC_FIFO_EMPTY)) {
		Writel(gMmcBaseAddr + MMC_CTRL, Readl(gMmcBaseAddr + MMC_CTRL) | MMC_CTRL_FIFO_RESET);
		timeout = 10000;
		while ((Readl(gMmcBaseAddr + MMC_CTRL) & (MMC_CTRL_FIFO_RESET)) && (timeout > 0)) {
			udelay(1);
			timeout--;
		}
		if (timeout == 0)
			return -1;
	}
	
	return 0;
}

static int mci_send_cmd(u32 cmd, u32 arg)
{
	volatile unsigned int RetryCount = 0;

	RetryCount = 1000;
	Writel(gMmcBaseAddr + MMC_CMD, cmd);
	while ((Readl(gMmcBaseAddr + MMC_CMD) & MMC_CMD_START) && (RetryCount > 0)) {
		udelay(1);
		RetryCount--;
	}
	if (RetryCount == 0)
		return -1;

	return 0;
}

static void emmcpoweren(char En)
{
	if (En) {
		Writel(gMmcBaseAddr + MMC_PWREN, 1);
		Writel(gMmcBaseAddr + MMC_RST_N, 1);
	} else {
		Writel(gMmcBaseAddr + MMC_PWREN, 0);
		Writel(gMmcBaseAddr + MMC_RST_N, 0);
	}
}

static void emmcreset(void)
{
	int data;

	data = ((1<<16)|(1))<<3;
	Writel(gCruBaseAddr + 0x1d8, data);
	udelay(100);
	data = ((1<<16)|(0))<<3;
	Writel(gCruBaseAddr + 0x1d8, data);
	udelay(200);
	emmcpoweren(1);
}

static void emmc_dev_reset(void)
{
	emmcpoweren(0);
	mdelay(5);	
	emmcpoweren(1);
	mdelay(1);
}

static void emmc_gpio_init(void) 
{
#ifndef CONFIG_SECOND_LEVEL_BOOTLOADER
	rk_iomux_config(RK_EMMC_IOMUX);
#endif
}

static int rk_emmc_init(struct mmc *mmc)
{
	int timeOut = 10000;

	emmc_dev_reset();
	emmcreset();
	emmc_gpio_init();
	Writel(gMmcBaseAddr + MMC_CTRL, MMC_CTRL_RESET | MMC_CTRL_FIFO_RESET);
	Writel(gMmcBaseAddr + MMC_PWREN,1);
	while ((Readl(gMmcBaseAddr + MMC_CTRL) & (MMC_CTRL_FIFO_RESET | MMC_CTRL_RESET)) && (timeOut > 0)) {
		udelay(1);
		timeOut--;
	}
	if (timeOut == 0)
		return -1;
#if USE_MMCDMA
	timeOut = 10000;
	Writel(gMmcBaseAddr + MMC_BMOD, (Readl(gMmcBaseAddr + MMC_BMOD) |BMOD_SWR));
	while ((Readl(gMmcBaseAddr + MMC_BMOD) & BMOD_SWR) && (timeOut > 0)) {
		udelay(1);
		timeOut--;
	}
	if (timeOut == 0)
		return -1;
#endif
	Writel(gMmcBaseAddr + MMC_RINTSTS, 0xFFFFFFFF);/* Clear the interrupts for the host controller */
	Writel(gMmcBaseAddr + MMC_INTMASK, 0); /* disable all mmc interrupt first */
	Writel(gMmcBaseAddr + MMC_TMOUT, 0xFFFFFF40);/* Put in max timeout */
	Writel(gMmcBaseAddr + MMC_FIFOTH, (0x3 << 28) |((FIFO_DETH/2 - 1) << 16) | ((FIFO_DETH/2) << 0));
	Writel(gMmcBaseAddr + MMC_CLKSRC, 0);

	return 0;
}

static u32 rk_mmc_prepare_command(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	u32 cmdr;

	cmdr = cmd->cmdidx;
	cmdr |= MMC_CMD_PRV_DAT_WAIT;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		/* We expect a response, so set this bit */
		cmdr |= MMC_CMD_RESP_EXP;
		if (cmd->resp_type & MMC_RSP_136)
			cmdr |= MMC_CMD_RESP_LONG;
	}
	if (cmd->resp_type & MMC_RSP_CRC)
		cmdr |= MMC_CMD_RESP_CRC;
	if (data) {
		cmdr |= MMC_CMD_DAT_EXP;
		if (data->flags & MMC_DATA_WRITE)
			cmdr |= MMC_CMD_DAT_WR;
	}

	return cmdr;
}

static int rk_mmc_start_command(struct mmc *mmc,
				  struct mmc_cmd *cmd, u32 cmd_flags)
{
	unsigned int RetryCount = 0;

	Writel(gMmcBaseAddr + MMC_CMDARG, cmd->cmdarg);
	Writel(gMmcBaseAddr + MMC_CMD, cmd_flags | MMC_CMD_START | MMC_USE_HOLD_REG);
	for (RetryCount = 0; RetryCount < 250000; RetryCount++) {
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_CMD_DONE) {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_CMD_DONE);
			break;
		}
		udelay(1);
	}
	if (RetryCount == MAX_RETRY_COUNT) {
		printf("EmmcSendCmd failed, Cmd: 0x%08x, Arg: 0x%08x\n", cmd_flags, cmd->cmdarg);
		return COMM_ERR;
	}
	if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_CMD_RES_TIME_OUT) {
		//printf("EmmcSendCmd Time out error, Cmd: 0x%08x, Arg: 0x%08x, RINTSTS: 0x%08x\n",
		//	cmd_flags,  cmd->cmdarg, (Readl(gMmcBaseAddr + MMC_RINTSTS)));
		Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_CMD_RES_TIME_OUT);
		return TIMEOUT;
	}
	if(Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_CMD_ERROR_FLAGS) {
		printf("EmmcSendCmd error, Cmd: 0x%08x, Arg: 0x%08x, RINTSTS: 0x%08x\n",
			cmd_flags,  cmd->cmdarg, (Readl(gMmcBaseAddr + MMC_RINTSTS)));
		Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_CMD_ERROR_FLAGS);
		return COMM_ERR;
	}
	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[3] = Readl(gMmcBaseAddr+MMC_RESP0);
			cmd->response[2] = Readl(gMmcBaseAddr+MMC_RESP1);
			cmd->response[1] = Readl(gMmcBaseAddr+MMC_RESP2);
			cmd->response[0] = Readl(gMmcBaseAddr+MMC_RESP3);
		} else {
			cmd->response[0] = Readl(gMmcBaseAddr+MMC_RESP0);
			cmd->response[1] = 0;
			cmd->response[2] = 0;
			cmd->response[3] = 0;
		}
	}
	return 0;
}

static int EmmcWriteData(void *Buffer, unsigned int Blocks)
{
	unsigned int *DataBuffer = Buffer;
	unsigned int FifoCount=0;
	unsigned int Count=0;
	int data_over_flag = 0;
	unsigned int Size32 = Blocks * BLKSZ / 4;

	while(Size32) {
		FifoCount = FIFO_DETH/4 - MMC_GET_FCNT(Readl(gMmcBaseAddr + MMC_STATUS));
		for (Count = 0; Count < FifoCount; Count++)
			Writel((gMmcBaseAddr + MMC_DATA), *DataBuffer++);
		Size32 -= FifoCount;
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_DATA_ERROR_FLAGS) {
			printf("Emmc::WriteSingleBlock data error, RINTSTS: 0x%08x\n",(Readl(gMmcBaseAddr + MMC_RINTSTS)));
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_DATA_ERROR_FLAGS);
			return -1;
		}
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_TXDR) {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_TXDR);
			continue;
		}
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_DATA_OVER) {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_DATA_OVER);
			Size32 = 0;
			data_over_flag = 1;
			break;
		}
	}
	if (data_over_flag == 0) {
		Count = MAX_RETRY_COUNT;
		while ((!(Readl ((gMmcBaseAddr + MMC_RINTSTS)) & MMC_INT_DATA_OVER))&&Count) {
			Count--;
	   		udelay(1);
		}
		if (Count == 0) {
			printf("write wait DTO timeout\n");
			return -1;
		} else {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_DATA_OVER);
		}
		
	}
	if (mmcwaitbusy()) {
		printf("in write wait busy time out\n");
		return -1;
	}
	if (Size32 != 0)
		return -1;

	return 0;
}

static int EmmcReadData(void *Buffer, unsigned int Blocks)
{
	unsigned int *DataBuffer = Buffer;
	unsigned int FifoCount=0;
	unsigned int Count=0;
	int data_over_flag = 0;
	unsigned int Size32 = Blocks * BLKSZ / 4;

	while (Size32) {
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_DATA_ERROR_FLAGS) {
			printf("Emmc::ReadSingleBlock data error, RINTSTS: 0x%08x\n",(Readl(gMmcBaseAddr + MMC_RINTSTS)));
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_DATA_ERROR_FLAGS);
			return -1;
		}
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_RXDR) {
			FifoCount = MMC_GET_FCNT(Readl(gMmcBaseAddr + MMC_STATUS));
			for (Count = 0; Count < FifoCount; Count++)
				*DataBuffer++ = Readl(gMmcBaseAddr + MMC_DATA);
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_RXDR);
			Size32 -= FifoCount;
		}
		if (Readl(gMmcBaseAddr + MMC_RINTSTS) & MMC_INT_DATA_OVER) {
			for (Count = 0; Count < Size32; Count++){
				*DataBuffer++ = Readl(gMmcBaseAddr + MMC_DATA);
			}
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_DATA_OVER);
			Size32 = 0;
			data_over_flag = 1;
			break;
		}
	}
	if (data_over_flag == 0) {
		Count = MAX_RETRY_COUNT;
		while ((!(Readl ((gMmcBaseAddr + MMC_RINTSTS)) & MMC_INT_DATA_OVER))&&Count){
			Count--;
	   		udelay(1);
		}
		if (Count == 0) {
			printf("read wait DTO timeout\n");
			return -1;
		} else {
			Writel(gMmcBaseAddr + MMC_RINTSTS, MMC_INT_DATA_OVER);
		}
	}
	if (mmcwaitbusy()) {
		printf("in read wait busy time out\n");
		return -1;
	}
	if (Size32 != 0)
		return -1;

	return 0;
}

#if USE_MMCDMA
static volatile SDMMC_DMA_DESC      IDMADesc[MAX_DESC_NUM_IDMAC];
static int set_idmadesc(struct mmc *mmc, void *buffer,
		uint32 BufSize)
{
	pSDC_REG_T  pReg = (pSDC_REG_T)gMmcBaseAddr;
	PSDMMC_DMA_DESC pDesc = &IDMADesc[0];
	pReg->SDMMC_DBADDR = (uint32)pDesc;
	uint32 i, size;

	for (i=0; i<MAX_DESC_NUM_IDMAC; i++, pDesc++)
	{
		size = MIN(MAX_BUFF_SIZE_IDMAC, BufSize);    
		pDesc->desc1 = ((size << DescBuf1SizeShift) & DescBuf1SizMsk);
		pDesc->desc2 =  (uint32)buffer;
		pDesc->desc0 = DescSecAddrChained | DescOwnByDma | ((i==0)? DescFirstDesc : 0) | DescDisInt;
		BufSize -= size;
		if (0 == BufSize)
			break;
		buffer += size;
		pDesc->desc3 = (uint32)(pDesc+1);
	}
	pDesc->desc0 |= DescLastDesc;
	pDesc->desc0 &= ~DescDisInt;

	return 0;
}

static int request_idma(struct mmc *mmc, struct mmc_cmd *cmd,
		struct mmc_data *data, uint32 data_len)
{
	u32 cmdflags, ret;
	uint32 value;
	uint32 timeout;
	pSDC_REG_T  pReg = (pSDC_REG_T)gMmcBaseAddr;

	timeout  = data_len*100;
	if (data->flags == MMC_DATA_READ) {
		set_idmadesc(mmc, (void*)data->dest, data_len);
	} else {
		set_idmadesc(mmc, (void*)data->src, data_len);
	}
	pReg->SDMMC_CTRL |= CTRL_USE_IDMAC;
	pReg->SDMMC_BMOD |= (BMOD_DE | BMOD_FB);
	cmdflags = rk_mmc_prepare_command(mmc, cmd, data);
	ret = rk_mmc_start_command(mmc, cmd, cmdflags);
	if (ret) {
		printf("dma send cmd err\n");
		return ret;
	}
	do {
		udelay(1);
		if ((--timeout) == 0) 
			break;
	} while ((Readl(gMmcBaseAddr + MMC_RINTSTS) & ( MMC_INT_DATA_OVER)) != (MMC_INT_DATA_OVER));
	value = pReg->SDMMC_RINISTS;
	if (value&MMC_CMD_RES_TIME_OUT) {
		ret = -1;
		printf("dma MMC_CMD_RES_TIME_OUT error\n");
	}
	else if (value&MMC_INT_SBE) {
		ret = -1;
		printf("dma MMC_INT_SBE error\n");
	}
	else if (value&MMC_INT_DTO) {
		ret = -1;
		printf("dma MMC_INT_DTO error\n");
	}
	else if (value&MMC_INT_DCRC) {
		ret = -1;
		printf("dma MMC_INT_DCRC error\n");
	}
	Writel(gMmcBaseAddr + MMC_RINTSTS, 0xffffffff);
	pReg->SDMMC_CTRL &= ~CTRL_USE_IDMAC;
	pReg->SDMMC_BMOD &= ~BMOD_DE;
	if (data->flags == MMC_DATA_WRITE) {
		timeout = 0;
		while ((value = pReg->SDMMC_STATUS) & MMC_BUSY) {
			udelay(1);
			timeout++;
			if (timeout > 250000 * 4) {
				printf("dma data busy\n");
				ret = -1;
				break;
			}
		}
	}

	return ret;
}
#endif

static int rk_emmc_request(struct mmc *mmc, struct mmc_cmd *cmd,
		struct mmc_data *data)
{
	u32 cmdflags;
	int ret;
	int Status;
	int data_len;

	if (data) {
		Status = reset_fifo();
		if (Status < 0) {
			printf("wait reset error\n");
			return Status;
		}
		Writel(gMmcBaseAddr +MMC_BYTCNT, data->blocksize*data->blocks);
		Writel(gMmcBaseAddr +MMC_BLKSIZ, data->blocksize);
#if USE_MMCDMA
		data_len = data->blocksize*data->blocks;
		if (data_len <= MAX_DATA_SIZE_IDMAC && data_len >= 512 && \
				((cmd->cmdidx== MMC_CMD_READ_SINGLE_BLOCK ||cmd->cmdidx == MMC_CMD_READ_MULTIPLE_BLOCK) \
				||(cmd->cmdidx== MMC_CMD_WRITE_SINGLE_BLOCK ||cmd->cmdidx == MMC_CMD_WRITE_MULTIPLE_BLOCK)))
			return  request_idma(mmc,cmd,data,data_len);
#endif
	}
	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION) {
		Status = reset_fifo();
		if (Status < 0) {
		      printf("wait reset error\n");
		      return Status;
  		}
	}
	cmdflags = rk_mmc_prepare_command(mmc, cmd,data);
	if (cmd->cmdidx == 0)
		cmdflags |= MMC_CMD_INIT;
	ret = rk_mmc_start_command(mmc, cmd, cmdflags);
	if (ret)
		return ret;
	if (data) {
		if (data->flags == MMC_DATA_READ) {
			ret = EmmcReadData((void*)data->dest, data->blocks);
		} else if (data->flags == MMC_DATA_WRITE) {
			ret = EmmcWriteData((void*)data->src, data->blocks);
		}
	}

	return ret;
}

static void rk_set_ios(struct mmc *mmc)
{
	int cfg = 0;
	uint suit_clk_div;
	uint src_clk;
	uint src_clk_div;
	uint second_freq;
	int value;

	switch (mmc->bus_width) {
		case 4:
			cfg = MMC_CTYPE_4BIT;
			break;
		case 8:
			cfg = MMC_CTYPE_8BIT;
			break;
		default:
			cfg = MMC_CTYPE_1BIT;
	}

	if (mmc->clock) {
		/* disable clock */
		Writel(gMmcBaseAddr + MMC_CLKENA, 0);
		/* inform CIU */
		mci_send_cmd(MMC_CMD_START | MMC_CMD_UPD_CLK | MMC_CMD_PRV_DAT_WAIT, 0);
		if (mmc->clock > mmc->cfg->f_max)
			mmc->clock = mmc->cfg->f_max;
		if (mmc->clock < mmc->cfg->f_min)
			mmc->clock = mmc->cfg->f_min;
		//rk32 emmc src generall pll, emmc automic divide setting freq to 1/2, for get the right freq, we divide this freq to 1/2
		src_clk = rkclk_get_general_pll()/2;
		src_clk_div = src_clk/mmc->clock;
		if (src_clk_div > 0x3e)
			src_clk_div = 0x3e;
		second_freq = src_clk/src_clk_div;
		suit_clk_div = (second_freq/mmc->clock);
	 	if (((suit_clk_div & 0x1) == 1) && (suit_clk_div != 1))
	        	suit_clk_div++;  //make sure this div is even number
	        if (suit_clk_div == 1)
			value =0;
		else
			value = (suit_clk_div >> 1);
		/* set clock to desired speed */
		Writel(gMmcBaseAddr + MMC_CLKDIV, value);
		/* inform CIU */
		mci_send_cmd(MMC_CMD_START | MMC_CMD_UPD_CLK | MMC_CMD_PRV_DAT_WAIT, 0);

		rkclk_emmc_set_clk(src_clk_div);
		/* enable clock */
		Writel(gMmcBaseAddr + MMC_CLKENA, MMC_CLKEN_ENABLE);
		/* inform CIU */
		mci_send_cmd(MMC_CMD_START | MMC_CMD_UPD_CLK | MMC_CMD_PRV_DAT_WAIT, 0);
		
	}

	Writel(gMmcBaseAddr + MMC_CTYPE, cfg);
}


static const struct mmc_ops rkmmc_ops = {
	.send_cmd	= rk_emmc_request,
	.set_ios	= rk_set_ios,
	.init		= rk_emmc_init,
	.getcd		= NULL,
	.getwp		= NULL,
};

static struct mmc_config rkmmc_cfg = {
	.name		= "rk emmc",
	.ops		= &rkmmc_ops,
	.host_caps	= MMC_MODE_8BIT | MMC_MODE_4BIT |MMC_MODE_HS |MMC_MODE_HS_52MHz,
	.voltages	= 0x00ff8080,
	.f_max		= MMC_BUS_CLOCK / 2,
	.f_min		= 400000,
	.b_max		= 255,
};


int rk_mmc_init(void)
{
	struct mmc *mmc = NULL;

	mmc = malloc(sizeof(struct mmc));
	if (!mmc)
		return -1;
	mmc->rca = 3;
	mmc_create(&rkmmc_cfg, NULL);

	return 0;
}

