// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *
 * Aneesh V <aneesh@ti.com>
 */

#include <common.h>
#include <config.h>
#include <bloblist.h>
#include <binman_sym.h>
#include <bootstage.h>
#include <dm.h>
#include <handoff.h>
#include <hang.h>
#include <init.h>
#include <irq_func.h>
#include <log.h>
#include <mapmem.h>
#include <serial.h>
#include <spl.h>
#include <spl_load.h>
#include <system-constants.h>
#include <asm/global_data.h>
#include <asm/sections.h>
#include <asm/u-boot.h>
#include <asm-generic/gpio.h>
#include <nand.h>
#include <fat.h>
#include <u-boot/crc.h>
#if CONFIG_IS_ENABLED(BANNER_PRINT)
#include <timestamp.h>
#endif
#include <version.h>
#include <image.h>
#include <malloc.h>
#include <mapmem.h>
#include <dm/root.h>
#include <dm/util.h>
#include <dm/device-internal.h>
#include <dm/uclass-internal.h>
#include <linux/compiler.h>
#include <fdt_support.h>
#include <bootcount.h>
#include <wdt.h>
#include <video.h>

DECLARE_GLOBAL_DATA_PTR;
DECLARE_BINMAN_MAGIC_SYM;

u32 *boot_params_ptr = NULL;

#if CONFIG_IS_ENABLED(BINMAN_UBOOT_SYMBOLS)
/* See spl.h for information about this */
#if defined(CONFIG_SPL_BUILD)
binman_sym_declare(ulong, u_boot_any, image_pos);
binman_sym_declare(ulong, u_boot_any, size);
#endif

#ifdef CONFIG_TPL
binman_sym_declare(ulong, u_boot_spl_any, image_pos);
binman_sym_declare(ulong, u_boot_spl_any, size);
#endif

#ifdef CONFIG_VPL
binman_sym_declare(ulong, u_boot_vpl_any, image_pos);
binman_sym_declare(ulong, u_boot_vpl_any, size);
#endif

#endif /* BINMAN_UBOOT_SYMBOLS */

/* Define board data structure */
static struct bd_info bdata __attribute__ ((section(".data")));

#if CONFIG_IS_ENABLED(SHOW_BOOT_PROGRESS)
/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}
#endif

#if defined(CONFIG_SPL_OS_BOOT) || CONFIG_IS_ENABLED(HANDOFF) || \
	defined(CONFIG_SPL_ATF)
/* weak, default platform-specific function to initialize dram banks */
__weak int dram_init_banksize(void)
{
	return 0;
}
#endif

/*
 * Default function to determine if u-boot or the OS should
 * be started. This implementation always returns 1.
 *
 * Please implement your own board specific funcion to do this.
 *
 * RETURN
 * 0 to not start u-boot
 * positive if u-boot should start
 */
#if CONFIG_IS_ENABLED(OS_BOOT)
__weak int spl_start_uboot(void)
{
	puts(PHASE_PROMPT
	     "Please implement spl_start_uboot() for your board\n");
	puts(PHASE_PROMPT "Direct Linux boot not active!\n");
	return 1;
}

/*
 * Weak default function for arch specific zImage check. Return zero
 * and fill start and end address if image is recognized.
 */
int __weak bootz_setup(ulong image, ulong *start, ulong *end)
{
	 return 1;
}

int __weak booti_setup(ulong image, ulong *relocated_addr, ulong *size, bool force_reloc)
{
	 return 1;
}
#endif

/* Weak default function for arch/board-specific fixups to the spl_image_info */
void __weak spl_perform_fixups(struct spl_image_info *spl_image)
{
}

/* Get the next stage process */
__weak void spl_next_stage(struct spl_image_info *spl)
{
	spl->next_stage = SPL_NEXT_STAGE_UBOOT;
}

/* Weak default function for arch/board-specific preppare before jumping */
int __weak spl_board_prepare_for_jump(struct spl_image_info *spl_image)
{
	return 0;
}

/* Fix storages, like iomux  */
__weak void spl_board_storages_fixup(struct spl_image_loader *loader)
{
	/* Nothing to do! */
}

void spl_fixup_fdt(void *fdt_blob)
{
#if defined(CONFIG_SPL_OF_LIBFDT)
	int err;

	if (!fdt_blob)
		return;

	err = fdt_check_header(fdt_blob);
	if (err < 0) {
		printf("fdt_root: %s\n", fdt_strerror(err));
		return;
	}

	/* fixup the memory dt node */
	err = fdt_shrink_to_minimum(fdt_blob, 0);
	if (err == 0) {
		printf(PHASE_PROMPT "fdt_shrink_to_minimum err - %d\n", err);
		return;
	}

	err = arch_fixup_fdt(fdt_blob);
	if (err) {
		printf(PHASE_PROMPT "arch_fixup_fdt err - %d\n", err);
		return;
	}
#endif
}

int spl_reserve_video_from_ram_top(void)
{
	if (CONFIG_IS_ENABLED(VIDEO)) {
		ulong addr;
		int ret;

		addr = gd->ram_top;
		ret = video_reserve(&addr);
		if (ret)
			return ret;
		debug("Reserving %luk for video at: %08lx\n",
		      ((unsigned long)gd->relocaddr - addr) >> 10, addr);
		gd->relocaddr = addr;
	}

	return 0;
}

