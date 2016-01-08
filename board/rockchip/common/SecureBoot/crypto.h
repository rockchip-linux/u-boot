/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _CRYPTO_H
#define _CRYPTO_H

#include "SecureBoot.h"

extern int32 CryptoSHAInit(uint32 MsgLen, int hash_bits);
extern void CryptoSHAInputByteSwap(int en);
extern int32 CryptoSHAStart(uint32 *data, uint32 DataLen);
extern int32 CryptoSHAEnd(uint32 *result);
extern int32 CryptoSHACheck(uint32 *InHash);
extern int32 CryptoRSAEnd(uint32 *result);
extern int32 CryptoRSAInit(uint32 *AddrM, uint32 *AddrN, uint32 *AddrE, uint32 *AddrC);
extern int32 CryptoRSAStart(uint32 *AddrM, uint32 *AddrN, uint32 *AddrE, uint32 *AddrC);
extern int32 CryptoRSACheck(void);
extern int32 CryptoRSAVerify(BOOT_HEADER *pHead, uint32 SigOffset);
extern void CryptoInit(void);

#endif /* _CRYPTO_H */
