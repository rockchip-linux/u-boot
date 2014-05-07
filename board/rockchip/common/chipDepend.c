
#include    "config.h"
#include   <asm/io.h>


 
/***************************************************************************
函数描述:延时
入口参数:us数
出口参数:
调用函数:
***************************************************************************/
void DRVDelayUs(uint32 count)
{
	__udelay(count);
}



/***************************************************************************
函数描述:延时
入口参数:ms数
出口参数:
调用函数:
***************************************************************************/
void DRVDelayMs(uint32 count)
{
	DRVDelayUs(1000*count);
}


/***************************************************************************
函数描述:延时
入口参数:s数
出口参数:
调用函数:
***************************************************************************/
void DRVDelayS(uint32 count)
{
	while (count--)
		DRVDelayMs(1000);
}


//定义Loader启动异常类型
//系统中设置指定的sdram值为该标志，重启即可进入rockusb
//系统启动失败标志
uint32 IReadLoaderFlag(void)
{
	return readl(RKIO_PMU_PHYS + PMU_SYS_REG0);
}

void ISetLoaderFlag(uint32 flag)
{
	writel(flag, RKIO_PMU_PHYS + PMU_SYS_REG0);
}

uint32 IReadLoaderMode(void)
{
	return readl(RKIO_PMU_PHYS + PMU_SYS_REG1);
}


void FW_NandDeInit(void)
{
#ifdef CONFIG_NAND//RK_FLASH_BOOT_EN
	if(gpMemFun->flag == BOOT_FROM_FLASH) {
		FtlDeInit();
		FlashDeInit();
	}
#endif
}


void rkplat_uart2UsbEn(uint32 en)
{
#if (CONFIG_RKCHIPTYPE == CONFIG_RK3288)
	if (en) {
		uint32 con = 0;

		grf_writel((0x0000 | (0x00C0 << 16)), GRF_UOC0_CON3); // usbphy0 bypass select and otg disable.

		/* if define force enable usb to uart, maybe usb function will be affected */
#ifdef CONFIG_RKUSB2UART_FORCE
		grf_writel((0x00C0 | (0x00C0 << 16)), GRF_UOC0_CON3); // usb uart enable.
#else
		con = grf_readl(GRF_SOC_STATUS2);
		if (!(con & (1<<14)) && (con & (1<<17))) { // check IO domain voltage select.
			grf_writel((0x0004 | (0x0004 << 16)), GRF_UOC0_CON2); // software control usb phy disable
			grf_writel((0x002A | (0x003F << 16)), GRF_UOC0_CON3); // usb phy enter suspend
			grf_writel((0x00C0 | (0x00C0 << 16)), GRF_UOC0_CON3); // uart enable
		}
#endif /* CONFIG_RKUSB2UART_FORCE */
	} else {
		grf_writel((0x0000 | (0x00C0 << 16)), GRF_UOC0_CON3); // usb uart disable
		grf_writel((0x0000 | (0x0004 << 16)), GRF_UOC0_CON2); // software control usb phy enable
	}
#else
	#error "PLS check CONFIG_RKCHIPTYPE if support uart2usb."
#endif /* CONFIG_RKPLATFORM */
}




