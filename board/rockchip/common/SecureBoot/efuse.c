/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "../config.h"
#include "efuse.h"

#if defined(CONFIG_RKCHIP_RK3128)

#define READ_SKIP_BITS      64
#define EFUSE_SIZE_BITS     512
#define EFUSE_ADDR(n)       ((n)<<7)
#define EFUSE_SIZE_BYTES    (EFUSE_SIZE_BITS/8) 

//#define EFUSE_CSB         0x1
#define EFUSE_AEN           0x2
#define EFUSE_RDEN          0x4
#define EFUSE_PGMEN         0x8


int32 EfuseRead(void *buff, uint32 addr, uint32 size)
{
	volatile uint32 *CtrlReg;
	volatile uint32 *DoutReg;
	uint8 *data;
	int32 i;

	CtrlReg = (volatile uint32 *)EFUSE_BASE_ADDR;
	DoutReg = (volatile uint32 *)(EFUSE_BASE_ADDR + 4);
	/* char unit */
	data = (uint8 *)buff;

	/* DVDD must be high, AVDD must be low */
	*CtrlReg = EFUSE_RDEN;

	/* read data, char unit */
	for(i = 0; i < size; i++, addr++)
	{
		*CtrlReg = EFUSE_RDEN | EFUSE_ADDR(addr); 
		DRVDelayUs(1);
		*CtrlReg |= EFUSE_AEN;
		DRVDelayUs(1);
		*data++ = *DoutReg;
		*CtrlReg = EFUSE_RDEN | EFUSE_ADDR(addr); //AEN to Address hold time 10ns
	}

	DRVDelayUs(1);  //AEN to RDEN signal hold time 100ns                        
	*CtrlReg = 0;
	DRVDelayUs(1);

	return 0;
}

#elif defined(CONFIG_RKCHIP_RK3288) || defined(CONFIG_RKCHIP_RK3368)

#define EFUSE_CSB       0x1
#define EFUSE_STROBE    0x2
#define EFUSE_LOAD      0x4
#define EFUSE_PGENB     0x8
#define EFUSE_PS        0x10
#define EFUSE_PD        0x20
#define EFUSE_ADDR(n)   ((n)<<6)

#define EFUSE_SIZE_BITS   1024
#define EFUSE_SIZE_WORDS  (EFUSE_SIZE_BITS/8)

int32 EfuseRead(void *buff, uint32 addr, uint32 size)
{
	volatile uint32 *CtrlReg;
	volatile uint32 *DoutReg;
	uint32 *data;
	int32 i;

	CtrlReg = (volatile uint32 *)EFUSE_BASE_ADDR;
	DoutReg = (volatile uint32 *)(EFUSE_BASE_ADDR + 4);
	/* word unit */
	data = (uint32 *)buff;
	addr = addr / 4;
	size = size / 4;

	*CtrlReg = EFUSE_CSB;       //Active low chip select
	DRVDelayUs(1);
	*CtrlReg = EFUSE_PGENB | EFUSE_LOAD; 
	DRVDelayUs(1);

	/* read data, word unit */
	for(i = 0; i < size; i++)
	{
		*CtrlReg = EFUSE_PGENB | EFUSE_LOAD | EFUSE_ADDR(addr + i); //A[7:0] to STROBE setup time in read mode MIN 25ns
		DRVDelayUs(1);
		*CtrlReg = EFUSE_PGENB | EFUSE_LOAD | EFUSE_ADDR(addr + i) | EFUSE_STROBE;
		DRVDelayUs(1);                                                 //DQ[7:0] delay time after STROBE high MAX 8ns

		*data = *DoutReg;    //port0:8*32, port1:32*32                        
		*CtrlReg = EFUSE_PGENB | EFUSE_LOAD | EFUSE_ADDR(addr + i); //A[7:0] to STROBE hold time MIN 3ns
		data++;
	}

	*CtrlReg = EFUSE_CSB;
	DRVDelayUs(1);

	return 0;
}

#else
	#error: "PLS: config chip for efuse read api function!"
#endif