ulong spl_get_image_pos(void)
{
	if (!CONFIG_IS_ENABLED(BINMAN_UBOOT_SYMBOLS))
		return BINMAN_SYM_MISSING;

#ifdef CONFIG_VPL
	if (xpl_next_phase() == PHASE_VPL)
		return binman_sym(ulong, u_boot_vpl_any, image_pos);
#endif
#if defined(CONFIG_TPL) && !defined(CONFIG_VPL)
	if (xpl_next_phase() == PHASE_SPL)
		return binman_sym(ulong, u_boot_spl_any, image_pos);
#endif
#if defined(CONFIG_SPL_BUILD)
	return binman_sym(ulong, u_boot_any, image_pos);
#endif

	return BINMAN_SYM_MISSING;
}

ulong spl_get_image_size(void)
{
	if (!CONFIG_IS_ENABLED(BINMAN_UBOOT_SYMBOLS))
		return BINMAN_SYM_MISSING;

#ifdef CONFIG_VPL
	if (xpl_next_phase() == PHASE_VPL)
		return binman_sym(ulong, u_boot_vpl_any, size);
#endif
	return xpl_next_phase() == PHASE_SPL ?
		binman_sym(ulong, u_boot_spl_any, size) :
		binman_sym(ulong, u_boot_any, size);
}

ulong spl_get_image_text_base(void)
{
#ifdef CONFIG_VPL
	if (xpl_next_phase() == PHASE_VPL)
		return CONFIG_VPL_TEXT_BASE;
#endif
	return xpl_next_phase() == PHASE_SPL ? CONFIG_SPL_TEXT_BASE :
		CONFIG_TEXT_BASE;
}

/*
 * Weak default function for board specific cleanup/preparation before
 * Linux boot. Some boards/platforms might not need it, so just provide
 * an empty stub here.
 */
__weak void spl_board_prepare_for_linux(void)
{
	/* Nothing to do! */
}

__weak void spl_board_prepare_for_optee(void *fdt)
{
}

__weak const char *spl_board_loader_name(u32 boot_device)
{
	return NULL;
}

#if CONFIG_IS_ENABLED(OPTEE_IMAGE)
__weak void __noreturn jump_to_image_optee(struct spl_image_info *spl_image)
{
	spl_optee_entry(NULL, NULL, spl_image->fdt_addr,
			(void *)spl_image->entry_point);
}
#endif

__weak void spl_board_prepare_for_boot(void)
{
	/* Nothing to do! */
}

__weak struct legacy_img_hdr *spl_get_load_buffer(ssize_t offset, size_t size)
{
	return map_sysmem(CONFIG_TEXT_BASE + offset, 0);
}

void spl_set_header_raw_uboot(struct spl_image_info *spl_image)
{
	ulong u_boot_pos = spl_get_image_pos();

#if CONFIG_SYS_MONITOR_LEN != 0
	spl_image->size = CONFIG_SYS_MONITOR_LEN;
#else
	/* Unknown U-Boot size, let's assume it will not be more than 200 KB */
	spl_image->size = 200 * 1024;
#endif

	/*
	 * Binman error cases: address of the end of the previous region or the
	 * start of the image's entry area (usually 0) if there is no previous
	 * region.
	 */
	if (u_boot_pos && u_boot_pos != BINMAN_SYM_MISSING) {
		/* Binman does not support separated entry addresses */
		spl_image->entry_point = spl_get_image_text_base();
		spl_image->load_addr = spl_get_image_text_base();
		spl_image->size = spl_get_image_size();
		log_debug("Next load addr %lx\n", spl_image->load_addr);
	} else {
		spl_image->entry_point = CONFIG_SYS_UBOOT_START;
		spl_image->load_addr = CONFIG_TEXT_BASE;
		log_debug("Default load addr %x (u_boot_pos=%lx)\n",
			  CONFIG_TEXT_BASE, u_boot_pos);
	}
	spl_image->os = IH_OS_U_BOOT;
	spl_image->name = xpl_name(xpl_next_phase());
	log_debug("Next phase: %s at %lx size %lx\n", spl_image->name,
		  spl_image->load_addr, (ulong)spl_image->size);
}

__weak int spl_parse_board_header(struct spl_image_info *spl_image,
				  const struct spl_boot_device *bootdev,
				  const void *image_header, size_t size)
{
	return -EINVAL;
}

__weak int spl_parse_legacy_header(struct spl_image_info *spl_image,
				   const struct legacy_img_hdr *header)
{
	/* LEGACY image not supported */
	debug("Legacy boot image support not enabled, proceeding to other boot methods\n");
	return -EINVAL;
}

