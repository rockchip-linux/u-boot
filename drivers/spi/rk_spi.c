/*
 * spi driver for rockchip
 *
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>
#include "rk_spi.h"

static void rkspi_dump_regs(struct rk_spi_slave *spi) {
	debug("RK SPI registers:\n");
	debug("=================================\n");
	debug("CTRL0: \t\t0x%08x\n", rkspi_readl(spi, SPI_CTRLR0));
	debug("CTRL1: \t\t0x%08x\n", rkspi_readl(spi, SPI_CTRLR1));
	debug("SSIENR: \t\t0x%08x\n", rkspi_readl(spi, SPI_ENR));
	debug("SER: \t\t0x%08x\n", rkspi_readl(spi, SPI_SER));
	debug("BAUDR: \t\t0x%08x\n", rkspi_readl(spi, SPI_BAUDR));
	debug("TXFTLR: \t\t0x%08x\n", rkspi_readl(spi, SPI_TXFTLR));
	debug("RXFTLR: \t\t0x%08x\n", rkspi_readl(spi, SPI_RXFTLR));
	debug("TXFLR: \t\t0x%08x\n", rkspi_readl(spi, SPI_TXFLR));
	debug("RXFLR: \t\t0x%08x\n", rkspi_readl(spi, SPI_RXFLR));
	debug("SR: \t\t0x%08x\n", rkspi_readl(spi, SPI_SR));
	debug("IMR: \t\t0x%08x\n", rkspi_readl(spi, SPI_IMR));
	debug("ISR: \t\t0x%08x\n", rkspi_readl(spi, SPI_ISR));
	debug("DMACR: \t\t0x%08x\n", rkspi_readl(spi, SPI_DMACR));
	debug("DMATDLR: \t0x%08x\n", rkspi_readl(spi, SPI_DMATDLR));
	debug("DMARDLR: \t0x%08x\n", rkspi_readl(spi, SPI_DMARDLR));
	debug("=================================\n");
}

static inline void rkspi_enable_chip(struct rk_spi_slave *spi, int enable)
{
	rkspi_writel(spi, SPI_ENR, (enable ? 1 : 0));
}

static inline void rkspi_set_clk(struct rk_spi_slave *spi, u16 div)
{
	rkspi_writel(spi, SPI_BAUDR, div);
}

static int rkspi_wait_till_not_busy(struct rk_spi_slave *spi)
{
	unsigned int delay = 1000;

	while (delay--) {
		if (!(rkspi_readw(spi, SPI_SR) & SR_BUSY)) {
			return 0;
		}

		udelay(1);
	}

	printf("RK SPI: Status keeps busy for 1000us after a read/write!\n");
	return -1;
}

static int rkspi_wait_till_tf_empty(struct rk_spi_slave *spi)
{
	unsigned int delay = 1000;

	while (delay--) {
		if (rkspi_readw(spi, SPI_SR) & SR_TF_EMPT) {
			return 0;
		}

		udelay(1);
	}

	printf("DW SPI: Status noe empty for 1000us after a read/write!\n");
	return -1;
}

static void rkspi_flush(struct rk_spi_slave *spi)
{
	while (!(rkspi_readw(spi, SPI_SR) & SR_RF_EMPT))
		rkspi_readw(spi, SPI_RXDR);

	rkspi_wait_till_not_busy(spi);
}

static void rkspi_cs_control(struct rk_spi_slave *spi, uint32 cs, u8 flag)
{
	if (flag)
		rkspi_writel(spi, SPI_SER, 1 << cs);
	else 		
		rkspi_writel(spi, SPI_SER, 0);
}

static void rkspi_iomux_init(unsigned int bus, unsigned int cs)
{
    rk_spi_iomux_config(RK_SPI0_CS0_IOMUX+2*bus+cs);
}

static int rkspi_null_writer(struct rk_spi_slave *spi)
{
	u8 n_bytes = spi->n_bytes;

	if ((rkspi_readw(spi, SPI_SR) & SR_TF_FULL)
		|| (spi->tx == spi->tx_end))
		return 0;

	rkspi_writew(spi, SPI_TXDR, 0);
	spi->tx += n_bytes;

	return 1;
}

static int rkspi_null_reader(struct rk_spi_slave *spi)
{
	u8 n_bytes = spi->n_bytes;

	while ((!(rkspi_readw(spi, SPI_SR) & SR_RF_EMPT))
		&& (spi->rx < spi->rx_end)) {
		rkspi_readw(spi, SPI_RXDR);
		spi->rx += n_bytes;
	}
	rkspi_wait_till_not_busy(spi);

	return spi->rx == spi->rx_end;
}

static int rkspi_u8_writer(struct rk_spi_slave *spi)
{	
	rkspi_dump_regs(spi);

	if ((rkspi_readw(spi, SPI_SR) & SR_TF_FULL)
		|| (spi->tx == spi->tx_end))
		return 0;

	rkspi_writew(spi, SPI_TXDR, *(u8 *)(spi->tx));
	++spi->tx;

	return 1;
}

static int rkspi_u8_reader(struct rk_spi_slave *spi)
{
	rkspi_dump_regs(spi);

	while (!(rkspi_readw(spi, SPI_SR) & SR_RF_EMPT)
		&& (spi->rx < spi->rx_end)) {
		*(u8 *)(spi->rx) = rkspi_readw(spi, SPI_RXDR) & 0xFFU;
		++spi->rx;
	}

	rkspi_wait_till_not_busy(spi);

	return spi->rx == spi->rx_end;
}

static int rkspi_u16_writer(struct rk_spi_slave *spi)
{	
	if ((rkspi_readw(spi, SPI_SR) & SR_TF_FULL)
		|| (spi->tx == spi->tx_end))
		return 0;

	rkspi_writew(spi, SPI_TXDR, *(u16 *)(spi->tx));
	spi->tx += 2;

	return 1;
}

static int rkspi_u16_reader(struct rk_spi_slave *spi)
{
	u16 temp;

	while (!(rkspi_readw(spi, SPI_SR) & SR_RF_EMPT)
		&& (spi->rx < spi->rx_end)) {
		temp = rkspi_readw(spi, SPI_RXDR);
		*(u16 *)(spi->rx) = temp;
		spi->rx += 2;
	}

	rkspi_wait_till_not_busy(spi);
	return spi->rx == spi->rx_end;
}

static void rkspi_set_speed(struct rk_spi_slave *spi, uint speed)
{
	u16 clk_div = 0;

	if (spi->speed_hz != speed) {
		/* clk_div doesn't support odd number */
		clk_div = (768*1000*1000 / 2 / 4) / speed;
		clk_div = (clk_div + 1) & 0xfffe;

		rkspi_set_clk(spi, clk_div);

		spi->speed_hz = speed;
	}
}


