// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <version.h>
#include <button.h>
#include <cpu_func.h>
#include <debug_uart.h>
#include <dm.h>
#include <env_fragment.h>
#include <hang.h>
#include <image.h>
#include <init.h>
#include <led.h>
#include <log.h>
#include <misc.h>
#include <mtd_blk.h>
#include <power/fuel_gauge.h>
#include <ram.h>
#include <serial.h>
#include <spl.h>
#include <spl_ab.h>
#include <time.h>
#include <asm/arch-rockchip/bootrom.h>
#ifdef CONFIG_ROCKCHIP_PRELOADER_ATAGS
#include <asm/arch-rockchip/atags.h>
#endif
#include <asm/arch-rockchip/pcie_ep_boot.h>
#include <asm/arch-rockchip/sdram.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/sys_proto.h>
#include <asm/arch-rockchip/param.h>
#include <asm/arch-rockchip/hwid_dtb.h>
#include <asm/arch-rockchip/meta.h>
#include <asm/arch-rockchip/timer.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/bitops.h>

DECLARE_GLOBAL_DATA_PTR;

int board_return_to_bootrom(struct spl_image_info *spl_image,
			    struct spl_boot_device *bootdev)
{
	back_to_bootrom(BROM_BOOT_NEXTSTAGE);

	return 0;
}

__weak const char * const boot_devices[BROM_LAST_BOOTSOURCE + 1] = {
};

__weak void spl_rk_board_prepare_for_jump(struct spl_image_info *spl_image)
{
}

const char *board_spl_was_booted_from(void)
{
	static u32 brom_bootsource_id_cache = BROM_BOOTSOURCE_UNKNOWN;
	u32 bootdevice_brom_id;
	const char *bootdevice_ofpath = NULL;

	if (brom_bootsource_id_cache != BROM_BOOTSOURCE_UNKNOWN)
		bootdevice_brom_id = brom_bootsource_id_cache;
	else
		bootdevice_brom_id = readl(BROM_BOOTSOURCE_ID_ADDR);

	if ((bootdevice_brom_id & BROM_DOWNLOAD_MASK) == BROM_DOWNLOAD_MASK)
		bootdevice_brom_id = BROM_BOOTSOURCE_USB;

	bootdevice_brom_id = bootdevice_brom_id & BROM_BOOTSOURCE_MASK;
	if (bootdevice_brom_id < ARRAY_SIZE(boot_devices))
		bootdevice_ofpath = boot_devices[bootdevice_brom_id];

	if (bootdevice_ofpath) {
		brom_bootsource_id_cache = bootdevice_brom_id;
		debug("%s: brom_bootdevice_id %x maps to '%s'\n",
		      __func__, bootdevice_brom_id, bootdevice_ofpath);
	} else {
		debug("%s: failed to resolve brom_bootdevice_id %x\n",
		      __func__, bootdevice_brom_id);
	}

	return bootdevice_ofpath;
}

u32 spl_boot_device(void)
{
	u32 boot_device = BOOT_DEVICE_MMC1;

#if defined(CONFIG_TARGET_CHROMEBOOK_JERRY) || \
		defined(CONFIG_TARGET_CHROMEBIT_MICKEY) || \
		defined(CONFIG_TARGET_CHROMEBOOK_MINNIE) || \
		defined(CONFIG_TARGET_CHROMEBOOK_SPEEDY) || \
		defined(CONFIG_TARGET_CHROMEBOOK_BOB) || \
		defined(CONFIG_TARGET_CHROMEBOOK_KEVIN)
	return BOOT_DEVICE_SPI;
#endif
	if (CONFIG_IS_ENABLED(ROCKCHIP_BACK_TO_BROM))
		return BOOT_DEVICE_BOOTROM;

	return boot_device;
}

u32 spl_mmc_boot_mode(struct mmc *mmc, const u32 boot_device)
{
	return MMCSD_MODE_RAW;
}

