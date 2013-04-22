#include <fastboot.h>
#include "../common/armlinux/config.h"

#define MISC_PAGES          3
#define MISC_COMMAND_PAGE   1
#define PAGE_SIZE           (16 * 1024)//16K
#define MISC_SIZE           (MISC_PAGES * PAGE_SIZE)//48K
#define MISC_COMMAND_OFFSET (MISC_SIZE / RK_BLK_SIZE)//32

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

