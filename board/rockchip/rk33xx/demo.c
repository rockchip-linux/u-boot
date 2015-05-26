/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#ifdef CONFIG_OF_LIBFDT
#include <fdt_support.h>
#endif /* CONFIG_OF_LIBFDT */
#include <asm/arch/rkplat.h>

#include <u-boot/zlib.h>
#include "../common/config.h"

DECLARE_GLOBAL_DATA_PTR;

/* demo function struct */
typedef void (*demo_func)(void);
typedef const char * demo_name;

typedef struct _board_demo_t {
	demo_name name;
	demo_func demo;
} board_demo_t;


/*
 * 	timer function module test
 * udelay and get_timer api function test
 */
static void board_timer_test(void)
{
	uint64_t ts;
	int delay;

	printf("Timer test start.\n");

	printf("First udelay 3s test\n");
	udelay(3*1000*1000);
	printf("Delay 3s test end.\n");

	printf("Then delay 10s display test\n");
	delay = 10;
	while (delay > 0) {
		--delay;
		/* delay 1000 ms */
		ts = get_timer(0);
		do {
			udelay(10000);
		} while (get_timer(ts) < 1000);

		printf("\b\b\b%2d ", delay);
	}
	printf("\nThen delay 10s display test end.\n");

	printf("Timer test end.\n");
}


/*
 *	gic function module test
 * using timer for interrupt api test
 */
#define DEMO_TIMER_BASE		(RKIO_TIMER0_6CH_PHYS + 0x20)
#define DEMO_TIMER_IRQ		IRQ_TIMER0_6CH_1

#define DEMO_TIMER_LOADE_COUNT0		0x00
#define DEMO_TIMER_LOADE_COUNT1		0x04
#define DEMO_TIMER_CURRENT_VALUE0	0x08
#define DEMO_TIMER_CURRENT_VALUE1	0x0C
#define DEMO_TIMER_CONTROL_REG		0x10
#define DEMO_TIMER_EOI			0x14
#define DEMO_TIMER_INTSTATUS		0x18

#define DEMO_TIMER_LOADE_COUNT		DEMO_TIMER_LOADE_COUNT0
#define DEMO_TIMER_CURR_VALUE		DEMO_TIMER_CURRENT_VALUE0

#define DEMO_TIMER_CTRL_REG		DEMO_TIMER_CONTROL_REG
#define DEMO_TIMER_LOAD_VAL		0x00FFFFFF

static inline void board_timer_isr(void)
{
	writel(0x01, DEMO_TIMER_BASE + DEMO_TIMER_INTSTATUS);

	printf("Timer isr.\n");
}

static void board_gic_test(void)
{
	printf("gic demo: using timer as interrupt source.\n");

	writel(DEMO_TIMER_LOAD_VAL, DEMO_TIMER_BASE + DEMO_TIMER_CURR_VALUE);
	writel(DEMO_TIMER_LOAD_VAL, DEMO_TIMER_BASE + DEMO_TIMER_LOADE_COUNT);
	/* auto reload & enable the timer */
	writel(0x05, DEMO_TIMER_BASE + DEMO_TIMER_CTRL_REG);

	irq_install_handler(DEMO_TIMER_IRQ, board_timer_isr, NULL);
	irq_handler_enable(DEMO_TIMER_IRQ);

	printf("gic demo: loop 10s for timer irq test.\n");
	udelay(10*1000*1000);

	writel(0x00, DEMO_TIMER_BASE + DEMO_TIMER_CTRL_REG);
	irq_handler_disable(DEMO_TIMER_IRQ);
	irq_uninstall_handler(DEMO_TIMER_IRQ);

	printf("gic demo: test end.\n");
}


#ifdef CONFIG_RK_DMAC
static struct rk_dma_client rk_dma_memcpy_client = {
        .name = "rk-dma-memcpy",
};

#define DMA_MEM_TEST_SIZE	(1024 * 4)
static uint32 *dma_src_addr = (uint32 *)CONFIG_RAM_PHY_END;
static uint32 *dma_dst_addr = (uint32 *)CONFIG_RAM_PHY_END + DMA_MEM_TEST_SIZE;
static uint32 dma_finish = 0;
static void rk_dma_memcpy_callback(void *buf_id, int size, enum rk_dma_buffresult result)
{
	if (result != RK_RES_OK) {
		printf("%s error:%d\n", __func__, result);
	} else {
		rk_dma_ctrl(DMACH_DMAC0_MEMTOMEM, RK_DMAOP_STOP);
		rk_dma_ctrl(DMACH_DMAC0_MEMTOMEM, RK_DMAOP_FLUSH);

		dma_finish = 1;
		printf("%s ok\n", __func__);
	}
}


static void board_dmac_test(void)
{
	int i = 0;

	printf("rk dmac test start...\n");

	for(i = 0; i < DMA_MEM_TEST_SIZE / sizeof(uint32); i++) {
		dma_src_addr[i] = 0x55aa55aa;
		dma_dst_addr[i] = 0x00000000;
	}
	dma_finish = 0;

	if (rk_dma_request(DMACH_DMAC0_MEMTOMEM, &rk_dma_memcpy_client, NULL) == -EBUSY) {
		printf("dma ch: DMACH_DMAC0_MEMTOMEM request fail!\n");
	} else {
		rk_dma_config(DMACH_DMAC0_MEMTOMEM, 8, 16);
		rk_dma_set_buffdone_fn(DMACH_DMAC0_MEMTOMEM, rk_dma_memcpy_callback);

		rk_dma_devconfig(DMACH_DMAC0_MEMTOMEM, RK_DMASRC_MEMTOMEM, dma_src_addr);
		rk_dma_enqueue(DMACH_DMAC0_MEMTOMEM, NULL, dma_dst_addr, DMA_MEM_TEST_SIZE);
		rk_dma_ctrl(DMACH_DMAC0_MEMTOMEM, RK_DMAOP_START);

		printf("Waiting for dma finish.\n");
		while(dma_finish == 0);

		rk_dma_free(DMACH_DMAC0_MEMTOMEM, &rk_dma_memcpy_client);

		int j = 0, k = 0;

		printf("dump dma memcopy data:\n");
		for (j = 0; j < (DMA_MEM_TEST_SIZE / sizeof(uint32) / 4); j++) {
			for (k = 0; k < 4; k++) {
				printf("%08x", dma_dst_addr[j * 4 + k]);
			}
			printf("\n");
		}
	}

	printf("rk dmac test end\n");
}
#endif /* CONFIG_RK_DMAC */