__weak void rockchip_stimer_init(void)
{
	/* If Timer already enabled, don't re-init it */
	u32 reg = readl(CONFIG_ROCKCHIP_STIMER_BASE + 0x10);
	if ( reg & 0x1 )
		return;
#ifdef COUNTER_FREQUENCY
#ifndef CONFIG_ARM64
	asm volatile("mcr p15, 0, %0, c14, c0, 0"
		     : : "r"(COUNTER_FREQUENCY));
#endif
#endif
	writel(0, CONFIG_ROCKCHIP_STIMER_BASE + 0x10);
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE);
	writel(0xffffffff, CONFIG_ROCKCHIP_STIMER_BASE + 4);
	writel(1, CONFIG_ROCKCHIP_STIMER_BASE + 0x10);
}

__weak int board_early_init_f(void)
{
	return 0;
}

__weak int arch_cpu_init(void)
{
	return 0;
}

__weak int rk_board_init_f(void)
{
	return 0;
}

#ifndef CONFIG_SPL_LIBGENERIC_SUPPORT
void udelay(unsigned long usec)
{
	__udelay(usec);
}

void hang(void)
{
	bootstage_error(BOOTSTAGE_ID_NEED_RESET);
	for (;;)
		;
}

/**
 * memset - Fill a region of memory with the given value
 * @s: Pointer to the start of the area.
 * @c: The byte to fill the area with
 * @count: The size of the area.
 *
 * Do not use memset() to access IO space, use memset_io() instead.
 */
void *memset(void *s, int c, size_t count)
{
	unsigned long *sl = (unsigned long *)s;
	char *s8;

#if !CONFIG_IS_ENABLED(TINY_MEMSET)
	unsigned long cl = 0;
	int i;

	/* do it one word at a time (32 bits or 64 bits) while possible */
	if (((ulong)s & (sizeof(*sl) - 1)) == 0) {
		for (i = 0; i < sizeof(*sl); i++) {
			cl <<= 8;
			cl |= c & 0xff;
		}
		while (count >= sizeof(*sl)) {
			*sl++ = cl;
			count -= sizeof(*sl);
		}
	}
#endif /* fill 8 bits at a time */
	s8 = (char *)sl;
	while (count--)
		*s8++ = c;

	return s;
}
#endif

#ifdef CONFIG_SPL_DM_RESET
static void brom_download(void)
{
	if (gd->console_evt == 0x02) {
		printf("ctrl+b: Bootrom download!\n");
		writel(BOOT_BROM_DOWNLOAD, CONFIG_ROCKCHIP_BOOT_MODE_REG);
		do_reset(NULL, 0, 0, NULL);
	}
}
#endif

