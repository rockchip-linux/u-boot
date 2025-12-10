// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd
 */
#include <stdlib.h>
#include <common.h>
#include <command.h>
#include <crypto_manager.h>
#include <dm.h>
#include <time.h>
#include <hexdump.h>
#include <rockchip/crypto_fix_test_data.h>

#define PERF_TOTAL_SIZE			(128 * 1024 * 1024)
#define PERF_BUFF_SIZE			(4 * 1024 * 1024)

#define CALC_RATE_MPBS(bytes, ms)	(((bytes) / 1024) / (ms))

struct hash_test_data {
	const char	*algo_name;
	const char	*mode_name;
	u32		algo;
	const u8	*data;
	u32		data_len;
	const u8	*hash;
	u32		hash_len;
	const u8	*key;
	u32		key_len;
};

struct cipher_test_data {
	const char	*algo_name;
	const char	*mode_name;
	u32		algo;
	u32		mode;
	const u8	*key;
	const u8	*twk_key;
	u32		key_len;
	const u8	*iv;
	u32		iv_len;
	const u8	*plain;
	u32		plain_len;
	const u8	*cipher;
	u32		cipher_len;
	const u8	*aad;
	u32		aad_len;
	const u8	*tag;
	u32		tag_len;
};

struct rsa_test_data {
	const char	*algo_name;
	const char	*mode_name;
	u32		algo;
	const u8	*der_key;
	u32		der_key_len;
	const u8	*hash_data;
	u32		hash_data_len;
	const u8	*sig_data;
	u32		sig_data_len;
};

struct ec_test_data {
	const char	*algo_name;
	u32		algo;
	const u8	*pub_x;
	u32		pub_x_len;
	const u8	*pub_y;
	u32		pub_y_len;
	const u8	*hash_in;
	u32		hash_in_len;
	const u8	*sign_in;
	u32		sign_in_len;
};

#define IS_MAC_MODE(mode)	((mode) == CIPHER_MODE_CBC_MAC || \
				 (mode) == CIPHER_MODE_CMAC)

#define IS_AE_MODE(mode)	((mode) == CIPHER_MODE_CCM || \
				 (mode) == CIPHER_MODE_GCM)
#define HASH_TEST(algo_type, data_in, hash_val) {\
	.algo_name = "HASH", \
	.mode_name = #algo_type, \
	.algo      = HASH_ALGO_##algo_type, \
	.data      = (data_in),\
	.data_len  = sizeof(data_in), \
	.hash      = (hash_val), \
	.hash_len  = sizeof(hash_val) \
}

#define HMAC_TEST(algo_type, data_in, hash_val, hmac_key) {\
	.algo_name = "HMAC", \
	.mode_name = #algo_type, \
	.algo      = HMAC_ALGO_##algo_type, \
	.data      = (data_in),\
	.data_len  = sizeof(data_in), \
	.hash      = (hash_val), \
	.hash_len  = sizeof(hash_val), \
	.key       = (hmac_key), \
	.key_len   = sizeof(hmac_key)\
}

#define CIPHER_XTS_TEST(algo_type, mode_type, key1, key2, iv_val, in, out) { \
	.algo_name  = #algo_type, \
	.mode_name  = #mode_type, \
	.algo       = CIPHER_ALGO_##algo_type,\
	.mode       = CIPHER_MODE_##mode_type, \
	.key        = (key1), \
	.twk_key    = (key2), \
	.key_len    = sizeof(key1), \
	.iv         = (iv_val), \
	.iv_len     = sizeof(iv_val), \
	.plain      = (in), \
	.plain_len  = sizeof(in), \
	.cipher     = (out), \
	.cipher_len = sizeof(out) \
}

#define CIPHER_TEST(algo, mode, key, iv, plain, cipher) \
		CIPHER_XTS_TEST(algo, mode, key, NULL, iv, plain, cipher)

