// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd.
 *
 * Copyright (C) 2019 Collabora Inc - https://www.collabora.com/
 *      Rohan Garg <rohan.garg@collabora.com>
 *
 * Based on puma-rk3399.c:
 *      (C) Copyright 2017 Theobroma Systems Design und Consulting GmbH
 */
#include <config.h>
#include <clk.h>
#include <cpu_func.h>
#include <env.h>
#include <android_ab.h>
#ifdef CONFIG_AVB_VERIFY
#include <avb_verify.h>
#endif
#include <button.h>
#include <exports.h>
#include <cli.h>
#include <clk.h>
#include <cpu_func.h>
#include <debug_uart.h>
#include <tee/optee.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <efi_loader.h>
#include <exports.h>
#include <fastboot.h>
#include <hash.h>
#include <fdt_support.h>
#include <hotkey.h>
#include <init.h>
#include <log.h>
#include <mmc.h>
#include <mini_dump.h>
#include <dm/uclass-internal.h>
#include <misc.h>
#include <part.h>
#include <ram.h>
#include <syscon.h>
#include <u-boot/uuid.h>
#include <u-boot/crc.h>
#include <u-boot/sha256.h>
#include <u-boot/uuid.h>
#include <video_rockchip.h>
#include <asm/cache.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <asm/arch-rockchip/atags.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/common.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/periph.h>
#include <asm/arch-rockchip/misc.h>
#include <asm/arch-rockchip/periph.h>
#include <asm/arch-rockchip/pstore.h>
#include <asm/arch-rockchip/param.h>
#include <asm/arch-rockchip/vendor.h>
#include <asm/arch-rockchip/atags.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/param.h>
#include <linux/input.h>
#include <power/charge_display.h>
#include <power/regulator.h>
#include <tee/optee.h>

DECLARE_GLOBAL_DATA_PTR;

__weak int soc_id_init(void) { return 0; }
__weak int clk_cpu_raise(void) { return 0; }
__weak int rk_board_fdt_fixup(void *blob) { return 0; }
__weak int rk_board_dm_fdt_fixup(void *blob) { return 0; }
__weak int rk_board_init(void) { return 0; }
__weak int rk_board_late_init(void) { return 0; }

/* override weak */
int board_kernel_dtb_read(void *fdt)
{
	return rockchip_read_dtb_file(fdt);
}

static void board_debug_init(void)
{
	if (!gd->serial.using_pre_serial &&
	    !(gd->flags & GD_FLG_DISABLE_CONSOLE))
		debug_uart_init();

	if (tstc()) {
		gd->console_evt = getchar();
		if (gd->console_evt <= 0x1a) /* 'z' */
			printf("Hotkey: ctrl+%c\n", gd->console_evt + 'a' - 1);
	}

	if (IS_ENABLED(CONFIG_CONSOLE_DISABLE_CLI))
		printf("Cmd interface: disabled\n");
}

/*
 * 32-bit U-Boot: MMU is setup according to gd->bd->bi_dram[..] where
 * the OP-TEE region has been reserved, so it region is dcache off.
 * Let's map it here.
 */
static int optee_region_map(void)
{
#if defined(CONFIG_OPTEE_CLIENT) && !defined(CONFIG_ARM64)
	struct memblock mem;
	int ret;

	mem = param_parse_optee_mem();
	ret = bidram_reserve(MEM_OPTEE, mem.base, mem.size);
	if (ret)
		return ret;

	if (mem.size)
		mmu_set_region_dcache_behaviour(mem.base, mem.size,
						DCACHE_WRITEBACK);
#endif
	return 0;
}

static void scan_run_cmd(void)
{
	char *config = CONFIG_ROCKCHIP_CMD;
	char *cmd, *key;

	key = strchr(config, ' ');
	if (!key)
		return;

	cmd = strdup(config);
	cmd[key - config] = 0;
	key++;

	if (!strcmp(key, "-")) {
		run_command(cmd, 0);
	} else {
#ifdef CONFIG_BUTTON
		ulong map;

		map = simple_strtoul(key, NULL, 10);
		if (button_is_on(map)) {
			printf("## Key<%ld> pressed... run cmd '%s'\n", map, cmd);
			run_command(cmd, 0);
		}
#endif
	}
}

static void env_setup(void)
{
	/* disable bootm relcation to save boot time */
	env_set_hex("fdt_high", -1UL);
	env_set_hex("initrd_high", -1UL);
}

