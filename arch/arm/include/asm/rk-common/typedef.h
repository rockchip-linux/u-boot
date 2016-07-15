/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef	_DRIVER_TYPEDEF_H
#define _DRIVER_TYPEDEF_H

#include <common.h>

#ifndef		TRUE
#  define	TRUE    1
#endif

#ifndef		FALSE
#  define	FALSE   0
#endif

#ifndef		NULL
#  define	NULL	0
#endif

#ifndef		OK
#  define	OK	0
#endif

#ifndef		ERROR
#  define	ERROR	!0
#endif

typedef bool		BOOL;


typedef	uint8_t			uint8;
typedef	int8_t			int8;
typedef	uint16_t		uint16;
typedef	int16_t			int16;
typedef uint32_t		uint32;
typedef	int32_t			int32;
typedef	uint64_t		uint64;
typedef	int64_t			int64;

typedef uint8_t 		UINT8;
typedef uint16_t		UINT16;
typedef uint32_t		UINT32;

typedef int8_t			INT8;
typedef int16_t			INT16;
typedef int32_t			INT32;

typedef uint8_t			INT8U;
typedef int8_t   		INT8S;
typedef uint16_t		INT16U;
typedef int16_t  		INT16S;
typedef int32_t 	  	INT32S;
typedef uint32_t   		INT32U;

typedef unsigned char		BYTE;
typedef unsigned long		ULONG;

typedef volatile unsigned char	REG8;
typedef volatile unsigned short	REG16;
typedef volatile unsigned int	REG32;

typedef volatile unsigned int	data_t;
typedef volatile unsigned int*	addr_t;

#define ReadReg32(addr)			(*(volatile uint32 *)(addr))
#define WriteReg32(addr, data)		(*(volatile uint32 *)(addr) = data)
#define MaskRegBits32(addr, y, z)	WriteReg32(addr, (ReadReg32(addr)&~(y))|(z))


/* void callback function type */
typedef void (* v_callback_f)(void);
typedef void (* pFunc)(void);


#endif  /*_DRIVER_TYPEDEF_H */
