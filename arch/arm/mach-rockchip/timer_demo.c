/*
 * (C) Copyright 2015 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

 #include <common.h>
 #include <clk.h>
 #include <dm.h>
 #include <debug_uart.h>
 #include <ram.h>
 #include <syscon.h>
 #include <asm/io.h>
 #include <asm/arch/clock.h>
 #include <asm/arch/periph.h>
 #include <asm/arch/pmu_rk3288.h>
 #include <asm/arch/boot_mode.h>
 #include <asm/arch/timer.h>
 #include <asm/arch/irqs.h>
 #include <asm/gpio.h>
 #include <dm/pinctrl.h>

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
#define RKIO_TIMER_BASE     0xFF6B0000
#define RKIRQ_TIMER0		IRQ_TIMER_6CH_0
#define RKIRQ_TIMER1		IRQ_TIMER_6CH_1
#define RKIRQ_TIMER2		IRQ_TIMER_6CH_2
#define RKIRQ_TIMER3		IRQ_TIMER_6CH_3

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

/* demo function list */
static board_demo_t g_module_demo[] = {
	{ .name = "timer",	.demo = board_timer_test },
	{ .name = "gic-timer",	.demo = board_gic_test },
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
