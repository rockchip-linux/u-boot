#ifndef __REBOOT_MODE_H
#define __REBOOT_MODE_H

/* high 24 bits is tag, low 8 bits is type */
#define REBOOT_FLAG		0x5242C300
/* cold boot */
#define BOOT_COLD		0
/* normal boot */
#define BOOT_NORMAL		(REBOOT_FLAG + 0)
/* enter loader rockusb mode */
#define BOOT_LOADER		(REBOOT_FLAG + 1)
/* enter recovery */
#define BOOT_RECOVERY		(REBOOT_FLAG + 3)
/* reboot by panic */
#define BOOT_PANIC		(REBOOT_FLAG + 7)
/* reboot by watchdog */
#define BOOT_WATCHDOG		(REBOOT_FLAG + 8)
/* enter fastboot mode */
#define BOOT_FASTBOOT		(REBOOT_FLAG + 9)
/* enter charging mode */
#define BOOT_CHARGING		(REBOOT_FLAG + 11)
/* enter usb mass storage mode */
#define BOOT_UMS		(REBOOT_FLAG + 12)
/* enter dfu download mode */
#define BOOT_DFU                (REBOOT_FLAG + 13)
/* reboot system quiescent */
#define BOOT_QUIESCENT		(REBOOT_FLAG + 14)
/* enter bootrom download mode */
#define BOOT_BROM_DOWNLOAD	0xEF08A53C

/* This is a copy from Android boot loader */
enum _boot_mode {
	BOOT_MODE_NORMAL = 0,
	BOOT_MODE_RECOVERY,
	BOOT_MODE_BOOTLOADER,	/* Android: Fastboot mode */
	BOOT_MODE_LOADER,	/* Rockchip: Rockusb download mode */
	BOOT_MODE_CHARGING,
	BOOT_MODE_UMS,
	BOOT_MODE_BROM_DOWNLOAD,
	BOOT_MODE_PANIC,
	BOOT_MODE_WATCHDOG,
	BOOT_MODE_DFU,
	BOOT_MODE_QUIESCENT,
	BOOT_MODE_UNDEFINE,
};

#ifndef __ASSEMBLY__
int setup_boot_mode(void);
#endif

#define BCB_MESSAGE_BLK_OFFSET		(16 * 1024 >> 9)

#endif