#define CIPHER_AE_TEST(algo_type, mode_type, key_val, iv_val, \
		       in, out, aad_val, tag_val) { \
	.algo_name  = #algo_type, \
	.mode_name  = #mode_type, \
	.algo       = CIPHER_ALGO_##algo_type,\
	.mode       = CIPHER_MODE_##mode_type, \
	.key        = (key_val), \
	.key_len    = sizeof(key_val), \
	.iv         = (iv_val), \
	.iv_len     = sizeof(iv_val), \
	.plain      = (in), \
	.plain_len  = sizeof(in), \
	.cipher     = (out), \
	.cipher_len = sizeof(out), \
	.aad        = (aad_val), \
	.aad_len    = sizeof(aad_val), \
	.tag        = (tag_val), \
	.tag_len    = sizeof(tag_val), \
}

#define RSA_TEST(nbits, key, hash, sig) { \
	.algo_name     = "RSA", \
	.mode_name     = #nbits, \
	.algo          = ASYM_ALGO_RSA, \
	.der_key       = (key), \
	.der_key_len   = sizeof(key), \
	.hash_data     = (hash), \
	.hash_data_len = sizeof(hash), \
	.sig_data      = (sig), \
	.sig_data_len  = sizeof(sig), \
}

#define EC_TEST(name, x, y, hash, sign) { \
	.algo_name   = #name, \
	.algo        = ASYM_ALGO_ECC, \
	.pub_x       = (x), \
	.pub_x_len   = sizeof(x), \
	.pub_y       = (y), \
	.pub_y_len   = sizeof(y), \
	.hash_in     = (hash), \
	.hash_in_len = sizeof(hash), \
	.sign_in     = (sign), \
	.sign_in_len = sizeof(sign), \
}

#define EMPTY_TEST() {}

const struct hash_test_data hash_data_set[] = {
	HASH_TEST(MD5,    foo_data, hash_md5),
	HASH_TEST(SHA1,   foo_data, hash_sha1),
	HASH_TEST(SHA256, foo_data, hash_sha256),
	HASH_TEST(SHA384, foo_data, hash_sha384),
	HASH_TEST(SHA512, foo_data, hash_sha512),
	HASH_TEST(SM3,    foo_data, hash_sm3),
	HASH_TEST(SHA224, foo_data, hash_sha224),
	HASH_TEST(SHA512_224,    foo_data, hash_sha512_224),
	HASH_TEST(SHA512_256,    foo_data, hash_sha512_256),
};

const struct hash_test_data hmac_data_set[] = {
#if CONFIG_IS_ENABLED(ROCKCHIP_HMAC)
	HMAC_TEST(MD5,    foo_data, hmac_md5,    hmac_key),
	HMAC_TEST(SHA1,   foo_data, hmac_sha1,   hmac_key),
	HMAC_TEST(SHA256, foo_data, hmac_sha256, hmac_key),
	HMAC_TEST(SHA512, foo_data, hmac_sha512, hmac_key),
	HMAC_TEST(SM3,    foo_data, hmac_sm3,    hmac_key),
#else
	EMPTY_TEST(),
#endif
};

