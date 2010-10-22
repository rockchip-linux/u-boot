/********************************************************************************
*********************************************************************************
			COPYRIGHT (c)   2004 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:  typedef.h
Author:     RK28XX Driver Develop Group
Created:    25th OCT 2008
Modified:
Revision:   1.00
********************************************************************************
********************************************************************************/
#ifndef     _DRIVER_TYPEDEF_H
#define     _DRIVER_TYPEDEF_H

typedef     unsigned char           uint8;
typedef     signed char             int8;
typedef     unsigned short int      uint16;
typedef     signed short int        int16;
typedef     unsigned int            uint32;
typedef     signed int              int32;
typedef     unsigned long long      uint64;
typedef     signed long long        int64;
typedef     unsigned char           bool;
typedef     void (*pFunc)(void);	        //定义函数指针, 用于调用绝对地址

#ifndef 	TRUE
#define 	TRUE    1
#endif

#ifndef	 	FALSE
#define 	FALSE   0
#endif

#ifndef 	NULL
#define NULL		0
#endif

#ifndef 	OK
#define     OK                  0
#endif

#ifndef 	ERROR
#define     ERROR               !0
#endif

#endif