static void spl_hotkey_init(void)
{
	/* If disable console, skip getting uart reg */
	if (!gd || gd->flags & GD_FLG_DISABLE_CONSOLE)
		return;
	if (!(gd->flags & GD_FLG_HAVE_CONSOLE))
		return;

	/* serial uclass only exists when enable CONFIG_SPL_FRAMEWORK */
#ifdef CONFIG_SPL_FRAMEWORK
	if (serial_tstc()) {
		gd->console_evt = serial_getc();
#else
	if (debug_uart_tstc()) {
		gd->console_evt = debug_uart_getc();
#endif
		if (gd->console_evt <= 0x1a) /* 'z' */
			printf("SPL Hotkey: ctrl+%c\n",
				gd->console_evt + 'a' - 1);
	}

	return;
}

void board_init_f(ulong dummy)
{
	int ret;

	gd->flags = dummy;
	board_early_init_f();

#define EARLY_UART
#if defined(EARLY_UART) && defined(CONFIG_DEBUG_UART)
	/*
	 * Debug UART can be used from here if required:
	 *
	 * debug_uart_init();
	 * printch('a');
	 * printhex8(0x1234);
	 * printascii("string");
	 */
	if (!gd->serial.using_pre_serial &&
	    !(gd->flags & GD_FLG_DISABLE_CONSOLE))
		debug_uart_init();
	printascii("U-Boot SPL board init\n");
#endif
	gd->sys_start_tick = get_ticks();

#ifdef CONFIG_SPL_PCIE_EP_SUPPORT
	rockchip_pcie_ep_init();
#endif
	ret = spl_early_init();
	if (ret) {
		printf("spl_early_init() failed: %d\n", ret);
		hang();
	}

	printf("Model: %s\n", (char *)fdt_getprop(gd->fdt_blob, 0, "model", NULL));
	arch_cpu_init();

#ifdef CONFIG_SYS_ARCH_TIMER
	/* Init ARM arch timer in arch/arm/cpu/armv7/arch_timer.c */
	timer_init();
#endif
#if !defined(CONFIG_TPL) || defined(CONFIG_SPL_RAM)
	debug("\nspl:init dram\n");
	ret = dram_init();
	if (ret) {
		printf("DRAM init failed: %d\n", ret);
		return;
	}
	gd->ram_top = gd->ram_base + get_effective_memsize();
	gd->ram_top = board_get_usable_ram_top(gd->ram_size);

	if (IS_ENABLED(CONFIG_ARM64) && !CONFIG_IS_ENABLED(SYS_DCACHE_OFF)) {
		gd->relocaddr = gd->ram_top;
		arch_reserve_mmu();
		enable_caches();
	}
#endif
	preloader_console_init();
	/* Get hotkey and store in gd */
	spl_hotkey_init();
#ifdef CONFIG_SPL_DM_RESET
	brom_download();
#endif
	rk_board_init_f();
#if defined(CONFIG_SPL_RAM_DEVICE) && defined(CONFIG_SPL_PCIE_EP_SUPPORT)
	rockchip_pcie_ep_get_firmware();
#endif
#if CONFIG_IS_ENABLED(ROCKCHIP_BACK_TO_BROM) && !defined(CONFIG_SPL_BOARD_INIT)
	back_to_bootrom(BROM_BOOT_NEXTSTAGE);
#endif
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* Just empty function now - can't decide what to choose */
	debug("%s: %s\n", __func__, name);

	return 0;
}
#endif

int board_init_f_init_misc(void)
{
	int boot_flags = 0;

#ifdef CONFIG_ARM64
	asm volatile("mrs %0, cntfrq_el0" : "=r" (gd->arch.timer_rate_hz));
#else
	asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r" (gd->arch.timer_rate_hz));
#endif

#if CONFIG_IS_ENABLED(ROCKCHIP_FPGA)
	arch_fpga_init();
#endif
#ifdef CONFIG_PSTORE
	param_parse_pstore();
#endif
	/* pre-loader serial */
#if defined(CONFIG_ROCKCHIP_PRELOADER_SERIAL) && \
    defined(CONFIG_ROCKCHIP_PRELOADER_ATAGS)
	struct tag *t;

	t = atags_get_tag(ATAG_SERIAL);
	if (t) {
		gd->serial.using_pre_serial = 1;
		gd->serial.enable = t->u.serial.enable;
		gd->serial.baudrate = t->u.serial.baudrate;
		gd->serial.addr = t->u.serial.addr;
		gd->serial.id = t->u.serial.id;
		gd->baudrate = t->u.serial.baudrate;
		if (!gd->serial.enable)
			boot_flags |= GD_FLG_DISABLE_CONSOLE;
		debug("preloader: enable=%d, addr=0x%x, baudrate=%d, id=%d\n",
		      t->u.serial.enable, (u32)t->u.serial.addr,
		      t->u.serial.baudrate, t->u.serial.id);
	} else
#endif
	{
		gd->baudrate = CONFIG_BAUDRATE;
		gd->serial.baudrate = CONFIG_BAUDRATE;
		gd->serial.addr = CONFIG_DEBUG_UART_BASE;
	}

	/* The highest priority to turn off (override) console */
#if defined(CONFIG_DISABLE_CONSOLE)
	boot_flags |= GD_FLG_DISABLE_CONSOLE;
#endif

	return boot_flags;
}

void spl_board_prepare_for_boot(void)
{
	if (!IS_ENABLED(CONFIG_ARM64) || CONFIG_IS_ENABLED(SYS_DCACHE_OFF))
		return;

	cleanup_before_linux();
}

#ifdef CONFIG_SPL_BOARD_INIT
__weak int rk_spl_board_init(void)
{
	return 0;
}

static int setup_led(void)
{
#ifdef CONFIG_SPL_LED
	struct udevice *dev;
	char *led_name;
	int ret;

	led_name = fdtdec_get_config_string(gd->fdt_blob, "u-boot,boot-led");
	if (!led_name)
		return 0;
	ret = led_get_by_label(led_name, &dev);
	if (ret) {
		debug("%s: get=%d\n", __func__, ret);
		return ret;
	}
	ret = led_set_state(dev, LEDST_ON);
	if (ret)
		return ret;
#endif

	return 0;
}

void spl_board_init(void)
{
	int ret;

	ret = setup_led();

	if (ret) {
		debug("LED ret=%d\n", ret);
		hang();
	}

	rk_spl_board_init();
#if CONFIG_IS_ENABLED(ROCKCHIP_BACK_TO_BROM)
	back_to_bootrom(BROM_BOOT_NEXTSTAGE);
#endif
	return;
}
#endif

#ifdef CONFIG_SPL_KERNEL_BOOT
static int spl_rockchip_dnl_key_pressed(void)
{
#if defined(CONFIG_SPL_BUTTON_ADC)
	return button_is_on(KEY_VOLUMEUP);
#else
	return 0;
#endif
}

#ifdef CONFIG_SPL_DM_FUEL_GAUGE
bool spl_is_low_power(void)
{
	struct udevice *dev;
	int ret, voltage;

	ret = uclass_get_device(UCLASS_FG, 0, &dev);
	if (ret) {
		debug("Get charge display failed, ret=%d\n", ret);
		return false;
	}

	voltage = fuel_gauge_get_voltage(dev);
	if (voltage >= CONFIG_SPL_POWER_LOW_VOLTAGE_THRESHOLD)
		return false;

	return true;
}
#endif

void spl_next_stage(struct spl_image_info *spl)
{
	const char *reason[] = { "Recovery key", "Ctrl+c", "LowPwr", "Other" };
	uint32_t reg_boot_mode;
	int i = 0;

	if (spl_rockchip_dnl_key_pressed()) {
		i = 0;
		spl->next_stage = SPL_NEXT_STAGE_UBOOT;
		goto out;
	}

	if (gd->console_evt == 0x03) {
		i = 1;
		spl->next_stage = SPL_NEXT_STAGE_UBOOT;
		goto out;
	}

#ifdef CONFIG_SPL_DM_FUEL_GAUGE
	if (spl_is_low_power()) {
		i = 2;
		spl->next_stage = SPL_NEXT_STAGE_UBOOT;
		goto out;
	}
#endif

	reg_boot_mode = readl((void *)CONFIG_ROCKCHIP_BOOT_MODE_REG);
	switch (reg_boot_mode) {
	case BOOT_LOADER:
	case BOOT_FASTBOOT:
	case BOOT_CHARGING:
	case BOOT_UMS:
	case BOOT_DFU:
		i = 3;
		spl->next_stage = SPL_NEXT_STAGE_UBOOT;
		break;
	default:
		spl->next_stage = SPL_NEXT_STAGE_KERNEL;
	}

out:
	if (spl->next_stage == SPL_NEXT_STAGE_UBOOT)
		printf("Enter uboot reason: %s\n", reason[i]);

	return;
}

const char *spl_kernel_partition(struct spl_image_info *spl,
				 struct spl_load_info *info)
{
	struct bootloader_message *bmsg = NULL;
	u32 boot_mode;
	int ret, cnt;
	u32 sector = 0;

#ifdef CONFIG_SPL_LIBDISK_SUPPORT
	disk_partition_t part_info;

	ret = part_get_info_by_name(info->priv, PART_MISC, &part_info);
	if (ret >= 0)
		sector = part_info.start;
#else
	sector = CONFIG_SPL_MISC_SECTOR;
#endif
	if (sector) {
		cnt = DIV_ROUND_UP(sizeof(*bmsg), info->bl_len);
		bmsg = memalign(ARCH_DMA_MINALIGN, cnt * info->bl_len);
		ret = info->read(info, BLK_SIZE(info, sector + BCB_MESSAGE_BLK_OFFSET),
				 BLK_SIZE(cnt), bmsg);
		if (ret < BLK_SIZE(cnt) && !strcmp(bmsg->command, "boot-recovery")) {
			free(bmsg);
			return PART_RECOVERY;
		} else {
			free(bmsg);
		}
	}

	boot_mode = readl((void *)CONFIG_ROCKCHIP_BOOT_MODE_REG);

	return (boot_mode == BOOT_RECOVERY) ? PART_RECOVERY : PART_BOOT;
}

__weak void spl_fdt_fixup_memory(struct spl_image_info *spl_image)
{
	void *blob = spl_image->fdt_addr;
	struct tag *t;
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];
	int i, count, err;

	err = fdt_check_header(blob);
	if (err < 0) {
		printf("Invalid dtb\n");
		return;
	}

	/* Fixup memory node based on ddr_mem atags */
	t = atags_get_tag(ATAG_DDR_MEM);
	if (t && t->u.ddr_mem.count) {
		count = t->u.ddr_mem.count;
		for (i = 0; i < count; i++) {
			start[i] = t->u.ddr_mem.bank[i];
			size[i] = t->u.ddr_mem.bank[i + count];
#ifdef SPL_RESV_MEM_SIZE
			if ((start[i] == CONFIG_SYS_SDRAM_BASE) &&
			    (start[i] + size[i] > CONFIG_SYS_SDRAM_BASE + SPL_RESV_MEM_SIZE))
				start[i] += SPL_RESV_MEM_SIZE;
#endif
			if (size[i] == 0)
				continue;
			debug("Adding bank: 0x%08llx - 0x%08llx (size: 0x%08llx)\n",
			       start[i], start[i] + size[i], size[i]);
		}

		fdt_increase_size(blob, 512);

		err = fdt_fixup_memory_banks(blob, start, size, count);
		if (err < 0) {
			printf("Fixup kernel dtb memory node failed: %s\n", fdt_strerror(err));
			return;
		}
	}

	return;
}

