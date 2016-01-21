/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
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

#include "../common/config.h"

DECLARE_GLOBAL_DATA_PTR;

/* demo function struct */
typedef void (*demo_func)(void);
typedef const char *demo_name;

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
#define DEMO_TIMER_BASE		(RKIO_TIMER_BASE + 0x20 * 3)
#define DEMO_TIMER_IRQ		RKIRQ_TIMER3


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

static void board_timer_isr(void)
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

	irq_install_handler(DEMO_TIMER_IRQ, (interrupt_handler_t *)board_timer_isr, (void *)NULL);
	irq_handler_enable(DEMO_TIMER_IRQ);

	printf("gic demo: loop 10s for timer irq test.\n");
	udelay(10*1000*1000);

	writel(0x00, DEMO_TIMER_BASE + DEMO_TIMER_CTRL_REG);
	irq_handler_disable(DEMO_TIMER_IRQ);
	irq_uninstall_handler(DEMO_TIMER_IRQ);

	printf("gic demo: test end.\n");
}


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
	irq_install_handler(gpio_irq, (interrupt_handler_t *)powerkey_gpio_irq_isr, (void *)NULL);
	irq_set_irq_type(gpio_irq, IRQ_TYPE_LEVEL_LOW);
	irq_handler_enable(gpio_irq);

	printf("gpio demo: loop 10s for powerkey gpio irq test.\n");
	udelay(10 * 1000 * 1000);

	irq_uninstall_handler(gpio_irq);
	irq_handler_disable(gpio_irq);

	printf("powerkey gpio irq test end\n");
}
#endif /* CONFIG_RK_GPIO */


#ifdef CONFIG_RK_PL330_DMAC
static struct rk_dma_client rk_dma_memcpy_client = {
        .name = "rk-dma-memcpy",
};

#define DMA_TEST_MEM2MEM_CH	DMACH_DMAC2_MEMTOMEM
#define DMA_MEM_TEST_SIZE	(1024 * 1024 * 48)
static uint32 *dma_src_addr = (uint32 *)(CONFIG_RAM_PHY_END + SZ_128M);
static uint32 *dma_dst_addr = (uint32 *)(CONFIG_RAM_PHY_END + SZ_128M + DMA_MEM_TEST_SIZE);
static volatile uint32 dma_finish;

static void rk_dma_memcpy_callback(void *buf_id, int size, enum rk_dma_buffresult result)
{
	if (result != RK_RES_OK) {
		printf("%s error:%d\n", __func__, result);
	} else {
		rk_dma_ctrl(DMA_TEST_MEM2MEM_CH, RK_DMAOP_STOP);
		rk_dma_ctrl(DMA_TEST_MEM2MEM_CH, RK_DMAOP_FLUSH);

		dma_finish = 1;
		printf("%s ok\n", __func__);
	}
}

static void board_dmac_test(void)
{
	int i = 0;

	printf("rk dmac test start...\n");

	for (i = 0; i < DMA_MEM_TEST_SIZE / sizeof(uint32); i++) {
		dma_src_addr[i] = 0x55aa55aa;
		dma_dst_addr[i] = 0x00000000;
	}
	dma_finish = 0;

	if (rk_dma_request(DMA_TEST_MEM2MEM_CH, &rk_dma_memcpy_client, NULL) == -EBUSY) {
		printf("dma ch: DMA_TEST_MEM2MEM_CH request fail!\n");
	} else {
		rk_dma_config(DMA_TEST_MEM2MEM_CH, 8, 16);
		rk_dma_set_buffdone_fn(DMA_TEST_MEM2MEM_CH, rk_dma_memcpy_callback);

		rk_dma_devconfig(DMA_TEST_MEM2MEM_CH, RK_DMASRC_MEMTOMEM, (unsigned long)dma_src_addr);
		rk_dma_enqueue(DMA_TEST_MEM2MEM_CH, NULL, (unsigned long)dma_dst_addr, DMA_MEM_TEST_SIZE);
		rk_dma_ctrl(DMA_TEST_MEM2MEM_CH, RK_DMAOP_START);

		printf("Waiting for dma finish.\n");
		while (dma_finish == 0);

		rk_dma_free(DMA_TEST_MEM2MEM_CH, &rk_dma_memcpy_client);

		if (DMA_MEM_TEST_SIZE <= SZ_1M) {
			int j = 0, k = 0;

			printf("dump dma memcopy data:\n");
			for (j = 0; j < (DMA_MEM_TEST_SIZE / sizeof(uint32) / 4); j++) {
				for (k = 0; k < 4; k++) {
					printf("%08x", dma_dst_addr[j * 4 + k]);
				}
				printf("\n");
			}
		} else {
			printf("dma memcopy check ");
			if (memcmp(dma_dst_addr, dma_src_addr, DMA_MEM_TEST_SIZE) != 0)
				printf("fail!\n");
			else
				printf("ok!\n");
		}
	}

	printf("rk dmac test end\n");
}
#endif /* CONFIG_RK_PL330_DMAC */


static void board_emmc_test(void)
{
	printf("rk emmc test start...\n");

	void *buff = (void *)(CONFIG_RAM_PHY_END + SZ_128M);
	uint32 blocks;
	uint32 start;

	start = 0;
	blocks = 1024;
	printf("Read LBA = 0x%08x, blocks = 0x%08x\n", start, blocks);
	StorageReadLba(start, buff, blocks);

	/* rk emmc max blocks = 0xFFFF (32M) */
	start = 0;
	blocks = (SZ_32M + SZ_1M) / 512;
	printf("Read LBA = 0x%08x, blocks = 0x%08x\n", start, blocks);
	StorageReadLba(start, buff, blocks);

	printf("rk emmc test end\n");
}


/* demo function list */
static board_demo_t g_module_demo[] = {
	{ .name = "timer",	.demo = board_timer_test },
	{ .name = "gic-timer",	.demo = board_gic_test },
#ifdef CONFIG_RK_GPIO
	{ .name = "gic-gpio",	.demo = board_gpio_irq_test },
#endif
#ifdef CONFIG_RK_PL330_DMAC
	{ .name = "dma",	.demo = board_dmac_test },
#endif
	{ .name = "emmc",	.demo = board_emmc_test },
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

		for (index = 0; index < DEMO_MODULE_MAX; index++) {
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
void board_module_demo(char * module)
{
	char *demo_cmd[2] = {"demo", module};

	do_board_test(NULL, 0, 2, demo_cmd);
}

U_BOOT_CMD(
	demo, 2, 1,	do_board_test,
	"Board Module Test",
	""
);

