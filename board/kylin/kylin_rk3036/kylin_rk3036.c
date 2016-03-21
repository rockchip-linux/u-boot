/*
 * (C) Copyright 2015 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <config.h>

#include <android_image.h>
#include <dm.h>
#include <fastboot.h>
#include <image.h>
#include <memalign.h>
#include <misc.h>
#include <part.h>
#include <asm/io.h>
#include <asm/arch/uart.h>
#include <asm/arch-rockchip/grf_rk3036.h>
#include <asm/arch/sdram_rk3036.h>
#include <asm/arch/timer.h>
#include <asm/gpio.h>
#include <linux/ctype.h>

#ifdef CONFIG_USB_GADGET
#include <usb.h>
#include <usb/s3c_udc.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#define GRF_BASE	0x20008000
static struct rk3036_grf * const grf = (void *)GRF_BASE;

static block_dev_desc_t *dev_desc = NULL;

#define RECOVERY_KEY_GPIO 87

int recovery_key_pressed(void)
{
	gpio_request(RECOVERY_KEY_GPIO, "recovery_key");
	gpio_direction_input(RECOVERY_KEY_GPIO);
	return !gpio_get_value(RECOVERY_KEY_GPIO);
}

#define FASTBOOT_KEY_GPIO 93

int fastboot_key_pressed(void)
{
	gpio_request(FASTBOOT_KEY_GPIO, "fastboot_key");
	gpio_direction_input(FASTBOOT_KEY_GPIO);
	return !gpio_get_value(FASTBOOT_KEY_GPIO);
}

#define ROCKCHIP_BOOT_MODE_FASTBOOT	0x5242C309

int fb_set_reboot_flag(void)
{
	writel(ROCKCHIP_BOOT_MODE_FASTBOOT, &grf->os_reg[4]);
	return 0;
}

#define SLOT_A			"_a"
#define SLOT_B			"_b"
#define SLOT_BOOT_A		"boot" SLOT_A
#define SLOT_BOOT_B		"boot" SLOT_B
#define SLOT_PART_NAME(slot)	(!slot ? SLOT_BOOT_A : SLOT_BOOT_B)
#define SLOT_IS_ERR(slot)	(slot < 0 || slot > 1)
#define SLOT_NAME(slot) \
	(SLOT_IS_ERR(slot) ? "ERROR" : (!slot ? SLOT_A : SLOT_B))
#define SLOT_OTHER(slot)	(!slot)
#define SLOT_INDEX(name)	(name ? name[1] - 'a' : 0)
#define ALL_SLOTS		SLOT_A "," SLOT_B

typedef union _slot_attributes {
	struct {
		u64 required_to_function:1;
		u64 no_block_io_protocol:1;
		u64 legacy_bios_bootable:1;
		u64 priority:4;
		u64 tries:4;
		u64 successful:1;
		u64 reserved:36;
		u64 type_guid_specific:16;
	} fields;
	unsigned long long raw;
} __packed slot_attributes;

typedef struct _slot_partition {
	int		part;
	lbaint_t	start;
	lbaint_t	size;
	uchar		name[PARTNAME_SZ + 1];
	slot_attributes	attr;
} slot_partition;

static int slot_has_slot(const char *part, int slot)
{
	disk_partition_t info;
	char slot_name[PARTNAME_SZ / 2];

	snprintf(slot_name, sizeof(slot_name), "%s%s", part, SLOT_NAME(slot));

	if (!dev_desc)
		return -1;

	if (get_partition_info_efi_by_name(dev_desc, slot_name, &info) < 0)
		return -1;

	return 1;
}

static void gpt_to_slot(gpt_entry *gpt_pte, int part, slot_partition *slot_part)
{
        int i;
        for (i = 0; i < PARTNAME_SZ; i++) {
                u8 c;
                c = gpt_pte[part].partition_name[i] & 0xff;
                c = (c && !isprint(c)) ? '.' : c;
                slot_part->name[i] = c;
        }
        slot_part->name[PARTNAME_SZ] = 0;

	slot_part->part = part;
	slot_part->attr.raw = gpt_pte[part].attributes.raw;
	slot_part->start = (lbaint_t)le64_to_cpu(gpt_pte[part].starting_lba);
	slot_part->size = (lbaint_t)le64_to_cpu(gpt_pte[part].ending_lba) + 1
		- slot_part->start;
}

static int slot_is_bootable(slot_partition *slot_part)
{
	slot_attributes *attr = &slot_part->attr;

	if ((attr->fields.successful || attr->fields.tries) &&
	    attr->fields.priority)
		return 1;

	attr->fields.priority = 0;
	return -1;
}

static int slot_get_part(int slot,
		  slot_partition *slot_part)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1,
			dev_desc->blksz);
	gpt_entry *gpt_pte = NULL;
	int ret = 0;
	int i;

	ret = gpt_verify_headers(dev_desc, gpt_head,
			&gpt_pte);
	if (ret)
		return -1;

	for (i = 0; i < GPT_ENTRY_NUMBERS; i++) {
		gpt_to_slot(gpt_pte, i, slot_part);

		if (strcmp(SLOT_PART_NAME(slot),
			   (const char *)slot_part->name) == 0) {
			/* matched */
			printf("Slot-%d:(s:%d, t:%d, p:%d)\n",
			       slot, slot_part->attr.fields.successful,
			       slot_part->attr.fields.tries,
			       slot_part->attr.fields.priority);
			return 1;
		}
	}

	return -1;
}

