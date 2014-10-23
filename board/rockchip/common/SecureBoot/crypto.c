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

#include "../config.h"
#include "SecureBoot.h"

#ifdef SECUREBOOT_CRYPTO_EN

// CRIPTO registers
typedef volatile struct tagCRYPTO_STRUCT {
	uint32 CRYPTO_INTSTS;
	uint32 CRYPTO_INTENA;
	uint32 CRYPTO_CTRL;
	uint32 CRYPTO_CONF;
	uint32 CRYPTO_BRDMAS;
	uint32 CRYPTO_BTDMAS;
	uint32 CRYPTO_BRDMAL;
	uint32 CRYPTO_HRDMAS;
	uint32 CRYPTO_HRDMAL;   // in word
	uint32 reserved0[(0x80-0x24)/4];

	uint32 CRYPTO_AES_CTRL;
	uint32 CRYPTO_AES_STS;
	uint32 CRYPTO_AES_DIN[4];
	uint32 CRYPTO_AES_DOUT[4];
	uint32 CRYPTO_AES_IV[4];
	uint32 CRYPTO_AES_KEY[8];
	uint32 CRYPTO_AES_CNT[4];
	uint32 reserved1[(0x100-0xe8)/4];

	uint32 CRYPTO_TDES_CTRL;
	uint32 CRYPTO_TDES_STS;
	uint32 CRYPTO_TDES_DIN[2];
	uint32 CRYPTO_TDES_DOUT[2];
	uint32 CRYPTO_TDES_IV[2];
	uint32 CRYPTO_TDES_KEY1[2];
	uint32 CRYPTO_TDES_KEY2[2];
	uint32 CRYPTO_TDES_KEY3[2];
	uint32 reserved2[(0x180-0x138)/4];

	uint32 CRYPTO_HASH_CTRL;
	uint32 CRYPTO_HASH_STS;
	uint32 CRYPTO_HASH_MSG_LEN; // in byte
	uint32 CRYPTO_HASH_DOUT[8];
	uint32 CRYPTO_HASH_SEED[5];
	uint32 reserved3[(0x200-0x1c0)/4];

	uint32 CRYPTO_TRNG_CTRL;
	uint32 CRYPTO_TRNG_DOUT[8];
	uint32 reserved4[(0x280-0x224)/4];

	uint32 CRYPTO_PKA_CTRL;
	uint32 reserved5[(0x400-0x284)/4];

	uint32 CRYPTO_PKA_M;
	uint32 reserved6[(0x500-0x404)/4];

	uint32 CRYPTO_PKA_C;
	uint32 reserved7[(0x600-0x504)/4];

	uint32 CRYPTO_PKA_N;
	uint32 reserved8[(0x700-0x604)/4];

	uint32 CRYPTO_PKA_E;
} CRYPTO_REG, *pCRYPTO_REG;

#define CryptoReg	((pCRYPTO_REG)CRYPTO_BASE_ADDR)


int32 CryptoSHAInit(uint32 MsgLen, int hash_bits)
{
	CryptoReg->CRYPTO_HASH_MSG_LEN = MsgLen;
	if(hash_bits == 256)
	{
		CryptoReg->CRYPTO_HASH_CTRL = 0x0a; // sha256 & byte swap
		CryptoReg->CRYPTO_CONF &= ~(1<<5);      
	}
	else
	{
		CryptoReg->CRYPTO_CONF |= (1<<5); // sha160 input byte swap
		CryptoReg->CRYPTO_HASH_CTRL = 0x08; // sha160 & byte swap
	}
	CryptoReg->CRYPTO_CTRL = (1<<6)|((1<<6)<<16);
	while (CryptoReg->CRYPTO_CTRL & (1<<6));

	return 0;
}


int32 CryptoSHAStart(uint32 *data, uint32 DataLen)
{
	// if data len = 0, return, fixed crypto handup
	if (DataLen == 0) {
		return 0;
	}

	while (CryptoReg->CRYPTO_CTRL & 0x08); // wait last complete

	CryptoReg->CRYPTO_INTSTS = (0x1<<4); // Hash Done Interrupt

	CryptoReg->CRYPTO_HRDMAS = (uint32)data;

	CryptoReg->CRYPTO_HRDMAL = ((DataLen+3)>>2);

	CryptoReg->CRYPTO_CTRL = (0x8<<16 | 0x8); //write 1 to start. When finishes, the core will clear it

	return 0;
}


int32 CryptoSHAEnd(uint32 *result)
{
	int32 i;

	while (CryptoReg->CRYPTO_CTRL & 0x08);	// wait last complete

	while(!CryptoReg->CRYPTO_HASH_STS); //When HASH finishes, it will be HIGH, And it will not be LOW until it restart 

	// read back 256bit output data
	for(i = 0; i < 8; i ++)
		*result++ = CryptoReg->CRYPTO_HASH_DOUT[i];

	return 0;
}


