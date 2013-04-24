#include <fastboot.h>
#include "../common/armlinux/config.h"

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
