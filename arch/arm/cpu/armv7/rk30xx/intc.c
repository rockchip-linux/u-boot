/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	intc.c
Desc 	:	ARM中断控制器驱动程序
Author 	:  	yangkai
Date 	:	2008-11-05
Notes 	:
$Log: intc.c,v $
Revision 1.3  2009/03/09 06:25:35  hcy
(hcy)文件系统修改了卡的mount实现，相应的卡驱动进行修改：文件系统卡mount从以前主动发起IO_CTL_GET_MEDIUM_STATUS命令改为由卡调用FS_MountDevice来实现mount，因此在插卡开机时需要跟着修改。
(yk)更新clock管理

Revision 1.2  2009/03/05 12:37:14  hxy
添加CVS版本自动注释

********************************************************************/
#define IN_INTC
#include  <asm/arch/rk30_drivers.h>
#ifdef DRIVERS_INTC

#define ARMEnableIRQ enable_interrupts
#define ARMDisableIRQ disable_interrupts
#define ARMDisableFIQ 	disable_interrupts
#define ARMEnableFIQ	enable_interrupts
#ifdef Assert
#undef Assert
#define Assert(expr, func...)
#else
#define Assert(expr, func...)
#endif
/*
     int g_irqVectorTableParam1[104];
     int g_irqVectorTableParam2[104];
    int g_fiqVectorTableParam1[104];
     int g_fiqVectorTableParam2[104];*/
/*----------------------------------------------------------------------
Name	: IRQRegISR
Desc	: 注册中断处理线程，将中断处理线程与中断号关联起来
Params  : intNum:中断号
          Routine:中断服务程序
Return  :
Notes   :
----------------------------------------------------------------------*/
uint32 IRQRegISR(eINT_NUM intNum, pFunc Routine,int param1,pFunc cb)
{
    Assert((intNum < INT_MAXNUM), "---ERROR---INT Number error:", intNum);
    Assert(Routine, "---ERROR---no INT routine, INT num:", intNum);

    if(intNum >= INT_MAXNUM)
    {
        return(-1);
    }
    g_irqVectorTable[intNum] = Routine;
    g_irqVectorTableParam1[intNum] = param1;
    g_irqVectorTableParam2[intNum] = cb;

    return(0);
}
uint32 IRQUnRegISR(eINT_NUM intNum)
{
    Assert((intNum < INT_MAXNUM), "---ERROR---INT Number error:", intNum);

    if(intNum >= INT_MAXNUM)
    {
        return(-1);
    }
    g_irqVectorTable[intNum] = NULL;
    g_irqVectorTableParam1[intNum] = 0;
    g_irqVectorTableParam2[intNum] = 0;

    return(0);
}
uint32 FIQRegISR(eINT_NUM intNum, pFunc Routine,int param1,pFunc cb)
{
    Assert((intNum < INT_MAXNUM), "---ERROR---INT Number error:", intNum);
    Assert(Routine, "---ERROR---no INT routine, INT num:", intNum);

    if(intNum >= INT_MAXNUM)
    {
        return(-1);
    }
    g_fiqVectorTable[intNum] = Routine;
    g_fiqVectorTableParam1[intNum] = param1;
    g_fiqVectorTableParam2[intNum] = cb;

    return(0);
}
int32 FIQUnRegISR(eINT_NUM intNum)
{
    Assert((intNum < INT_MAXNUM), "---ERROR---INT Number error:", intNum);


    if(intNum >= INT_MAXNUM)
    {
        return(-1);
    }
    g_fiqVectorTable[intNum] = NULL;
    g_fiqVectorTableParam1[intNum] = 0;
    g_fiqVectorTableParam2[intNum] = 0;
    return(0);
}


