#include <fastboot.h>
#include "../common/armlinux/config.h"
#include "rkloader.h"

//from MainLoop.c
uint32 g_bootRecovery;
uint32 g_FwEndLba;
uint32 g_BootRockusb;

uint32 krnl_load_addr = 0;

char bootloader_ver[24]="";
uint16 internal_boot_bloader_ver = 0;
uint16 update_boot_bloader_ver = 0;

uint32* gLoaderTlb ;//= (uint32*)0x60040000;

int get_bootloader_ver(char *boot_ver)
{
    int i=0;
    uint8 *buf = (uint8*)&gIdDataBuf[0];
    memset(bootloader_ver,0,24);

    if( *(uint32*)buf == 0xfcdc8c3b )
    {
        uint16 year, date;
       // GetIdblockDataNoRc4((uint8*)&gIdDataBuf[0],512);
        GetIdblockDataNoRc4((uint8*)&gIdDataBuf[128*2],512);
        GetIdblockDataNoRc4((uint8*)&gIdDataBuf[128*3],512);
        year = *(uint16*)((uint8*)buf+512+18);
        date = *(uint16*)((uint8*)buf+512+20);
        internal_boot_bloader_ver = *(uint16*)((uint8*)buf+512+22);
        //loader_tag_set_version( year<<16 |date , ver>>8 , ver&0xff );
        sprintf(bootloader_ver,"%04X-%02X-%02X#%X.%02X",
                year,
                (uint8)((date>>8)&0x00FF), (uint8)(date&0x00FF),
                (uint8)((internal_boot_bloader_ver>>8)&0x00FF), (uint8)(internal_boot_bloader_ver&0x00FF));
        return 0;
    }
    return -1;
}

uint8* g_32secbuf;
uint8* g_cramfs_check_buf;

uint8* g_pIDBlock;
uint8* g_pLoader;
uint8* g_pReadBuf;
uint8* g_pFlashInfoData;

void setup_space(uint32 begin_addr)
{
    uint32 next = 0;
    g_32secbuf = (uint8*)begin_addr;
    next += 32*528;
    g_cramfs_check_buf = (uint8*)begin_addr;
    g_pIDBlock = (uint8*)begin_addr;
    next = begin_addr + 2048*528;
    g_pLoader = (uint8*)next;
    next += 192*1024;
    g_pReadBuf = (uint8*)next;
    next += MAX_WRITE_SECTOR*528;
    g_pFlashInfoData = (uint8*)next;
    next += 2048;
}

void Switch2MSC(void)
{
#ifdef DRIVERS_USB_APP
    uint8 *paramBuffer = (uint8*)DataBuf;
    if( 0 == GetParam(parameter_lba, (void*)paramBuffer) )
    {
        int UserPartIndex;
        memset(&gBootInfo, 0, sizeof(gBootInfo));
        ParseParam( &gBootInfo, ((PLoaderParam)paramBuffer)->parameter, ((PLoaderParam)paramBuffer)->length );
        UserPartIndex = find_mtd_part(&gBootInfo.cmd_mtd, "user");
        if(UserPartIndex > 0)
        {
            extern uint32 UserPartOffset;
            UserPartOffset = gBootInfo.cmd_mtd.parts[UserPartIndex].offset;
ReConnectUsbBoot:
            RkPrintf("UserPartOffset = 0x%x\n",UserPartOffset);
            FW_ReIntForUpdate();
            MscInit();
        }
    }
    else
    {
        //RkPrintf("no  parameter\n");
        UserPartOffset = 0;
        goto ReConnectUsbBoot;
    }
#endif
}

void SysLowFormatCheck(void)
{
    if(FWLowFormatEn)
    {
        RkPrintf("FTLLowFormat,tick=%d\n" , RkldTimerGetTick());
        FW_SorageLowFormat();
        FWLowFormatEn = 0;
    }
}


#define MISC_PAGES          3
#define MISC_COMMAND_PAGE   1
#define PAGE_SIZE           (16 * 1024)//16K
#define MISC_SIZE           (MISC_PAGES * PAGE_SIZE)//48K
#define MISC_COMMAND_OFFSET (MISC_COMMAND_PAGE * PAGE_SIZE / RK_BLK_SIZE)//32

