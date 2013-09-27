/*********************************************************************************
*     Copyright (C),2004-2005,  Fuzhou Rockchip Co.,Ltd.
*         All Rights Reserved
*          V1.00
* FileName :  Hw_include.h
* Author :  lzy
* Description:
* History  :
*   <author>  <time>    <version>    <desc>
*    lzy        07/9/10        1.0     ORG
$Log: hw_include.h,v $
Revision 1.1  2011/01/14 10:03:10  Administrator
*** empty log message ***

Revision 1.1  2011/01/07 03:28:04  Administrator
*** empty log message ***

Revision 1.6  2007/11/20 14:13:11  Nizhenyu
LCDC 驱动包含头文件整理

Revision 1.5  2007/11/10 04:23:57  Huangxinyu
调试修改

Revision 1.4  2007/10/15 09:04:01  Huangxinyu
根据RK27提交修改driver

Revision 1.3  2007/10/08 02:38:41  Lingzhaojun
添加版本自动注释脚本


*********************************************************************************/
#ifndef _HW_INCULDE_H
#define _HW_INCULDE_H
#define USB_VBUS_INTR


#include "Typedef.h"
#include "debug.h"
#include "hw_define.h"
#include "hw_memmap.h"
//#include "hw_common.h"

#include "drivers_define.h"
#include "api_drivers.h"

extern uint32    DummyWriteReg  ;            /* In case that both code&data are cacheable,write a word before
                                                             read any uncacheable io reg.*/


#endif    //_HW_INCULDE_H
