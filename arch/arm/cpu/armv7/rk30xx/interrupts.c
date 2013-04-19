#include <common.h>
#include <asm/arch/rk30_drivers.h>

#ifdef CONFIG_USE_RK30IRQ
static int rk30_interrupt_inited = 0;
void do_irq (struct pt_regs *pt_regs)
{
	//printf("do_irq\n");
	IrqHandler();
}

int arch_interrupt_init (void)
{
	if(!rk30_interrupt_inited)
	{
		printf("arch_interrupt_init\n");
		InterruptInit();
		//rk30_reg_irq(irq_init_reg);
		rk30_interrupt_inited = 1;
	}
	return 0;
}
#endif /* CONFIG_USE_IRQ */