static int slot_apply(slot_partition slot_part)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1,
			dev_desc->blksz);
	gpt_entry *gpt_pte = NULL;
	int ret = 0;

	ret = gpt_verify_headers(dev_desc, gpt_head,
			&gpt_pte);
	if (ret)
		return -1;

	if (gpt_pte[slot_part.part].attributes.raw == slot_part.attr.raw)
		return 0;

	gpt_pte[slot_part.part].attributes.raw = slot_part.attr.raw;

	printf("Apply attr for part-%d(s:%d, t:%d, p:%d)\n",
			slot_part.part, slot_part.attr.fields.successful,
			slot_part.attr.fields.tries,
			slot_part.attr.fields.priority);

	gpt_head->my_lba = cpu_to_le64(1);
	gpt_head->alternate_lba = cpu_to_le64(dev_desc->lba - 1);
	gpt_head->partition_entry_lba = cpu_to_le64(2);
	gpt_head->header_crc32 = 0;

	return write_gpt_table(dev_desc, gpt_head, gpt_pte);
}

static int slot_set_active(int slot, int active)
{
	slot_partition slot_part;
	int ret = 0;

	ret = slot_get_part(slot, &slot_part);
	if (ret < 0)
		return -1;

	printf("%s:%s, active:%d\n", __func__, SLOT_NAME(slot), active);

	if (active) {
		slot_part.attr.fields.priority = 15;
		slot_part.attr.fields.tries = 7;
	} else {
		slot_part.attr.fields.priority = 0;
		slot_part.attr.fields.tries = 0;
	}

	ret = slot_apply(slot_part);
	if (ret < 0)
		return -1;

	if (!active)
		return 0;

	ret = slot_get_part(SLOT_OTHER(slot), &slot_part);
	if (ret < 0)
		return -1;

	if (slot_part.attr.fields.priority)
		slot_part.attr.fields.priority = 14;

	ret = slot_apply(slot_part);
	if (ret < 0)
		return -1;

	return 0;
}

static int slot_image_size(int slot)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(struct andr_img_hdr, hdr, 1,
			dev_desc->blksz);
	slot_partition slot_part;
	int ret = 0;

	ret = slot_get_part(slot, &slot_part);
	if (ret < 0)
		return -1;

	if (dev_desc->block_read(0, slot_part.start, 2, hdr) < 0) {
		printf("Error reading blocks\n");
		return -1;
	}

	return android_image_get_end(hdr) - (ulong)hdr;
}

static int slot_check(int slot)
{
	ALLOC_CACHE_ALIGN_BUFFER_PAD(struct andr_img_hdr, hdr, 1,
			dev_desc->blksz);
	slot_partition slot_part;
	int ret = 0;

	ret = slot_get_part(slot, &slot_part);
	if (ret < 0)
		return -1;

	if (dev_desc->block_read(0, slot_part.start, 2, hdr) < 0) {
		printf("Error reading blocks\n");
		return -1;
	}

	ret = android_image_check_header(hdr);
	if (ret)
		return -1;

	return 0;
}

