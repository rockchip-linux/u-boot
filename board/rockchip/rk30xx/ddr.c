/******************************************************************/
/*  Copyright (C)  ROCK-CHIPS FUZHOU . All Rights Reserved.       */
/*******************************************************************
File    :   ddr.c
Desc    :   dram main function
Author  :   hcy
Date    :   2014-03-18
Notes   :
$Log: ddr.c,v $
********************************************************************/
#include "../common/armlinux/config.h"

//获取容量，返回字节数
uint32 ddr_get_cap(void)
{
    uint32 reg=*(volatile uint32*)(PMU_BASE_ADDR+0x9c);
    uint32 cap[2]={0,0};

    if((reg>>28)&1)
    {
        cap[0] = (1 << ((13+((reg>>6)&0x3))+(9+((reg>>9)&0x3))+(3-((reg>>8)&0x1))+(2>>((reg>>2)&0x3))));
        if((1+((reg>>11)&0x1)) > 1)
        {
            cap[0] += cap[0] >> (((reg>>6)&0x3)-((reg>>4)&0x3));
        }
        if(((reg>>30)&0x1))
        {
            cap[0] = cap[0]*3/4;
        }
    }
    if((reg>>29)&1)
    {
        cap[1] = (1 << ((13+((reg>>22)&0x3))+(9+((reg>>25)&0x3))+(3-((reg>>24)&0x1))+(2>>((reg>>18)&0x3))));
        if((1+((reg>>27)&0x1)) > 1)
        {
            cap[1] += cap[1] >> (((reg>>22)&0x3)-((reg>>20)&0x3));
        }
        if(((reg>>31)&0x1))
        {
            cap[1] = cap[1]*3/4;
        }
    }

    return (cap[0]+cap[1]);
}

