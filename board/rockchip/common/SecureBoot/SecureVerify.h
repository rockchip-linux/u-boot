/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _RK_SECUREVERIFY_H_
#define _RK_SECUREVERIFY_H_


bool SecureModeVerifyLoader(RK28BOOT_HEAD *hdr);
bool SecureModeVerifyUbootImage(second_loader_hdr *pHead);
bool SecureModeVerifyBootImage(rk_boot_img_hdr *pHead);
bool SecureModeBootImageCheck(rk_boot_img_hdr *hdr, int unlocked);
bool SecureModeRSAKeyCheck(uint8 *pKey);
void SecureModeLockLoader(void);
uint32 SecureModeInit(void);

#endif	/* _RK_SECUREVERIFY_H_ */