int checkMisc() {
    struct bootloader_message *bmsg = NULL;
    unsigned char buf[RK_BLK_SIZE];
    fbt_partition_t *ptn = fastboot_find_ptn(MISC_NAME);
    if (!ptn) {
        printf("misc partition not found!\n");
        return false;
    }
    bmsg = (struct bootloader_message *)buf;
    if (StorageReadLba(ptn->offset + MISC_COMMAND_OFFSET, (void *) bmsg, 1) != 0) {
        printf("failed to read misc\n");
        return false;
    }
    if(!strcmp(bmsg->command, "boot-recovery")) {
        printf("got recovery cmd from misc.\n");
        return true;
    } else if((!strcmp(bmsg->command, "bootloader")) ||
            (!strcmp(bmsg->command, "loader"))) {
        printf("got bootloader cmd from misc.\n");
        dispose_bootloader_cmd(bmsg, gBootInfo.cmd_mtd.parts+gBootInfo.index_misc);
    }
    return false;
}
void setBootloaderMsg(struct bootloader_message bmsg)
{
    int i;
    unsigned char buf[RK_BLK_SIZE];
    fbt_partition_t *ptn = fastboot_find_ptn(MISC_NAME);
    if (!ptn) {
        printf("misc partition not found!\n");
        return;
    }

    //erase misc, should use fastboot's interface?
    memset(buf, 0, sizeof(buf));
    for (i = 0;i < (MISC_SIZE/RK_BLK_SIZE);i++) {
        StorageWriteLba(ptn->offset + i, buf, 1, 0);
    }

    memcpy(buf, &bmsg, sizeof(bmsg));
    StorageWriteLba(ptn->offset + MISC_COMMAND_OFFSET, buf, 1, 0);
}

#define IDBLOCK_SN          3//the sector 3
#define IDBLOCK_SECTORS     1024
#define IDBLOCK_NUM         4
#define IDBLOCK_SIZE        512
#define SECTOR_OFFSET       528

extern uint16 g_IDBlockOffset[];
int getSn(char* buf)
{
    int i, size;
    Sector3Info *pSec3;
    int idbCount = FindAllIDB();
    if (idbCount <= 0) {
        printf("id block not found.\n");
        return false;
    }

    memset((void*)g_pIDBlock, 0, SECTOR_OFFSET * IDBLOCK_NUM);

    if (StorageReadPba(g_IDBlockOffset[0] * IDBLOCK_SECTORS, 
                (void*)g_pIDBlock, IDBLOCK_NUM) != ERR_SUCCESS) {
        printf("read id block error.\n");
        return false;
    }

    pSec3 = (Sector3Info *)(g_pIDBlock + SECTOR_OFFSET * IDBLOCK_SN);
    P_RC4((void *)pSec3, IDBLOCK_SIZE);

    size = pSec3->snSize;
    if (size <= 0 || size > SN_MAX_SIZE) {
        printf("empty serial no.\n");
        return false;
    }
    strncpy(buf, pSec3->sn, size);
    buf[size] = '\0';
    printf("sn:%s\n", buf);
    return true;
}


void checkBoot(struct fastboot_boot_img_hdr *hdr)
{
    rk_boot_img_hdr *boothdr = (rk_boot_img_hdr *)hdr;
    SecureBootCheckOK = 0;

    if (memcmp(hdr->magic, FASTBOOT_BOOT_MAGIC,
                           FASTBOOT_BOOT_MAGIC_SIZE)) {
        goto end;
    }
    if(SecureBootEn && boothdr->signTag == SECURE_BOOT_SIGN_TAG)
    {
        if(SecureBootSignCheck(boothdr->rsaHash, boothdr->hdr.id,
                    boothdr->signlen) == FTL_OK)
        {
            SecureBootCheckOK = 1;
        }
    }

end:
    if(SecureBootCheckOK == 0)
    {
        SecureBootDisable();
    }

#ifdef SECURE_BOOT_TEST
    SetSysData2Kernel(1);
#else
    SetSysData2Kernel(SecureBootCheckOK);
#endif
}