static int slot_get_bootable(void)
{
	int slot = -1;
	slot_partition part_a, part_b;

	part_a.attr.raw = part_b.attr.raw = 0;

	slot_get_part(0, &part_a);
	slot_get_part(1, &part_b);

	if (slot_is_bootable(&part_a) >= 0)
		slot = 0;

	if (slot_is_bootable(&part_b) >= 0) {
		if (SLOT_IS_ERR(slot))
			slot = 1;
		if (part_b.attr.fields.priority > part_a.attr.fields.priority)
			slot = 1;
	}

	slot_apply(part_a);
	slot_apply(part_b);

	printf("%s: %s\n", __func__, SLOT_NAME(slot));
	return slot;
}

static int slot_get_current(void)
{
	int slot = -1;

	do {
		slot = slot_get_bootable();

		if (SLOT_IS_ERR(slot))
			return -1;

		if (slot_check(slot) < 0) {
			slot_set_active(slot, 0);
			continue;
		}
	} while (SLOT_IS_ERR(slot));

	printf("%s: %s\n", __func__, SLOT_NAME(slot));
	return slot;
}

static int slot_boot(int slot)
{
	slot_partition slot_part;
	char buf[128];
	int ret;

	ret = slot_get_part(slot, &slot_part);
	if (ret < 0)
		return -1;

	if (slot_part.attr.fields.tries)
		slot_part.attr.fields.tries -= 1;

	slot_apply(slot_part);

	snprintf(buf, sizeof(buf), "%lx", slot_part.start);
	setenv("boot_start", buf);
	snprintf(buf, sizeof(buf), "%lx",
		 slot_image_size(slot) / dev_desc->blksz);
	setenv("boot_size", buf);

	setenv("slot_suffix", SLOT_NAME(slot));

	setenv("bootloader", BOOTLOADER_VERSION);

	run_command("env set bootargs "
			"androidboot.serialno=${serial#} "
			"androidboot.bootloader=${bootloader} "
			"androidboot.slot_suffix=${slot_suffix}\0", 0);

	run_command("mmc read ${loadaddr} ${boot_start} ${boot_size};" \
			"bootm start ${loadaddr}; bootm ramdisk;" \
			"bootm prep; bootm go;\0", 0);
	return 0;
}

static const char hex_asc[] = "0123456789abcdef";
#define hex_asc_lo(x)   hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)   hex_asc[((x) & 0xf0) >> 4]

static inline char *pack_hex_byte(char *buf, u8 byte)
{
	*buf++ = hex_asc_hi(byte);
	*buf++ = hex_asc_lo(byte);
        return buf;
}

int setup_serialno(void)
{
#define SERIAL_NUMBER_LEN 16

	struct udevice *dev;
	char serialno[SERIAL_NUMBER_LEN + 1];
	char buf[SERIAL_NUMBER_LEN >> 1];
	int ret, i;

	/* the first misc device will be used */
	ret = uclass_first_device(UCLASS_MISC, &dev);
	if (ret || !dev)
		return -1;
	ret = misc_read(dev, 7, &buf, sizeof(buf));
	if (ret || !buf[0])
		return -1;

	memset(serialno, 0, sizeof(serialno));

	for (i = 0; i < sizeof(buf); i++) {
		pack_hex_byte(serialno + i*2, buf[i]);
	}

	setenv("serial#", serialno);
	return 0;
}

int board_late_init(void)
{
	int boot_mode = readl(&grf->os_reg[4]);

	/* Clear boot mode */
	writel(0, &grf->os_reg[4]);

	if (setup_serialno() < 0) {
		setenv("serial#", DEFAULT_SERIAL_NUMBER);
		printf("Failed to set serialno, use default one!\n");
	}

	dev_desc = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN)
		dev_desc = NULL;

	if (boot_mode == ROCKCHIP_BOOT_MODE_FASTBOOT ||
	    fastboot_key_pressed()) {
		run_command("echo Enter fastboot!; fastboot 0;", 0);
	}

	if (recovery_key_pressed()) {
		setenv("bootdelay", "2");
		return 0;
	}
	setenv("bootdelay", "0");

	if (dev_desc) {
		int slot = slot_get_current();

		if (!SLOT_IS_ERR(slot))
			slot_boot(slot);
	}

	return 0;
}

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

