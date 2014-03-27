
#undef GET_BYTE
#undef PUT_BYTE
#undef GET_CHAR
#undef PUT_CHAR

#define GET_BYTE(pb,ib)    (pb)[(ib)]
#define PUT_BYTE(pb,ib,b)  (pb)[(ib)]=(b)
#define GET_CHAR(pch,ich)  (pch)[(ich)]
#define PUT_CHAR(pch,ich,ch)  (pch)[(ich)]=(ch)

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

unsigned short CRC_16(unsigned char * aData, unsigned long aSize);
unsigned long CRC_32(unsigned char * aData, unsigned long aSize) ;
extern unsigned long gIdDataBuf[512];
#if 0
DRM_VOID DRM_API NAND_RC4_KeySetup(
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
DRM_VOID DRM_API NAND_RC4_Cipher(
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
#define RK_IDBLOCK_RC4KEY_KEY   {124,78,3,4,85,5,9,7,45,44,123,56,23,13,23,17};
#define RK_IDBLOCK_RC4KEY_LEN   16

void GetIdblockDataNoRc4(char * fwbuf, int len)
{
    RC4_KEYSTRUCT       rc4KeyStruct;
    unsigned char       rc4Key[RK_IDBLOCK_RC4KEY_LEN] = RK_IDBLOCK_RC4KEY_KEY
    
    NAND_RC4_KeySetup(&rc4KeyStruct, RK_IDBLOCK_RC4KEY_LEN , rc4Key);
    NAND_RC4_Cipher(&rc4KeyStruct , len, (DRM_BYTE *)fwbuf);
}
#else
extern void P_RC4(unsigned char * buf, unsigned short len);

void GetIdblockDataNoRc4(char * fwbuf, int len)
{
    P_RC4(fwbuf, len);
}

#endif
#if 0
int GetIdBlockSysData(char * buf, int Sector)
{
    int ret = -1;
    if(Sector <= 3)
    {
        memcpy(buf,gIdDataBuf+128*Sector,512);
        if(Sector!=1)
            GetIdblockDataNoRc4(buf,512);
        ret = 0;
    }
    return ret;
}


char GetSNSectorInfo(char * pbuf)
{
    return (GetIdBlockSysData(pbuf,3));    
}

char GetChipSectorInfo(char * pbuf)
{
    return (GetIdBlockSysData(pbuf,2));    
}
#endif
