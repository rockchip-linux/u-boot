// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */
#include <init.h>
#include <configs/rk182x_common.h>
#include <clk.h>
#include <dm.h>
#include <spl.h>
#include <asm/io.h>

/* T-HEAD C9xx M mode CSR.  */
#define THEAD_C9XX_CSR_MXSTATUS		0x7c0
#define THEAD_C9XX_CSR_MHCR		0x7c1
#define THEAD_C9XX_CSR_MCOR		0x7c2
#define THEAD_C9XX_CSR_MCCR2		0x7c3
#define THEAD_C9XX_CSR_MCER2		0x7c4
#define THEAD_C9XX_CSR_MHINT		0x7c5
#define THEAD_C9XX_CSR_MRMR		0x7c6
#define THEAD_C9XX_CSR_MRVBR		0x7c7
#define THEAD_C9XX_CSR_MCER		0x7c8
#define THEAD_C9XX_CSR_MCOUNTERWEN	0x7c9
#define THEAD_C9XX_CSR_MCOUNTERINTEN	0x7ca
#define THEAD_C9XX_CSR_MCOUNTEROF	0x7cb
#define THEAD_C9XX_CSR_MHINT2		0x7cc
#define THEAD_C9XX_CSR_MHINT3		0x7cd
#define THEAD_C9XX_CSR_MRADDR		0x7e0
#define THEAD_C9XX_CSR_MEXSTATUS	0x7e1
#define THEAD_C9XX_CSR_MNMICAUSE	0x7e2
#define THEAD_C9XX_CSR_MNMIPC		0x7e3
#define THEAD_C9XX_CSR_MHPMCR		0x7f0
#define THEAD_C9XX_CSR_MHPMSR		0x7f1
#define THEAD_C9XX_CSR_MHPMER		0x7f2
#define THEAD_C9XX_CSR_MSMPR		0x7f3
#define THEAD_C9XX_CSR_MTEECFG		0x7f4
#define CSR_MHCR_IE			BIT(0)
#define CSR_MHCR_DE			BIT(1)

#ifndef CONFIG_TPL_BUILD
#ifdef CONFIG_SPL_BUILD
static void caches_init(void)
{
	unsigned long csr_mhint;
	unsigned long csr_mhcr;
	unsigned long csr_mccr2;

	/* Clear MAFE bit(disable THEAD MMU extension) */
	csr_clear(THEAD_C9XX_CSR_MXSTATUS, BIT(21));

	csr_mhint = csr_read(THEAD_C9XX_CSR_MHINT);
	csr_mhint |= (0x1 << 2);   // DCACHE prefetch enable
	csr_mhint &= ~(0x3 << 3);
	csr_mhint |= (0x1 << 3);   // Auto write stream enable
	csr_mhint |= (0x1 << 8);   // ICACHE prefetch enable
	csr_mhint &= ~(0x3 << 13);
	csr_mhint |= (0x01 << 13); // DCACHE prefetch distance = 4
	csr_mhint |= (0x1 << 15);  // L2 CACHE prefetch enable
	csr_mhint |= (0x1 << 20);  // L2 CACHE store prefetch enable
	csr_mhint &= ~(0x1 << 21); // Disable TLB broadcast
	csr_mhint &= ~(0x1 << 23); // Disable fence.i broadcast
	csr_write(THEAD_C9XX_CSR_MHINT, csr_mhint);

	csr_mhcr = csr_read(THEAD_C9XX_CSR_MHCR);
	csr_mhcr |= (0x1 << 2);    // DCACHE write allocate enable
	csr_mhcr |= (0x1 << 4);    // Return stack enable
	csr_mhcr |= (0x1 << 5);    // Branch prediction enable
	csr_mhcr |= (0x1 << 6);    // Branch target buffer enable
	csr_mhcr |= (0x1 << 7);    // Indirect branch prediction enable
	csr_mhcr |= (0x1 << 12);   // L0 branch target buffer enable
	csr_mhcr |= (0x1 << 24);   // Instruction fusion enable
	csr_write(THEAD_C9XX_CSR_MHCR, csr_mhcr);

	csr_mccr2 = csr_read(THEAD_C9XX_CSR_MCCR2);
	csr_mccr2 &= ~(0x3 << 29);
	csr_mccr2 |= (0x1 << 29);  // Instruction prefetch depth(in cacheline)
	csr_write(THEAD_C9XX_CSR_MCCR2, csr_mccr2);

	/* Because ICACHE snoop DCACHE depend on SMP enable, so it should */
	/* be enabled even under a single core. */
	csr_set(THEAD_C9XX_CSR_MSMPR, BIT(0));   // SMP enable()

	/* icache enable */
	csr_set(THEAD_C9XX_CSR_MHCR, CSR_MHCR_IE);
	/* dcache enable */
	csr_set(THEAD_C9XX_CSR_MHCR, CSR_MHCR_DE);
}
#endif

int arch_cpu_init(void)
{
#ifdef CONFIG_SPL_BUILD
	caches_init();
#endif
	return 0;
}
#endif

void board_debug_uart_init(void)
{
	/* TODO: configure rk182x debug UART iomux/pins here. */
}