int fb_locked(void)
{
	return getenv_yesno("fb_lock");
}

#define FASTBOOT_ENABLE_UNLOCK 0
int fb_lock(int lock)
{
#if FASTBOOT_ENABLE_UNLOCK
	if (!lock && fb_locked())
		return -1;
#endif

	setenv("fb_lock", lock ? "1" : "0");
	saveenv();
	return 1;
}

void fb_getvar_all(char* response, size_t chars_left)
{
	fastboot_info("product: " PRODUCT_NAME);
	fastboot_info("bootloader-version: " BOOTLOADER_VERSION);
	fastboot_info("version-baseband: N/A");
	fastboot_info("variant: ");
	fastboot_info("off-mode-charge: 0");
	fastboot_info("battery-voltage: 0mV");
	fastboot_info("battery-soc-ok: yes");
	fastboot_info("secure: %s", fb_locked() ? "yes" : "no");
	fastboot_info("unlocked: %s", !fb_locked() ? "yes" : "no");
	fastboot_info("slot-suffixes: " ALL_SLOTS);
	fastboot_info("current-slot: %s", SLOT_NAME(slot_get_current()));

	if (dev_desc) {
		slot_partition part;
		disk_partition_t info;
		int i;
		int slot;

		for (i = 1; i < GPT_ENTRY_NUMBERS; i++) {
			if (get_partition_info_efi(dev_desc, i, &info) < 0)
				break;
			fastboot_info("partition-size:%s: %016lx", info.name, info.size);
			fastboot_info("partition-type:%s: raw", info.name);

			if (strcmp_l1("boot", (const char *)info.name) != 0)
				continue;

			slot = !!strcmp(SLOT_BOOT_A, (const char *)info.name);

			slot_get_part(slot, &part);

			fastboot_info("slot-successful:%s: %s", info.name,
					part.attr.fields.successful ? "yes" : "no");
			fastboot_info("slot-unbootable:%s: %s", info.name,
					slot_is_bootable(&part) < 0 ? "yes" : "no");
			fastboot_info("slot-retry-count:%s: %d", info.name,
					part.attr.fields.tries);
		}
	}
}