const struct cipher_test_data cipher_data_set[] = {
#if CONFIG_IS_ENABLED(ROCKCHIP_CIPHER)
	CIPHER_TEST(DES, ECB, des_key, des_iv, foo_data, des_ecb_cipher),
	CIPHER_TEST(DES, CBC, des_key, des_iv, foo_data, des_cbc_cipher),
	CIPHER_TEST(DES, CFB, des_key, des_iv, foo_data, des_cfb_cipher),
	CIPHER_TEST(DES, OFB, des_key, des_iv, foo_data, des_ofb_cipher),

	EMPTY_TEST(),
	CIPHER_TEST(DES, ECB, tdes_key, tdes_iv, foo_data, tdes_ecb_cipher),
	CIPHER_TEST(DES, CBC, tdes_key, tdes_iv, foo_data, tdes_cbc_cipher),
	CIPHER_TEST(DES, CFB, tdes_key, tdes_iv, foo_data, tdes_cfb_cipher),
	CIPHER_TEST(DES, OFB, tdes_key, tdes_iv, foo_data, tdes_ofb_cipher),

	EMPTY_TEST(),
	CIPHER_TEST(AES, BYPASS, aes_key, aes_iv, foo_data, foo_data),
	CIPHER_TEST(AES, ECB, aes_key, aes_iv, foo_data, aes_ecb_cipher),
	CIPHER_TEST(AES, CBC, aes_key, aes_iv, foo_data, aes_cbc_cipher),
	CIPHER_TEST(AES, CFB, aes_key, aes_iv, foo_data, aes_cfb_cipher),
	CIPHER_TEST(AES, OFB, aes_key, aes_iv, foo_data, aes_ofb_cipher),
	CIPHER_TEST(AES, CTS, aes_key, aes_iv, foo_data, aes_cts_cipher),
	CIPHER_TEST(AES, CTR, aes_key, aes_iv, foo_data, aes_ctr_cipher),
	CIPHER_XTS_TEST(AES, XTS, aes_key, aes_twk_key,
			aes_iv, foo_data, aes_xts_cipher),
	CIPHER_TEST(AES, CBC_MAC, aes_key, aes_iv, foo_data, aes_cbc_mac),
	CIPHER_TEST(AES, CMAC, aes_key, aes_iv, foo_data, aes_cmac),
	CIPHER_AE_TEST(AES, CCM, aes_key, aes_ccm_iv, foo_data, aes_ccm_cipher,
		       ad_data, aes_ccm_tag),
	CIPHER_AE_TEST(AES, GCM, aes_key, aes_iv, foo_data, aes_gcm_cipher,
		       ad_data, aes_gcm_tag),

	EMPTY_TEST(),
	CIPHER_TEST(SM4, ECB, sm4_key, sm4_iv, foo_data, sm4_ecb_cipher),
	CIPHER_TEST(SM4, CBC, sm4_key, sm4_iv, foo_data, sm4_cbc_cipher),
	CIPHER_TEST(SM4, CFB, sm4_key, sm4_iv, foo_data, sm4_cfb_cipher),
	CIPHER_TEST(SM4, OFB, sm4_key, sm4_iv, foo_data, sm4_ofb_cipher),
	CIPHER_TEST(SM4, CTS, sm4_key, sm4_iv, foo_data, sm4_cts_cipher),
	CIPHER_TEST(SM4, CTR, sm4_key, sm4_iv, foo_data, sm4_ctr_cipher),
	CIPHER_XTS_TEST(SM4, XTS, sm4_key, sm4_twk_key,
			sm4_iv, foo_data, sm4_xts_cipher),
	CIPHER_TEST(SM4, CBC_MAC, sm4_key, sm4_iv, foo_data, sm4_cbc_mac),
	CIPHER_TEST(SM4, CMAC, sm4_key, sm4_iv, foo_data, sm4_cmac),
	CIPHER_AE_TEST(SM4, CCM, sm4_key, sm4_ccm_iv, foo_data, sm4_ccm_cipher,
		       ad_data, sm4_ccm_tag),
	CIPHER_AE_TEST(SM4, GCM, sm4_key, sm4_iv, foo_data, sm4_gcm_cipher,
		       ad_data, sm4_gcm_tag),
#else
	EMPTY_TEST(),
#endif
};

const struct rsa_test_data rsa_data_set[] = {
#if CONFIG_IS_ENABLED(ROCKCHIP_RSA)
	RSA_TEST(2048, rsa2048_key_der, rsa_hash_data, rsa2048_sig_data),
#endif
};

const struct ec_test_data ec_data_set[] = {
#if CONFIG_IS_ENABLED(ROCKCHIP_EC)
	EC_TEST(secp192r1, ecc192r1_pub_x, ecc192r1_pub_y, ecc192r1_hash, ecc192r1_sign),
	EC_TEST(sm2p256v1, sm2_pub_x, sm2_pub_y, sm2_hash, sm2_sign),
#else
	EMPTY_TEST(),
#endif
};

