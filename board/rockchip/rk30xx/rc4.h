/**@@@+++@@@@******************************************************************
**
** Microsoft Windows Media
** Copyright (C) Microsoft Corporation. All rights reserved.
**
$Log: rc4.h,v $
Revision 1.1  2010/12/06 02:43:53  Administrator
*** empty log message ***

Revision 1.1  2009/08/07 09:52:53  Administrator
*** empty log message ***

Revision 1.1  2009/03/16 07:37:15  Administrator
*** empty log message ***

Revision 1.1  2009/03/02 01:47:58  Administrator
*** empty log message ***

Revision 1.4  2007/12/24 07:56:09  Lingzhaojun
同步蓝魔版本基本模块

Revision 1.3  2007/10/11 04:08:07  Huangshilin
System\os

Revision 1.2  2007/10/08 02:57:03  Lingzhaojun
添加版本自动注释脚本


***@@@---@@@@******************************************************************
*/

#ifndef __DRM_RC4_H__
#define __DRM_RC4_H__
/******************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

#undef GET_BYTE
#undef PUT_BYTE
#undef GET_CHAR
#undef PUT_CHAR

#define GET_BYTE(pb,ib)    (pb)[(ib)]
#define PUT_BYTE(pb,ib,b)  (pb)[(ib)]=(b)
#define GET_CHAR(pch,ich)  (pch)[(ich)]
#define PUT_CHAR(pch,ich,ch)  (pch)[(ich)]=(ch)

//////////////////////////////////////////////////////////////
//重新定义类型
#define DRM_VOID  void
#define DRM_API
#define IN
#define OUT

#define DRM_BYTE  unsigned char
#define DRM_DWORD unsigned int
#define DRM_UINT  unsigned int
#define DRM_INT   int

//////////////////////////////////////////////////////////////
#define RC4_TABLESIZE 256

    /* Key structure */
    typedef struct __tagRC4_KEYSTRUCT
    {
        unsigned char S[(RC4_TABLESIZE)];		/* State table */
        unsigned char i, j;						/* Indices */
    } RC4_KEYSTRUCT;


    /***********************************************************************/

    /*********************************************************************
    **
    **  Function:  DRM_RC4_KeySetup
    **
    **  Synopsis:  Generate the key control structure.  Key can be any size.
    **
    **  Arguments:
    **     [pKS] -- A KEYSTRUCT structure that will be initialized.
    **     [cbKey] -- Size of the key, in bytes.
    **     [pbKey] -- Pointer to the key.
    **
    **  Returns:  None
    **
    *********************************************************************/

    DRM_VOID DRM_API DRM_RC4_KeySetup(
        OUT       RC4_KEYSTRUCT  *pKS,
        IN        DRM_DWORD       cbKey,
        IN  const DRM_BYTE       *pbKey);

    /*********************************************************************
    **
    **  Function:  DRM_RC4_Cipher
    **
    **  Synopsis:
    **
    **  Arguments:
    **     [pKS] -- Pointer to the KEYSTRUCT created using DRM_RC4_KeySetup.
    **     [cbBuffer] -- Size of buffer, in bytes.
    **     [pbBuffer] -- Buffer to be encrypted in place.
    **
    **  Returns:  None
    *********************************************************************/

    DRM_VOID DRM_API DRM_RC4_Cipher(
        IN OUT RC4_KEYSTRUCT *pKS,
        IN     DRM_UINT       cbBuffer,
        IN OUT DRM_BYTE      *pbBuffer);

    unsigned short CRC_16(unsigned char * aData, unsigned long aSize);
    unsigned long CRC_32(unsigned char * aData, unsigned long aSize) ;
    unsigned long CRC_32CheckBuffer(unsigned char * aData, unsigned long aSize);

#ifdef __cplusplus
}
#endif

#endif /* __DRM_RC4_H__ */
