/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * Configuation settings for the rk3xxx chip platform.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#ifdef CONFIG_OF_LIBFDT
#include <fdt_support.h>
#endif /* CONFIG_OF_LIBFDT */
#include <asm/arch/rkplat.h>

DECLARE_GLOBAL_DATA_PTR;


//#define BOARD_TEST_TIMER_IO

//#define BOARD_TEST_CACHE_IO
//#define BOARD_TEST_GPIO_IRQ_IO
//#define BOARD_TEST_EMMC_IO
//#define BOARD_TEST_PM_IO
#define BOARD_TEST_DMAC_IO


#ifdef BOARD_TEST_GPIO_IRQ_IO
static inline void board_gic_irq_test(void)
{
	printf("gic irq test start...\n");
}
#endif


#ifdef BOARD_TEST_GPIO_IRQ_IO

unsigned int isr_cnt = 0, last_cnt = 0;
static void test_gpio_irq_isr(void)
{
	printf("test_gpio_irq_isr\n");
	udelay(20000);
	isr_cnt++;
}

static inline void board_gpio_irq_test(void)
{
	int gpio_irq;

	printf("gpio irq test start...\n");

	gpio_irq = gpio_to_irq(GPIO_BANK4 | GPIO_C5);
	irq_install_handler(gpio_irq, test_gpio_irq_isr, NULL);
	//irq_set_irq_type(gpio_irq, IRQ_TYPE_EDGE_FALLING);
	irq_set_irq_type(gpio_irq, IRQ_TYPE_LEVEL_LOW);
	irq_handler_enable(gpio_irq);
	irq_handler_enable(gpio_irq);

	last_cnt = -1;
	isr_cnt = 0;
	while(1) {
		if (last_cnt != isr_cnt) {
			printf("isr_cnt = %d, last_cnt = %d\n", isr_cnt, last_cnt);
			last_cnt = isr_cnt;
		}

		udelay(500000);
	}
}
#endif


#ifdef BOARD_TEST_TIMER_IO
#define LPJ_24MHZ  100UL

static void clk_loop_delayus(uint32_t us)
{
	volatile uint32_t i;

	/* copro seems to need some delay between reading and writing */
	for (i = 0; i < LPJ_24MHZ * us; i++) {
		nop();
	}
}

static inline void board_timer_test(void)
{
	int ts, delay;
	uint32 TimeOutBase = 0;
	pTIMER_REG ptimerReg = (pTIMER_REG)RKIO_TIMER0_PHYS;
	uint32 tcount0 = 0, tload0 = 0, tcount1 = 0, tload1 = 0, tctrl = 0;

	printf("Timer test start...\n");

	printf("First udelay 3s test\n");
	udelay(3*1000*1000);
	printf("Delay 3s test end.\n");

	printf("Then delay 3s display test\n");
	delay = 3;
	while (delay > 0) {
		--delay;
		/* delay 1000 ms */
		ts = get_timer(0);
		do {
			udelay(10000);
		} while (get_timer(ts) < 1000);

		printf("\b\b\b%2d ", delay);
	}
	printf("\nThen delay 3s display test end.\n");

	printf("Timer test end.\n");
}
#endif


#ifdef BOARD_TEST_PM_IO
static unsigned wakeup_gpio_cnt = 0;

static void wakeup_gpio_irq_isr(void)
{
	printf("test_gpio_irq_isr\n");
	wakeup_gpio_cnt++;
}

static inline void board_pm_test(void)
{
	uint32 con = 0;
	int gpio_irq = 0;

	printf("PM test start...\n");

	gpio_irq = gpio_to_irq(GPIO_BANK0 | GPIO_A4);
	irq_install_handler(gpio_irq, wakeup_gpio_irq_isr, NULL);
	irq_set_irq_type(gpio_irq, IRQ_TYPE_LEVEL_LOW);
	irq_handler_enable(gpio_irq);
	irq_handler_enable(IRQ_GPIO0);

#ifdef CONFIG_PM_SUBSYSTEM
	rk_pm_enter();
#endif
	printf("wakeup_gpio_cnt = %d\n", wakeup_gpio_cnt);

	printf("PM test end.\n");
}
#endif

#ifdef BOARD_TEST_CACHE_IO
static inline void board_cache_test(void)
{
	uint32 *g_fb_buff = gd->fb_base+(1280*180+200)*4;
	uint32 i = 0;

	printf("Cache test: dcache test by lcdc start...\n");
	printf("Notice that fb base should in the cache TLB\n");
	printf("so if test, change rk30xx config fb base and ramdisk size such as:\n");
	printf("PHYS_SDRAM_1_SIZE = (512 << 20), gd->fb_base = 0x64000000");

	flush_dcache_all();
	for(i = 0; i < 40; i++)
		g_fb_buff[i] = 0x55555555;
	flush_cache(g_fb_buff, 40*4);
	printf("Dcache: fill fb buffer 0x55 and flush.\n");
	udelay(1000*2000);
	for(i = 0; i < 20; i++)
		g_fb_buff[i] = 0xFFFFFFFF;
	printf("Dcache: fill fb buffer 0xff and flush.\n");
	flush_cache(g_fb_buff, 20*4);
	udelay(1000*2000);
	for(i = 0; i < 20; i++)
		g_fb_buff[i] = 0x00000000;
	printf("Dcache: fill fb buffer 0x00 and no flush.\n");
	udelay(1000*2000);
	printf("Cache test: dcache end.\n");
}
#endif