int spl_parse_image_header(struct spl_image_info *spl_image,
			   const struct spl_boot_device *bootdev,
			   const struct legacy_img_hdr *header)
{
	int ret;

	if (CONFIG_IS_ENABLED(LOAD_FIT_FULL)) {
		ret = spl_load_fit_image(spl_image, header);

		if (!ret)
			return ret;
	}
	if (image_get_magic(header) == IH_MAGIC) {
		int ret;

		ret = spl_parse_legacy_header(spl_image, header);
		if (ret)
			return ret;
		return 0;
	}

	if (IS_ENABLED(CONFIG_SPL_PANIC_ON_RAW_IMAGE)) {
		/*
		 * CONFIG_SPL_PANIC_ON_RAW_IMAGE is defined when the
		 * code which loads images in SPL cannot guarantee that
		 * absolutely all read errors will be reported.
		 * An example is the LPC32XX MLC NAND driver, which
		 * will consider that a completely unreadable NAND block
		 * is bad, and thus should be skipped silently.
		 */
		panic("** no mkimage signature but raw image not supported");
	}

	if (CONFIG_IS_ENABLED(OS_BOOT) && IS_ENABLED(CONFIG_CMD_BOOTI)) {
		ulong start, size;

		if (!booti_setup((ulong)header, &start, &size, 0)) {
			spl_image->name = "Linux";
			spl_image->os = IH_OS_LINUX;
			spl_image->load_addr = start;
			spl_image->entry_point = start;
			spl_image->size = size;
			debug(PHASE_PROMPT
			      "payload Image, load addr: 0x%lx size: %d\n",
			      spl_image->load_addr, spl_image->size);
			return 0;
		}
	} else if (CONFIG_IS_ENABLED(OS_BOOT) && IS_ENABLED(CONFIG_CMD_BOOTZ)) {
		ulong start, end;

		if (!bootz_setup((ulong)header, &start, &end)) {
			spl_image->name = "Linux";
			spl_image->os = IH_OS_LINUX;
			spl_image->load_addr = CONFIG_SYS_LOAD_ADDR;
			spl_image->entry_point = CONFIG_SYS_LOAD_ADDR;
			spl_image->size = end - start;
			debug(PHASE_PROMPT
			      "payload zImage, load addr: 0x%lx size: %d\n",
			      spl_image->load_addr, spl_image->size);
			return 0;
		}
	}

	if (!spl_parse_board_header(spl_image, bootdev, (const void *)header,
				    sizeof(*header)))
		return 0;

	if (IS_ENABLED(CONFIG_SPL_RAW_IMAGE_SUPPORT)) {
		/* Signature not found - assume u-boot.bin */
		debug("mkimage signature not found - ih_magic = %x\n",
		      header->ih_magic);
		spl_set_header_raw_uboot(spl_image);
	} else {
		/* RAW image not supported, proceed to other boot methods. */
		debug("Raw boot image support not enabled, proceeding to other boot methods\n");
		return -EINVAL;
	}

	return 0;
}

#if SPL_LOAD_USERS > 1
int spl_load(struct spl_image_info *spl_image,
	     const struct spl_boot_device *bootdev, struct spl_load_info *info,
	     size_t size, size_t offset)
{
	return _spl_load(spl_image, bootdev, info, size, offset);
}
#endif

__weak void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	typedef void __noreturn (*image_entry_noargs_t)(void);

	image_entry_noargs_t image_entry =
		(image_entry_noargs_t)spl_image->entry_point;

	debug("image entry point: 0x%lx\n", spl_image->entry_point);
	image_entry();
}

/*
 * 64-bit: No special operation.
 *
 * 32-bit: Initial gd->bd->bi_dram[] to active dcache attr of memory.
 *	   Assuming 256MB is enough for SPL(MMU still maps 4GB size).
 */
#ifndef CONFIG_SPL_SYS_DCACHE_OFF
static int spl_dcache_enable(void)
{
	bool free_bd = false;

#ifndef CONFIG_ARM64
	if (!gd->bd) {
		gd->bd = calloc(1, sizeof(bd_t));
		if (!gd->bd) {
			debug("spl: no bd_t memory\n");
			return -ENOMEM;
		}
		/*
		 * If you want mmu init based on real dram configs from atags,
		 * call dram_init_banksize() here.
		 */
		gd->bd->bi_dram[0].start = CFG_SYS_SDRAM_BASE;
		gd->bd->bi_dram[0].size  = SZ_256M;
		free_bd = true;
	}
#endif
	/* TLB memory should be SZ_16K base align and 4KB end align */
	gd->arch.tlb_size = PGTABLE_SIZE;
	gd->arch.tlb_addr = (ulong)memalign(SZ_16K, ALIGN(PGTABLE_SIZE, SZ_4K));
	if (!gd->arch.tlb_addr) {
		debug("spl: no TLB memory\n");
		return -ENOMEM;
	}

	dcache_enable();
	if (free_bd)
		free(gd->bd);

	return 0;
}
#endif

#if CONFIG_IS_ENABLED(HANDOFF)
/**
 * Set up the SPL hand-off information
 *
 * This is initially empty (zero) but can be written by
 */
static int setup_spl_handoff(void)
{
	struct spl_handoff *ho;

	ho = bloblist_ensure(BLOBLISTT_U_BOOT_SPL_HANDOFF, sizeof(struct spl_handoff));
	if (!ho)
		return -ENOENT;

	return 0;
}

__weak int handoff_arch_save(struct spl_handoff *ho)
{
	return 0;
}

static int write_spl_handoff(void)
{
	struct spl_handoff *ho;
	int ret;

	ho = bloblist_find(BLOBLISTT_U_BOOT_SPL_HANDOFF, sizeof(struct spl_handoff));
	if (!ho)
		return -ENOENT;
	handoff_save_dram(ho);
	ret = handoff_arch_save(ho);
	if (ret)
		return ret;
	debug(PHASE_PROMPT "Wrote SPL handoff\n");

	return 0;
}
#else
static inline int setup_spl_handoff(void) { return 0; }
static inline int write_spl_handoff(void) { return 0; }