static void dump_hex(const char *name, const u8 *array, u32 len)
{
	int i;

	printf("[%s]: %uByte", name, len);
	for (i = 0; i < len; i++) {
		if (i % 32 == 0)
			printf("\n");
		printf("%02x ", array[i]);
	}
	printf("\n");
}

static inline void print_result_MBps(const char *algo_name,
				     const char *mode_name,
				     const char *crypt, ulong MBps,
				     const u8 *expect, const u8 *actual,
				     u32 len)
{
	if (memcmp(expect, actual, len) == 0) {
		printf("[%-4s] %-12s%-8s PASS    (%luMBps)\n",
		       algo_name, mode_name, crypt, MBps);
	} else {
		printf("[%-4s] %-12s%-8s FAIL\n",
		       algo_name, mode_name, crypt);
		dump_hex("expect", expect, len);
		dump_hex("actual", actual, len);
	}
}

static inline void print_result_ms(const char *algo_name, const char *mode_name,
				   const char *crypt, ulong time_cost,
				   const u8 *expect, const u8 *actual, u32 len)
{
	if (memcmp(expect, actual, len) == 0) {
		printf("[%s]  %-12s%-8s PASS    (%lums)\n",
		       algo_name, mode_name, crypt, time_cost);
	} else {
		printf("[%s]  %-12s%-8s FAIL\n",
		       algo_name, mode_name, crypt);
		dump_hex("expect", expect, len);
		dump_hex("actual", actual, len);
	}
}

int test_hash_perf(struct udevice *dev, u32 algo,
		   const u8 *key, u32 key_len, ulong *MBps)
{
	u32 total_size = PERF_TOTAL_SIZE;
	u32 data_size = PERF_BUFF_SIZE;
	void *ctx = NULL;
	u8 *data = NULL;
	u8 hash_out[64];
	int ret, i;

	*MBps = 0;

	data = (u8 *)memalign(CONFIG_SYS_CACHELINE_SIZE, data_size);
	if (!data) {
		printf("%s, %d: memalign %u error!\n",
		       __func__, __LINE__, data_size);
		return -EINVAL;
	}

	memset(data, 0xab, data_size);

	ulong start = get_timer(0);

	ret = hash_init(dev, algo, &ctx);
	if (ret) {
		printf("hash_init error ret = %d!\n", ret);
		goto exit;
	}

	for (i = 0; i < total_size / data_size; i++) {
		ret = hash_update(dev, ctx, data, data_size);
		if (ret) {
			printf("hash_update error!\n");
			goto exit;
		}
	}

	ret = hash_finish(dev, ctx, hash_out);
	if (ret) {
		printf("crypto_sha_final error ret = %d!\n", ret);
		goto exit;
	}

	ulong time_cost = get_timer(start);

	*MBps = CALC_RATE_MPBS(total_size, time_cost);

exit:
	free(data);

	return ret;
}

int test_cipher_perf(struct udevice *dev, cipher_context *ctx, ulong *MBps, bool enc)
{
	u32 total_size = PERF_TOTAL_SIZE;
	u32 data_size = PERF_BUFF_SIZE;
	u8 *plain = NULL, *cipher = NULL;
	u8 aad[128], tag[16];
	int ret = 0, i;

	*MBps = 0;

	plain = (u8 *)memalign(CONFIG_SYS_CACHELINE_SIZE, data_size);
	if (!plain) {
		printf("%s, %d: memalign %u error!\n",
		       __func__, __LINE__, data_size);
		return -EINVAL;
	}

	cipher = (u8 *)memalign(CONFIG_SYS_CACHELINE_SIZE, data_size);
	if (!cipher) {
		printf("%s, %d: memalign %u error!\n",
		       __func__, __LINE__, data_size);
		free(plain);
		return -EINVAL;
	}

	memset(plain, 0xab, data_size);
	memset(aad, 0xcb, sizeof(aad));
	memset(tag, 0x00, sizeof(tag));

	ulong start = get_timer(0);

	for (i = 0; i < total_size / data_size; i++) {
		if (IS_MAC_MODE(ctx->mode))
			ret = crypto_mac(dev, ctx, plain, data_size, cipher);
		else if (IS_AE_MODE(ctx->mode))
			ret = crypto_ae(dev, ctx, plain, data_size,
					aad, sizeof(aad), cipher, tag);
		else
			ret = crypto_cipher(dev, ctx, plain, cipher,
					    data_size, enc);
		if (ret) {
			if (ret != -ENOSYS)
				printf("%s, %d:crypto calc error! ret = %d\n",
				       __func__, __LINE__, ret);
			goto exit;
		}
	}

	ulong time_cost = get_timer(start);

	*MBps = CALC_RATE_MPBS(total_size, time_cost);
exit:
	free(plain);
	free(cipher);

	return ret;
}

