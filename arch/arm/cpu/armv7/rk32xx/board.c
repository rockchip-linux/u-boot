

#include <common.h>


#ifdef CONFIG_DISPLAY_BOARDINFO
/**
 * Print board information
 */
int checkboard(void)
{
	puts("Board:\tRK32xx platform Board\n");
	return 0;
}
#endif



#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	stdio_print_current_devices();


	return 0;
}
#endif