#endif /* HANDOFF */

/**
 * get_bootstage_id() - Get the bootstage ID to emit
 *
 * @start: true if this is for starting SPL, false for ending it
 * Return: bootstage ID to use
 */
static enum bootstage_id get_bootstage_id(bool start)
{
	enum xpl_phase_t phase = xpl_phase();

	if (IS_ENABLED(CONFIG_TPL_BUILD) && phase == PHASE_TPL)
		return start ? BOOTSTAGE_ID_START_TPL : BOOTSTAGE_ID_END_TPL;
	else if (IS_ENABLED(CONFIG_VPL_BUILD) && phase == PHASE_VPL)
		return start ? BOOTSTAGE_ID_START_VPL : BOOTSTAGE_ID_END_VPL;
	else
		return start ? BOOTSTAGE_ID_START_SPL : BOOTSTAGE_ID_END_SPL;
}

static int spl_common_init(bool setup_malloc)
{
	int ret;

#if CONFIG_IS_ENABLED(SYS_MALLOC_F)
	if (setup_malloc) {
#ifdef CFG_MALLOC_F_ADDR
		gd->malloc_base = CFG_MALLOC_F_ADDR;
#endif
		gd->malloc_limit = CONFIG_VAL(SYS_MALLOC_F_LEN);
		gd->malloc_ptr = 0;
	}
#endif

	/*
	 * setup D-cache as early as possible after malloc setup
	 * I-cache has been setup at early assembly code by default.
	 */
#ifndef CONFIG_SPL_SYS_DCACHE_OFF
	ret = spl_dcache_enable();
	if (ret) {
		debug("spl_dcache_enable() return error %d\n", ret);
		return ret;
	}
#endif
	ret = bootstage_init(xpl_is_first_phase());
	if (ret) {
		debug("%s: Failed to set up bootstage: ret=%d\n", __func__,
		      ret);
		return ret;
	}
	if (!xpl_is_first_phase()) {
		ret = bootstage_unstash_default();
		if (ret)
			log_debug("Failed to unstash bootstage: ret=%d\n", ret);
	}
	bootstage_mark_name(get_bootstage_id(true), xpl_name(xpl_phase()));
#if CONFIG_IS_ENABLED(LOG)
	ret = log_init();
	if (ret) {
		debug("%s: Failed to set up logging\n", __func__);
		return ret;
	}
#endif
	if (CONFIG_IS_ENABLED(OF_REAL)) {
		ret = fdtdec_setup();
		if (ret) {
			debug("fdtdec_setup() returned error %d\n", ret);
			return ret;
		}
	}
	if (CONFIG_IS_ENABLED(DM)) {
		bootstage_start(BOOTSTAGE_ID_ACCUM_DM_SPL,
				xpl_phase() == PHASE_TPL ? "dm tpl" : "dm_spl");
		/* With CONFIG_SPL_OF_PLATDATA, bring in all devices */
		ret = dm_init_and_scan(!CONFIG_IS_ENABLED(OF_PLATDATA));
		bootstage_accum(BOOTSTAGE_ID_ACCUM_DM_SPL);
		if (ret) {
			debug("dm_init_and_scan() returned error %d\n", ret);
			return ret;
		}

		ret = dm_autoprobe();
		if (ret)
			return ret;
	}

	return 0;
}

#if !defined(CONFIG_SPL_SKIP_RELOCATE) && !defined(CONFIG_TPL_BUILD)
static void spl_setup_relocate(void)
{
	gd->relocaddr = CONFIG_SPL_RELOC_TEXT_BASE;
	gd->new_gd = (gd_t *)gd;
	gd->start_addr_sp = gd->relocaddr;
	gd->fdt_size = ALIGN(fdt_totalsize(gd->fdt_blob) + 0x1000, 32);

	gd->start_addr_sp -= gd->fdt_size;
	gd->new_fdt = (void *)gd->start_addr_sp;
	memcpy(gd->new_fdt, gd->fdt_blob, gd->fdt_size);
	gd->fdt_blob = gd->new_fdt;

	gd->reloc_off = gd->relocaddr - (unsigned long)__image_copy_start;
	printf("Relocate from 0x%08lx to 0x%08lx.\n",
	       (unsigned long)__image_copy_start, gd->relocaddr);
}
#else
static void spl_setup_relocate(void)
{

}
#endif

void spl_set_bd(void)
{
	/*
	 * NOTE: On some platforms (e.g. x86) bdata may be in flash and not
	 * writeable.
	 */
	if (!gd->bd)
		gd->bd = &bdata;
}

int spl_early_init(void)
{
	int ret;

	debug("%s\n", __func__);

	ret = spl_common_init(true);
	if (ret)
		return ret;
	gd->flags |= GD_FLG_SPL_EARLY_INIT;

	spl_setup_relocate();

	return 0;
}

int spl_init(void)
{
	int ret;
	bool setup_malloc = !(IS_ENABLED(CONFIG_SPL_STACK_R) &&
			IS_ENABLED(CONFIG_SPL_SYS_MALLOC_SIMPLE));

	debug("%s\n", __func__);

	if (!(gd->flags & GD_FLG_SPL_EARLY_INIT)) {
		ret = spl_common_init(setup_malloc);
		if (ret)
			return ret;
	}
	gd->flags |= GD_FLG_SPL_INIT;

	return 0;
}