int test_hash_result(void)
{
	const struct hash_test_data *test_data = NULL;
	void *ctx = NULL;
	struct udevice *dev;
	unsigned int i;
	u8 out[64];
	int ret;

	printf("\n=================== hash test ===================\n");

	for (i = 0; i < ARRAY_SIZE(hash_data_set); i++) {
		test_data = &hash_data_set[i];
		if (test_data->algo == 0 && test_data->algo_name == NULL) {
			printf("\n");
			continue;
		}

		ret = uclass_get_device(UCLASS_HASH, 0, &dev);
		if (ret) {
			printf("failed to get hash device, rc=%d\n", ret);
			return -1;
		}

		memset(out, 0x00, sizeof(out));

		ret = hash_init(dev, test_data->algo, &ctx);
		if (ret == -ENOSYS) {
			printf("[%s] %-16s unsupported!!!\n",
			       test_data->algo_name,
			       test_data->mode_name);
			continue;
		}

		ret |= hash_update(dev, ctx, (void *)test_data->data, test_data->data_len);
		ret |= hash_finish(dev, ctx, out);
		if (ret) {
			printf("hash calc error ret = %d\n", ret);
			goto error;
		}

		ulong MBps = 0;

		test_hash_perf(dev, test_data->algo,
			       test_data->key, test_data->key_len, &MBps);
		print_result_MBps(test_data->algo_name, test_data->mode_name,
				  "", MBps, test_data->hash, out,
				  test_data->hash_len);
		printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	}

	return 0;
error:
	printf("%s %s test error!\n",
	       test_data->algo_name, test_data->mode_name);
	return ret;
}

int test_hmac_perf(struct udevice *dev, u32 algo,
		   const u8 *key, u32 key_len, ulong *MBps)
{
	u32 total_size = PERF_TOTAL_SIZE;
	u32 data_size = PERF_BUFF_SIZE;
	void *ctx = NULL;
	u8 *data = NULL;
	u8 hash_out[64];
	int ret, i;

	*MBps = 0;

	data = (u8 *)memalign(CONFIG_SYS_CACHELINE_SIZE, data_size);
	if (!data) {
		printf("%s, %d: memalign %u error!\n",
		       __func__, __LINE__, data_size);
		return -EINVAL;
	}

	memset(data, 0xab, data_size);

	ulong start = get_timer(0);

	ret = hmac_init(dev, algo, key, key_len, &ctx);
	if (ret) {
		printf("hmac_init error ret = %d!\n", ret);
		goto exit;
	}

	for (i = 0; i < total_size / data_size; i++) {
		ret = hmac_update(dev, ctx, data, data_size);
		if (ret) {
			printf("hmac_update error!\n");
			goto exit;
		}
	}

	ret = hmac_finish(dev, ctx, hash_out);
	if (ret) {
		printf("hmac_finish error ret = %d!\n", ret);
		goto exit;
	}

	ulong time_cost = get_timer(start);

	*MBps = CALC_RATE_MPBS(total_size, time_cost);

exit:
	free(data);

	return ret;
}

