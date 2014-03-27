




#include <common.h>


#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{

	return 0;
}
#endif


#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
	printf("CPU:\tRK3288\n");
#elif (CONFIG_RKCHIPTYPE == CONFIG_RK3299)
     printf("CPU:\tRK3299\n");
#endif
     return 0;
}
#endif


void reset_cpu(ulong ignored)
{
	//SoftReset();
}
