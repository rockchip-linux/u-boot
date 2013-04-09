#include <common.h>
#include <asm/io.h>
#include <asm/arch/clk.h>


#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	rk30_set_cpu_id();

	rk30_clock_init();

	return 0;
}
#endif

u32 get_device_type(void)
{
#if 0
	return s5p_cpu_id;
#else
	return 0;
#endif
}

#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
     printf("CPU:\tRK29XX\n");
     return 0;

}
#endif