static inline void rkspi_io_config(struct rk_spi_slave *spi, unsigned int len, const void *dout, void *din)
{
	if (spi->n_bytes == 1) {
		spi->read = rkspi_u8_reader;
		spi->write = rkspi_u8_writer;
	} else if (spi->n_bytes == 2) {
		spi->read = rkspi_u16_reader;
		spi->write = rkspi_u16_writer;
	} else {
		spi->read = rkspi_null_reader;
		spi->write = rkspi_null_writer;
	}

	if (spi->tx == NULL && spi->rx == NULL) {
		spi->read = rkspi_null_reader;
		spi->write = rkspi_null_writer;
	} else if (spi->tx == NULL) {
		spi->write = rkspi_null_writer;
	} else if (spi->rx == NULL) {
		spi->read = rkspi_null_reader;
	}

	rkspi_writew(spi, SPI_CTRLR1, len-1);
}


static int rkspi_xfer_pio(struct rk_spi_slave *spi, unsigned int len,
			const void *dout, void *din, unsigned long flags)
{
	spi->tx = (void *)dout;
	spi->tx_end = spi->tx + len;
	spi->rx = (void *)din;
	spi->rx_end = spi->rx + len;

	rkspi_io_config(spi, len, dout, din);

	rkspi_enable_chip(spi, 1);

	/* Assert CS before transfer */
	if (flags & SPI_XFER_BEGIN) {
		spi_cs_activate(&spi->slave);
	}

	while (spi->write(spi)) {
		rkspi_wait_till_not_busy(spi);
		spi->read(spi);
	}

	/* Deassert CS after transfer */
	if (flags & SPI_XFER_END) {
		spi_cs_deactivate(&spi->slave);
	}

	rkspi_enable_chip(spi, 0);

	return 0;
}


void spi_init(void)
{

}


int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	if ((bus >= RK_SPI_BUS_MAX) || (cs >= RK_SPI_CS_MAX))
		return 0;

	return 1;
}

void spi_cs_activate(struct spi_slave *slave)
{
	struct rk_spi_slave *spi = to_rk_spi(slave);

	if (spi->cs_ctrl != NULL) {
	    spi->cs_ctrl(spi, slave->cs, RK_SPI_CS_ASSERT);
	} else {
		rkspi_writel(spi, SPI_SER, 1 << slave->cs);
	}
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	struct rk_spi_slave *spi = to_rk_spi(slave);

	if (spi->cs_ctrl != NULL) {
	    spi->cs_ctrl(spi, slave->cs, RK_SPI_CS_DEASSERT);
	} else {
		rkspi_writel(spi, SPI_SER, 0 << slave->cs);
	}
}

