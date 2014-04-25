/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
typedef unsigned long           uint32;
typedef	signed long		int32;
typedef	unsigned long long	uint64;
typedef	signed long long	int64;

typedef unsigned long		UINT32;
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
typedef unsigned long   	INT32U;

typedef unsigned long		L32U;
typedef signed	long		L32S;

typedef unsigned char		BYTE;
typedef unsigned long		ULONG;


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