void IntSetPrioFilt(uint32 nprio)
{
	Assert((nprio <= PRIO_MAX), "---ERROR---PRIO TOO BIG:", nprio);
	g_giccReg->ICCPMR=(nprio&0xff);
}
void IntEnalbeDistributor()
{
	g_gicdReg->ICDDCR=0x01;
}
void IntEnalbeSecureSignal()
{
	g_giccReg->ICCICR|=0x01;
}
void IntDisalbeSecureSignal()
{
	g_giccReg->ICCICR&=(~0x01);
}
void IntEnalbeNoSecureSignal()
{
	g_giccReg->ICCICR|=0x02;
}
void IntDisalbeNoSecureSignal()
{
	g_giccReg->ICCICR&=(~0x02);
}
void IntDisalbeDistributor()
{
	g_gicdReg->ICDDCR=0x00;
}
int32 IntEnableIntSrc(eINT_NUM intNum)
{
	 uint32 M,N;
	 Assert((intNum < INT_MAXNUM), "---ERROR---IRQ Number error:", intNum);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
   M=intNum/32;
   N=intNum%32;
   g_gicdReg->ICDISER[M]=(0x1<<N);
   return 0; 
}
int32 IntDisableIntSrc(eINT_NUM intNum)
{
	 uint32 M,N;
	 Assert((intNum < INT_MAXNUM), "---ERROR---IRQ Number error:", intNum);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
   M=intNum/32;
   N=intNum%32;
   g_gicdReg->ICDICER[M]=(0x1<<N);
   return 0; 
}
uint32 IntSetPending(eINT_NUM intNum)
{
	 uint32 M,N;
	 Assert((intNum < INT_MAXNUM), "---ERROR---IRQ Number error:", intNum);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
   M=intNum/32;
   N=intNum%32;
   g_gicdReg->ICDISPR[M]=(0x1<<N);
   return 0; 
}
void IntSetIRQorFIQ(eINT_SIGTYPE ntype)
{
	if(ntype==INT_SIGTYPE_IRQ)
	 g_giccReg->ICCICR&=(~0x08);
	else
		{
		if(ntype==INT_SIGTYPE_FIQ)
			g_giccReg->ICCICR|=0x08;
		}	  
}
uint32 IntGetIntID(void)
{
	return g_giccReg->ICCIAR&0x3ff;
}
uint32 IntFinishIntSrv(uint32 nSrvIntID)
{
	g_giccReg->ICCEOIR=nSrvIntID;
}
uint32 IntClearPending(eINT_NUM intNum)
{
	 uint32 M,N;
	 Assert((intNum < INT_MAXNUM), "---ERROR---IRQ Number error:", intNum);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
   M=intNum/32;
   N=intNum%32;
   g_gicdReg->ICDICPR[M]=(0x1<<N);
   return 0; 
}
uint32 IntSetTrig(eINT_NUM intNum,eINT_TRIG ntrig)
{
	uint32 M,N;
	 Assert((intNum < INT_MAXNUM), "---ERROR---IRQ Number error:", intNum);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
   M=intNum/16;
   N=intNum%16;
   if(ntrig==INT_LEVEL_TRIG)
	g_gicdReg->ICDICFR[M]&=(~(1<<(2*N+1)));   
   else	   
   g_gicdReg->ICDICFR[M]|=(ntrig)<<(2*N+1);
   return 0; 
}
uint32 IntSetSecure(eINT_NUM intNum,eINT_SECURE nsecure)
{
	uint32 M,N;
	 Assert((intNum < INT_MAXNUM), "---ERROR---IRQ Number error:", intNum);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
   M=intNum/32;
   N=intNum%32;
   g_gicdReg->ICDISER[M]|=(nsecure)<<(N);
   return 0; 
}


/*----------------------------------------------------------------------
Name	: ARMEnableIRQ
Desc	: ARM CPSR中使能IRQ
Params  :
Return  :
Notes   :
----------------------------------------------------------------------*/
#if 0
void ARMEnableIRQ(void)
{
    uint32 tmp;

    __asm
    {
        MRS tmp, CPSR
        BIC tmp, tmp, #0x80
        MSR CPSR_cxsf, tmp
    }

}

/*----------------------------------------------------------------------
Name	: ARMDisableIRQ
Desc	: ARM CPSR中禁止IRQ
Params  :
Return  :
Notes   :
----------------------------------------------------------------------*/
void ARMDisableIRQ(void)
{
    uint32 tmp;

    __asm
    {
        MRS tmp, CPSR
        ORR tmp, tmp, #0x80
        MSR CPSR_cxsf, tmp
    }

}


/*----------------------------------------------------------------------
Name	: ARMEnableFIQ
Desc	: ARM CPSR中使能FIQ
Params  :
Return  :
Notes   :
----------------------------------------------------------------------*/
void ARMEnableFIQ(void)
{
    uint32 tmp;

    __asm
    {
        MRS tmp, CPSR
        BIC tmp, tmp, #0x40
        MSR CPSR_c, tmp
    }

}

/*----------------------------------------------------------------------
Name	: ARMDisableFIQ
Desc	: ARM CPSR中禁止FIQ
Params  :
Return  :
Notes   :
----------------------------------------------------------------------*/
void ARMDisableFIQ(void)
{
    uint32 tmp;

    __asm
    {
        MRS tmp, CPSR
        ORR tmp, tmp, #0x40
        MSR CPSR_c, tmp
    }

}
#endif
/*----------------------------------------------------------------------
Name	: IRQHandler
Desc	: IRQ中断处理函数
Params  :
Return  :
Notes   :
----------------------------------------------------------------------*/
uint32 IRQHandler(void)
{
    uint32 nIntID;
    
    nIntID=IntGetIntID();
    if(nIntID>=INT_MAXNUM)
    	{
    		printf("in irq,ERROR IntID 0x%x \n",nIntID);
    		return -1;
    	}
    else
    	{
    		if(g_irqVectorTable[nIntID]!=NULL)
    			{
    				g_irqVectorTable[nIntID]();
    				IntFinishIntSrv(nIntID);
    			}
    		else
    			{
    				printf("in irq,IntID 0x%x has not serveice\n",nIntID);
    				return -1;
    			}	
    			
    	}	
    return 0;   
}

