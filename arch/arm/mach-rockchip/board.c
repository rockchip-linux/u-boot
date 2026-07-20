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
#include <android_bootloader.h>
#include <button.h>
#include <crypto_manager.h>
#include <exports.h>
#include <cli.h>
#include <debug_uart.h>
#include <tee/optee.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <efi_loader.h>
#include <fastboot.h>
#include <hash.h>
#include <fdt_support.h>
#include <hotkey.h>
#include <init.h>
#include <log.h>
#include <mmc.h>
#include <mini_dump.h>
#include <misc.h>
#include <part.h>
#include <ram.h>
#include <syscon.h>
#include <u-boot/uuid.h>
#include <u-boot/crc.h>
#include <video_rockchip.h>
#include <asm/cache.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <asm/arch-rockchip/atags.h>
#include <asm/arch-rockchip/boot_mode.h>
#include <asm/arch-rockchip/common.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/arch-rockchip/misc.h>
#include <asm/arch-rockchip/periph.h>
#include <asm/arch-rockchip/pstore.h>
#include <asm/arch-rockchip/param.h>
#include <asm/arch-rockchip/vendor.h>
#include <linux/input.h>
#include <power/charge_display.h>
#include <power/regulator.h>
#include <rk_ebook.h>
#include <rockusb.h>
#include <amp.h>

DECLARE_GLOBAL_DATA_PTR;

__weak int soc_id_init(void) { return 0; }
__weak int set_armclk_rate(void) { return 0; }
__weak int rk_board_fdt_fixup(void *blob) { return 0; }
__weak int rk_board_dm_fdt_fixup(void *blob) { return 0; }
__weak int rk_board_init(void) { return 0; }
__weak int rk_board_late_init(void) { return 0; }
__weak bool rk_board_req_mem_layout1(void)
{
	return gd->ram_size <= SZ_128M;
}

#if !defined(CONFIG_DM_USB_GADGET)
__weak int rkusb_dev_bind_to_udc_data(struct udevice *dev)
{
	return 0;
}

static int rkusb_dev_bind(struct udevice *dev)
{
	return rkusb_dev_bind_to_udc_data(dev);
}

U_BOOT_DRIVER(rkusb) = {
	.name = "rkusb",
	.id = UCLASS_USB_GADGET_GENERIC,
	.bind = rkusb_dev_bind,
	.flags = DM_FLAG_PRE_RELOC,
};