#ifdef BOARD_TEST_EMMC_IO
static inline void board_emmc_test(void)
{
	printf("emmc test start...\n");

	memset(DataBuf, 0, sizeof(DataBuf));

	if (StorageInit() == 0) {
		printf("emmc init OK!\n");
	} else {
		printf("Fail!\n");
	}

	if (!GetParam(0, DataBuf)) {
		printf("get parameter from emmc ok.");
	} else {
		printf("get parameter from emmc error.\n");
	}
	printf("parameter buffer is: %s\n", DataBuf);

	printf("Rockusb test end.\n");
}
#endif


#ifdef BOARD_TEST_DMAC_IO
static struct rk_dma_client rk_dma_memcpy_client = {
        .name = "rk-dma-memcpy",
};

#define DMA_MEM_TEST_SIZE	(1024 * 256)
static char *dma_src_addr0 = (char *)CONFIG_RAM_PHY_END;
static char *dma_src_addr1 = (char *)CONFIG_RAM_PHY_END + DMA_MEM_TEST_SIZE;
static char *dma_dst_addr = 0;
static uint32 dma_cnt = 0;
static void rk_dma_memcpy_callback(void *buf_id, int size, enum rk_dma_buffresult result)
{
	if (result != RK_RES_OK) {
		printf("%s error:%d\n", __func__, result);
	} else {
		rk_dma_ctrl(DMACH_DMAC2_MEMTOMEM, RK_DMAOP_STOP);
		rk_dma_ctrl(DMACH_DMAC2_MEMTOMEM, RK_DMAOP_FLUSH);
		printf("%s ok\n", __func__);
		dma_cnt ++;
		rk_dma_devconfig(DMACH_DMAC2_MEMTOMEM, RK_DMASRC_MEMTOMEM, (dma_cnt & 0x01) ? dma_src_addr0 : dma_src_addr1);
		rk_dma_enqueue(DMACH_DMAC2_MEMTOMEM, NULL, dma_dst_addr, DMA_MEM_TEST_SIZE);
		rk_dma_ctrl(DMACH_DMAC2_MEMTOMEM, RK_DMAOP_START);
	}
}


static inline void board_dmac_test(void)
{
	uint32 dmac_id = 1;

	printf("rk dmac test start...\n");

	dma_dst_addr = (char *)gd->fb_base;

	memset(dma_src_addr0, 0x55, DMA_MEM_TEST_SIZE);
	memset(dma_src_addr1, 0xaa, DMA_MEM_TEST_SIZE);
	udelay(1*1000*1000);
	dma_cnt = 0;

//	rk_pl330_dmac_init(dmac_id);
	if (rk_dma_request(DMACH_DMAC2_MEMTOMEM, &rk_dma_memcpy_client, NULL) == -EBUSY) {
		printf("DMACH_DMAC2_MEMTOMEM request fail!\n");
	} else {
		rk_dma_config(DMACH_DMAC2_MEMTOMEM, 8, 16);
		rk_dma_set_buffdone_fn(DMACH_DMAC2_MEMTOMEM, rk_dma_memcpy_callback);

		rk_dma_devconfig(DMACH_DMAC2_MEMTOMEM, RK_DMASRC_MEMTOMEM, (dma_cnt & 0x01) ? dma_src_addr0 : dma_src_addr1);
		rk_dma_enqueue(DMACH_DMAC2_MEMTOMEM, NULL, dma_dst_addr, DMA_MEM_TEST_SIZE);
		rk_dma_ctrl(DMACH_DMAC2_MEMTOMEM, RK_DMAOP_START);
		while (1);
	}
	udelay(1*1000*1000);
//	rk_pl330_dmac_deinit(dmac_id);

	printf("rk dmac test end\n");
}
#endif


static void board_module_test(void)
{
	printf("board_module_test start...\n");

#ifdef BOARD_TEST_TIMER_IO
	board_timer_test();
#endif

#ifdef BOARD_TEST_GPIO_IRQ_IO
	board_gpio_irq_test();
#endif

#ifdef BOARD_TEST_PM_IO
	board_pm_test();
#endif

#ifdef BOARD_TEST_CACHE_IO
	board_cache_test();
#endif


#ifdef BOARD_TEST_EMMC_IO
	board_emmc_test();
#endif

#ifdef BOARD_TEST_DMAC_IO
	board_dmac_test();
#endif

	printf("board_module_test end.\n");
}



static int do_board_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("Board Module Test.\n");

	board_module_test();
	return 0;
}

U_BOOT_CMD(
	board_test, 1, 0,	do_board_test,
	"Board Module Test",
	""
);