#if defined(CONFIG_SPL_ROCKCHIP_HWID_DTB)
int spl_find_hwid_dtb(const char *fdt_name)
{
	hwid_init_data();

	return hwid_dtb_is_available(fdt_name);
}
#endif

int spl_fdt_chosen_bootargs(struct spl_load_info *info, void *fdt)
{
	__maybe_unused struct blk_desc *desc = info->priv;
	__maybe_unused char *env = NULL;
	__maybe_unused int ret = 0;

#ifdef CONFIG_SPL_AB
	char slot_suffix[3] = {0};

	if (!spl_get_current_slot(desc, "misc", slot_suffix))
		spl_ab_bootargs_append_slot(fdt, slot_suffix);
#endif

#ifdef CONFIG_SPL_ENVF
	char *part_type[] = { "mtdparts", "blkdevparts" };
	char *part_list;
	int id = 0;

	env = envf_get(desc, part_type[id]);
	if (!env)
		env = envf_get(desc, part_type[++id]);
	if (env) {
		if (!strstr(env, part_type[id])) {
			part_list = calloc(1, strlen(env) + strlen(part_type[id]) + 2);
			if (part_list) {
				strcat(part_list, part_type[id]);
				strcat(part_list, "=");
				strcat(part_list, env);
			}
		} else {
			part_list = env;
		}
		ret = fdt_bootargs_append(fdt, part_list);
		if (ret) {
			printf("Append parts info to bootargs fail");
			return ret;
		}
		debug("## parts: %s\n\n", part_list);

		env = envf_get(desc, "sys_bootargs");
		if (env) {
			env = env + strlen("sys_bootargs=");
			ret = fdt_bootargs_append(fdt, env);
			if (ret) {
				printf("Append sys_bootargs to bootargs fail, ret=%d\n", ret);
				return ret;
			}
			debug("## sys_bootargs: %s\n\n", env);
		}
	}
#endif
#ifdef CONFIG_MTD_BLK
	if (!env && desc->if_type == IF_TYPE_MTD) {
		char *mtd_par_info = mtd_part_parse(desc);

		ret = fdt_bootargs_append(fdt, mtd_par_info);
		if (ret) {
			printf("Append mtdparts info to bootargs fail");
			return ret;
		}
		debug("## mtdparts: %s\n\n", mtd_par_info);
	}
#endif
#ifdef CONFIG_ROCKCHIP_META
	rk_meta_bootargs_append(fdt);
#endif

	return 0;
}
#endif

