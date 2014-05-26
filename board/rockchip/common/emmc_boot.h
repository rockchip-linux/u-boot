#ifndef _EMMC_BOOT_H
#define _EMMC_BOOT_H
extern int32 emmcInit(void);
extern uint32 emmcReadID(void *buf);
extern uint32 emmcBootReadPBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec );
extern uint32 emmcBootWritePBA(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec );
extern uint32 emmcBootReadLBA(uint8 ChipSel,uint32 LBA , void *pbuf , uint16 nSec);
extern uint32 emmcBootWriteLBA(uint8 ChipSel,uint32 LBA,  void *pbuf ,uint16 nSec  ,uint16 mode);
extern uint32 emmcBootErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
extern int emmcReadFlashInfo(void *buf);
extern void emmcCheckIdBlock(void);
extern uint32 emmcGetCapacity(uint8 ChipSel);
extern uint32 emmcSysDataLoad(uint8 ChipSel, uint32 Index,void *Buf);
extern uint32 emmcSysDataStore(uint8 ChipSel, uint32 Index,void *Buf);
#endif