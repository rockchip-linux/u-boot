/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * Configuation settings for the rk3xxx chip platform.
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


