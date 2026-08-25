// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <linux/errno.h>
#include <rockchip/crypto_mpa.h>
#include <rockchip/crypto_v2.h>
#include <rockchip/crypto_ecc.h>

#define WORDS2BYTES(words)	((words) * 4)

#define RK_ECP_IS_BIGNUM_INVALID(b) (!b || !b->d || b->size > RK_ECP_MAX_WORDS)
#define RK_ECP_IS_POINT_INVALID(p) (RK_ECP_IS_BIGNUM_INVALID(p->x) || \
				    RK_ECP_IS_BIGNUM_INVALID(p->y))

/*************************************************************/
/* Macros for waiting EC machine ready states               */
/*************************************************************/
#define RK_ECP_WRITE_REG(offset, val)		crypto_write((val), (offset))
#define RK_ECP_READ_REG(offset)			crypto_read((offset))

static __inline void rk_ecp_ram_for_ecc(void)
{
	if ((RK_ECP_READ_REG(RK_ECC_RAM_CTL) & 0x3) == RK_ECC_RAM_CTL_ECC)
		return;

	if (RK_ECP_READ_REG(RK_ECC_RAM_ST) & RK_ECC_RAM_ST_RDY)
		RK_ECP_WRITE_REG(RK_ECC_RAM_ST, RK_ECC_RAM_ST_RDY);

	RK_ECP_WRITE_REG(RK_ECC_RAM_CTL, RK_ECC_RAM_CTL_SEL_MASK | RK_ECC_RAM_CTL_ECC);

	while ((RK_ECP_READ_REG(RK_ECC_RAM_ST) & RK_ECC_RAM_ST_RDY) != RK_ECC_RAM_ST_RDY)
		;

	RK_ECP_WRITE_REG(RK_ECC_RAM_ST, RK_ECC_RAM_ST_RDY);
}

static __inline void rk_ecp_ram_for_cpu(void)
{
	if ((RK_ECP_READ_REG(RK_ECC_RAM_CTL) & 0x3) == RK_ECC_RAM_CTL_CPU)
		return;

	if (RK_ECP_READ_REG(RK_ECC_RAM_ST) & RK_ECC_RAM_ST_RDY)
		RK_ECP_WRITE_REG(RK_ECC_RAM_ST, RK_ECC_RAM_ST_RDY);

	RK_ECP_WRITE_REG(RK_ECC_RAM_CTL, RK_ECC_RAM_CTL_SEL_MASK | RK_ECC_RAM_CTL_CPU);

	while ((RK_ECP_READ_REG(RK_ECC_RAM_ST) & RK_ECC_RAM_ST_RDY) != RK_ECC_RAM_ST_RDY)
		;

	RK_ECP_WRITE_REG(RK_ECC_RAM_ST, RK_ECC_RAM_ST_RDY);
}

/* big endian to little endian */
#define RK_ECP_LOAD_DATA(dst, big_src) rk_ecp_load_data(dst, big_src)

/* little endian to littel endian */
#define RK_ECP_LOAD_DATA_EXT(dst, src, n_bytes) \
		do { \
			util_word_memset((void *)(dst), 0, RK_ECP_MAX_WORDS);\
			util_word_memcpy((void *)(dst), (void *)(src), (n_bytes) / 4); \
		} while (0)

#define RK_GET_GRPOUP_NBYTES(grp)		((grp)->p_len)