int test_hmac_result(void)
{
	const struct hash_test_data *test_data = NULL;
	void *ctx = NULL;
	struct udevice *dev;
	unsigned int i;
	u8 out[64];
	int ret;

	printf("\n=================== hmac test ===================\n");

	for (i = 0; i < ARRAY_SIZE(hmac_data_set); i++) {
		test_data = &hmac_data_set[i];
		if (test_data->algo == 0 && test_data->algo_name == NULL) {
			printf("\n");
			continue;
		}

		ret = uclass_get_device(UCLASS_HMAC, 0, &dev);
		if (ret) {
			printf("failed to get hmac device, rc=%d\n", ret);
			return -1;
		}

		memset(out, 0x00, sizeof(out));

		ret = hmac_init(dev, test_data->algo,
				test_data->key, test_data->key_len, &ctx);
		if (ret == -ENOSYS) {
			printf("[%s] %-16s unsupported!!!\n",
			       test_data->algo_name,
			       test_data->mode_name);
			continue;
		}

		ret |= hmac_update(dev, ctx, (void *)test_data->data, test_data->data_len);
		ret |= hmac_finish(dev, ctx, out);
		if (ret) {
			printf("hmac calc error ret = %d\n", ret);
			goto error;
		}

		ulong MBps = 0;

		test_hmac_perf(dev, test_data->algo,
			       test_data->key, test_data->key_len, &MBps);
		print_result_MBps(test_data->algo_name, test_data->mode_name,
				  "", MBps, test_data->hash, out,
				  test_data->hash_len);
		printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	}

	return 0;
error:
	printf("%s %s test error!\n",
	       test_data->algo_name, test_data->mode_name);
	return ret;
}

int test_cipher_result(void)
{
	const struct cipher_test_data *test_data = NULL;
	struct udevice *dev;
	cipher_context ctx;
	u8 out[256], tag[16];
	int ret;
	u32 i;

	printf("\n===================== cipher test ======================\n");

	for (i = 0; i < ARRAY_SIZE(cipher_data_set); i++) {
		test_data = &cipher_data_set[i];
		if (test_data->algo == 0 && test_data->algo_name == NULL) {
			printf("\n");
			continue;
		}

		ret = uclass_get_device(UCLASS_CIPHER, 0, &dev);
		if (ret) {
			printf("failed to get cipher device, rc=%d\n", ret);
			return -1;
		}

		memset(&ctx, 0x00, sizeof(ctx));

		ctx.algo    = test_data->algo;
		ctx.mode    = test_data->mode;
		ctx.key     = test_data->key;
		ctx.twk_key = test_data->twk_key;
		ctx.key_len = test_data->key_len;
		ctx.iv      = test_data->iv;
		ctx.iv_len  = test_data->iv_len;

		ulong MBps = 0;

		test_cipher_perf(dev, &ctx, &MBps, true);

		/* AES/SM4 mac */
		if (IS_MAC_MODE(ctx.mode))
			ret = crypto_mac(dev, &ctx, test_data->plain,
					 test_data->plain_len, out);
		else if (IS_AE_MODE(ctx.mode))
			ret = crypto_ae(dev, &ctx,
					test_data->plain, test_data->plain_len,
					test_data->aad, test_data->aad_len,
					out, tag);
		else
			ret = crypto_cipher(dev, &ctx, test_data->plain,
					    out, test_data->plain_len, true);
		if (ret == -ENOSYS) {
			printf("[%s] %-16s unsupported!!!\n",
			       test_data->algo_name,
			       test_data->mode_name);
			continue;
		}

		if (ret)
			goto error;

		if (test_data->tag &&
		    memcmp(test_data->tag, tag, test_data->tag_len) != 0) {
			printf("tag mismatch!!!\n");
			dump_hex("expect", test_data->tag, test_data->tag_len);
			dump_hex("actual", tag, test_data->tag_len);
			goto error;
		}

		print_result_MBps(test_data->algo_name, test_data->mode_name,
				  "encrypt", MBps, test_data->cipher, out,
				  test_data->cipher_len);

		if (!IS_MAC_MODE(ctx.mode) && !IS_AE_MODE(ctx.mode)) {
			test_cipher_perf(dev, &ctx, &MBps, false);
			ret = crypto_cipher(dev, &ctx, test_data->cipher,
					    out, test_data->cipher_len, false);
			if (ret)
				goto error;

			print_result_MBps(test_data->algo_name,
					  test_data->mode_name,
					  "decrypt", MBps,
					  test_data->plain, out,
					  test_data->plain_len);
		}
		printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	}
	return 0;
error:
	printf("%s %s test error, ret = %d!\n",
	       test_data->algo_name, test_data->mode_name, ret);
	return ret;
}

