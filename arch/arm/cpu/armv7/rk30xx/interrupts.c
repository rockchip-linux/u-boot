#include <common.h>
#include <asm/arch/rk30_drivers.h>

#ifdef CONFIG_USE_RK30IRQ

void do_irq (struct pt_regs *pt_regs)
{
	IrqHandler();
}

int arch_interrupt_init (void)
{
	printf("arch_interrupt_init\n");
	InterruptInit();
	//rk30_reg_irq(irq_init_reg);
	return 0;
}
#endif /* CONFIG_USE_IRQ */

