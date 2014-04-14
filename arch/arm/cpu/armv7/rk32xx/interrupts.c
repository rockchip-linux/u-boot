/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/
#include <asm/arch/drivers.h>
#include <common.h>
#define N_IRQS			143

struct _irq_handler {
	void                *m_data;
	void (*m_func)( void *data);
};

static struct _irq_handler IRQ_HANDLER[N_IRQS];

uint32 IRQEnable(eINT_NUM intNum)
{
	uint32 M,N;
	uint32 shift = (intNum % 4) * 8;
	uint32 offset = (intNum /4);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
    M=intNum/32;
    N=intNum%32;
    g_giccReg->ICCICR&=(~0x08);     //IntSetIRQ
    g_gicdReg->ICDISER[M]=(0x1<<(N));
    g_gicdReg->ITARGETSR[offset] |= (1 << shift);
    return(0);
}
uint32 IRQDisable(eINT_NUM intNum)
{
	uint32 M,N;
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
    M=intNum/32;
    N=intNum%32;
    g_gicdReg->ICDICER[M]=(0x1<<(N));
    return(0);
}



/***************************************************************************
函数描述:中断寄存器初始化
入口参数:
出口参数:
调用函数:
***************************************************************************/
void EnableOtgIntr(void)
{
    //g_giccReg->ICCICR&=(~0x08);     //IntSetIRQ
    //g_gicdReg->ICDISER[USB_OTG_INT_CH/32]=(0x1<<(USB_OTG_INT_CH % 32));
    IRQEnable(INT_USB_OTG);
}

/***************************************************************************
函数描述:中断寄存器初始化
入口参数:
出口参数:
调用函数:
***************************************************************************/
void DisableOtgIntr(void)
{
    //g_gicdReg->ICDICER[USB_OTG_INT_CH/32]=(0x1<<(USB_OTG_INT_CH % 32));
    IRQDisable(INT_USB_OTG);
}


/**************************************************************************
IRQ中断服务子程序
***************************************************************************/
void IrqHandler(void)
{
    uint32 intSrc;

	intSrc=g_giccReg->ICCIAR&0x3ff;     //IntGetIntID
	//g_giccReg->ICCEOIR=intSrc;
	//if((intSrc != USB_OTG_INT_CH) && (intSrc != INT_eMMC))
	//	serial_printf("Irq: %d\n", intSrc);
    if (intSrc == INT_USB_OTG)
    {
        UsbIsr();
#ifdef DRIVERS_USB_APP
        else
            MscUsbIsr();
#endif
    }
#ifdef    RK_SDMMC_BOOT_EN
    else if(intSrc == INT_eMMC)
    {
        _SDC2IST();
    }
#endif    
#ifdef CONFIG_PL330_DMA
    #if (CONFIG_RKCHIPTYPE == CONFIG_RK3026)
    else if(intSrc >= INT_DMAC2_0 && intSrc<=INT_DMAC2_0)
    {
	    pl330_irq_handler(intSrc);
    }
    #else	
    else if(intSrc >= INT_DMAC1_0 && intSrc<=INT_DMAC2_1)
    {
	    pl330_irq_handler(intSrc);
    }
    #endif	
	
#endif
    else if(intSrc >= INT_GPIO0 && intSrc <= INT_GPIO8)
    {
		gpio_isr(intSrc-INT_GPIO0);
    }
    g_giccReg->ICCEOIR=intSrc;
}



static void default_isr(void *data)
{
    uint32 intSrc = 0;
	//intSrc=g_giccReg->ICCIAR&0x3ff;     //IntGetIntID
	//printf("default_isr():  called for IRQ %d, Interrupt Status=%x PR=%x\n",
	//       (int)data, *IXP425_ICIP, *IXP425_ICIH);
}

static int next_irq(void)
{
	return 0;//(((*IXP425_ICIH & 0x000000fc) >> 2) - 1);
}


void irq_install_handler (int irq, interrupt_handler_t handle_irq, void *data)
{
	if (irq >= N_IRQS || !handle_irq)
		return;

	IRQ_HANDLER[irq].m_data = data;
	IRQ_HANDLER[irq].m_func = handle_irq;
}

void do_irq (struct pt_regs *pt_regs)
{
	//int irq = next_irq();

	//IRQ_HANDLER[irq].m_func(IRQ_HANDLER[irq].m_data);
	IrqHandler();
}

int arch_interrupt_init (void)
{
	int i;
	printf("arch_interrupt_init\n");
    g_giccReg->ICCEOIR=INT_USB_OTG;
    g_giccReg->ICCEOIR=INT_eMMC;
    g_giccReg->ICCICR=0x00;   //disable signalling the interrupt
    g_gicdReg->ICDDCR=0x00;  
                                              
    g_gicdReg->ICDICER[0]=0xFFFFFFFF;
    g_gicdReg->ICDICER[1]=0xFFFFFFFF;
    g_gicdReg->ICDICER[2]=0xFFFFFFFF;
    g_gicdReg->ICDICER[3]=0xFFFFFFFF;
    g_gicdReg->ICDICFR[3]&=(~(1<<1));
	g_giccReg->ICCPMR=0xff;     //IntSetPrioFilt
	g_giccReg->ICCICR|=0x01;    //IntEnalbeSecureSignal
	g_giccReg->ICCICR|=0x02;    //IntEnalbeNoSecureSignal
	g_gicdReg->ICDDCR=0x01;     //IntEnalbeDistributor

	/* install default interrupt handlers */
	for (i = 0; i < N_IRQS; i++)
		irq_install_handler(i, default_isr, (void *)i);
	return 0;
}


