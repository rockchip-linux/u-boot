/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
#include <fastboot.h>
#include "../common/armlinux/config.h"
#include "rkloader.h"
#include "rkimage.h"


DECLARE_GLOBAL_DATA_PTR;


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
    next += 1024*1024;
    g_pReadBuf = (uint8*)next;
    next += MAX_WRITE_SECTOR*528;
    g_pFlashInfoData = (uint8*)next;
    next += 2048;
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

#define MaxFlashReadSize  16384  //8MB
int32 CopyFlash2Memory(uint32 dest_addr, uint32 src_addr, uint32 total_sec)
{
    uint8 * pSdram = (uint8*)dest_addr;
    uint16 sec = 0;
    uint32 LBA = src_addr;
    uint32 remain_sec = total_sec;

//  RkPrintf("Enter >> src_addr=0x%08X, dest_addr=0x%08X, total_sec=%d\n", src_addr, dest_addr, total_sec);

//  RkPrintf("(0x%X->0x%X)  size: %d\n", src_addr, dest_addr, total_sec);

    while(remain_sec > 0)
    {
        sec = (remain_sec > MaxFlashReadSize) ? MaxFlashReadSize : remain_sec;
        if(StorageReadLba(LBA,(uint8*)pSdram, sec) != 0)
        {
            return -1;
        }
        remain_sec -= sec;
        LBA += sec;
        pSdram += sec*512;
    }

//  RkPrintf("Leave\n");
    return 0;
}

int CopyMemory2Flash(uint32 src_addr, uint32 dest_offset, int sectors)
{
    uint16 sec = 0;
    uint32 remain_sec = sectors;

    while(remain_sec > 0)
    {
        sec = (remain_sec>32)?32:remain_sec;

        if(StorageWriteLba(dest_offset, src_addr, sec, 0) != 0)
        {
            return -2;
        }

        remain_sec -= sec;
        src_addr += sec*512;
        dest_offset += sec;
    }

    return 0;
}

void fixInitrd(PBootInfo pboot_info, int ramdisk_addr, int ramdisk_sz)
{
    ramdisk_sz = (ramdisk_sz + 0x3FFFF)&0xFFFF0000;//64KB ¶ÔÆë
#define MAX_BUF_SIZE 100
    char str[MAX_BUF_SIZE];
    char *cmd_line = strdup(pboot_info->cmd_line);
    char *s_initrd_start = NULL;
    char *s_initrd_end = NULL;
    int len = 0;

    if (!cmd_line)
		return;

    s_initrd_start = strstr(cmd_line, "initrd=");
    if (s_initrd_start) {
        len = strlen(cmd_line);
        s_initrd_end = strstr(s_initrd_start, " ");
        if (!s_initrd_end)
            *s_initrd_start = '\0';
        else {
            len = cmd_line + len - s_initrd_end;
            memcpy(s_initrd_start, s_initrd_end, len);
            *(s_initrd_start + len) = '\0';
        }
    }
    snprintf(str, sizeof(str), "initrd=0x%08X,0x%08X", ramdisk_addr, ramdisk_sz);

#ifndef CONFIG_OF_LIBFDT
    snprintf(pboot_info->cmd_line, sizeof(pboot_info->cmd_line),
            "%s %s", str, cmd_line);
#else
    snprintf(pboot_info->cmd_line, sizeof(pboot_info->cmd_line),
            "%s", str);
#endif
    free(cmd_line);
}

int execute_cmd(PBootInfo pboot_info, char* cmdlist, bool* reboot)
{
    char* cmd = cmdlist;

    *reboot = FALSE;
    while(*cmdlist)
    {
        if(*cmdlist=='\n') *cmdlist='\0';
        ++cmdlist;
    }

    while(*cmd)
    {
        PRINT_I("bootloader cmd: %s\n", cmd);

        if( !strcmp(cmd, "update-bootloader") )// Éý¼¶ bootloader
        {
            PRINT_I("--- update bootloader ---\n");
            if( update_loader(false)==0 )
            {// cmy: Éý¼¶Íê³ÉºóÖØÆô
                *reboot = TRUE;
            }
            else
            {// cmy: Éý¼¶Ê§°Ü
                return -1;
            }
        }
        else
            PRINT_I("Unsupport cmd: %s\n", cmd);

        cmd += strlen(cmd)+1;
    }
    return 0;
}

#define MISC_PAGES          3
#define MISC_COMMAND_PAGE   1
#define PAGE_SIZE           (16 * 1024)//16K
#define MISC_SIZE           (MISC_PAGES * PAGE_SIZE)//48K
#define MISC_COMMAND_OFFSET (MISC_COMMAND_PAGE * PAGE_SIZE / RK_BLK_SIZE)//32

