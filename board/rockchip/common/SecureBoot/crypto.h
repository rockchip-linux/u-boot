/*
 * (C) Copyright 2008-2014 Rockchip Electronics
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _CRYPTO_H
#define _CRYPTO_H

#include "SecureBoot.h"

extern int32 CryptoSHAInit(uint32 MsgLen, int hash_bits);
extern int32 CryptoSHAStart(uint32 *data, uint32 DataLen);
extern int32 CryptoSHAEnd(uint32 *result);
extern int32 CryptoSHACheck(uint32 *InHash);
extern int32 CryptoRSAInit(uint32 *AddrM, uint32 *AddrN, uint32 *AddrE, uint32 *AddrC);
extern int32 CryptoRSAStart(uint32 *AddrM, uint32 *AddrN, uint32 *AddrE, uint32 *AddrC);
extern int32 CryptoRSACheck(void);
extern int32 CryptoRSAVerify(BOOT_HEADER *pHead, uint32 SigOffset);
extern void CryptoInit(void);

#endif /* _CRYPTO_H */
