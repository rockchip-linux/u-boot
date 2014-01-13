/********************************************************************************
*********************************************************************************
		COPYRIGHT (c)   2001-2012 BY ROCK-CHIP FUZHOU
				--  ALL RIGHTS RESERVED  --

File Name:	    paltform.h
Author:		    YIFENG ZHAO
Created:        2012-02-07
Modified:       ZYF
Revision:		1.00
********************************************************************************
********************************************************************************/
#ifndef _PALTFORM_H
#define  _PALTFORM_H


#if (CONFIG_RKCHIPTYPE == CONFIG_RK3026 || CONFIG_RKCHIPTYPE == CONFIG_RK2928)
#include "rk2928/Memap.h"
#include "rk2928/Reg.h"
#include "rk2928/chipDepend.h"
#else
#include "rk30/Memap.h"
#include "rk30/Reg.h"
#include "rk30/chipDepend.h"
#endif
#include "rk30/Isr.h"
#endif
