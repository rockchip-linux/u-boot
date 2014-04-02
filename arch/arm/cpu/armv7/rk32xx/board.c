

#include <common.h>
#include <fastboot.h>
#include "config.h"
#include <asm/io.h>
#include <lcd.h>
//#include "rkimage.h"
//#include "rkloader.h"
#include "i2c.h"
#include <power/pmic.h>
#include <version.h>
#include <asm/arch/rk_i2c.h>

extern char bootloader_ver[];
DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_DISPLAY_BOARDINFO
/**
 * Print board information
 */
int checkboard(void)
{
	puts("Board:\t\tRK32xx platform Board\n");
	return 0;
}
#endif


/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	/* Set Initial global variables */

	gd->bd->bi_arch_number = MACH_TYPE_RK30XX;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x88000;

	return 0;
}


#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	stdio_print_current_devices();

	printf("board_late_init\n");
	
#ifdef CONFIG_RK_I2C 
		rk_i2c_init();
#endif
	
#if defined( CONFIG_POWER_ACT8846) || defined(CONFIG_POWER_RK808) 
		pmic_init(0);
#endif
		key_init();
		SecureBootCheck();
		get_bootloader_ver(NULL);
		//printf("#Boot ver: %s\n", bootloader_ver);
		ChargerStateInit();
		getParameter();
	
		//TODO:set those buffers in a better way, and use malloc?
		setup_space(gd->arch.rk_extra_buf_addr);
	
		char tmp_buf[30];
		if (getSn(tmp_buf)) {
			tmp_buf[sizeof(tmp_buf)-1] = 0;
			setenv("fbt_sn#", tmp_buf);
		}
		fbt_preboot();
		return 0;

	return 0;
}
#endif
