/*
 * spi driver for rockchip
 *
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RK_SPI_H
#define __RK_SPI_H


/* SPI register offsets */
#define SPI_CTRLR0			0x0000
#define SPI_CTRLR1			0x0004
#define SPI_ENR				0x0008
#define SPI_SER				0x000c
#define SPI_BAUDR			0x0010
#define SPI_TXFTLR			0x0014
#define SPI_RXFTLR			0x0018
#define SPI_TXFLR			0x001c
#define SPI_RXFLR			0x0020
#define SPI_SR				0x0024
#define SPI_IPR				0x0028
#define SPI_IMR				0x002c
#define SPI_ISR				0x0030
#define SPI_RISR			0x0034
#define SPI_ICR				0x0038
#define SPI_DMACR			0x003c
#define SPI_DMATDLR			0x0040
#define SPI_DMARDLR			0x0044
#define SPI_TXDR			0x0400
#define SPI_RXDR			0x0800


/* --------Bit fields in CTRLR0--------begin */

#define SPI_DFS_OFFSET			0                  /* Data Frame Size */
#define SPI_DFS_MASK			0x3
#define SPI_DFS_4BIT			0x00
#define SPI_DFS_8BIT			0x01
#define SPI_DFS_16BIT			0x02
#define SPI_DFS_RESV			0x03

#define SPI_CFS_OFFSET			2                  /* Control Frame Size */
#define SPI_CFS_MASK			0xF

#define SPI_SCPH_OFFSET			6                  /* Serial Clock Phase */
#define SPI_SCPH_MASK			0x1
#define SPI_SCPH_TOGMID			0                  /* Serial clock toggles in middle of first data bit */
#define SPI_SCPH_TOGSTA			1                  /* Serial clock toggles at start of first data bit */

#define SPI_SCOL_OFFSET			7                  /* Serial Clock Polarity */
#define SPI_SCOL_MASK			0x1
#define SPI_SCOL_LOW			0                  /* Inactive state of clock serial clock is low */
#define SPI_SCOL_HIGH			1                  /* Inactive state of clock serial clock is high */

#define SPI_CSM_OFFSET			8                  /* Chip Select Mode */
#define SPI_CSM_MASK			0x3
#define SPI_CSM_KEEP			0x00               /* ss_n keep low after every frame data is transferred */
#define SPI_CSM_HALF			0x01               /* ss_n be high for half sclk_out cycles after every frame data is transferred */
#define SPI_CSM_ONE			0x02               /* ss_n be high for one sclk_out cycle after every frame data is transferred */
#define SPI_CSM_RESV			0x03

#define SPI_SSN_DELAY_OFFSET		10                 /* SSN to Sclk_out delay */
#define SPI_SSN_DELAY_MASK		0x1
#define SPI_SSN_DELAY_HALF		0x00               /* the peroid between ss_n active and sclk_out active is half sclk_out cycles */
#define SPI_SSN_DELAY_ONE		0x01               /* the peroid between ss_n active and sclk_out active is one sclk_out cycle */

#define SPI_SEM_OFFSET			11                 /* Serial Endian Mode */
#define SPI_SEM_MASK			0x1
#define SPI_SEM_LITTLE			0x00               /* little endian */
#define SPI_SEM_BIG			0x01               /* big endian */

#define SPI_FBM_OFFSET			12                 /* First Bit Mode */
#define SPI_FBM_MASK			0x1
#define SPI_FBM_MSB			0x00               /* first bit in MSB */
#define SPI_FBM_LSB			0x01               /* first bit in LSB */

#define SPI_HALF_WORLD_TX_OFFSET	13                 /* Byte and Halfword Transform */
#define SPI_HALF_WORLD_MASK		0x1
#define SPI_HALF_WORLD_ON		0x00               /* apb 16bit write/read, spi 8bit write/read */
#define SPI_HALF_WORLD_OFF		0x01               /* apb 8bit write/read, spi 8bit write/read */

#define SPI_RXDSD_OFFSET		14                 /* Rxd Sample Delay */
#define SPI_RXDSD_MASK			0x3

#define SPI_FRF_OFFSET			16                 /* Frame Format */
#define SPI_FRF_MASK			0x3
#define SPI_FRF_SPI			0x00               /* motorola spi */
#define SPI_FRF_SSP			0x01               /* Texas Instruments SSP*/
#define SPI_FRF_MICROWIRE		0x02               /*  National Semiconductors Microwire */
#define SPI_FRF_RESV			0x03

#define SPI_TMOD_OFFSET			18                 /* Transfer Mode */
#define SPI_TMOD_MASK			0x3
#define	SPI_TMOD_TR			0x00		   /* xmit & recv */
#define SPI_TMOD_TO			0x01		   /* xmit only */
#define SPI_TMOD_RO			0x02		   /* recv only */
#define SPI_TMOD_RESV			0x03

#define SPI_OMOD_OFFSET			20                 /* Operation Mode */
#define SPI_OMOD_MASK			0x1
#define	SPI_OMOD_MASTER			0x00		   /* Master Mode */
#define SPI_OMOD_SLAVE			0x01		   /* Slave Mode */

/* --------Bit fields in CTRLR0--------end */


/* Bit fields in SR, 7 bits */
#define SR_MASK				0x7f		/* cover 7 bits */
#define SR_BUSY				(1 << 0)
#define SR_TF_FULL			(1 << 1)
#define SR_TF_EMPT			(1 << 2)
#define SR_RF_EMPT			(1 << 3)
#define SR_RF_FULL			(1 << 4)

/* Bit fields in ISR, IMR, RISR, 7 bits */
#define SPI_INT_TXEI			(1 << 0)
#define SPI_INT_TXOI			(1 << 1)
#define SPI_INT_RXUI			(1 << 2)
#define SPI_INT_RXOI			(1 << 3)
#define SPI_INT_RXFI			(1 << 4)

/* Bit fields in DMACR */
#define SPI_DMACR_TX_ENABLE		(1 << 1)
#define SPI_DMACR_RX_ENABLE		(1 << 0)

/* Bit fields in ICR */
#define SPI_CLEAR_INT_ALL		(1<< 0)
#define SPI_CLEAR_INT_RXUI		(1 << 1)
#define SPI_CLEAR_INT_RXOI		(1 << 2)
#define SPI_CLEAR_INT_TXOI		(1 << 3)


struct rk_spi_slave {
	struct spi_slave 	slave;
	void __iomem 		*regs;

	size_t			len;
	void			*tx;
	void			*tx_end;
	void			*rx;
	void			*rx_end;

	u8			n_bytes;	/* current is a 1/2 bytes op */
	u8			bits_per_word;	/* maxim is 16b */

	unsigned int 		speed_hz;
	unsigned int 		mode;
	unsigned int 		tmode;

	int			(*write)(struct rk_spi_slave *spi);
	int			(*read)(struct rk_spi_slave *spi);
	void (*cs_ctrl)(struct rk_spi_slave *spi, u32 cs, u8 flag);
};


static inline struct rk_spi_slave *to_rk_spi(struct spi_slave *slave)
{
	return container_of(slave, struct rk_spi_slave, slave);
}


#if defined(CONFIG_RKCHIP_RK3288)
#define RK_SPI_BUS_MAX			3
#else
#define RK_SPI_BUS_MAX			2
#endif
#define RK_SPI_CS_MAX			2

/* rk spi cs assert and deassert */
#define RK_SPI_CS_DEASSERT		0
#define RK_SPI_CS_ASSERT		1

#define RK_SPI_MAX_FREQ			48000000

#define rkspi_readl(spi, offset)		__raw_readl(spi->regs + offset)
#define rkspi_writel(spi, offset, value)	__raw_writel(value, spi->regs + offset)
#define rkspi_readw(spi, offset) 		__raw_readw(spi->regs + offset)
#define rkspi_writew(spi, offset, value) 	__raw_writel(value, spi->regs + offset)

#endif /* __RK_SPI_H */