int board_late_init(void)
{
#ifdef CONFIG_ROCKCHIP_SET_ETHADDR
	rockchip_setup_macaddr();
#endif
#ifdef CONFIG_ROCKCHIP_SET_SN
	rockchip_setup_serial_number();
#endif
	rockusb_download();

	scan_run_cmd();
#ifdef CONFIG_ROCKCHIP_USB_BOOT
	usb_boot_init();
#endif
#ifdef CONFIG_DM_CHARGE_DISPLAY
	charge_display();
#endif
#ifdef CONFIG_ROCKCHIP_MINIDUMP
	minidump_init();
#endif
#ifdef CONFIG_DRM_ROCKCHIP
	if (plat_boot_mode() != BOOT_MODE_QUIESCENT)
		rockchip_show_logo();
#endif
#ifdef CONFIG_ROCKCHIP_EINK_DISPLAY
	rockchip_eink_show_uboot_logo();
#endif
#if (CONFIG_ROCKCHIP_BOOT_MODE_REG > 0)
	setup_boot_mode();
#endif
	env_setup();
	soc_clk_dump();
	bootargs_setup();
#if IS_ENABLED(CONFIG_EFI_HAVE_CAPSULE_SUPPORT) && IS_ENABLED(CONFIG_EFI_PARTITION)
	gpt_capsule_update_setup();
#endif
#ifdef CONFIG_AMP
	amp_cpus_on();
#endif
//	run_command("download", 0);

	return rk_board_late_init();
}

int board_init(void)
{
	board_debug_init();
#ifdef DEBUG
	soc_clk_dump();
#endif
#ifdef CONFIG_OPTEE
	optee_region_map();
	optee_client_init();
#endif
#ifdef CONFIG_DM_KERNEL_DTB
	kernel_dtb_init();
#endif
#ifdef CONFIG_HOTKEY
	rbrom_download();
#endif
#ifdef CONFIG_CLK
	clk_init();
#endif
#ifdef CONFIG_DM_REGULATOR
	regulators_enable_boot_on(false);
#endif
#ifdef CONFIG_ROCKCHIP_IO_DOMAIN
	io_domain_init();
#endif
#ifdef CONFIG_CLK
	clk_cpu_raise();
#endif
#ifdef CONFIG_DM_DVFS
	dvfs_init(true);
#endif
#ifdef CONFIG_ANDROID_AB
	if (ab_decrease_tries())
		printf("Decrease ab tries count fail!\n");
#endif
	soc_id_init();

	return rk_board_init();
}

#if !defined(CONFIG_SYS_DCACHE_OFF) && !defined(CONFIG_ARM64)
void enable_caches(void)
{
	icache_enable();
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

#if IS_ENABLED(CONFIG_USB_GADGET)
#include <usb.h>

#if IS_ENABLED(CONFIG_USB_GADGET_DOWNLOAD)
#define ROCKCHIP_G_DNL_UMS_PRODUCT_NUM	0x0010

int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	if (!strcmp(name, "usb_dnl_ums"))
		put_unaligned(ROCKCHIP_G_DNL_UMS_PRODUCT_NUM, &dev->idProduct);
	else
		put_unaligned(CONFIG_USB_GADGET_PRODUCT_NUM, &dev->idProduct);

	return 0;
}
#endif /* CONFIG_USB_GADGET_DOWNLOAD */

#if IS_ENABLED(CONFIG_USB_GADGET_DWC2_OTG) && !IS_ENABLED(CONFIG_DM_USB_GADGET)
#include <linux/usb/otg.h>
#include <usb/dwc2_udc.h>

static struct dwc2_plat_otg_data otg_data = {
	.rx_fifo_sz	= 512,
	.np_tx_fifo_sz	= 16,
	.tx_fifo_sz	= 128,
};

int board_usb_init(int index, enum usb_init_type init)
{
	ofnode node;
	bool matched = false;

	/* find the usb_otg node */
	node = ofnode_by_compatible(ofnode_null(), "snps,dwc2");
	while (ofnode_valid(node)) {
		switch (usb_get_dr_mode(node)) {
		case USB_DR_MODE_OTG:
		case USB_DR_MODE_PERIPHERAL:
			matched = true;
			break;

		default:
			break;
		}

		if (matched)
			break;

		node = ofnode_by_compatible(node, "snps,dwc2");
	}
	if (!matched) {
		debug("Not found usb_otg device\n");
		return -ENODEV;
	}
	otg_data.regs_otg = ofnode_get_addr(node);

#ifdef CONFIG_ROCKCHIP_USB2_PHY
	int ret;
	u32 phandle, offset;
	ofnode phy_node;

	ret = ofnode_read_u32(node, "phys", &phandle);
	if (ret)
		return ret;

	node = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(node)) {
		debug("Not found usb phy device\n");
		return -ENODEV;
	}

	phy_node = ofnode_get_parent(node);
	if (!ofnode_valid(node)) {
		debug("Not found usb phy device\n");
		return -ENODEV;
	}

	otg_data.phy_of_node = phy_node;
	ret = ofnode_read_u32(node, "reg", &offset);
	if (ret)
		return ret;
	otg_data.regs_phy =  offset +
		(u32)syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