#ifndef BOOT_DEVICE_NONE
#define BOOT_DEVICE_NONE 0xdeadbeef
#endif

__weak void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = spl_boot_device();
}

__weak int spl_check_board_image(struct spl_image_info *spl_image,
				 const struct spl_boot_device *bootdev)
{
	return 0;
}

static int spl_load_image(struct spl_image_info *spl_image,
			  struct spl_image_loader *loader)
{
	int ret;
	struct spl_boot_device bootdev;

	bootdev.boot_device = loader->boot_device;
	bootdev.boot_device_name = NULL;

	ret = loader->load_image(spl_image, &bootdev);
#ifdef CONFIG_SPL_LEGACY_IMAGE_CRC_CHECK
	if (!ret && spl_image->dcrc_length) {
		/* check data crc */
		ulong dcrc = crc32_wd(0, (unsigned char *)spl_image->dcrc_data,
				      spl_image->dcrc_length, CHUNKSZ_CRC32);
		if (dcrc != spl_image->dcrc) {
			puts("SPL: Image data CRC check failed!\n");
			ret = -EINVAL;
		}
	}
#endif
	if (!ret)
		ret = spl_check_board_image(spl_image, &bootdev);

	return ret;
}

/**
 * boot_from_devices() - Try loading a booting U-Boot from a list of devices
 *
 * @spl_image: Place to put the image details if successful
 * @spl_boot_list: List of boot devices to try
 * @count: Number of elements in spl_boot_list
 * Return: 0 if OK, -ENODEV if there were no boot devices
 *	if CONFIG_SHOW_ERRORS is enabled, returns -ENXIO if there were
 *	devices but none worked
 */
static int boot_from_devices(struct spl_image_info *spl_image,
			     u32 spl_boot_list[], int count)
{
	struct spl_image_loader *drv =
		ll_entry_start(struct spl_image_loader, spl_image_loader);
	const int n_ents =
		ll_entry_count(struct spl_image_loader, spl_image_loader);
	int ret = -ENODEV;
	int i;

	for (i = 0; i < count && spl_boot_list[i] != BOOT_DEVICE_NONE; i++) {
		struct spl_image_loader *loader;
		int bootdev = spl_boot_list[i];

		if (CONFIG_IS_ENABLED(SHOW_ERRORS))
			ret = -ENXIO;
		for (loader = drv; loader != drv + n_ents; loader++) {
			if (bootdev != loader->boot_device)
				continue;
			if (!CONFIG_IS_ENABLED(SILENT_CONSOLE)) {
				if (loader)
					printf("Trying to boot from %s\n",
					       spl_loader_name(loader));
				else if (CONFIG_IS_ENABLED(SHOW_ERRORS)) {
					printf(PHASE_PROMPT
					       "Unsupported Boot Device %d\n",
					       bootdev);
				} else {
					puts(PHASE_PROMPT
					     "Unsupported Boot Device!\n");
				}
			}
			if (loader) {
				ret = spl_load_image(spl_image, loader);
				if (!ret) {
					spl_image->boot_device = bootdev;
					return 0;
				}
				spl_board_storages_fixup(loader);
				printf("Error: %d\n", ret);
			}
		}
	}

	return ret;
}

#if defined(CONFIG_DM) && !defined(CONFIG_SPL_SKIP_RELOCATE) && !defined(CONFIG_TPL_BUILD)
static int spl_initr_dm(void)
{
	int ret;

	/* Save the pre-reloc driver model and start a new one */
	gd->dm_root_f = gd->dm_root;
	gd->dm_root = NULL;
	bootstage_start(BOOTSTAGE_ID_ACCUM_DM_R, "dm_r");
	ret = dm_init_and_scan(false);
	bootstage_accum(BOOTSTAGE_ID_ACCUM_DM_R);
	if (ret)
		return ret;

#if defined(CONFIG_TIMER)
	gd->timer = NULL;
#endif
	serial_init();

	return 0;
}
#else
static int spl_initr_dm(void)
{
	return 0;
}
#endif

#if defined(CONFIG_SPL_KERNEL_BOOT) && !defined(CONFIG_ARM64)
static void boot_jump_linux(struct spl_image_info *spl_image)
{
	void (*kernel_entry)(int zero, int arch, ulong params);

	printf("Jumping to %s(0x%08lx)\n", "Kernel",
	       (ulong)spl_image->entry_point_os);
	spl_cleanup_before_jump(spl_image);
	kernel_entry = (void (*)(int, int, ulong))spl_image->entry_point_os;
	kernel_entry(0, 0, (ulong)spl_image->fdt_addr);
}
#endif

#if defined(CONFIG_SPL_FRAMEWORK_BOARD_INIT_F)
void board_init_f(ulong dummy)
{
	if (CONFIG_IS_ENABLED(OF_CONTROL)) {
		int ret;

		ret = spl_early_init();
		if (ret) {
			debug("spl_early_init() failed: %d\n", ret);
			hang();
		}
	}

	preloader_console_init();
}
#endif

