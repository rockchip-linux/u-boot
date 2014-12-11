/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef	_DRIVER_TYPEDEF_H
#define _DRIVER_TYPEDEF_H


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

typedef int 	BOOL;

typedef	unsigned char		uint8;
typedef	signed char		int8;
typedef	unsigned short		uint16;
typedef	signed short		int16;
typedef unsigned int            uint32;
typedef	signed int 		int32;
typedef	unsigned long 		uint64;
typedef	signed long 		int64;

typedef unsigned int		UINT32;
typedef unsigned short		UINT16;
typedef unsigned char 		UINT8;
typedef long			INT32;
typedef short			INT16;
typedef char			INT8;

typedef unsigned char		INT8U;
typedef signed	char   		INT8S;
typedef unsigned short		INT16U;
typedef signed	short  		INT16S;
typedef int 	  		INT32S;
typedef unsigned int 	  	INT32U;

typedef unsigned char		BYTE;


typedef volatile unsigned int	REG32;
typedef volatile unsigned short	REG16;
typedef volatile unsigned char	REG8;

typedef volatile unsigned int	data_t;
typedef volatile unsigned int*	addr_t;

#define ReadReg32(addr)			(*(volatile uint32 *)(addr))
#define WriteReg32(addr, data)		(*(volatile uint32 *)(addr) = data)
#define MaskRegBits32(addr, y, z)	WriteReg32(addr, (ReadReg32(addr)&~(y))|(z))


/* void callback function type */
typedef void (* v_callback_f)(void);
typedef void (* pFunc)(void);	//定义函数指针, 用于调用绝对地址


#endif  /*_DRIVER_TYPEDEF_H */
