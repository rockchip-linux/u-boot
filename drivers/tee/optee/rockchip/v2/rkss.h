/*
 * Copyright 2023, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef TEE_SUPP_RK_FS_H
#define TEE_SUPP_RK_FS_H

#include <tee.h>
#include "../../optee_msg.h"

/*
 * Operations and defines shared with TEE.
 */
#define OPTEE_MRF_OPEN			0
#define OPTEE_MRF_CREATE		1
#define OPTEE_MRF_CLOSE			2
#define OPTEE_MRF_READ			3
#define OPTEE_MRF_WRITE			4
#define OPTEE_MRF_TRUNCATE		5
#define OPTEE_MRF_REMOVE		6
#define OPTEE_MRF_RENAME		7
#define OPTEE_MRF_OPENDIR		8
#define OPTEE_MRF_CLOSEDIR		9
#define OPTEE_MRF_READDIR		10

/*
 * Open flags, defines shared with TEE.
 */
#define TEE_FS_O_RDONLY			0x1
#define TEE_FS_O_WRONLY			0x2
#define TEE_FS_O_RDWR			0x4
#define TEE_FS_O_CREAT			0x8
#define TEE_FS_O_EXCL			0x10
#define TEE_FS_O_APPEND			0x20

/*
 * Seek flags, defines shared with TEE.
 */
#define TEE_FS_SEEK_SET			0x1
#define TEE_FS_SEEK_END			0x2
#define TEE_FS_SEEK_CUR			0x4

/*
 * Mkdir flags, defines shared with TEE.
 */
#define TEE_FS_S_IWUSR			0x1
#define TEE_FS_S_IRUSR			0x2

/*
 * Access flags, X_OK not supported, defines shared with TEE.
 */
#define TEE_FS_R_OK			0x1
#define TEE_FS_W_OK			0x2
#define TEE_FS_F_OK			0x4

#define RK_FS_R				0x1
#define RK_FS_W				0x2
#define RK_FS_D				0x8

/* Function Defines */
#define UNREFERENCED_PARAMETER(P) (P = P)
#define CHECKFLAG(flags, flag) (flags & flag)
#define ADDFLAG(flags, flag) (flags | flag)

#define RKSS_VERSION_V1			1
#define RKSS_VERSION_V2			2
#define RKSS_VERSION_ERR		100

int tee_supp_rk_fs_init_v1(void);

int tee_supp_rk_fs_process_v1(size_t num_params,
			      struct optee_msg_param *params);

int tee_supp_rk_fs_init_v2(void);

int tee_supp_rk_fs_process_v2(size_t num_params,
			      struct optee_msg_param *params);

#endif