void board_init_r(gd_t *dummy1, ulong dummy2)
{
	u32 spl_boot_list[] = {
		BOOT_DEVICE_NONE,
		BOOT_DEVICE_NONE,
		BOOT_DEVICE_NONE,
		BOOT_DEVICE_NONE,
		BOOT_DEVICE_NONE,
	};
	spl_jump_to_image_t jump_to_image = &jump_to_image_no_args;
	struct spl_image_info spl_image;
	int ret, os;

	debug(">>" PHASE_PROMPT "board_init_r()\n");
	gd->flags |= GD_FLG_RELOC;

	spl_initr_dm();

	spl_set_bd();

	if (IS_ENABLED(CONFIG_SPL_SYS_MALLOC)) {
		mem_malloc_init(SPL_SYS_MALLOC_START, SPL_SYS_MALLOC_SIZE);
		gd->flags |= GD_FLG_FULL_MALLOC_INIT;
	}
	if (!(gd->flags & GD_FLG_SPL_INIT)) {
		if (spl_init())
			hang();
	}
	timer_init();
	if (CONFIG_IS_ENABLED(BLOBLIST)) {
		ret = bloblist_init();
		if (ret) {
			debug("%s: Failed to set up bloblist: ret=%d\n",
			      __func__, ret);
			puts(PHASE_PROMPT "Cannot set up bloblist\n");
			hang();
		}
	}
	if (CONFIG_IS_ENABLED(HANDOFF)) {
		int ret;

		ret = setup_spl_handoff();
		if (ret) {
			puts(PHASE_PROMPT "Cannot set up SPL handoff\n");
			hang();
		}
	}

	if (CONFIG_IS_ENABLED(SOC_INIT))
		spl_soc_init();

	if (IS_ENABLED(CONFIG_SPL_WATCHDOG) && CONFIG_IS_ENABLED(WDT))
		initr_watchdog();

	if (IS_ENABLED(CONFIG_SPL_RAM)) {
		dram_init();
		gd->ram_top = CFG_SYS_SDRAM_BASE + gd->ram_size;
		printf("Ram: 0x%08lx - 0x%08lx\n",
		       (ulong)CFG_SYS_SDRAM_BASE, (ulong)gd->ram_top);
	}

	if (IS_ENABLED(CONFIG_SPL_OS_BOOT) || CONFIG_IS_ENABLED(HANDOFF) ||
	    IS_ENABLED(CONFIG_SPL_ATF) || IS_ENABLED(CONFIG_SPL_NET))
		dram_init_banksize();

	if (IS_ENABLED(CONFIG_SPL_LMB))
		lmb_init();

	if (CONFIG_IS_ENABLED(PCI) && !(gd->flags & GD_FLG_DM_DEAD)) {
		ret = pci_init();
		if (ret)
			puts(PHASE_PROMPT "Cannot initialize PCI\n");
		/* Don't fail. We still can try other boot methods. */
	}

	if (CONFIG_IS_ENABLED(BOARD_INIT))
		spl_board_init();

	bootcount_inc();

	/* Dump driver model states to aid analysis */
	if (CONFIG_IS_ENABLED(DM_STATS)) {
		struct dm_stats mem;

		dm_get_mem(&mem);
		dm_dump_mem(&mem);
	}

	memset(&spl_image, '\0', sizeof(spl_image));

#ifdef CONFIG_MP_BOOT
	mpb_init_x(0);
#endif

#if CONFIG_IS_ENABLED(ATF)
	/*
	 * Bl32 ep is optional, initial it as an invalid value.
	 * BL33 ep is mandatory, but initial it as a default value is better.
	 */
	spl_image.entry_point_bl32 = -1;
	spl_image.entry_point_bl33 = CONFIG_TEXT_BASE;
#endif

#if CONFIG_IS_ENABLED(OPTEE_IMAGE)
	/* default address */
	spl_image.entry_point_os = CONFIG_TEXT_BASE;
#endif

#ifdef CONFIG_SYS_SPL_ARGS_ADDR
	spl_image.arg = (void *)CONFIG_SYS_SPL_ARGS_ADDR;
#endif
	if (IS_ENABLED(CONFIG_SPL_OS_BOOT))
		spl_image.arg = (void *)SPL_PAYLOAD_ARGS_ADDR;
	spl_image.boot_device = BOOT_DEVICE_NONE;
	board_boot_order(spl_boot_list);
	spl_next_stage(&spl_image);
	ret = boot_from_devices(&spl_image, spl_boot_list,
				ARRAY_SIZE(spl_boot_list));
	if (ret) {
		if (CONFIG_IS_ENABLED(SHOW_ERRORS))
			printf(PHASE_PROMPT "failed to boot from all boot devices (err=%d)\n",
			       ret);
		else
			puts(PHASE_PROMPT "failed to boot from all boot devices\n");
		hang();
	}

	spl_perform_fixups(&spl_image);

#ifdef CONFIG_MP_BOOT
	mpb_init_x(2);
#endif

#ifdef CONFIG_CPU_V7M
	spl_image.entry_point |= 0x1;
#endif
	os = spl_image.os;
	if (os == IH_OS_U_BOOT) {
		debug("Jumping to %s...\n", xpl_name(xpl_next_phase()));
		spl_cleanup_before_jump(&spl_image);
	} else if (CONFIG_IS_ENABLED(ATF) && os == IH_OS_ARM_TRUSTED_FIRMWARE) {
		printf("Jumping to %s(0x%08lx) via ARM Trusted Firmware(0x%08lx)\n",
		       spl_image.next_stage == SPL_NEXT_STAGE_UBOOT ? "U-Boot" :
		       (spl_image.next_stage == SPL_NEXT_STAGE_KERNEL ? "Kernel" : "Unknown"),
		       (ulong)spl_image.entry_point_bl33,
		       (ulong)spl_image.entry_point);
#ifndef CONFIG_ARCH_ROCKCHIP
		spl_fixup_fdt(spl_image_fdt_addr(&spl_image));
#endif
		jump_to_image = &spl_invoke_atf;
	} else if (CONFIG_IS_ENABLED(OPTEE_IMAGE) &&
		   (os == IH_OS_OP_TEE || os == IH_OS_TEE)) {
		printf("Jumping to %s(0x%08lx) via OP-TEE(0x%08lx)\n",
		       spl_image.next_stage == SPL_NEXT_STAGE_UBOOT ? "U-Boot" :
		       (spl_image.next_stage == SPL_NEXT_STAGE_KERNEL ? "Kernel" : "Unknown"),
		       (ulong)spl_image.entry_point_os,
		       (ulong)spl_image.entry_point);
		spl_cleanup_before_jump(&spl_image);
		spl_board_prepare_for_optee(spl_image_fdt_addr(&spl_image));
		jump_to_image = &jump_to_image_optee;
	} else if (CONFIG_IS_ENABLED(OPENSBI) && os == IH_OS_OPENSBI) {
		debug("Jumping to U-Boot via RISC-V OpenSBI\n");
		jump_to_image = &spl_invoke_opensbi;
	} else if (CONFIG_IS_ENABLED(OS_BOOT) && os == IH_OS_LINUX) {
#ifdef CONFIG_SPL_OS_BOOT
		debug("Jumping to Linux\n");
		spl_fixup_fdt((void *)SPL_PAYLOAD_ARGS_ADDR);
		spl_board_prepare_for_linux();
		jump_to_image = &jump_to_image_linux;
#elif defined(CONFIG_SPL_KERNEL_BOOT) && !defined(CONFIG_ARM64)
		boot_jump_linux(&spl_image);
#endif
	} else {
		debug("Unsupported OS image.. Jumping nevertheless..\n");
	}
	if (CONFIG_IS_ENABLED(SYS_MALLOC_F) &&
	    !IS_ENABLED(CONFIG_SPL_SYS_MALLOC_SIZE))
		debug("SPL malloc() used 0x%x bytes (%d KB)\n",
		      gd_malloc_ptr(), gd_malloc_ptr() / 1024);

	debug("loaded - jumping to U-Boot...\n");
	bootstage_mark_name(get_bootstage_id(false), "end phase");
	ret = bootstage_stash_default();
	if (ret)
		debug("Failed to stash bootstage: err=%d\n", ret);

	if (IS_ENABLED(CONFIG_SPL_VIDEO_REMOVE)) {
		struct udevice *dev;
		int rc;

		rc = uclass_find_device(UCLASS_VIDEO, 0, &dev);
		if (!rc && dev) {
			rc = device_remove(dev, DM_REMOVE_NORMAL);
			if (rc)
				printf("Cannot remove video device '%s' (err=%d)\n",
				       dev->name, rc);
		}
	}
	if (CONFIG_IS_ENABLED(HANDOFF)) {
		ret = write_spl_handoff();
		if (ret)
			printf(PHASE_PROMPT
			       "SPL hand-off write failed (err=%d)\n", ret);
	}
	if (CONFIG_IS_ENABLED(UPL_OUT) && (gd->flags & GD_FLG_UPL)) {
		ret = spl_write_upl_handoff(&spl_image);
		if (ret) {
			printf(PHASE_PROMPT
			       "UPL hand-off write failed (err=%d)\n", ret);
			hang();
		}
	}
	if (CONFIG_IS_ENABLED(BLOBLIST)) {
		ret = bloblist_finish();
		if (ret)
			printf("Warning: Failed to finish bloblist (ret=%d)\n",
			       ret);
	}

	debug("Jumping to U-Boot(0x%08lx)\n", spl_image.entry_point);
	spl_board_prepare_for_boot();

	if (CONFIG_IS_ENABLED(RELOC_LOADER)) {
		int ret;

		ret = spl_reloc_jump(&spl_image, jump_to_image);
		if (ret) {
			if (xpl_phase() == PHASE_VPL)
				printf("jump failed %d\n", ret);
			hang();
		}
	}

	jump_to_image(&spl_image);
}