void spi_set_speed(struct spi_slave *slave, uint hz)
{
	struct rk_spi_slave *spi = to_rk_spi(slave);

	rkspi_set_speed(spi, hz);
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
				  unsigned int max_hz, unsigned int mode)
{
	struct rk_spi_slave	*spi;
	void __iomem *regs;

	switch (bus) {
	case 0:
		regs = (void __iomem *)RKIO_SPI0_BASE;
		break;
	case 1:
		regs = (void __iomem *)RKIO_SPI1_BASE;
		break;
	case 2:
		regs = (void __iomem *)RKIO_SPI2_BASE;
		break;
	default:
		printf("SPI error: unsupported bus %i. \
			Supported busses 0 - 1\n", bus);
		return NULL;
	}

	if (cs >= RK_SPI_CS_MAX) {
		printf("SPI error: unsupported chip select %i \
			on bus %i\n", cs, bus);
		return NULL;
	}

	if (max_hz > RK_SPI_MAX_FREQ) {
		printf("SPI error: unsupported frequency %i Hz. \
			Max frequency is 48 Mhz\n", max_hz);
		return NULL;
	}

	if (mode > SPI_MODE_3) {
		printf("SPI error: unsupported SPI mode %i\n", mode);
		return NULL;
	}

	spi = spi_alloc_slave(struct rk_spi_slave, bus, cs);
	if (!spi) {
		printf("SPI error: malloc of SPI structure failed\n");
		return NULL;
	}

	rkspi_iomux_init(bus, cs);

	spi->mode = mode; /* spi mode 0...3 */
	spi->regs = regs;
	spi->bits_per_word = 8;
	spi->tmode = SPI_TMOD_TR; /* Tx & Rx */
	spi->speed_hz = max_hz;

	spi->cs_ctrl = NULL;
	spi->read = rkspi_null_reader;
	spi->write = rkspi_null_writer;

	return &spi->slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct rk_spi_slave *spi = to_rk_spi(slave);

	free(spi);
}

int spi_claim_bus(struct spi_slave *slave)
{
	struct rk_spi_slave *spi = to_rk_spi(slave);
	u32 ctrlr0 = 0;
	u16 clk_div = 0;	
	u8 spi_dfs = 0, spi_tf = 0;

	/* Disable the SPI hardware */
	rkspi_enable_chip(spi, 0);

	switch (spi->bits_per_word) {
		case 8:
			spi->n_bytes = 1;
			spi_dfs = SPI_DFS_8BIT;
			spi_tf = SPI_HALF_WORLD_OFF;
			break;
		case 16:
			spi->n_bytes = 2;
			spi_dfs = SPI_DFS_16BIT;
			spi_tf = SPI_HALF_WORLD_ON;
			break;
		default:
			printf("MRST SPI: unsupported bits: %dbits\n", spi->bits_per_word);
	}

	/* Calculate clock divisor.  */
	clk_div = (768*1000*1000 / 2 / 4) / spi->speed_hz;
	clk_div = (clk_div + 1) & 0xfffe;
	rkspi_set_clk(spi, clk_div);

	/* Operation Mode */
	ctrlr0 = (SPI_OMOD_MASTER << SPI_OMOD_OFFSET);

	/* Data Frame Size */
	ctrlr0 |= (spi_dfs & SPI_DFS_MASK) << SPI_DFS_OFFSET;

	/* set SPI mode 0..3 */
	if (spi->mode & SPI_CPOL)
		ctrlr0 = (SPI_SCOL_HIGH << SPI_SCOL_OFFSET);
	if (spi->mode & SPI_CPHA)
		ctrlr0 = (SPI_SCPH_TOGSTA << SPI_SCPH_OFFSET);

	/* Chip Select Mode */
	ctrlr0 |= (SPI_CSM_KEEP << SPI_CSM_OFFSET);

	/* SSN to Sclk_out delay */
	ctrlr0 |= (SPI_SSN_DELAY_ONE << SPI_SSN_DELAY_OFFSET);

	/* Serial Endian Mode */
	ctrlr0 |= (SPI_SEM_LITTLE << SPI_SEM_OFFSET);

	/* First Bit Mode */
	ctrlr0 |= (SPI_FBM_MSB << SPI_FBM_OFFSET);

	/* Byte and Halfword Transform */
	ctrlr0 |= ((spi_tf & SPI_HALF_WORLD_MASK) << SPI_HALF_WORLD_TX_OFFSET);

	/* Rxd Sample Delay */
	ctrlr0 |= (0 << SPI_RXDSD_OFFSET);

	/* Frame Format */
	ctrlr0 |= (SPI_FRF_SPI << SPI_FRF_OFFSET);

	/* Tx and Rx mode */
	ctrlr0 |= (spi->tmode & SPI_TMOD_MASK) << SPI_TMOD_OFFSET;

	rkspi_writel(spi, SPI_CTRLR0, ctrlr0);

	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
	return;
}


int spi_xfer(struct spi_slave *slave, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	struct rk_spi_slave *spi = to_rk_spi(slave);
	unsigned int len;
	int ret;

	len = bitlen >> 3;

	debug("spi_xfer: slave %u:%u dout %08X din %08X bitlen %u\n",
	      slave->bus, slave->cs, *(uint *) dout, *(uint *) din, bitlen);

	ret = rkspi_xfer_pio(spi, len, dout, din, flags);

	return ret;
}