int32 CryptoSHACheck(uint32 *InHash)
{
	uint32 dataHash[8];

	// Hash the DATA
	CryptoSHAEnd(dataHash);
	// cpmpare the result with hash of data
	return memcmp(InHash, dataHash, 32);
}


static inline int32 CryptoRSAConfig(void)
{
	CryptoReg->CRYPTO_PKA_CTRL = 2;     // 2048bit It specifies the bits of N in PKA calculation

	CryptoReg->CRYPTO_CTRL = (0xc0<<16)| 0xc0;   // flush SHA & RSA
	CryptoReg->CRYPTO_INTSTS = 0xffffffff;

	while (CryptoReg->CRYPTO_CTRL & 0x90); 

	return 0;
}


int32 CryptoRSAStart(uint32 *AddrM, uint32 *AddrN, uint32 *AddrE, uint32 *AddrC)
{
	CryptoReg->CRYPTO_INTSTS = (0x1<<5);  //clean PKA Done Interrupt

	memcpy(&CryptoReg->CRYPTO_PKA_M, AddrM, 256);
	memcpy(&CryptoReg->CRYPTO_PKA_N, AddrN, 256);
	memcpy(&CryptoReg->CRYPTO_PKA_E, AddrE, 256);
	memcpy(&CryptoReg->CRYPTO_PKA_C, AddrC, 256);

	while(CryptoReg->CRYPTO_CTRL & 0x10);

	CryptoReg->CRYPTO_CTRL = (0x10<<16) | 0x10;     //Starts/initializes PKA

	return 0;
}


int32 CryptoRSAEnd(uint32 *result)
{
	int32 i;

	while(CryptoReg->CRYPTO_CTRL & 0x10); //wait PKA Done

	// read back 256bit output data
	for(i = 0; i < 8; i ++)
		*result++ = *((uint32 *)(&CryptoReg->CRYPTO_PKA_M + i));

	return 0;
}


int32 CryptoRSAInit(uint32 *AddrM, uint32 *AddrN, uint32 *AddrE, uint32 *AddrC)
{
	CryptoRSAConfig();

	return CryptoRSAStart(AddrM, AddrN, AddrE, AddrC);
}


int32 CryptoRSACheck(void)
{
	uint32 dataHash[8];
	uint32 rsaResult[8];

	// Hash the DATA
	CryptoSHAEnd(dataHash);
	CryptoRSAEnd(rsaResult);
	// cpmpare the result with hash of data
	return memcmp(rsaResult, dataHash, 32);
}


int32 CryptoRSAVerify(BOOT_HEADER *pHead, uint32 SigOffset)
{
	int32 ret;
	uint32 rsaResult[8];

	CryptoRSAInit((uint32*)((uint32)pHead + SigOffset), pHead->RSA_N, pHead->RSA_E, pHead->RSA_C);
	CryptoSHAInit(SigOffset, 256);
	CryptoSHAStart((uint32 *)pHead, SigOffset);

	ret = CryptoRSACheck();
	if (0 != ret)
		return ERROR;

	return OK;
}

/* define for check crypto hardware ok */
#undef CRYPTO_HW_CHECK

#ifdef CRYPTO_HW_CHECK
static void CryptoHashPrint(char *hash)
{
	int k = 0;

	for (k = 0; k < 20; k++) {
		printf("%02x", hash[k]);
	}
	printf("\n");
}


#define HASH_TEST_SIZE	512
static uint8 crypto_data_test[HASH_TEST_SIZE];
static uint32 crypto_size_test = 0x55aa;

static void CryptoHWCheckOK(void)
{
	char *dataHash;
	uint32 size;
	int i;

	for (i = 0; i < HASH_TEST_SIZE; i++) {
		crypto_data_test[i] = i;
	}

	SHA_CTX ctx;
	SHA_init(&ctx);
	SHA_update(&ctx, (uint32 *)crypto_data_test, HASH_TEST_SIZE);
	SHA_update(&ctx, (uint32 *)&crypto_size_test, sizeof(crypto_size_test));
	dataHash = (uint8_t *)SHA_final(&ctx);

	printf("Soft hash data:\n");
	CryptoHashPrint(dataHash);

	uint32 hwDataHash[8];
	size = HASH_TEST_SIZE + sizeof(crypto_size_test);
	CryptoSHAInit(size, 160);
	/* rockchip's second level image. */
	CryptoSHAStart((uint32 *)crypto_data_test, HASH_TEST_SIZE);
	CryptoSHAStart((uint32 *)&crypto_size_test, sizeof(crypto_size_test));
	CryptoSHAEnd(hwDataHash);
	dataHash = hwDataHash;

	printf("Crypto hash data:\n");
	CryptoHashPrint(dataHash);
}
#endif /* CRYPTO_HW_CHECK */


#define CRYPTO_MAX_FREQ		(100 * MHZ)
void CryptoInit(void)
{
	rkclk_set_crypto_clk(CRYPTO_MAX_FREQ);
#ifdef CRYPTO_HW_CHECK
	CryptoHWCheckOK();
#endif
}

#endif /* SECUREBOOT_CRYPTO_EN */