#endif
	return dwc2_udc_probe(&otg_data);
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	return 0;
}
#endif /* CONFIG_USB_GADGET_DWC2_OTG */
#endif /* CONFIG_USB_GADGET */

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

#if IS_ENABLED(CONFIG_BOARD_RNG_SEED) && IS_ENABLED(CONFIG_RNG_ROCKCHIP)
#include <rng.h>

/* Use hardware rng to seed Linux random.
 *
 * 'Android_14 + GKI' requires this information.
 */
__weak int board_rng_seed(struct abuf *buf)
{
	struct udevice *dev;
	ulong len = env_get_ulong("rng_seed_size", 10, 64);
	u64 *data;

	if (len < 64) {
		/*
		 * rng_seed_size should be at least 32 bytes for Linux 5.19+,
		 * or 64 for older Linux kernel versions
		 */
		log_warning("Value for rng_seed_size (%lu) too low, Linux kernel RNG may fail to initialize early\n",
			    len);
	}

	data = malloc(len);
	if (!data) {
		printf("Out of memory\n");
		return -ENOMEM;
	}

	if (uclass_get_device(UCLASS_RNG, 0, &dev) || !dev) {
		printf("No RNG device\n");
		return -ENODEV;
	}

	if (dm_rng_read(dev, data, len)) {
		printf("Reading RNG failed\n");
		return -EIO;
	}

	abuf_init_set(buf, data, len);

	return 0;
}
#endif

int mmc_get_env_dev(void)
{
	int devnum;
	const char *boot_device;
	struct udevice *dev;

#ifdef CONFIG_SYS_MMC_ENV_DEV
	devnum = CONFIG_SYS_MMC_ENV_DEV;
#else
	devnum = 0;
#endif

	boot_device = ofnode_read_chosen_string("u-boot,spl-boot-device");
	if (!boot_device) {
		debug("%s: /chosen/u-boot,spl-boot-device not set\n", __func__);
		return devnum;
	}

	debug("%s: booted from %s\n", __func__, boot_device);

	if (uclass_find_device_by_ofnode(UCLASS_MMC, ofnode_path(boot_device), &dev)) {
		debug("%s: no U-Boot device found for %s\n", __func__, boot_device);
		return devnum;
	}

	devnum = dev->seq_;
	debug("%s: get MMC env from mmc%d\n", __func__, devnum);
	return devnum;
}

void autoboot_command_fail_handle(void)
{
#ifdef CONFIG_ANDROID_AB
	if (ab_have_bootable_slot() == true)
		run_command("reset;", 0);
	else
		run_command("fastboot usb 0;", 0);
#endif

#ifdef CONFIG_AVB_VBMETA_PUBLIC_KEY_VALIDATE
	run_command("download", 0);
	run_command("fastboot usb 0;", 0);
#endif
}

#ifdef CONFIG_ROCKCHIP_SANITY_CPU_SWAP
static void sanity_cpu_swap(void *blob)
{
	int cpus_offset;
	int noffset;
	ulong mpidr;
	ulong reg;

	cpus_offset = fdt_path_offset(blob, "/cpus");
	if (cpus_offset < 0)
		return;

	for (noffset = fdt_first_subnode(blob, cpus_offset);
	     noffset >= 0;
	     noffset = fdt_next_subnode(blob, noffset)) {
		const struct fdt_property *prop;
		int len;

		prop = fdt_get_property(blob, noffset, "device_type", &len);
		if (!prop)
			continue;
		if (len < 4)
			continue;
		if (strcmp(prop->data, "cpu"))
			continue;

		/* only sanity first cpu */
		reg = (ulong)fdtdec_get_addr_size_auto_parent(blob, cpus_offset, noffset,
                                                              "reg", 0, NULL, false);
		mpidr = read_mpidr() & 0xfff;
		if ((mpidr & reg) != reg) {
			printf("CPU swap error: Loader and Kernel firmware mismatch! "
			       "Current cpu0 \"reg\" is 0x%lx but kernel dtb requires 0x%lx\n",
			       mpidr, reg);
			run_command("download", 0);
		}
		return;
	}
}
#endif

static int rockchip_dm_late_init(void *blob)
{
	struct udevice *dev;

	/* Prepare for board_rng_seed(), dryrun and ignore result */
	if (IS_ENABLED(CONFIG_BOARD_RNG_SEED) && IS_ENABLED(CONFIG_DM_RNG))
		uclass_get_device(UCLASS_RNG, 0, &dev);

	return 0;
}

