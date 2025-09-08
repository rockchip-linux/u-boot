/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * RK3399: Architecture common definitions
 *
 * Copyright (C) 2019 Collabora Inc - https://www.collabora.com/
 *      Rohan Garg <rohan.garg@collabora.com>
 *
 * Based on puma-rk3399.c:
 *      (C) Copyright 2017 Theobroma Systems Design und Consulting GmbH
 */

#include <common.h>
#include <env.h>
#include <dm.h>
#include <part.h>
#include <hash.h>
#include <log.h>
#include <net.h>
#include <rand.h>
#include <misc.h>
#include <irq-generic.h>
#include <rk_timer_irq.h>
#include <dm/uclass-internal.h>
#include <u-boot/crc.h>
#include <u-boot/sha256.h>
#include <asm/io.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/misc.h>
#include <asm/arch-rockchip/vendor.h>

struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[768];
	/*
	 * The 'recovery' field used to be 1024 bytes.  It has only ever
	 * been used to store the recovery command line, so 768 bytes
	 * should be plenty.  We carve off the last 256 bytes to store the
	 * stage string (for multistage packages) and possible future
	 * expansion.
	 */
	char stage[32];
	char slot_suffix[32];
	char reserved[192];
};

#ifdef CONFIG_ROCKCHIP_VENDOR_PARTITION
static int rockchip_vendor_serial_number_set(void)
{
#define VENDOR_SN_MAX        513
	char serialno_str[VENDOR_SN_MAX];
	int i, len, ret;

	memset(serialno_str, 0, VENDOR_SN_MAX);
	ret = vendor_storage_read(SN_ID, serialno_str, (VENDOR_SN_MAX-1));
	if (ret < 0)
		return -ENODATA;

	len = strlen(serialno_str);
	for (i = 0; i < len; i++) {
		if ((serialno_str[i] >= 'a' && serialno_str[i] <= 'z') ||
		    (serialno_str[i] >= 'A' && serialno_str[i] <= 'Z') ||
		    (serialno_str[i] >= '0' && serialno_str[i] <= '9')) {
			continue;
		} else {
			if (i > 0)
				serialno_str[i] = 0x0;
			break;
		}
	}

	/* valid character count > 0 */
	if (i > 0) {
		serialno_str[i + 1] = 0x0;
		env_set("serial#", serialno_str);
	}

	return 0;
}

int rockchip_setup_macaddr(void)
{
#define MAX_ETHERNET	0x2
	bool need_write = false;
	bool randomed = false;
	u8 ethaddr[ARP_HLEN * MAX_ETHERNET] = {0};
	char buf[ARP_HLEN_ASCII + 1], mac[16];
	int i, ret = -EINVAL;

	ret = vendor_storage_read(LAN_MAC_ID, ethaddr, sizeof(ethaddr));
	for (i = 0; i < MAX_ETHERNET; i++) {
		if (ret <= 0 || !is_valid_ethaddr(&ethaddr[i * ARP_HLEN])) {
			if (!randomed) {
				net_random_ethaddr(&ethaddr[i * ARP_HLEN]);
				randomed = true;
			} else {
				if (i > 0) {
					memcpy(&ethaddr[i * ARP_HLEN],
					       &ethaddr[(i - 1) * ARP_HLEN],
					       ARP_HLEN);
					ethaddr[i * ARP_HLEN] |= 0x02;
					ethaddr[i * ARP_HLEN] += (i << 2);
				}
			}

			need_write = true;
		}

		if (is_valid_ethaddr(&ethaddr[i * ARP_HLEN])) {
			snprintf(buf, ARP_HLEN_ASCII + 1, "%pM", &ethaddr[i * ARP_HLEN]);
			if (i == 0)
				memcpy(mac, "ethaddr", sizeof("ethaddr"));
			else
				sprintf(mac, "eth%daddr", i);
			env_set(mac, buf);
		}
	}

	if (need_write) {
		ret = vendor_storage_write(LAN_MAC_ID,
					   ethaddr, sizeof(ethaddr));
		if (ret < 0)
			printf("%s: vendor_storage_write failed %d\n",
			       __func__, ret);
	}

	return 0;
}

#else
int rockchip_setup_macaddr(void)
{
#if CONFIG_IS_ENABLED(HASH) && CONFIG_IS_ENABLED(SHA256)
	const char *cpuid = env_get("cpuid#");
	u8 hash[SHA256_SUM_LEN];
	u8 mac_addr[6];
	int size = sizeof(hash);
	int ret;

	/* Only generate a MAC address, if none is set in the environment */
	if (env_get("ethaddr"))
		return 0;

	if (!cpuid) {
		debug("%s: could not retrieve 'cpuid#'\n", __func__);
		return -1;
	}

	ret = hash_block("sha256", (void *)cpuid, strlen(cpuid), hash, &size);
	if (ret) {
		debug("%s: failed to calculate SHA256\n", __func__);
		return -1;
	}

	/* Copy 6 bytes of the hash to base the MAC address on */
	memcpy(mac_addr, hash, 6);

	/* Make this a valid MAC address and set it */
	mac_addr[0] &= 0xfe;  /* clear multicast bit */
	mac_addr[0] |= 0x02;  /* set local assignment bit (IEEE802) */
	eth_env_set_enetaddr("ethaddr", mac_addr);

	/* Make a valid MAC address for ethernet1 */
	mac_addr[5] ^= 0x01;
	eth_env_set_enetaddr("eth1addr", mac_addr);
#endif
	return 0;
}
#endif