/*
 * This requires UART clocks to be enabled.  In order for this to work the
 * caller must ensure that the gd pointer is valid.
 */
void preloader_console_init(void)
{
#if CONFIG_IS_ENABLED(SERIAL)
	gd->baudrate = CONFIG_BAUDRATE;

	serial_init();		/* serial communications setup */

	gd->flags |= GD_FLG_HAVE_CONSOLE;

#if CONFIG_IS_ENABLED(BANNER_PRINT)
#ifdef BUILD_SPL_TAG
	puts("U-Boot " PHASE_NAME " " PLAIN_VERSION " (" U_BOOT_DATE " - "
	     U_BOOT_TIME " " U_BOOT_TZ "), fwver: "BUILD_SPL_TAG"\n");
#else
	puts("U-Boot " PHASE_NAME " " PLAIN_VERSION " (" U_BOOT_DATE " - "
	     U_BOOT_TIME " " U_BOOT_TZ ")\n");
#endif
#endif
#ifdef CONFIG_SPL_DISPLAY_PRINT
	spl_display_print();
#endif
#endif
}

/**
 * This function is called before the stack is changed from initial stack to
 * relocated stack. It tries to dump the stack size used
 */
__weak void spl_relocate_stack_check(void)
{
#if CONFIG_IS_ENABLED(SYS_REPORT_STACK_F_USAGE)
	ulong init_sp = gd->start_addr_sp;
	ulong stack_bottom = init_sp - CONFIG_VAL(SIZE_LIMIT_PROVIDE_STACK);
	u8 *ptr = (u8 *)stack_bottom;
	ulong i;

	for (i = 0; i < CONFIG_VAL(SIZE_LIMIT_PROVIDE_STACK); i++) {
		if (*ptr != CONFIG_VAL(SYS_STACK_F_CHECK_BYTE))
			break;
		ptr++;
	}
	printf("SPL initial stack usage: %lu bytes\n",
	       CONFIG_VAL(SIZE_LIMIT_PROVIDE_STACK) - i);
#endif
}