static int rsa_test(void)
{
	const struct rsa_test_data *test_data = NULL;
	struct key_prop *prop = NULL;
	struct udevice *dev;
	ulong start, time_cost;
	u32 key_bytes;
	u8 out[512];
	int rc = 0, i;

	memset(out, 0xff, sizeof(out));

	printf("\n====================== rsa test ========================\n");

	for (i = 0; i < ARRAY_SIZE(rsa_data_set); i++) {
		test_data = &rsa_data_set[i];

		rc = rsa_gen_key_prop(test_data->der_key, test_data->der_key_len, &prop);
		if (rc) {
			printf("failed to rsa_gen_key_prop rc=%d\n", rc);
			goto exit;
		}

		key_bytes = prop->num_bits/ 8;

		rc = uclass_get_device(UCLASS_MOD_EXP, 0, &dev);
		if (rc) {
			printf("failed to get mod_exp device, rc=%d\n", rc);
			goto exit;
		}

		start = get_timer(0);

		rc = rsa_mod_exp(dev, test_data->sig_data, test_data->sig_data_len, prop, out);
		if (rc) {
			printf("rsa_mod_exp failed, rc=%d\n", rc);
			goto exit;
		}

		time_cost = get_timer(start);

		print_result_ms(test_data->algo_name, test_data->mode_name,
				"verify", time_cost, test_data->hash_data,
				out + key_bytes - test_data->hash_data_len,
				test_data->hash_data_len);

		rsa_free_key_prop(prop);
		prop = NULL;
		printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	}
exit:
	if (prop)
		rsa_free_key_prop(prop);

	if (rc)
		printf("%s %s test error!\n", test_data->algo_name, test_data->mode_name);

	return rc;
}

int test_ec_result(void)
{
	const struct ec_test_data *test_data = NULL;
	ulong start, time_cost;
	struct udevice *dev;
	const struct ecdsa_ops *ops;
	struct ecdsa_public_key ec_key;
	int ret, i;

	printf("\n====================== ec test ========================\n");
	for (i = 0; i < ARRAY_SIZE(ec_data_set); i++) {
		test_data = &ec_data_set[i];
		if (test_data->algo_name == NULL) {
			printf("\n");
			continue;
		}

		ret = uclass_get_device(UCLASS_ECDSA, 0, &dev);
		if (ret || !dev) {
			printf("[%s] %-16s unsupported!!!\n",
			       test_data->algo_name, "");
			continue;
		}

		/* verify test */
		memset(&ec_key, 0x00, sizeof(ec_key));
		ec_key.curve_name = test_data->algo_name;
		ec_key.x = test_data->pub_x;
		ec_key.y = test_data->pub_y;
		ec_key.size_bits = test_data->pub_x_len * 8;

		start = get_timer(0);
		ops = dev_get_driver_ops(dev);
		ret = ops->verify(dev, &ec_key,
				  (u8 *)test_data->hash_in,
				  test_data->hash_in_len,
				  (u8 *)test_data->sign_in,
				  test_data->sign_in_len);
		if (ret) {
			printf("verify test error, ret = %d\n", ret);
			goto error;
		}
		time_cost = get_timer(start);

		printf("[%-9s]        %-8s PASS    (%lums)\n",
		       test_data->algo_name, "verify", time_cost);

		printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	}

	return 0;
error:
	printf("%s test error!\n", test_data->algo_name);
	return ret;
}

static int do_crypto(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	test_cipher_result();

	test_hash_result();

	test_hmac_result();

	rsa_test();

	test_ec_result();

	return 0;
}

U_BOOT_CMD(
	crypto, 1, 1, do_crypto,
	"crypto test",
	""
);