int checkMisc() {
    struct bootloader_message *bmsg = NULL;
    unsigned char buf[DIV_ROUND_UP(sizeof(struct bootloader_message),
            RK_BLK_SIZE) * RK_BLK_SIZE];
    fbt_partition_t *ptn = fastboot_find_ptn(MISC_NAME);
    if (!ptn) {
        printf("misc partition not found!\n");
        return false;
    }
    bmsg = (struct bootloader_message *)buf;
    if (StorageReadLba(ptn->offset + MISC_COMMAND_OFFSET, buf, DIV_ROUND_UP(
                    sizeof(struct bootloader_message), RK_BLK_SIZE)) != 0) {
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
int setBootloaderMsg(struct bootloader_message* bmsg)
{
    unsigned char buf[DIV_ROUND_UP(sizeof(struct bootloader_message),
            RK_BLK_SIZE) * RK_BLK_SIZE];
    memcpy(buf, bmsg, sizeof(struct bootloader_message));
    fbt_partition_t *ptn = fastboot_find_ptn(MISC_NAME);
    if (!ptn) {
        printf("misc partition not found!\n");
        return -1;
    }

    return CopyMemory2Flash(&buf, ptn->offset + MISC_COMMAND_OFFSET,
            DIV_ROUND_UP(sizeof(struct bootloader_message), RK_BLK_SIZE));
}

void getParameter() {
    int i = 0;
    cmdline_mtd_partition *cmd_mtd;
    if (!GetParam(0, DataBuf)) {
        ParseParam( &gBootInfo, ((PLoaderParam)DataBuf)->parameter, \
                ((PLoaderParam)DataBuf)->length );
        cmd_mtd = &(gBootInfo.cmd_mtd);
        for(i = 0;i < cmd_mtd->num_parts;i++) {
            fbt_partitions[i].name = strdup(cmd_mtd->parts[i].name);
            fbt_partitions[i].offset = cmd_mtd->parts[i].offset;
            if (cmd_mtd->parts[i].size == SIZE_REMAINING) {
                fbt_partitions[i].size_kb = SIZE_REMAINING;
            } else {
                fbt_partitions[i].size_kb = cmd_mtd->parts[i].size >> 1;
            }
            printf("partition(%s): offset=0x%08X, size=0x%08X\n", \
                    cmd_mtd->parts[i].name, cmd_mtd->parts[i].offset, \
                    cmd_mtd->parts[i].size);
        }
    }
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

int fixHdr(struct fastboot_boot_img_hdr *hdr)
{
    hdr->ramdisk_addr = gBootInfo.ramdisk_load_addr;
#ifndef CONFIG_USE_PARAMETER_INITRD_ADDR
    //load it to fastboot_buf.
    hdr->ramdisk_addr = (u8 *)gd->arch.fastboot_buf_addr;
    printf("fix ramdisk_addr:%p\n", hdr->ramdisk_addr);
#endif

    //set kernel addr at 32M.
    hdr->kernel_addr = gd->bd->bi_dram[0].start + 32 * 1024 * 1024;
    printf("fix kernel_addr:%p\n", hdr->kernel_addr);
    return 0;
}

int secureCheck(struct fastboot_boot_img_hdr *hdr, int unlocked)
{
    rk_boot_img_hdr *boothdr = (rk_boot_img_hdr *)hdr;

    SecureBootCheckOK = 0;

    if (memcmp(hdr->magic, FASTBOOT_BOOT_MAGIC,
                           FASTBOOT_BOOT_MAGIC_SIZE)) {
        goto end;
    }

    if(!unlocked && SecureBootEn &&
           boothdr->signTag == SECURE_BOOT_SIGN_TAG)
    {
        if(SecureBootSignCheck(boothdr->rsaHash, boothdr->hdr.id,
                    boothdr->signlen) == FTL_OK)
        {
            SecureBootCheckOK = 1;
        } else {
            printf("SecureBootSignCheck failed\n");
        }
    }

end:
    printf("SecureBootCheckOK:%d\n", SecureBootCheckOK);
    if(SecureBootCheckOK == 0)
    {
        SecureBootDisable();
    }

#ifdef SECURE_BOOT_TEST
    SetSysData2Kernel(1);
#else
    SetSysData2Kernel(SecureBootCheckOK);
#endif
    return 0;
}

int eraseDrmKey() {
    char buf[RK_BLK_SIZE];
    memset(buf, 0, RK_BLK_SIZE);
    StorageSysDataStore(1, buf);
    gDrmKeyInfo.publicKeyLen = 0;
    return 0;
}

#define FDT_PATH        "rk-kernel.dtb"
const char* get_fdt_name() {
    if (!gBootInfo.fdt_name[0]) {
        return FDT_PATH;
    }
    return gBootInfo.fdt_name;
}