int fb_getvar(char *cmd, char* response, size_t chars_left)
{
	if (!strcmp_l1("version", cmd)) {
		strncat(response, "0.4", chars_left);
	} else if (!strcmp_l1("bootloader-version", cmd)) {
		strncat(response, BOOTLOADER_VERSION, chars_left);
	} else if (!strcmp_l1("all", cmd)) {
		fb_getvar_all(response, chars_left);
	} else if (!strcmp_l1("product", cmd)) {
		strncat(response, PRODUCT_NAME, chars_left);
	} else if (!strcmp_l1("bootloader-version", cmd)) {
		strncat(response, BOOTLOADER_VERSION, chars_left);
	} else if (!strcmp_l1("version-baseband", cmd)) {
		strncat(response, "N/A", chars_left);
	} else if (!strcmp_l1("variant", cmd)) {
		strncat(response, "\0", chars_left);
	} else if (!strcmp_l1("off-mode-charge", cmd)) {
		strncat(response, "0", chars_left);
	} else if (!strcmp_l1("battery-voltage", cmd)) {
		strncat(response, "0mV", chars_left);
	} else if (!strcmp_l1("battery-soc-ok", cmd)) {
		strncat(response, "yes", chars_left);
	} else if (!strcmp_l1("secure", cmd)) {
		strncat(response, fb_locked() ? "yes" : "no", chars_left);
	} else if (!strcmp_l1("unlocked", cmd)) {
		strncat(response, !fb_locked() ? "yes" : "no", chars_left);
	} else if (!strcmp_l1("partition-type:", cmd)) {
		strncat(response, "raw", chars_left);
	} else if (!strcmp_l1("downloadsize", cmd) ||
			!strcmp_l1("max-download-size", cmd)) {
		char str_num[12];

		sprintf(str_num, "0x%08x", CONFIG_FASTBOOT_BUF_SIZE);
		strncat(response, str_num, chars_left);
	} else if (!strcmp_l1("has-slot:", cmd)) {
		strsep(&cmd, ":");
		if (slot_has_slot(cmd, 0) < 0 || slot_has_slot(cmd, 1) < 0)
			strncat(response, "no", chars_left);
		else
			strncat(response, "yes", chars_left);
	} else if (!strcmp_l1("slot-suffixes", cmd)) {
		strncat(response, ALL_SLOTS, chars_left);
	} else if (!strcmp_l1("slot-successful:", cmd)) {
		slot_partition part;

		strsep(&cmd, ":");
		if (slot_get_part(SLOT_INDEX(cmd), &part) < 0 ||
		    !part.attr.fields.successful)
			strncat(response, "no", chars_left);
		else
			strncat(response, "yes", chars_left);
	} else if (!strcmp_l1("slot-unbootable:", cmd)) {
		slot_partition part;

		strsep(&cmd, ":");
		if (slot_get_part(SLOT_INDEX(cmd), &part) < 0 &&
		    slot_is_bootable(&part) < 0)
			strncat(response, "yes", chars_left);
		else
			strncat(response, "no", chars_left);
	} else if (!strcmp_l1("slot-retry-count:", cmd)) {
		slot_partition part;

		strsep(&cmd, ":");
		slot_get_part(SLOT_INDEX(cmd), &part);
		strncat(response, simple_itoa(part.attr.fields.tries), chars_left);
	} else if (!strcmp_l1("current-slot", cmd)) {
		strncat(response, SLOT_NAME(slot_get_current()), chars_left);
	} else
		return -1;

	return 0;
}

int fb_unknown_command(char *cmd, char* response, size_t chars_left)
{
	if (!strcmp_l1("set_active:", cmd)) {
		if (!dev_desc) {
			strcpy(response, "FAILinvalid mmc device");
			return -1;
		}

		strsep(&cmd, ":");
		if (!strcmp_l1(SLOT_A, cmd) || !strcmp_l1(SLOT_B, cmd)) {
			slot_set_active(SLOT_INDEX(cmd), 1);
			strcpy(response, "OKAY");
		} else
			strcpy(response, "FAIL");
	} else if (!strcmp_l1("flashing", cmd)) {
		strsep(&cmd, " ");

		if (!strcmp_l1("get_unlock_ability", cmd)) {
			strcpy(response, "OKAY0");
		} else if (!strcmp_l1("unlock_critical", cmd)) {
			strcpy(response, "OKAY");
		} else if (!strcmp_l1("lock_critical", cmd)) {
			strcpy(response, "FAIL");
		} else if (!strcmp_l1("unlock", cmd)) {
			strcpy(response, (fb_lock(0) < 0) ? "FAIL" : "OKAY");
		} else if (!strcmp_l1("lock", cmd)) {
			strcpy(response, (fb_lock(1) < 0) ? "FAIL" : "OKAY");
		} else
			return -1;
	} else
		return -1;

	return 0;
}

int board_init(void)
{
	rockchip_timer_init();

	return 0;
}

#ifdef CONFIG_USB_GADGET
#define RKIO_GRF_PHYS				0x20008000
#define RKIO_USBOTG_BASE			0x10180000
#define RK_USB_PHY_CONTROL			0x10180e00

static struct s3c_plat_otg_data rk_otg_data = {
	.regs_phy	= RKIO_GRF_PHYS,
	.regs_otg	= RKIO_USBOTG_BASE,
	.usb_phy_ctrl	= RK_USB_PHY_CONTROL,
	.usb_gusbcfg	= 0x00001408
};

int board_usb_init(int index, enum usb_init_type init)
{
	debug("%s: performing s3c_udc_probe\n", __func__);
	return s3c_udc_probe(&rk_otg_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	debug("%s\n", __func__);
	return 0;
}
#endif

int dram_init(void)
{
	gd->ram_size = sdram_size();

	return 0;
}

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif
