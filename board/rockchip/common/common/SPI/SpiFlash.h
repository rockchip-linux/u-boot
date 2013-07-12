
#ifndef SPIFLASH_H
#define SPIFLASH_H
//#include "fs_comm.h"


enum _FLASH_ERASE_TYPE
{
    CHIP_ERASE = 0,
    BLOCK_ERASE,
    SECTOR_ERASE
};
//#define     PAGE_SIZE           256
//#define     NOR_PAGE_SIZE       256
//#define     NOR_SECTOR_SIZE     4096
//#define     NOR_BLOCK_SIZE      1024*64
#define     READ_DATA           0x03
#define     BYTE4_READ          0x13
#define     BYTE_WRITE          0x02
#define     BYTE4_WRITE         0x12

#define     WRITE_ENABLE        0x06
#define     WRITE_DISABLE       0x04
#define     READ_AD             0x9F

extern uint32 SPIFlashInit(uint32 SpiBaseAddr);
extern void SPIFlashReadID(uint8 ChipSel,void * buf);
extern uint32 SPIFlashWrite(uint32 addr, void *pData, uint32 len);
extern uint32 SPIFlashRead(uint32 addr, void *pData, uint32 len);
extern  uint32 Chiperase(void);


#endif

