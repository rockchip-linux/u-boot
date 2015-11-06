/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "../config.h"

//#pragma arm section code = "LOADER2"

void P_RC4(unsigned char * buf, unsigned short len)
{
	unsigned char S[256],K[256],temp;
	unsigned short i,j,t,x;
	unsigned char key[16]={124,78,3,4,85,5,9,7,45,44,123,56,23,13,23,17};

	j = 0;
	for(i=0; i<256; i++){
		S[i] = (unsigned char)i;
		j&=0x0f;
		K[i] = key[j];
		j++;
	}
	
	j = 0;
	for(i=0; i<256; i++){
		j = (j + S[i] + K[i]) % 256;
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;
	}
	
	i = j = 0;
	for(x=0; x<len; x++){
		i = (i+1) % 256;
		j = (j + S[i]) % 256;
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;
		t = (S[i] + (S[j] % 256)) % 256;
		buf[x] = buf[x] ^ S[t];
	}
}

//#pragma arm section code

void P_RC4_ext(unsigned char * buf, unsigned short len)
{
	unsigned char S[256],K[256],temp;
	unsigned short i,j,t,x;
	unsigned char key[16];//={124,78,3,4,85,5,9,7,45,44,123,56,23,13,23,17};
	unsigned int value = *(unsigned int*)(buf+len);

//	RkPrintf("value=%X\n", value);

	for(i=0; i<16; i++)
	{
		key[i] = 0xFF & (value >> (i*2));
	}

	j = 0;
	for(i=0; i<256; i++){
		S[i] = (unsigned char)i;
		j&=0x0f;
		K[i] = key[j];
		j++;
	}
	
	j = 0;
	for(i=0; i<256; i++){
		j = (j + S[i] + K[i]) % 256;
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;
	}
	
	i = j = 0;
	for(x=0; x<len; x++){
		i = (i+1) % 256;
		j = (j + S[i]) % 256;
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;
		t = (S[i] + (S[j] % 256)) % 256;
		buf[x] = buf[x] ^ S[t];
	}
}