int board_fdt_fixup(void *blob)
{
#ifdef CONFIG_ROCKCHIP_SANITY_CPU_SWAP
	sanity_cpu_swap(blob);
#endif
	/*
	 * Device's platdata points to orignal fdt blob property,
	 * access DM device before any fdt fixup.
	 *
	 * Do board specific init and common init.
	 */
	rk_board_dm_fdt_fixup(blob);
	rockchip_dm_late_init(blob);

	/* Common fixup for DRM */
#ifdef CONFIG_DRM_ROCKCHIP
	rockchip_display_fixup(blob);
#endif
#ifdef CONFIG_ROCKCHIP_VENDOR_PARTITION
	vendor_storage_fixup(blob);
#endif

	return rk_board_fdt_fixup(blob);
}

void arch_preboot_os(uint32_t bootm_state)
{
#if 0 // TODO
	if (!(bootm_state & BOOTM_STATE_OS_PREP))
		return;

#ifdef CONFIG_ARM64
	u8 *data = (void *)images.ep;

	/*
	 * Fix kernel 5.10 arm64 boot warning:
	 * "[Firmware Bug]: Kernel image misaligned at boot, please fix your bootloader!"
	 *
	 * kernel: 5.10 commit 120dc60d0bdb ("arm64: get rid of TEXT_OFFSET")
	 * arm64 kernel version:
	 *	data[10] == 0x00 if kernel version >= 5.10: N*2MB align
	 *	data[10] == 0x08 if kernel version <  5.10: N*2MB + 0x80000(TEXT_OFFSET)
	 *
	 * Why fix here?
	 *   1. this is the common and final path for any boot command.
	 *   2. don't influence original boot flow, just fix it exactly before
	 *	jumping kernel.
	 *
	 * But relocation is in board_quiesce_devices() until all decompress
	 * done, mainly for saving boot time.
	 */

	orig_images_ep = images.ep;

	if (data[10] == 0x00) {
		if (round_down(images.ep, SZ_2M) != images.ep)
			images.ep = round_down(images.ep, SZ_2M);
	} else {
		if (IS_ALIGNED(images.ep, SZ_2M))
			images.ep += 0x80000;
	}
#endif
	hotkey_run(HK_CLI_OS_PRE);
#endif
}

int board_init_f_init_misc(void)
{
	int boot_flags = 0;

#ifdef CONFIG_ARM64
	asm volatile("mrs %0, cntfrq_el0" : "=r" (gd->arch.timer_rate_hz));
#else
	asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r" (gd->arch.timer_rate_hz));
#endif

#if CONFIG_IS_ENABLED(FPGA_ROCKCHIP)
	arch_fpga_init();
#endif
#ifdef CONFIG_PSTORE
	param_parse_pstore();
#endif
	param_parse_pre_serial(&boot_flags);

	/* The highest priority to turn off (override) console */
#if defined(CONFIG_DISABLE_CONSOLE)
	boot_flags |= GD_FLG_DISABLE_CONSOLE;
#endif

	return boot_flags;
}

void board_quiesce_devices(void)
{
#ifdef CONFIG_ROCKCHIP_PRELOADER_ATAGS
	/* Destroy atags makes next warm boot safer */
	atags_destroy();
#endif
#ifdef CONFIG_FIT_ROLLBACK_PROTECT
	int ret;

	ret = fit_write_optee_rollback_index(gd->rollback_index);
	if (ret) {
		panic("Failed to write fit rollback index %d, ret=%d",
		      gd->rollback_index, ret);
	}
#endif
#ifdef CONFIG_ROCKCHIP_HW_DECOMPRESS
	misc_decompress_cleanup();
#endif

#if 0 // TODO
#ifdef CONFIG_ARM64
	/* relocate kernel after decompress cleanup */
	if (orig_images_ep && orig_images_ep != images.ep) {
		memmove((char *)images.ep, (const char *)orig_images_ep,
			images.os.image_len);
		printf("== DO RELOCATE == Kernel from 0x%08lx to 0x%08lx\n",
		       orig_images_ep, images.ep);
	}
#endif
#endif
	hotkey_run(HK_CMDLINE);
	hotkey_run(HK_CLI_OS_GO);
#ifdef CONFIG_ROCKCHIP_REBOOT_TEST
	do_reset(NULL, 0, 0, NULL);
#endif

}

int board_init_f_boot_flags(void)
{
	int boot_flags = 0;

#ifdef CONFIG_ARM64
	asm volatile("mrs %0, cntfrq_el0" : "=r" (gd->arch.timer_rate_hz));
#else
	asm volatile("mrc p15, 0, %0, c14, c0, 0" : "=r" (gd->arch.timer_rate_hz));
#endif

#if CONFIG_IS_ENABLED(FPGA_ROCKCHIP)
	arch_fpga_init();
#endif
#ifdef CONFIG_PSTORE
	param_parse_pstore();
#endif
	param_parse_pre_serial(&boot_flags);

	/* The highest priority to turn off (override) console */
#if defined(CONFIG_DISABLE_CONSOLE)
	boot_flags |= GD_FLG_DISABLE_CONSOLE;
#endif

	return boot_flags;
}