U_BOOT_DRVINFO(rkusb) = {
	.name = "rkusb",
};
#endif

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
#if defined(CONFIG_OPTEE)
static int optee_region_map(void)
{
#if !defined(CONFIG_ARM64)
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
#endif

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
	struct memblock mem;
	ulong ramdisk_addr;
	ulong optee_addr;
	phys_size_t end;
	char *addr_r;

	/* disable bootm relcation to save boot time */
	env_set_hex("fdt_high", -1UL);
	env_set_hex("initrd_high", -1UL);

#ifdef ENV_MEM_LAYOUT_SETTINGS1
	const char *env_addr0[] = {
		"scriptaddr", "pxefile_addr_r", "fdt_addr_r",
		"kernel_addr_r", "kernel_addr_aarch32_r", "kernel_addr_c",
		"ramdisk_addr_r",
	};
	const char *env_addr1[] = {
		"scriptaddr1", "pxefile_addr1_r", "fdt_addr1_r",
		"kernel_addr1_r", "kernel_addr1_aarch32_r", "kernel_addr1_c",
		"ramdisk_addr1_r",
	};
	int i;

	if (rk_board_req_mem_layout1()) {
		/* Replace orignal xxx_addr_r */
		for (i = 0; i < ARRAY_SIZE(env_addr1); i++) {
			addr_r = env_get(env_addr1[i]);
			if (addr_r)
				env_set(env_addr0[i], addr_r);
		}
	}
#endif
	/*
	 * ramdisk_addr_low_r to handle the case:
	 *
	 * The 256MB board uses large ramdisk(i.e. 40MB+) on linux platform, it
	 * may cause sysmem overlay(no memory) issue when run boot_fit command:
	 *
	 *	0       162MB                             256MB
	 * 	[ ....... | ramdisk | fit boot.img | U-Boot ]
	 *
	 * Do this whether there is BL32 or not.
	 */
	if (gd->ram_size > SZ_128M && gd->ram_size <= SZ_256M) {
		ramdisk_addr = env_get_ulong("ramdisk_addr_low_r", 16, 0);
		if (ramdisk_addr)
			env_set_hex("ramdisk_addr_r", ramdisk_addr);
	}

	/* No BL32 ? */
	if (!(gd->pflags & GD_P_FLG_BL32_ENABLED)) {
		/*
		 * [1] Move kernel to lower address if possible.
		 */
		addr_r = env_get("kernel_addr_no_low_bl32_r");
		if (addr_r)
			env_set("kernel_addr_r", addr_r);

		/*
		 * [2] Move ramdisk at BL32 position if need.
		 *
		 * 0x0a200000 and 0x08400000 offset are rockchip traditional address
		 * of BL32 and ramdisk:
		 *
		 * |------------|------------|
		 * |    BL32    |  ramdisk   |
		 * |------------|------------|
		 *
		 * Move ramdisk to BL32 address to fix sysmem alloc failed
		 * issue on the board with critical memory(ie. 256MB).
		 */
		if (gd->ram_size > SZ_128M && gd->ram_size <= SZ_256M) {
			ramdisk_addr = env_get_ulong("ramdisk_addr_r", 16, 0);
			if (ramdisk_addr == CFG_SYS_SDRAM_BASE + 0x0a200000) {
				optee_addr = CFG_SYS_SDRAM_BASE + 0x08400000;
				env_set_hex("ramdisk_addr_r", optee_addr);
			}
		}
	} else {
		mem = param_parse_optee_mem();

		/*
		 * [1] Move kernel forward if possible.
		 */
		if (mem.base > SZ_128M) {
			addr_r = env_get("kernel_addr_no_low_bl32_r");
			if (addr_r)
				env_set("kernel_addr_r", addr_r);
		}

		/*
		 * [2] Move ramdisk backward if optee enlarge.
		 */
		end = mem.base + mem.size;
		ramdisk_addr = env_get_ulong("ramdisk_addr_r", 16, 0);
		if (ramdisk_addr >= mem.base && ramdisk_addr < end)
			env_set_hex("ramdisk_addr_r", end);
	}

	/* bootm memory limit */
	bootm_mem_init();
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
#ifdef CONFIG_ROCKCHIP_EBOOK_DISPLAY
	rockchip_ebook_show_uboot_logo();
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
#ifdef CONFIG_CRYPTO_MANAGER
	crypto_dump_best(false);
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
	set_armclk_rate();
#endif
#ifdef CONFIG_DM_DVFS
	dvfs_init(true);
#endif
	if (ab_is_enabled() && ab_decrease_tries())
		printf("Decrease ab tries count fail!\n");
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
	int ret = 0;

	switch (reason) {
		case FASTBOOT_REBOOT_REASON_BOOTLOADER:
			printf("Setting reboot to bootloader flag ...\n");
			writel(BOOT_FASTBOOT, CONFIG_ROCKCHIP_BOOT_MODE_REG);
			break;
		case FASTBOOT_REBOOT_REASON_FASTBOOTD:
			printf("Setting reboot to fastboot flag ...\n");
			if (android_bcb_write("boot-fastboot")) {
				fastboot_tx_write_str("FAIL: Cannot set boot-fastboot");
				return -EIO;
			}
			break;
		case FASTBOOT_REBOOT_REASON_RECOVERY:
			printf("Setting reboot to recovery flag ...\n");
			if (android_bcb_write("boot-recovery")) {
				fastboot_tx_write_str("FAIL: Cannot set boot-recovery");
				return -EIO;
			}
			break;
		default:
			ret = -ENOTSUPP;
			break;
	}

	return ret;
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
	if (ab_is_enabled()) {
		if (ab_have_bootable_slot() == true)
			run_command("reset;", 0);
		else
			run_command("fastboot usb 0;", 0);
	}

#ifdef CONFIG_LIBAVB_VBMETA_PUBLIC_KEY_VALIDATE
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
	hotkey_run(HK_CLI_OS_PRE);
}

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
	param_parse_pre_serial(&boot_flags);

	/* The highest priority to turn off (override) console */
#if defined(CONFIG_DISABLE_CONSOLE)
	boot_flags |= GD_FLG_DISABLE_CONSOLE;
#endif

	return boot_flags;
}

void board_quiesce_devices(void *images)
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
	hotkey_run(HK_CMDLINE);
	hotkey_run(HK_CLI_OS_GO);
#ifdef CONFIG_ROCKCHIP_REBOOT_TEST
	do_reset(NULL, 0, 0, NULL);
#endif

}