/*----------------------------------------------------------------------
Name	: FIQHandler
Desc	: FIQ中断处理函数
Params  :
Return  :
Notes   :
----------------------------------------------------------------------*/
uint32 FIQHandler(void)
{
    uint32 nIntID;
    
    nIntID=IntGetIntID();
    if(nIntID>=INT_MAXNUM)
    	{
    		printf("in fiq,ERROR IntID 0x%x \n",nIntID);
    		return -1;
    	}
    else
    	{
    		if(g_fiqVectorTable[nIntID]!=NULL)
    			{
    				g_fiqVectorTable[nIntID]();
    				IntFinishIntSrv(nIntID);
    			}
    		else
    			{
    				printf("in fiq,IntID 0x%x has not serveice\n",nIntID);
    				return -1;
    			}	
    			
    	}	
   return 0; 
}

uint32 IRQEnable(eINT_NUM intNum)
{
	 uint32 M,N;
	 Assert((intNum < INT_MAXNUM), "---ERROR---IRQ Number error:", intNum);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
    M=intNum/32;
    N=intNum%32;
    IntSetIRQorFIQ(INT_SIGTYPE_IRQ);
    IntSetTrig(intNum,INT_LEVEL_TRIG);
    IntEnableIntSrc(intNum);
    return(0);
}
uint32 IRQDisable(eINT_NUM intNum)
{
	 uint32 M,N;
	 Assert((intNum < INT_MAXNUM), "---ERROR---IRQ Number error:", intNum);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
    M=intNum/32;
    N=intNum%32;
    IntDisableIntSrc(intNum);
    return(0);
}
uint32 FIQEnable(eINT_NUM intNum)
{
	 uint32 M,N;
	 Assert((intNum < INT_MAXNUM), "---ERROR---IRQ Number error:", intNum);
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
    M=intNum/32;
    N=intNum%32;
    IntSetIRQorFIQ(INT_SIGTYPE_FIQ);
    IntSetTrig(intNum,INT_LEVEL_TRIG);
    IntEnableIntSrc(intNum);
    return(0);
}
uint32 FIQDisable(eINT_NUM intNum)
{
	 uint32 M,N;
    if (intNum >= INT_MAXNUM)
    {
        return(-1);
    }
   M=intNum/32;
   N=intNum%32;
   IntDisableIntSrc(intNum);
   return(0);
}
/*----------------------------------------------------------------------
Name	: INTCInit
Desc	: 中断寄存器初始化，打开ARM的IRQ FIQ屏蔽位，禁止所有中断
Params  :
Return  : 0:成功,1:参数错误，失败
Notes   :
----------------------------------------------------------------------*/
uint32 INTCInit(void)
{
	  int i;
	  
	  g_giccReg->ICCEOIR = USB_OTG_INT_CH;
	  
    g_giccReg->ICCICR=0x00;   //disable signalling the interrupt
    g_gicdReg->ICDDCR=0x00;                                            
    g_gicdReg->ICDISER[0]=0x00; //disable the interrupt
    g_gicdReg->ICDISER[1]=0x00;
    g_gicdReg->ICDISER[2]=0x00;
    g_gicdReg->ICDISER[3]=0x00;
                                        

    ARMEnableFIQ();
   //  ARMDisableFIQ();
    ARMEnableIRQ();
    for(i=0;i<INT_MAXNUM;i++)
    {
    	g_irqVectorTable[i]=NULL;
    	g_irqVectorTableParam1[i]=0;
    	g_irqVectorTableParam2[i]=0;
    	g_fiqVectorTable[i]=NULL;
    	g_fiqVectorTableParam1[i]=0;
    	g_fiqVectorTableParam2[i]=0;
    }
    
    IntSetPrioFilt(0xff);   
    IntEnalbeSecureSignal();
    IntEnalbeNoSecureSignal();
    IntEnalbeDistributor();

    return(0);
}
extern uint32 INTCInit(void);
extern uint32 IRQHandler(void);
extern int32 IntEnableIntSrc(eINT_NUM intNum);
extern uint32 IntSetPending(eINT_NUM intNum);

#endif