#ifdef CONFIG_GZIP
static int rk_load_zimage(uint32 offset, unsigned char *load_addr, size_t *image_size)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, buf, RK_BLK_SIZE);
	unsigned blocks;
	rk_kernel_image *image = (rk_kernel_image *)buf;
	unsigned head_offset = 8;//tag_rk_kernel_image's tag & size

	if (StorageReadLba(offset, (void *) image, 1) != 0) {
		printf("failed to read image header\n");
		return -1;
	}
	if(image->tag != TAG_KERNEL) {
		printf("bad image magic.\n");
		return -1;
	}
	*image_size = image->size;
	//image not align to blk size, so should memcpy some.
	memcpy((void *)load_addr, image->image, RK_BLK_SIZE - head_offset);

	//read the rest blks.
	blocks = DIV_ROUND_UP(*image_size, RK_BLK_SIZE);
	if (rkloader_CopyFlash2Memory((uint32) load_addr + RK_BLK_SIZE - head_offset,
				offset + 1, blocks - 1) != 0) {
		printf("failed to read image\n");
		return -1;
	}

	return 0;
}


static void board_gzip_test(void)
{
	const disk_partition_t* ptn = NULL;
	void *kaddr = 0, *laddr = 0;
	size_t ksize = 0;

	printf("rk gzip uncompress test start...\n");

	kaddr = (void*)CONFIG_KERNEL_LOAD_ADDR;
#ifndef CONFIG_SKIP_RELOCATE_UBOOT
	laddr = (void*)(CONFIG_RAM_PHY_START + (CONFIG_SYS_TEXT_BASE - CONFIG_RAM_PHY_START) + SZ_512K);
#else
	laddr = (void*)(SZ_128M);
#endif
	ptn = get_disk_partition(KERNEL_NAME);
	if (ptn == NULL) {
		printf("kernel partition error!\n");
		return ;
	}
	if (rk_load_zimage(ptn->start, kaddr, &ksize) != 0) {
		printf("load kernel image failed!\n");
		return ;
	}

	printf("Uncompressing Kernel zImage ...\n");
	if (gunzip(laddr, SZ_32M, kaddr, &ksize) != 0) {
		puts("GUNZIP: uncompress, out-of-mem or overwrite error\n");
		return ;
	}

	printf("rk gzip uncompress test end\n");
}
#endif /* CONFIG_GZIP */


#ifdef CONFIG_RK_GPIO
static void powerkey_gpio_irq_isr(void)
{
	printf("power key gpio irq isr\n");
	udelay(20000);
}

#define POWER_KEY_GPIO	(GPIO_BANK0 | GPIO_A2)
static inline void board_gpio_irq_test(void)
{
	int gpio_irq;

	printf("powerkey gpio irq test start...\n");

	gpio_irq = gpio_to_irq(POWER_KEY_GPIO);
	irq_install_handler(gpio_irq, powerkey_gpio_irq_isr, NULL);
	irq_set_irq_type(gpio_irq, IRQ_TYPE_LEVEL_LOW);
	irq_handler_enable(gpio_irq);

	printf("gpio demo: loop 10s for powerkey gpio irq test.\n");
	udelay(10 * 1000 *1000);

	irq_uninstall_handler(gpio_irq);
	irq_handler_disable(gpio_irq);

	printf("powerkey gpio irq test end\n");
}
#endif /* CONFIG_RK_GPIO */


/* demo function list */
static board_demo_t g_module_demo[] = {
	{ .name = "timer",	.demo = board_timer_test },
	{ .name = "gic",	.demo = board_gic_test },
#ifdef CONFIG_RK_DMAC
	{ .name = "dma",	.demo = board_dmac_test },
#endif
#ifdef CONFIG_GZIP
	{ .name = "gzip",	.demo = board_gzip_test },
#endif
#ifdef CONFIG_RK_GPIO
	{ .name = "gpio",	.demo = board_gpio_irq_test },
#endif
};
#define DEMO_MODULE_MAX		(sizeof(g_module_demo)/sizeof(board_demo_t))


/* demo command function */
static int do_board_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *module_name = NULL;

	if (argc >= 2) {
		module_name = argv[1];
	}

	printf("Board Module Test start.\n");

	if (module_name != NULL) {
		unsigned int index = 0;
		board_demo_t *module = NULL;

		for(index = 0; index < DEMO_MODULE_MAX; index++) {
			module = &g_module_demo[index];
			if ((module != NULL) && (strcmp(module->name, module_name) == 0)) {
				module->demo();
			}
		}
	}

	printf("Board Module Test end.\n");
	return 0;
}


/* demo function call api */
void board_module_demo(const char *module)
{
	char *const demo_cmd[2] = {"demo", module};

	do_board_test(NULL, NULL, 2, demo_cmd);
}


U_BOOT_CMD(
	demo, 2, 1,	do_board_test,
	"Board Module Test",
	""
);

