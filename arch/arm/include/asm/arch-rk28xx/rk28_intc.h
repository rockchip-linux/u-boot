/******************************************************************/
/*	Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.  	  */
/*******************************************************************
File 	:	intc.h
Desc 	:	
Author 	:  	yangkai
Date 	:	2008-12-16
Notes 	:

********************************************************************/
#ifndef _INTC_H
#define _INTC_H

#define     USB_OTG_INT_CH                  (1<<8)
	
//INTC Registers
typedef volatile struct tagINTC_STRUCT 
 {
    volatile unsigned int IRQ_INTEN_L; 
    volatile unsigned int IRQ_INTEN_H;
    volatile unsigned int IRQ_INTMASK_L;
    volatile unsigned int IRQ_INTMASK_H; 
    volatile unsigned int IRQ_INTFORCE_L; 
    volatile unsigned int IRQ_INTFORCE_H; 
    volatile unsigned int IRQ_RAWSTATUS_L; 
    volatile unsigned int IRQ_RAWSTATUS_H; 
    volatile unsigned int IRQ_STATUS_L; 
    volatile unsigned int IRQ_STATUS_H; 
    volatile unsigned int IRQ_MASKSTATUS_L; 
    volatile unsigned int IRQ_MASKSTATUS_H; 
    volatile unsigned int IRQ_FINALSTATUS_L; 
    volatile unsigned int IRQ_FINALSTATUS_H; 
    volatile unsigned int RESERVED1[(0xc0-0x38)/4];
    volatile unsigned int FIQ_INTEN; 
    volatile unsigned int FIQ_INTMASK; 
    volatile unsigned int FIQ_INTFORCE;
    volatile unsigned int FIQ_RAWSTATUS; 
    volatile unsigned int FIQ_STATUS;
    volatile unsigned int FIQ_FINALSTATUS;
    volatile unsigned int IRQ_PLEVEL;
    volatile unsigned int RESERVED2[(0xe8-0xdc)/4];
    volatile unsigned int IRQ_PN_OFFSET[40];
    volatile unsigned int RESERVED3[(0x3f8-0x188)/4]; 
    volatile unsigned int AHB_ICTL_COMP_VERSION;
    volatile unsigned int ICTL_COMP_TYPE;
 } INTC_REG, *pINTC_REG;

#endif

