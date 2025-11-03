// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 */

#ifndef _ROCKCHIP_CRYPTO_ECC_H_
#define _ROCKCHIP_CRYPTO_ECC_H_

#include <asm/io.h>
#include <common.h>
#include <rockchip/crypto_v2.h>
#include <rockchip/crypto_v2_util.h>

#define SM2_RAM_BASE					((crypto_base) + 0x1000)

#define	_SBF(s,	v)			((v) <<	(s))
#define	_BIT(b)				_SBF(b,	1)

#define RK_ECP_MAX_BITS			256
#define RK_ECP_MAX_BYTES		((RK_ECP_MAX_BITS) / 8)
#define RK_ECP_MAX_WORDS		((RK_ECP_MAX_BITS) / 32)
#define RK_ECP_MAX_WORDS_ALL		(512 / 32)

#define RK_ECC_CTL					0x03F0
#define RK_ECC_CTL_RAND_K_SRC_SOFT			_SBF(12, 0)
#define RK_ECC_CTL_RAND_K_SRC_HARD			_SBF(12, 1)
#define RK_ECC_CTL_FUNC_SM2_CURVER			_SBF(8, 0x0)
#define RK_ECC_CTL_FUNC_ECC_CURVER			_SBF(8, 0x1)

#define RK_ECC_CTL_FUNC_SEL_MUL				_SBF(4, 0x0)
#define RK_ECC_CTL_FUNC_SEL_KG				_SBF(4, 0x1)
#define RK_ECC_CTL_FUNC_SEL_SIGN			_SBF(4, 0x2)
#define RK_ECC_CTL_FUNC_SEL_VERIFY			_SBF(4, 0x3)
#define RK_ECC_CTL_FUNC_SEL_MUL_MOD			_SBF(4, 0x4)
#define RK_ECC_CTL_FUNC_SEL_ADD_MOD			_SBF(4, 0x5)
#define RK_ECC_CTL_FUNC_SEL_DOUBLE_POINT		_SBF(4, 0x6)
#define RK_ECC_CTL_FUNC_SEL_ADD_POINT			_SBF(4, 0x7)
#define RK_ECC_CTL_FUNC_SEL_KP				_SBF(4, 0x8)
#define RK_ECC_CTL_FUNC_SEL_KP_KG			_SBF(4, 0xa)
#define RK_ECC_CTL_REQ_ECC				_SBF(0, 1)

#define RK_ECC_INT_EN					0x03F4
#define RK_ECC_INT_EN_DONE				_BIT(0)

#define RK_ECC_INT_ST					0x03F8
#define RK_ECC_INT_ST_DONE				_BIT(0)

#define RK_ECC_ABN_ST					0x03FC
#define RK_ECC_ABN_ST_BAD_INV_OUT			_BIT(8)
#define RK_ECC_ABN_ST_BAD_K_IN				_BIT(7)
#define RK_ECC_ABN_ST_BAD_R_IN				_BIT(6)
#define RK_ECC_ABN_ST_BAD_S_IN				_BIT(5)
#define RK_ECC_ABN_ST_BAD_R_K_MID			_BIT(4)
#define RK_ECC_ABN_ST_BAD_R_OUT				_BIT(3)
#define RK_ECC_ABN_ST_BAD_S_OUT				_BIT(2)
#define RK_ECC_ABN_ST_BAD_T_OUT				_BIT(1)
#define RK_ECC_ABN_ST_BAD_POINT_OUT			_BIT(0)

#define RK_ECC_CURVE_WIDE				0x0400
#define RK_ECC_CURVE_WIDE_192				192
#define RK_ECC_CURVE_WIDE_224				224
#define RK_ECC_CURVE_WIDE_256				256

#define RK_ECC_MAX_CURVE_WIDE				0x0404

#define RK_ECC_DATA_ENDIAN				0x0408
#define RK_ECC_DATA_ENDIAN_LITTLE			0x0
#define RK_ECC_DATA_ENDIAN_BIG				0x1

#define RK_ECC_RAM_CTL					0x0480
#define RK_ECC_RAM_CTL_SEL_MASK				_SBF(16, 3)
#define RK_ECC_RAM_CTL_CPU				_SBF(0, 0)
#define RK_ECC_RAM_CTL_PKA				_SBF(0, 1)
#define RK_ECC_RAM_CTL_ECC				_SBF(0, 2)

#define BYTE2WORD(bytes)	((bytes) / 4)
#define BITS2BYTE(nbits)	((nbits) / 8)
#define BITS2WORD(nbits)	((nbits) / 32)
#define WORD2BYTE(words)	((words) * 4)

struct rk_ecp_point {
	struct mpa_num *x;     /*!<  the point's X coordinate  */
	struct mpa_num *y;     /*!<  the point's Y coordinate  */
};

enum rk_ecp_group_id {
	RK_ECP_DP_NONE = 0,
	RK_ECP_DP_SECP192R1,      /*!< 192-bits NIST curve  */
	RK_ECP_DP_SECP224R1,      /*!< 224-bits NIST curve  */
	RK_ECP_DP_SECP256R1,      /*!< 256-bits NIST curve  */
	RK_ECP_DP_SM2P256V1,      /*!< */
};

struct rk_ecp_group {
	enum rk_ecp_group_id	id;         /*!<  internal group identifier                     */
	const char		*curve_name;
	uint32_t		wide;
	const uint8_t		*p;         /*!<  prime modulus of the base field               */
	const uint8_t		*a;         /*!<  1. A in the equation, or 2. (A + 2) / 4       */
	const uint8_t		*n;
	const uint8_t		*gx;
	const uint8_t		*gy;
	size_t			p_len;
	size_t			a_len;
	size_t			n_len;
	size_t			gx_len;
	size_t			gy_len;
};

struct rk_ecc_verify {
	uint32_t e[RK_ECP_MAX_WORDS_ALL];		// 0x00
	uint32_t r_[RK_ECP_MAX_WORDS_ALL];		// 0x40
	uint32_t s_[RK_ECP_MAX_WORDS_ALL];		// 0x80
	uint32_t p_x[RK_ECP_MAX_WORDS_ALL];		// 0xC0
	uint32_t p_y[RK_ECP_MAX_WORDS_ALL];		// 0x100
	uint32_t A[RK_ECP_MAX_WORDS_ALL];		// 0x140
	uint32_t P[RK_ECP_MAX_WORDS_ALL];		// 0x180
	uint32_t N[RK_ECP_MAX_WORDS_ALL];		// 0x1C0
	uint32_t G_x[RK_ECP_MAX_WORDS_ALL];		// 0x200
	uint32_t G_y[RK_ECP_MAX_WORDS_ALL];		// 0x240
	uint32_t r[RK_ECP_MAX_WORDS_ALL];		// 0x280
	uint32_t v[RK_ECP_MAX_WORDS_ALL];		// 0x2C0
};

bool rk_is_ec_supported(void);

int rockchip_ecc_verify(const char *curve_name, uint8_t *hash, uint32_t hash_len,
			struct rk_ecp_point *point_P, struct rk_ecp_point *point_sign);
#endif
