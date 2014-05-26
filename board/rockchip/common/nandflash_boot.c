#include    "config.h"
#include "nandflash_boot.h"


//#define LMemApiFlashInfo        ReadFlashInfo
pLOADER_MEM_API_T gp_loader_api = NULL;
uint32 gMedia = 0;
int LMemApiReadId(uint32 chipSel , void *pbuf)
{
    int ret = FTL_ERROR;
    if(gp_loader_api->ReadId)
    {
        gp_loader_api->ReadId(chipSel, pbuf);
        ret = FTL_OK;
    }
    return ret;
}

int LMemApiFlashInfo( void *pbuf)
{
    int ret = FTL_ERROR;
    if(gp_loader_api->ReadInfo)
    {
       gp_loader_api->ReadInfo(pbuf);
       ret = FTL_OK;
    }
    return ret;
}

int LMemApiLowFormat()
{
    int ret = FTL_ERROR;
    if(gp_loader_api->LowFormat)
    {
       gp_loader_api->LowFormat();
       ret = FTL_OK;
    }
    return ret;
}

int LMemApiErase(uint8 ChipSel, uint32 blkIndex, uint32 nblk, uint8 mod)
{
    int ret = FTL_ERROR;
    if(gp_loader_api->Erase)
    {
       gp_loader_api->Erase(ChipSel, blkIndex, nblk, mod);
       ret = FTL_OK;
    }
    return ret;
}

//int LMemApiReadPba(uint32 PBA , void *pbuf, uint16 nSec )
int LMemApiReadPba(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec )
{
    int ret = FTL_ERROR;
    if(gp_loader_api->ReadPba)
       ret = gp_loader_api->ReadPba(0, PBA, pbuf, nSec );
    return ret;
}

//int LMemApiWritePba(uint32 PBA , void *pbuf, uint16 nSec )
int LMemApiWritePba(uint8 ChipSel, uint32 PBA , void *pbuf, uint16 nSec )
{
    int ret = FTL_ERROR;
    if(gp_loader_api->WritePba)
       ret = gp_loader_api->WritePba(0, PBA , pbuf, nSec);
    return ret;
}

//int LMemApiReadLba( uint32 LBA ,void *pbuf  , uint16 nSec)
int LMemApiReadLba(uint8 ChipSel, uint32 LBA ,void *pbuf  , uint16 nSec)
{
    int ret = FTL_ERROR;
    if(gp_loader_api->ReadLba) {
       ret = gp_loader_api->ReadLba(0, LBA , nSec, pbuf);
       if(ret)
          printf("LMemApiReadLba:%d\n", ret);
    }
    return ret;
}

//int LMemApiWriteLba( uint32 LBA, void *pbuf  , uint16 nSec  ,uint16 mode)
int LMemApiWriteLba(uint8 ChipSel, uint32 LBA, void *pbuf  , uint16 nSec  ,uint16 mode)
{
    int ret = FTL_ERROR;
    if(gp_loader_api->WriteLba)
       ret = gp_loader_api->WriteLba(0, LBA , nSec, pbuf);
    return ret;
}

uint32 FtlDeInit()
{
    uint32 ret = FTL_ERROR;
    if(gp_loader_api->ftl_deinit)
       ret = gp_loader_api->ftl_deinit();
    return ret;
}

uint32 FlashDeInit()
{
    uint32 ret = FTL_ERROR;
    if(gp_loader_api->flash_deinit)
       ret = gp_loader_api->flash_deinit();
    return ret;
}

//uint32 LMemApiGetCapacity(void)
uint32 LMemApiGetCapacity(uint8 ChipSel)
{
    uint32 ret = FTL_ERROR;
    if(gp_loader_api->GetCapacity)
       ret = gp_loader_api->GetCapacity(gpMemFun->id);
    return ret;
}

//uint32 LMemApiSysDataLoad(uint32 Index,void *Buf)
uint32 LMemApiSysDataLoad(uint8 ChipSel, uint32 Index,void *Buf)
{
    uint32 ret = FTL_ERROR;
    ftl_memset(Buf,0,512);
    if(gp_loader_api->SysDataLoad)
       ret = gp_loader_api->SysDataLoad(gpMemFun->id, Index,Buf);
    return ret;
}

//uint32 LMemApiSysDataStore(uint32 Index,void *Buf)
uint32 LMemApiSysDataStore(uint8 ChipSel, uint32 Index,void *Buf)
{
    uint32 ret = FTL_ERROR;
    if(gp_loader_api->SysDataStore)
       ret = gp_loader_api->SysDataStore(gpMemFun->id, Index,Buf);
    return ret;
}

//uint32 lMemApiInit(void)
uint32 lMemApiInit(uint32 BaseAddr)
{
	printf("use uboot as second level loader\n");
    gp_loader_api = (pLOADER_MEM_API_T)(*((uint32*)CONFIG_RKNAND_API_ADDR)); // get api table
    if((gp_loader_api->tag & 0xFFFF0000) == 0x4e460000)
    {     //nand                   emmc
        if(gp_loader_api->id==1 || gp_loader_api->id==2)
        {
            return 0; 
        }
        else if(gp_loader_api->id==2)
        {
            return 2; 
        }
        else
        {
            return -1;
        }        
    }
    else
    {
        return -1;
        //error   
    }
}

void rknand_print_hex(char* prefix, char* buf, int count, int size) {
    printf("%s:\n", prefix);
    int i, j;
    for (i = 0;i < count;i++) {
        for (j = 0;j < size;j++) {
            printf("%02x", buf[i * size + j]);
        }
        printf("\n");
    }
    printf("\n");
}
uint8 testbuf[1024];
uint32 loaderapitest(void)
{
    gMedia = lMemApiInit(0);
    if(gMedia == 1) // nand flash
    {
        uint32 i,blksize;
        FLASH_INFO flashInfo;
        LMemApiFlashInfo(&flashInfo);
        blksize = flashInfo.BlockSize;
        //test
        LMemApiReadId(0,testbuf);
        rknand_print_hex("id",testbuf,1, 16);
        for(i=0;i<20;i++)
        {
            printf("read idb = %x\n",i);
            LMemApiReadPba(0, i*blksize,testbuf,1);
            rknand_print_hex("idb",testbuf,1, 16);
        }
    }
    return 0;
}















