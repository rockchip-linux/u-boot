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

const char* g_asym_algos_tbl[ASYM_ALGO_NUM] = {
	[ASYM_ALGO_RSA] = "RSA",
	[ASYM_ALGO_ECC] = "ECC",
	[ASYM_ALGO_SM2] = "SM2",
};

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

#define HASH_TEST(algo_type, data_in, hash_val) {\
	.algo_name = "HASH", \
	.mode_name = #algo_type, \
	.algo      = HASH_ALGO_##algo_type, \
	.data      = (data_in),\
	.data_len  = sizeof(data_in), \
	.hash      = (hash_val), \
	.hash_len  = sizeof(hash_val) \
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

#define EMPTY_TEST() {}

const struct hash_test_data hash_data_set[] = {
	HASH_TEST(MD5,    foo_data, hash_md5),
	HASH_TEST(SHA1,   foo_data, hash_sha1),
	HASH_TEST(SHA256, foo_data, hash_sha256),
	HASH_TEST(SHA384, foo_data, hash_sha384),
	HASH_TEST(SHA512, foo_data, hash_sha512),
};

const struct rsa_test_data rsa_data_set[] = {
#if CONFIG_IS_ENABLED(ROCKCHIP_RSA)
	RSA_TEST(2048, rsa2048_key_der, rsa_hash_data, rsa2048_sig_data),
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
		printf("[%s] %-8s%-8s PASS    (%luMBps)\n",
		       algo_name, mode_name, crypt, MBps);
	} else {
		printf("[%s] %-8s%-8s FAIL\n",
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
		printf("[%s] %-8s%-8s PASS    (%lums)\n",
		       algo_name, mode_name, crypt, time_cost);
	} else {
		printf("[%s] %-8s%-8s FAIL\n",
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

int test_hash_result(void)
{
	const struct hash_test_data *test_data = NULL;
	void *ctx = NULL;
	struct udevice *dev;
	unsigned int i;
	u8 out[64];
	int ret;

	printf("\n=================== hash & hmac test ===================\n");

	for (i = 0; i < ARRAY_SIZE(hash_data_set); i++) {
		test_data = &hash_data_set[i];
		if (test_data->algo == 0) {
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

static void dump_crypto_info(const struct crypto_impl *impl)
{
	const char *driver_name;
	u32 i;

	driver_name = crypto_get_driver_name(impl);
	if (!driver_name)
		driver_name = "Unknown";

	printf("================================================\n");
	printf("driver_name: %s\n", driver_name);
	printf("priority   : %u\n", impl->priority);

	if (impl->type == CRYPTO_TYPE_HASH) {
		printf("hash       : ");
		for (i = 0; i < HASH_ALGO_NUM; i++) {
			if (impl->check_valid(impl->dev, i, CRYPTO_MODE_NONE))
				printf("%s, ", hash_algo_name(i));
		}

		printf("\n");
	} else if (impl->type == CRYPTO_TYPE_ASYM) {
		printf("asym       : ");
		for (i = 0; i < ASYM_ALGO_NUM; i++) {
			if (impl->check_valid(impl->dev, i, CRYPTO_MODE_NONE))
				printf("%s, ", g_asym_algos_tbl[i]);
		}

		printf("\n");
	}
	printf("================================================\n\n");
}

static int do_crypto(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	const struct crypto_impl *impl = NULL;
	u32 type, index;

	for (type = 0; type < CRYPTO_TYPE_MAX; type++) {
		for (index = 0; index < CRYPTO_DRIVER_MAX; index++) {
			impl = crypto_get_impl_by_index(type, index);
			if (!impl)
				break;

			dump_crypto_info(impl);
		}
	}

	test_hash_result();

	rsa_test();

	return 0;
}

U_BOOT_CMD(
	crypto, 1, 1, do_crypto,
	"crypto test",
	""
);
