





#ifndef _NANDFLASH_BOOT_H
#define _NANDFLASH_BOOT_H
extern uint32 lMemApiInit(uint32 BaseAddr);
extern int LMemApiReadId(uint32 chipSel , void *pbuf);
extern int LMemApiReadPba(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec );
extern int LMemApiWritePba(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec );
extern int LMemApiReadLba(uint8 ChipSel, uint32 LBA ,void *pbuf  , uint16 nSec);
extern int LMemApiWriteLba(uint8 ChipSel, uint32 LBA, void *pbuf  , uint16 nSec  ,uint16 mode);
extern uint32 LMemApiGetCapacity(uint8 ChipSel);
extern uint32 LMemApiSysDataLoad(uint8 ChipSel, uint32 Index,void *Buf);
extern uint32 LMemApiSysDataStore(uint8 ChipSel, uint32 Index,void *Buf);
extern int LMemApiErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod);
extern int LMemApiLowFormat();
extern int LMemApiFlashInfo( void *pbuf);
#endif