int rockchip_cpuid_from_efuse(const u32 cpuid_offset,
			      const u32 cpuid_length,
			      u8 *cpuid)
{
#if IS_ENABLED(CONFIG_ROCKCHIP_EFUSE) || IS_ENABLED(CONFIG_ROCKCHIP_OTP)
	struct udevice *dev;
	int ret;

	/* retrieve the device */
#if IS_ENABLED(CONFIG_ROCKCHIP_EFUSE)
	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_DRIVER_GET(rockchip_efuse), &dev);
#elif IS_ENABLED(CONFIG_ROCKCHIP_OTP)
	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_DRIVER_GET(rockchip_otp), &dev);
#endif
	if (ret) {
		printf("No efuse/otp device!\n");
		return ret;
	}

	/* read the cpu_id */
	ret = misc_read(dev, cpuid_offset, cpuid, cpuid_length);
	if (ret < 0) {
		printf("Reading hw cpuid failed, ret=%d\n", ret);
		return ret;
	}
#endif
	return 0;
}

static int rockchip_cpuid_set(const u8 *cpuid, const u32 cpuid_length)
{
	u8 low[cpuid_length / 2], high[cpuid_length / 2];
	char cpuid_str[cpuid_length * 2 + 1];
	u64 serialno;
	char serialno_str[17];
	const char *oldid;
	int i;

	memset(cpuid_str, 0, sizeof(cpuid_str));
	for (i = 0; i < 16; i++)
		sprintf(&cpuid_str[i * 2], "%02x", cpuid[i]);

	debug("cpuid: %s\n", cpuid_str);

	/*
	 * Mix the cpuid bytes using the same rules as in
	 *   ${linux}/drivers/soc/rockchip/rockchip-cpuinfo.c
	 */
	for (i = 0; i < 8; i++) {
		low[i] = cpuid[1 + (i << 1)];
		high[i] = cpuid[i << 1];
	}

	serialno = crc32_no_comp(0, low, 8);
	serialno |= (u64)crc32_no_comp(serialno, high, 8) << 32;
	snprintf(serialno_str, sizeof(serialno_str), "%016llx", serialno);

	oldid = env_get("cpuid#");
	if (oldid && strcmp(oldid, cpuid_str) != 0)
		printf("cpuid: value %s present in env does not match hardware %s\n",
		       oldid, cpuid_str);

	env_set("cpuid#", cpuid_str);

	/* Only generate serial# when none is set yet */
	if (!env_get("serial#"))
		env_set("serial#", serialno_str);

	return 0;
}

/* Priority: vendor partition > otp cpuid > rand cpuid */
int rockchip_setup_serial_number(void)
{
#define CPUID_LEN	0x10
	u8 cpuid[CPUID_LEN];
	int ret, i;

#ifdef CONFIG_ROCKCHIP_VENDOR_PARTITION
	ret = rockchip_vendor_serial_number_set();
	if (!ret)
		return 0;
#endif
	ret = rockchip_cpuid_from_efuse(CFG_CPUID_OFFSET, CPUID_LEN, cpuid);
	if (ret) {
		for (i = 0; i < CPUID_LEN; i++)
			cpuid[i] = (u8)(rand());
	}

	return rockchip_cpuid_set(cpuid, CPUID_LEN);
}

#ifdef CONFIG_USB_FUNCTION_FASTBOOT
void board_run_recovery_wipe_data(void)
{
	struct bootloader_message bmsg;
	struct blk_desc *dev_desc;
	struct disk_partition part_info;
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	u32 bcb_offset = android_bcb_msg_sector_offset();
#else
	u32 bcb_offset = BCB_MESSAGE_BLK_OFFSET;
#endif
	int cnt, ret;

	printf("Rebooting into recovery to do wipe_data\n");
	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("%s: dev_desc is NULL!\n", __func__);
		return;
	}

	ret = part_get_info_by_name(dev_desc, PART_MISC, &part_info);
	if (ret < 0) {
		printf("%s: Could not found misc partition, just run recovery\n",
		       __func__);
		goto out;
	}

	memset((char *)&bmsg, 0, sizeof(struct bootloader_message));
	strcpy(bmsg.command, "boot-recovery");
	strcpy(bmsg.recovery, "recovery\n--wipe_data");
	bmsg.status[0] = 0;
	cnt = DIV_ROUND_UP(sizeof(struct bootloader_message), dev_desc->blksz);
	ret = blk_dwrite(dev_desc, part_info.start + bcb_offset, cnt, &bmsg);
	if (ret != cnt)
		printf("Wipe data failed, ret=%d\n", ret);
out:
	/* now reboot to recovery */
	env_set("reboot_mode", "recovery");
	run_command("run bootcmd", 0);
}
#endif

#if IS_ENABLED(CONFIG_FASTBOOT)
int fastboot_set_reboot_flag(enum fastboot_reboot_reason reason)
{
	if (reason != FASTBOOT_REBOOT_REASON_BOOTLOADER)
		return -ENOTSUPP;

	printf("Setting reboot to fastboot flag ...\n");
	/* Set boot mode to fastboot */
	writel(BOOT_FASTBOOT, CONFIG_ROCKCHIP_BOOT_MODE_REG);

	return 0;
}
#endif