#define RK_LOAD_GROUP_A(G)  do { \
				grp->curve_name = #G; \
				grp->wide   = G ## _wide;\
				grp->p      = G ## _p; \
				grp->p_len  = sizeof(G ## _p); \
				grp->a      = G ## _a; \
				grp->a_len  = sizeof(G ## _a); \
				grp->n      = G ## _n; \
				grp->n_len  = sizeof(G ## _n); \
				grp->gx     = G ## _gx; \
				grp->gx_len = sizeof(G ## _gx); \
				grp->gy     = G ## _gy; \
				grp->gy_len = sizeof(G ## _gy); \
			} while (0)

/* transform to big endian */
/*
 * Domain parameters for secp192r1
 */
static const uint32_t secp192r1_wide = RK_ECC_CURVE_WIDE_192;

static const uint8_t secp192r1_p[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const uint8_t secp192r1_a[] = {
	0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const uint8_t secp192r1_gx[] = {
	0x12, 0x10, 0xFF, 0x82, 0xFD, 0x0A, 0xFF, 0xF4,
	0x00, 0x88, 0xA1, 0x43, 0xEB, 0x20, 0xBF, 0x7C,
	0xF6, 0x90, 0x30, 0xB0, 0x0E, 0xA8, 0x8D, 0x18,
};

static const uint8_t secp192r1_gy[] = {
	0x11, 0x48, 0x79, 0x1E, 0xA1, 0x77, 0xF9, 0x73,
	0xD5, 0xCD, 0x24, 0x6B, 0xED, 0x11, 0x10, 0x63,
	0x78, 0xDA, 0xC8, 0xFF, 0x95, 0x2B, 0x19, 0x07,
};

static const uint8_t secp192r1_n[] = {
	0x31, 0x28, 0xD2, 0xB4, 0xB1, 0xC9, 0x6B, 0x14,
	0x36, 0xF8, 0xDE, 0x99, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

/*
 * Domain parameters for secp224r1
 */
static const uint32_t secp224r1_wide = RK_ECC_CURVE_WIDE_224;

static const uint8_t secp224r1_p[] = {
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t secp224r1_a[] = {
	0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t secp224r1_gx[] = {
	0x21, 0x1D, 0x5C, 0x11, 0xD6, 0x80, 0x32, 0x34,
	0x22, 0x11, 0xC2, 0x56, 0xD3, 0xC1, 0x03, 0x4A,
	0xB9, 0x90, 0x13, 0x32, 0x7F, 0xBF, 0xB4, 0x6B,
	0xBD, 0x0C, 0x0E, 0xB7,
};

static const uint8_t secp224r1_gy[] = {
	0x34, 0x7E, 0x00, 0x85, 0x99, 0x81, 0xD5, 0x44,
	0x64, 0x47, 0x07, 0x5A, 0xA0, 0x75, 0x43, 0xCD,
	0xE6, 0xDF, 0x22, 0x4C, 0xFB, 0x23, 0xF7, 0xB5,
	0x88, 0x63, 0x37, 0xBD,
};

static const uint8_t secp224r1_n[] = {
	0x3D, 0x2A, 0x5C, 0x5C, 0x45, 0x29, 0xDD, 0x13,
	0x3E, 0xF0, 0xB8, 0xE0, 0xA2, 0x16, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
};

/*
 * Domain parameters for secp256r1
 */
static const uint32_t secp256r1_wide = RK_ECC_CURVE_WIDE_256;

static const uint8_t secp256r1_p[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const uint8_t secp256r1_a[] = {
	0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const uint8_t secp256r1_gx[] = {
	0x96, 0xC2, 0x98, 0xD8, 0x45, 0x39, 0xA1, 0xF4,
	0xA0, 0x33, 0xEB, 0x2D, 0x81, 0x7D, 0x03, 0x77,
	0xF2, 0x40, 0xA4, 0x63, 0xE5, 0xE6, 0xBC, 0xF8,
	0x47, 0x42, 0x2C, 0xE1, 0xF2, 0xD1, 0x17, 0x6B,
};

static const uint8_t secp256r1_gy[] = {
	0xF5, 0x51, 0xBF, 0x37, 0x68, 0x40, 0xB6, 0xCB,
	0xCE, 0x5E, 0x31, 0x6B, 0x57, 0x33, 0xCE, 0x2B,
	0x16, 0x9E, 0x0F, 0x7C, 0x4A, 0xEB, 0xE7, 0x8E,
	0x9B, 0x7F, 0x1A, 0xFE, 0xE2, 0x42, 0xE3, 0x4F,
};

static const uint8_t secp256r1_n[] = {
	0x51, 0x25, 0x63, 0xFC, 0xC2, 0xCA, 0xB9, 0xF3,
	0x84, 0x9E, 0x17, 0xA7, 0xAD, 0xFA, 0xE6, 0xBC,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
};

/*
 * Domain parameters for sm2p256v1_p
 */
static const uint32_t sm2p256v1_wide = RK_ECC_CURVE_WIDE_256;

static const uint8_t sm2p256v1_p[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF,
};

static const uint8_t sm2p256v1_a[] = {
	0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF,
};

static const uint8_t sm2p256v1_gx[] = {
	0xC7, 0x74, 0x4C, 0x33, 0x89, 0x45, 0x5A, 0x71,
	0xE1, 0x0B, 0x66, 0xF2, 0xBF, 0x0B, 0xE3, 0x8F,
	0x94, 0xC9, 0x39, 0x6A, 0x46, 0x04, 0x99, 0x5F,
	0x19, 0x81, 0x19, 0x1F, 0x2C, 0xAE, 0xC4, 0x32,
};

static const uint8_t sm2p256v1_gy[] = {
	0xA0, 0xF0, 0x39, 0x21, 0xE5, 0x32, 0xDF, 0x02,
	0x40, 0x47, 0x2A, 0xC6, 0x7C, 0x87, 0xA9, 0xD0,
	0x53, 0x21, 0x69, 0x6B, 0xE3, 0xCE, 0xBD, 0x59,
	0x9C, 0x77, 0xF6, 0xF4, 0xA2, 0x36, 0x37, 0xBC,
};

static const uint8_t sm2p256v1_n[] = {
	0x23, 0x41, 0xD5, 0x39, 0x09, 0xF4, 0xBB, 0x53,
	0x2B, 0x05, 0xC6, 0x21, 0x6B, 0xDF, 0x03, 0x72,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF,
};

static inline u32 word_reverse(u32 word)
{
	u32 i;
	u32 new_word = 0;

	for (i = 0; i < sizeof(u32); i++) {
		new_word |= (word & 0xff) << (8 * (sizeof(u32) - i - 1));
		word >>= 8;
	}

	return new_word;
}

bool rk_is_ec_supported(void)
{
	return !!RK_ECP_READ_REG(RK_ECC_MAX_CURVE_WIDE);
}

/* reverse endian word copy */
static int rk_ecp_load_data(u32 *dst, struct mpa_num *src)
{
	u32 i;
	u32 dst_pos, src_pos;

	util_word_memset(dst, 0, RK_ECP_MAX_WORDS);

	dst_pos = src->size - 1;
	src_pos = 0;

	for (i = 0; i < src->size; i++)
		dst[dst_pos--] = word_reverse(src->d[src_pos++]);

	return 0;
}

static int rk_word_cmp_zero(uint32_t *buf1, uint32_t n_words)
{
	int ret = 0;
	uint32_t i;

	for (i = 0 ; i < n_words; i++) {
		if (buf1[i] != 0)
			ret = -EINVAL;
	}

	return ret;
}

/*
 * Set a group using well-known domain parameters
 */
static int rk_ecp_group_load(struct rk_ecp_group *grp, enum rk_ecp_group_id id)
{
	memset(grp, 0x00, sizeof(*grp));

	grp->id = id;

	switch (id) {
	case RK_ECP_DP_SECP192R1:
		RK_LOAD_GROUP_A(secp192r1);
		return 0;

	case RK_ECP_DP_SECP224R1:
		RK_LOAD_GROUP_A(secp224r1);
		return 0;

	case RK_ECP_DP_SECP256R1:
		RK_LOAD_GROUP_A(secp256r1);
		return 0;

	case RK_ECP_DP_SM2P256V1:
		RK_LOAD_GROUP_A(sm2p256v1);
		return 0;

	default:
		return -EINVAL;
	}
}

static int rockchip_ecc_request_set(uint32_t ecc_ctl, uint32_t wide)
{
	RK_ECP_WRITE_REG(RK_ECC_CURVE_WIDE, wide);

	RK_ECP_WRITE_REG(RK_ECC_INT_EN, 0);
	RK_ECP_WRITE_REG(RK_ECC_INT_ST, RK_ECP_READ_REG(RK_ECC_INT_ST));
	RK_ECP_WRITE_REG(RK_ECC_CTL, ecc_ctl);

	return 0;
}

static int rockchip_ecc_request_wait_done(void)
{
	int ret = 0;
	u32 reg_val = 0;

	do {
		reg_val = crypto_read(RK_ECC_INT_ST);
	} while ((reg_val & 0x01) != RK_ECC_INT_ST_DONE);

	if (RK_ECP_READ_REG(RK_ECC_ABN_ST)) {
		ret = -EFAULT;
		goto exit;
	}

exit:
	if (ret) {
		printf("RK_ECC_CTL        = %08x\n", RK_ECP_READ_REG(RK_ECC_CTL));
		printf("RK_ECC_INT_EN     = %08x\n", RK_ECP_READ_REG(RK_ECC_INT_EN));
		printf("RK_ECC_CURVE_WIDE = %08x\n", RK_ECP_READ_REG(RK_ECC_CURVE_WIDE));
		printf("RK_ECC_RAM_CTL    = %08x\n", RK_ECP_READ_REG(RK_ECC_RAM_CTL));
		printf("RK_ECC_INT_ST     = %08x\n", RK_ECP_READ_REG(RK_ECC_INT_ST));
		printf("RK_ECC_ABN_ST     = %08x\n", RK_ECP_READ_REG(RK_ECC_ABN_ST));
	}

	RK_ECP_WRITE_REG(RK_ECC_CTL, 0);
	rk_ecp_ram_for_cpu();

	return ret;
}

static int rockchip_ecc_request_trigger(void)
{
	uint32_t ecc_ctl = RK_ECP_READ_REG(RK_ECC_CTL);

	rk_ecp_ram_for_ecc();

	RK_ECP_WRITE_REG(RK_ECC_CTL, ecc_ctl | RK_ECC_CTL_REQ_ECC);

	return rockchip_ecc_request_wait_done();
}

static uint32_t rockchip_ecc_get_group_id(const char *curve_name)
{
	if (strcmp(curve_name, "secp192r1") == 0)
		return RK_ECP_DP_SECP192R1;
	else if (strcmp(curve_name, "secp224r1") == 0)
		return RK_ECP_DP_SECP224R1;
	else if (strcmp(curve_name, "prime256v1") == 0)
		return RK_ECP_DP_SECP256R1;
	else if (strcmp(curve_name, "sm2p256v1") == 0)
		return RK_ECP_DP_SM2P256V1;
	else
		return RK_ECP_DP_NONE;
}

int rockchip_ecc_verify(const char *curve_name, uint8_t *hash, uint32_t hash_len,
			struct rk_ecp_point *point_P, struct rk_ecp_point *point_sign)
{
	int ret;
	uint32_t curve_sel = 0;
	struct mpa_num *bn_hash = NULL;
	uint32_t group_id = rockchip_ecc_get_group_id(curve_name);
	struct rk_ecp_group grp;
	struct rk_ecc_verify *ecc_st = (struct rk_ecc_verify *)SM2_RAM_BASE;

	if (!rk_is_ec_supported())
		return -ENOSYS;

	if (!hash ||
	    hash_len == 0 ||
	    hash_len > RK_ECP_MAX_BYTES ||
	    RK_ECP_IS_POINT_INVALID(point_P) ||
	    RK_ECP_IS_POINT_INVALID(point_sign)) {
		ret = -EINVAL;
		goto exit;
	}

	ret = rk_ecp_group_load(&grp, group_id);
	if (ret)
		goto exit;

	rk_mpa_alloc(&bn_hash, hash, BYTE2WORD(hash_len));
	if (!bn_hash) {
		ret = -ENOMEM;
		goto exit;
	}

	rk_ecp_ram_for_cpu();

	curve_sel = group_id == RK_ECP_DP_SM2P256V1 ?
		    RK_ECC_CTL_FUNC_SM2_CURVER : RK_ECC_CTL_FUNC_ECC_CURVER;

	RK_ECP_LOAD_DATA(ecc_st->e, bn_hash);
	RK_ECP_LOAD_DATA(ecc_st->r_, point_sign->x);
	RK_ECP_LOAD_DATA(ecc_st->s_, point_sign->y);
	RK_ECP_LOAD_DATA(ecc_st->p_x, point_P->x);
	RK_ECP_LOAD_DATA(ecc_st->p_y, point_P->y);

	RK_ECP_LOAD_DATA_EXT(ecc_st->A, grp.a, grp.a_len);
	RK_ECP_LOAD_DATA_EXT(ecc_st->P, grp.p, grp.p_len);
	RK_ECP_LOAD_DATA_EXT(ecc_st->N, grp.n, grp.n_len);

	RK_ECP_LOAD_DATA_EXT(ecc_st->G_x, grp.gx, grp.gx_len);
	RK_ECP_LOAD_DATA_EXT(ecc_st->G_y, grp.gy, grp.gy_len);

	rockchip_ecc_request_set(curve_sel | RK_ECC_CTL_FUNC_SEL_VERIFY, grp.wide);

	ret = rockchip_ecc_request_trigger();
	if (ret ||
	    rk_word_cmp_zero(ecc_st->v, RK_ECP_MAX_WORDS) ||
	    rk_word_cmp_zero(ecc_st->r_, RK_ECP_MAX_WORDS) == 0) {
		ret = -EKEYREJECTED;
	}
exit:
	rk_mpa_free(&bn_hash);

	return ret;
}