void spl_hang_reset(void)
{
	printf("# Reset the board to bootrom #\n");
#ifdef CONFIG_SPL_SYSRESET
	/* reset is available after dm setup */
	if (gd->flags & GD_FLG_SPL_EARLY_INIT) {
		writel(BOOT_BROM_DOWNLOAD, CONFIG_ROCKCHIP_BOOT_MODE_REG);
		do_reset(NULL, 0, 0, NULL);
	}
#endif
}

#ifdef CONFIG_SPL_FIT_ROLLBACK_PROTECT
int fit_read_otp_rollback_index(uint32_t fit_index, uint32_t *otp_index)
{
	int ret = 0;

	*otp_index = 0;
#if defined(CONFIG_SPL_ROCKCHIP_SECURE_OTP)
	struct udevice *dev;
	u32 index, i, otp_version;
	u32 bit_count;

	dev = misc_otp_get_device(OTP_S);
	if (!dev)
		return -ENODEV;

	otp_version = 0;
	for (i = 0; i < OTP_UBOOT_ROLLBACK_WORDS; i++) {
		if (misc_otp_read(dev, OTP_UBOOT_ROLLBACK_OFFSET + i * 4,
		    &index,
		    4)) {
			printf("Can't read rollback index\n");
			return -EIO;
		}

		bit_count = fls(index);
		otp_version += bit_count;
	}
	*otp_index = otp_version;
#endif

	return ret;
}

