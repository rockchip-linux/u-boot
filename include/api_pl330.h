#ifndef __API_PL330_H__
#define __API_PL330_H__
typedef enum _DMA_RET
{
    DMAOK = 0,
    DMAERR = -1,
    DMABUSY = -100
}eDMA_RET;

typedef enum _DMA_MODE
{
    //DMAC0
	DMA_PERI_UART0_TX=0,
	DMA_PERI_UART0_RX,
	DMA_PERI_UART1_TX,
	DMA_PERI_UART1_RX,
	//DMA_PERI_I2S8ch_TX,
	//DMA_PERI_I2S8ch_RX,
	DMA_PERI_I2S_TX=6,
	DMA_PERI_I2S_RX,
	DMA_PERI_SPDIF_TX,
	//DMA_PERI_I2S2ch_TX=10,
	//DMA_PERI_I2S2ch_RX,
	DMA_PERI_DAMC0MAX,//
	//DMAC2
	//DMA_PERI_TSI,
	DMA_PERI_TSI_TX,
	DMA_PERI_TSI_RX,
	DMA_PERI_SDMMC_TX,
	DMA_PERI_SDMMC_RX,
	DMA_PERI_SDIO_TX,
	DMA_PERI_SDIO_RX,
	DMA_PERI_EMMC_TX,
	DMA_PERI_EMMC_RX,
	DMA_PERI_PIDFILTER_TX,
	DMA_PERI_PIDFILTER_RX,
	DMA_PERI_UART2_TX=32+6,
	DMA_PERI_UART2_RX,
	DMA_PERI_UART3_TX,
	DMA_PERI_UART3_RX,
	DMA_PERI_SPI0_TX,
	DMA_PERI_SPI0_RX,
	DMA_PERI_SPI1_TX,
	DMA_PERI_SPI1_RX,
	DMA_PERI_DMAC2MAX,//
	//M2M
	DMA_M2M_DMAC0=64,
	DMA_M2M_DMAC2,
	DMA_MODE_MAX
}eDMA_MODE;

enum dma0_peri {
	DMA0_PERI_UART0_TX=0,
	DMA0_PERI_UART0_RX,
	DMA0_PERI_UART1_TX,
	DMA0_PERI_UART1_RX,
	DMA0_PERI_I2S8ch_TX,
	DMA0_PERI_I2S8ch_RX,
	DMA0_PERI_I2S1ch_TX,
	DMA0_PERI_I2S1ch_RX,
	DMA0_PERI_SPDIF_TX,
	DMA0_PERI_I2S2ch_TX,
	DMA0_PERI_I2S2ch_RX,
};
enum dma2_peri {
	DMA2_PERI_TSI=0,
	DMA2_PERI_SDMMC,
	DMA2_PERI_SDIO=3,
	DMA2_PERI_EMMC,
	DMA2_PERI_PIDFILTER,
	DMA2_PERI_UART2_TX,
	DMA2_PERI_UART2_RX,
	DMA2_PERI_UART3_TX,
	DMA2_PERI_UART3_RX,
	DMA2_PERI_SPI0_TX,
	DMA2_PERI_SPI0_RX,
	DMA2_PERI_SPI1_TX,
	DMA2_PERI_SPI1_RX,
};

int     DMAInit(void);
void     DMADeInit(void);
int DMAStart(uint32 dstAddr, uint32 srcAddr, uint32 size, eDMA_MODE mode,pFunc CallBack);
int DMAStartM2M(uint32 dstAddr, uint32 srcAddr, uint32 size, uint32 bDMA2,pFunc CallBack);
int DMAStartI2S(uint32 dstAddr, uint32 srcAddr, uint32 size, 
                     enum dma0_peri iperi, pFunc CallBack);
#if 0
eDMA_RET DMAStopSDMMC(void);
eDMA_RET DMAStartSDMMC(uint32 dstAddr, uint32 srcAddr, uint32 size, eDMA_MODE mode,pFunc CallBack);
int pl330_probe(int bDMA2);
#endif
#endif
