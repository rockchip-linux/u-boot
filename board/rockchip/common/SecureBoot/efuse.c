/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "../config.h"
#include "efuse.h"

#if defined(CONFIG_RKEFUSE_V1)

#define READ_SKIP_BITS      64
#define EFUSE_SIZE_BITS     512
#define EFUSE_ADDR(n)       ((n)<<7)
#define EFUSE_SIZE_BYTES    (EFUSE_SIZE_BITS/8)

//#define EFUSE_CSB         0x1
#define EFUSE_AEN           0x2
#define EFUSE_RDEN          0x4
#define EFUSE_PGMEN         0x8

static inline int32 EfuseReadByteMode(void *base, void *buff, uint32 addr, uint32 size)
{
	volatile uint32 *CtrlReg;
	volatile uint32 *DoutReg;
	uint8 *data;
	int32 i;

	CtrlReg = (volatile uint32 *)base;
	DoutReg = (volatile uint32 *)(base + 4);
	/* char unit */
	data = (uint8 *)buff;

	/* DVDD must be high, AVDD must be low */
	*CtrlReg = EFUSE_RDEN;

	/* read data, char unit */
	for (i = 0; i < size; i++, addr++) {
		*CtrlReg = EFUSE_RDEN | EFUSE_ADDR(addr);
		DRVDelayUs(1);
		*CtrlReg |= EFUSE_AEN;
		DRVDelayUs(1);
		*data++ = *DoutReg;
		/* AEN to Address hold time 10ns */
		*CtrlReg = EFUSE_RDEN | EFUSE_ADDR(addr);
	}

	/* AEN to Address hold time 100ns */
	DRVDelayUs(1);
	*CtrlReg = 0;
	DRVDelayUs(1);

	return 0;
}

int32 SecureEfuseRead(void *base, void *buff, uint32 addr, uint32 size)
{
	return EfuseReadByteMode(base, buff, addr, size);
}

int32 FtEfuseRead(void *base, void *buff, uint32 addr, uint32 size)
{
	return EfuseReadByteMode(base, buff, addr, size);
}

#elif defined(CONFIG_RKEFUSE_V2)

#define EFUSE_CSB       0x1
#define EFUSE_STROBE    0x2
#define EFUSE_LOAD      0x4
#define EFUSE_PGENB     0x8
#define EFUSE_PS        0x10
#define EFUSE_PD        0x20
#define EFUSE_ADDR(n)   ((n)<<6)

#define EFUSE_SIZE_BITS   1024
#define EFUSE_SIZE_WORDS  (EFUSE_SIZE_BITS/8)

/* 256bits using byte read mode */
static inline int32 EfuseReadByteMode(void *base, void *buff, uint32 addr, uint32 size)
{
	volatile uint32 *CtrlReg;
	volatile uint32 *DoutReg;
	uint8 *data;
	int32 i;

	CtrlReg = (volatile uint32 *)(unsigned long)base;
	DoutReg = (volatile uint32 *)((unsigned long)base + 4);
	/* byte unit */
	data = (uint8 *)buff;

	*CtrlReg = EFUSE_CSB; /* Active low chip select */
	DRVDelayUs(1);
	*CtrlReg = EFUSE_PGENB | EFUSE_LOAD;
	DRVDelayUs(1);

	/* read data, word unit */
	for (i = 0; i < size; i++) {
		*CtrlReg = EFUSE_PGENB | EFUSE_LOAD | EFUSE_ADDR(addr + i);
		DRVDelayUs(1); /* A[7:0] to STROBE setup time in read mode MIN 25ns */
		*CtrlReg |= EFUSE_STROBE;
		DRVDelayUs(1); /* DQ[7:0] delay time after STROBE high MAX 8ns */

		*data++ = *DoutReg; /* port0:8*32, port1:32*32 */
		*CtrlReg &= ~EFUSE_STROBE; /* A[7:0] to STROBE hold time MIN 3ns */
	}

	*CtrlReg = EFUSE_CSB;
	DRVDelayUs(1);

	return 0;
}

/* 1024bits using byte word mode */
static inline int32 EfuseReadWordMode(void *base, void *buff, uint32 addr, uint32 size)
{
	volatile uint32 *CtrlReg;
	volatile uint32 *DoutReg;
	uint32 *data;
	int32 i;

	CtrlReg = (volatile uint32 *)(unsigned long)base;
	DoutReg = (volatile uint32 *)((unsigned long)base + 4);
	/* word unit */
	data = (uint32 *)buff;
	addr = addr / 4;
	size = size / 4;

	*CtrlReg = EFUSE_CSB; /* Active low chip select */
	DRVDelayUs(1);
	*CtrlReg = EFUSE_PGENB | EFUSE_LOAD;
	DRVDelayUs(1);

	/* read data, word unit */
	for (i = 0; i < size; i++) {
		*CtrlReg = EFUSE_PGENB | EFUSE_LOAD | EFUSE_ADDR(addr + i);
		DRVDelayUs(1); /* A[7:0] to STROBE setup time in read mode MIN 25ns */
		*CtrlReg |= EFUSE_STROBE;
		DRVDelayUs(1); /* DQ[7:0] delay time after STROBE high MAX 8ns */

		*data++ = *DoutReg; /* port0:8*32, port1:32*32 */
		*CtrlReg &= ~EFUSE_STROBE; /* A[7:0] to STROBE hold time MIN 3ns */
	}

	*CtrlReg = EFUSE_CSB;
	DRVDelayUs(1);

	return 0;
}

/* 1024bits using byte word mode */
int32 SecureEfuseRead(void *base, void *buff, uint32 addr, uint32 size)
{
	return EfuseReadWordMode(base, buff, addr, size);
}

/* 256bits using byte read mode */
int32 FtEfuseRead(void *base, void *buff, uint32 addr, uint32 size)
{
	return EfuseReadByteMode(base, buff, addr, size);
}

#else
	#error: "PLS config efuse version for chip type!"
#endif