static int fit_write_otp_rollback_index(u32 fit_index)
{
#if defined(CONFIG_SPL_ROCKCHIP_SECURE_OTP)
	struct udevice *dev;
	u32 index, i, otp_index;

	if (!fit_index)
		return 0;

	if (fit_index > OTP_UBOOT_ROLLBACK_WORDS * 32)
		return -EINVAL;

	dev = misc_otp_get_device(OTP_S);
	if (!dev)
		return -ENODEV;

	if (fit_read_otp_rollback_index(fit_index, &otp_index))
		return -EIO;

	if (otp_index < fit_index) {
		/* Write new SW version to otp */
		for (i = 0; i < OTP_UBOOT_ROLLBACK_WORDS; i++) {
			/*
			 * If fit_index is equal to 0, then execute 0xffffffff >> 32.
			 * But the operand can only be 0 - 31. The "0xffffffff >> 32" is
			 * actually be "0xffffffff >> 0".
			 */
			if (!fit_index)
				break;
			/* convert to base-1 representation */
			index = 0xffffffff >> (OTP_ALL_ONES_NUM_BITS -
				min(fit_index, (u32)OTP_ALL_ONES_NUM_BITS));
			fit_index -= min(fit_index,
					  (u32)OTP_ALL_ONES_NUM_BITS);
			if (index) {
				if (misc_otp_write(dev, OTP_UBOOT_ROLLBACK_OFFSET + i * 4,
				    &index,
				    4)) {
					printf("Can't write rollback index\n");
					return -EIO;
				}
			}
		}
	}
#endif

	return 0;
}
#endif

int spl_board_prepare_for_jump(struct spl_image_info *spl_image)
{
#ifdef CONFIG_SPL_FIT_ROLLBACK_PROTECT
	int ret;

	ret = fit_write_otp_rollback_index(gd->rollback_index);
	if (ret) {
		panic("Failed to write fit rollback index %d, ret=%d",
		      gd->rollback_index, ret);
	}
#endif

#ifdef CONFIG_SPL_ROCKCHIP_HW_DECOMPRESS
	misc_decompress_cleanup();
#endif
	spl_rk_board_prepare_for_jump(spl_image);

	return 0;
}