/**
 * spl_relocate_stack_gd() - Relocate stack ready for board_init_r() execution
 *
 * Sometimes board_init_f() runs with a stack in SRAM but we want to use SDRAM
 * for the main board_init_r() execution. This is typically because we need
 * more stack space for things like the MMC sub-system.
 *
 * This function calculates the stack position, copies the global_data into
 * place, sets the new gd (except for ARM, for which setting GD within a C
 * function may not always work) and returns the new stack position. The
 * caller is responsible for setting up the sp register and, in the case
 * of ARM, setting up gd.
 *
 * All of this is done using the same layout and alignments as done in
 * board_init_f_init_reserve() / board_init_f_alloc_reserve().
 *
 * Return: new stack location, or 0 to use the same stack
 */
ulong spl_relocate_stack_gd(void)
{
#if CONFIG_IS_ENABLED(STACK_R)
	gd_t *new_gd;
	ulong ptr = CONFIG_SPL_STACK_R_ADDR;

	if (CONFIG_IS_ENABLED(SYS_REPORT_STACK_F_USAGE))
		spl_relocate_stack_check();

#if defined(CONFIG_SPL_SYS_MALLOC_SIMPLE) && CONFIG_IS_ENABLED(SYS_MALLOC_F)
	if (CONFIG_SPL_STACK_R_MALLOC_SIMPLE_LEN) {
		debug("SPL malloc() before relocation used 0x%x bytes (%d KB)\n",
		      gd->malloc_ptr, gd->malloc_ptr / 1024);
		ptr -= CONFIG_SPL_STACK_R_MALLOC_SIMPLE_LEN;
		gd->malloc_base = ptr;
		gd->malloc_limit = CONFIG_SPL_STACK_R_MALLOC_SIMPLE_LEN;
		gd->malloc_ptr = 0;
	}
#endif
	/* Get stack position: use 8-byte alignment for ABI compliance */
	ptr = CONFIG_SPL_STACK_R_ADDR - roundup(sizeof(gd_t),16);
	gd->start_addr_sp = ptr;
	new_gd = (gd_t *)ptr;
	memcpy(new_gd, (void *)gd, sizeof(gd_t));
#if CONFIG_IS_ENABLED(DM)
	dm_fixup_for_gd_move(new_gd);
#endif
#if CONFIG_IS_ENABLED(LOG)
	log_fixup_for_gd_move(new_gd);
#endif
#if !defined(CONFIG_ARM) && !defined(CONFIG_RISCV)
	gd = new_gd;
#endif
	return ptr;
#else
	return 0;
#endif
}

/* cleanup before jump to next stage */
void spl_cleanup_before_jump(struct spl_image_info *spl_image)
{
	ulong us, tt_us;

	spl_board_prepare_for_jump(spl_image);

	disable_interrupts();

#ifdef CONFIG_ARM64
	disable_serror();
#else
	disable_async_abort();
#endif
	/*
	 * Turn off I-cache and invalidate it
	 */
	icache_disable();
	invalidate_icache_all();

	/*
	 * Turn off D-cache
	 * dcache_disable() in turn flushes the d-cache and disables MMU
	 */
	dcache_disable();
	invalidate_dcache_all();

	dsb();
	isb();

	us = (get_ticks() - gd->sys_start_tick) / (gd->arch.timer_rate_hz / 1000000);
	tt_us = get_ticks() / (gd->arch.timer_rate_hz / 1000000);
	printf("Total: %ld.%ld/%ld.%ld ms\n\n", us / 1000, us % 1000, tt_us / 1000, tt_us % 1000);
}

#if defined(CONFIG_BOOTCOUNT_LIMIT) && \
	((!defined(CONFIG_TPL_BUILD) && !defined(CONFIG_SPL_BOOTCOUNT_LIMIT)) || \
	 (defined(CONFIG_TPL_BUILD) && !defined(CONFIG_TPL_BOOTCOUNT_LIMIT)))
void bootcount_store(ulong a)
{
}

ulong bootcount_load(void)
{
	return 0;
}
#endif
