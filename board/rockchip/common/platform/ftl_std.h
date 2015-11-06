/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _FTL_STD_H_
#define _FTL_STD_H_


int ftl_memcmp(void *str1, void *str2, unsigned int count);
void* ftl_memcpy(void* pvTo, const void* pvForm, unsigned int size);
int ftl_memcmp(void *str1, void *str2, unsigned int count);
void * ftl_memset(void *s, int c, unsigned int n);

#endif	/* _FTL_STD_H_ */

