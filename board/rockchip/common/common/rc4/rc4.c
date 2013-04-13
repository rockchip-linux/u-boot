/**@@@+++@@@@******************************************************************
**
** Microsoft Windows Media
** Copyright (C) Microsoft Corporation. All rights reserved.
**
$Log: rc4.c,v $
Revision 1.1  2010/12/06 02:43:53  Administrator
*** empty log message ***

Revision 1.1.2.1  2010/05/19 09:37:18  Administrator
*** empty log message ***

Revision 1.1  2009/08/07 09:52:53  Administrator
*** empty log message ***

Revision 1.1  2009/03/16 07:37:15  Administrator
*** empty log message ***

Revision 1.1  2009/03/02 01:47:57  Administrator
*** empty log message ***

Revision 1.2  2008/06/19 04:43:24  Administrator
代码整理！

Revision 1.1.1.1  2008/05/07 04:15:08  Administrator
no message

Revision 1.1.1.1  2008/03/06 13:29:10  Lingzhaojun
no message

Revision 1.3  2007/10/11 04:08:07  Huangshilin
System\os

Revision 1.2  2007/10/08 02:57:02  Lingzhaojun
添加版本自动注释脚本


***@@@---@@@@******************************************************************
*/

#include "config.h"

#include "rc4.h"
/******************************************************************************/

DRM_VOID DRM_API DRM_RC4_KeySetup(
    OUT   RC4_KEYSTRUCT  *pKS,
    IN        DRM_DWORD       cbKey,
    IN  const DRM_BYTE       *pbKey)
{
    DRM_BYTE j;
    DRM_BYTE k;
    DRM_BYTE t;
    DRM_INT  i;

    for (i = 0;i < RC4_TABLESIZE;i++)
    {
        PUT_BYTE(pKS->S, i, (DRM_BYTE)i);
    }

    pKS->i = 0;
    pKS->j = 0;
    j      = 0;
    k      = 0;
    for (i = 0;i < RC4_TABLESIZE;i++)
    {
        t = GET_BYTE(pKS->S, i);
        j = (DRM_BYTE)((j + t + GET_BYTE(pbKey, k)) % RC4_TABLESIZE);
        PUT_BYTE(pKS->S, i, GET_BYTE(pKS->S, j));
        PUT_BYTE(pKS->S, j, t);
        k = (DRM_BYTE)((k + 1) % cbKey);
    }
}

/******************************************************************************/
DRM_VOID DRM_API DRM_RC4_Cipher(
    IN OUT RC4_KEYSTRUCT *pKS,
    IN     DRM_UINT       cbBuffer,
    IN OUT DRM_BYTE      *pbBuffer)
{
    DRM_BYTE  i = pKS->i;
    DRM_BYTE  j = pKS->j;
    DRM_BYTE *p = pKS->S;
    DRM_DWORD ib = 0;

    while (cbBuffer--)
    {
        DRM_BYTE bTemp1 = 0;
        DRM_BYTE bTemp2 = 0;

        i = ((i + 1) & (RC4_TABLESIZE - 1));
        bTemp1 = GET_BYTE(p, i);
        j = ((j + bTemp1) & (RC4_TABLESIZE - 1));

        PUT_BYTE(p, i, GET_BYTE(p, j));
        PUT_BYTE(p, j, bTemp1);
        bTemp2 = GET_BYTE(pbBuffer, ib);

        bTemp2 ^= GET_BYTE(p, (GET_BYTE(p, i) + bTemp1) & (RC4_TABLESIZE - 1));
        PUT_BYTE(pbBuffer, ib, bTemp2);
        ib++;
    }

    pKS->i = i;
    pKS->j = j;
}

/*****************************************************************************************/
#if(PALTFORM==RK28XX)
#if(CHIP_TPYE == RK28XX)
/* RK28XX、RK2728 SDK RC4 KEY */
#define RK_CHECK_RC4KEY_KEY       { 0x67,0x86,0xea,0x5d,0xc3,0x25,0x87,0x73,0x4b,0xf2 };
#elif(CHIP_TPYE == RK2729_TFT)
/* RK2729 TFT SDK RC4 KEY */
#define RK_CHECK_RC4KEY_KEY       { 0x76,0x68,0xae,0xd5,0x3c,0x25,0x87,0x73,0x4b,0xf2 };
#elif(CHIP_TPYE == RK2729_EINK)
/* RK2729 E-LIK SDK RC4 KEY */
#define RK_CHECK_RC4KEY_KEY       { 0x67,0x86,0xea,0x5d,0xc3,0x52,0x78,0x37,0xb4,0x2f };
#else
#error "CHIP TYPE ERROR"
#endif 
#else
#define RK_CHECK_RC4KEY_KEY       { 0x67,0x86,0xea,0x5d,0xc3,0x25,0x87,0x73,0x4b,0xf2 };
#endif

#define RK_CHECK_RC4KEY_LEN 10

void GetSrcCheckFw(char * fwbuf, int len)
{
    RC4_KEYSTRUCT       rc4KeyStruct;
    unsigned char       rc4Key[RK_CHECK_RC4KEY_LEN] = RK_CHECK_RC4KEY_KEY
    
    DRM_RC4_KeySetup(&rc4KeyStruct, RK_CHECK_RC4KEY_LEN , rc4Key);
    DRM_RC4_Cipher(&rc4KeyStruct , len , (DRM_BYTE *)fwbuf);
}

#if(PALTFORM==RK28XX)
#if(CHIP_TPYE == RK28XX)
#define RK_MFRSN_RC4KEY_KEY   {0x23,0x78,0x3E,0xF4,0x85,0x05,0xE9,0x07,0x45,0x44,0x69,0x56,0x34,0x13,0x83,0x17};
#else
#define RK_MFRSN_RC4KEY_KEY   {0x32,0x87,0xE3,0x4F,0x58,0x50,0x9E,0x70,0x45,0x44,0x69,0x56,0x34,0x13,0x83,0x17};
#endif

#define RK_MFRSN_RC4KEY_LEN   16

void GetMfrSnInfo(char * fwbuf, int len)
{
    RC4_KEYSTRUCT       rc4KeyStruct;
    unsigned char       rc4Key[RK_MFRSN_RC4KEY_LEN] = RK_MFRSN_RC4KEY_KEY
    
    DRM_RC4_KeySetup(&rc4KeyStruct, RK_MFRSN_RC4KEY_LEN , rc4Key);
    DRM_RC4_Cipher(&rc4KeyStruct , len, (DRM_BYTE *)fwbuf);
}
#endif

