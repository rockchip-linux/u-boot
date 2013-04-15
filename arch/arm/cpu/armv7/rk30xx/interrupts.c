#include <common.h>
#include <asm/arch/rk30_drivers.h>

#ifdef CONFIG_USE_IRQ
extern ulong _start;
extern ulong _end_vect;
extern void int_test(void);

extern void UsbIsr(void);

struct rk30_irq_map
{
	eINT_NUM	int_num;
	pFunc		func;
	int			enable_flag;
};

struct rk30_irq_map irq_init_reg[] = 
{
	//{INT_USB_OTG0 ,UsbIsr,0},
	{INT_MAXNUM,0,0}
	
};

void int_test(void)
{
	int i;
	i++;
	return ;
}

void copy_vect(void)
{
	unsigned int *det = 0;
	int i = 0;
	__asm__ __volatile__("": : :"memory");
	unsigned int *src = (unsigned int *)((unsigned int)_start + 4);
	//unsigned int *src = 0x61000004;
	__asm__ __volatile__("": : :"memory");
	unsigned int size = (unsigned int *)((unsigned int)_end_vect - (unsigned int)_start);
	__asm__ __volatile__("": : :"memory");

	for(i = 0; i < size; i++){
		*det++ = *src++;
	}
	
	return;
}

void init_vect_addr (void)
{
	*(unsigned long volatile *)(0x15000000)   = 0x2;
	*(unsigned long volatile *)(0x10000000)   = 0x2;
	*(unsigned long volatile *)(0x200080c0)   = 0x00300000;
	return ;
}

int rk30_reg_irq(struct rk30_irq_map *imp)
{
	int ret = 0;

	while(imp->int_num < INT_MAXNUM){
		ret = IRQRegISR(imp->int_num,imp->func,0,0);
		if(imp->enable_flag == 1)
			IntEnableIntSrc(imp->int_num);
		imp++;
	}
	
	return ret;
}

void do_irq (struct pt_regs *pt_regs)
{
	IRQHandler();
}

int arch_interrupt_init (void)
{

	INTCInit();
	rk30_reg_irq(irq_init_reg);
	return 0;
}
#endif /* CONFIG_USE_IRQ */

