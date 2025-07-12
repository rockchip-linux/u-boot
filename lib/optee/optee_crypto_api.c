// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 */

#include <common.h>
#include <cpu_func.h>
#include <log.h>
#include <malloc.h>
#include <mmc.h>
#include <tee.h>
#include <linux/ioport.h>
#include <tee/optee.h>

#define RK_CRYPTO_SERVICE_UUID	{ 0x0cacdb5d, 0x4fea, 0x466c, \
		{ 0x97, 0x16, 0x3d, 0x54, 0x16, 0x52, 0x83, 0x0f } }

#define CRYPTO_SERVICE_CMD_OEM_OTP_KEY_PHYS_CIPHER	2
#define CRYPTO_SERVICE_CMD_FW_KEY_PHYS_CIPHER		7
#define CRYPTO_SERVICE_CMD_VERIFY_CONFIG_IP		9

static struct udevice *tee;
static uint32_t session;

static uint32_t crypto_ta_open_session(void)
{
	const struct tee_optee_ta_uuid uuid = RK_CRYPTO_SERVICE_UUID;
	struct tee_open_session_arg arg;
	int rc;

	tee = tee_find_device(tee, NULL, NULL, NULL);
	if (!tee)
		return TEE_ERROR_CANCEL;

	memset(&arg, 0, sizeof(arg));
	tee_optee_ta_uuid_to_octets(arg.uuid, &uuid);

	rc = tee_open_session(tee, &arg, 0, NULL);
	if (!rc)
		session = arg.session;

	return arg.ret;
}

static uint32_t invoke_func(uint32_t func, ulong num_param, struct tee_param *param)
{
	struct tee_invoke_arg arg;

	memset(&arg, 0, sizeof(arg));
	arg.func = func;
	arg.session = session;

	tee_invoke_func(tee, &arg, num_param, param);

	return arg.ret;
}

static void crypto_flush_cacheline(uint32_t addr, uint32_t size)
{
	ulong alignment = CONFIG_SYS_CACHELINE_SIZE;
	ulong aligned_input, aligned_len;

	if (!addr || !size)
		return;

	/* Must flush dcache before crypto DMA fetch data region */
	aligned_input = round_down(addr, alignment);
	aligned_len = round_up(size + (addr - aligned_input), alignment);
	flush_cache(aligned_input, aligned_len);
}

static void crypto_invalidate_cacheline(uint32_t addr, uint32_t size)
{
	ulong alignment = CONFIG_SYS_CACHELINE_SIZE;
	ulong aligned_input, aligned_len;

	if (!addr || !size)
		return;

	/* Must invalidate dcache after crypto DMA write data region */
	aligned_input = round_down(addr, alignment);
	aligned_len = round_up(size + (addr - aligned_input), alignment);
	invalidate_dcache_range(aligned_input, aligned_input + aligned_len);
}

uint32_t optee_oem_otp_key_cipher(enum RK_OEM_OTP_KEYID key_id, rk_cipher_config *config,
				  uint32_t src_phys_addr, uint32_t dst_phys_addr,
				  uint32_t len)
{
	int rc = 0;
	uint32_t ret;
	uint32_t length;
	struct tee_shm *shm_buf;
	struct tee_param param[4];

	if (key_id != RK_OEM_OTP_KEY0 &&
	    key_id != RK_OEM_OTP_KEY1 &&
	    key_id != RK_OEM_OTP_KEY2 &&
	    key_id != RK_OEM_OTP_KEY3 &&
	    key_id != RK_OEM_OTP_KEY_FW)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!config)
		return TEE_ERROR_BAD_PARAMETERS;

	if (config->algo != RK_ALGO_AES && config->algo != RK_ALGO_SM4)
		return TEE_ERROR_BAD_PARAMETERS;

	if (config->mode >= RK_CIPHER_MODE_XTS)
		return TEE_ERROR_BAD_PARAMETERS;

	if (config->operation != RK_MODE_ENCRYPT &&
	    config->operation != RK_MODE_DECRYPT)
		return TEE_ERROR_BAD_PARAMETERS;

	if (config->key_len != 16 &&
	    config->key_len != 24 &&
	    config->key_len != 32)
		return TEE_ERROR_BAD_PARAMETERS;

	if (key_id == RK_OEM_OTP_KEY_FW && config->key_len != 16)
		return TEE_ERROR_BAD_PARAMETERS;

#if defined(CONFIG_ROCKCHIP_RV1126)
	if (config->key_len == 24)
		return TEE_ERROR_BAD_PARAMETERS;
#endif

	if (len % AES_BLOCK_SIZE || len == 0)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!src_phys_addr || !dst_phys_addr)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!tee) {
		if (crypto_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	length = sizeof(rk_cipher_config);
	rc = tee_shm_alloc(tee, length,
			   TEE_SHM_ALLOC, &shm_buf);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	memcpy(shm_buf->addr, config, sizeof(rk_cipher_config));

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = key_id;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].u.memref.shm = shm_buf;
	param[1].u.memref.size = length;
	param[2].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].u.value.a = src_phys_addr;
	param[2].u.value.b = len;
	param[3].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].u.value.a = dst_phys_addr;

	crypto_flush_cacheline(src_phys_addr, len);
	crypto_flush_cacheline(dst_phys_addr, len);

	ret = invoke_func(CRYPTO_SERVICE_CMD_OEM_OTP_KEY_PHYS_CIPHER,
				ARRAY_SIZE(param), param);

	crypto_invalidate_cacheline(dst_phys_addr, len);

	tee_shm_free(shm_buf);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_fw_key_cipher(enum RK_FW_KEYID key_id, rk_cipher_config *config,
			     uint32_t src_phys_addr, uint32_t dst_phys_addr,
			     uint32_t len)
{
	int rc = 0;
	uint32_t ret;
	uint32_t length;
	struct tee_shm *shm_buf;
	struct tee_param param[4];

	if (key_id != RK_FW_KEY0)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!config)
		return TEE_ERROR_BAD_PARAMETERS;

	if (config->algo != RK_ALGO_AES && config->algo != RK_ALGO_SM4)
		return TEE_ERROR_BAD_PARAMETERS;

	if (config->mode >= RK_CIPHER_MODE_XTS)
		return TEE_ERROR_BAD_PARAMETERS;

	if (config->operation != RK_MODE_ENCRYPT &&
	    config->operation != RK_MODE_DECRYPT)
		return TEE_ERROR_BAD_PARAMETERS;

	if (config->key_len != 16 &&
	    config->key_len != 24 &&
	    config->key_len != 32)
		return TEE_ERROR_BAD_PARAMETERS;

	if (len % AES_BLOCK_SIZE || len == 0)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!src_phys_addr || !dst_phys_addr)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!tee) {
		if (crypto_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	length = sizeof(rk_cipher_config);
	rc = tee_shm_alloc(tee, length,
			   TEE_SHM_ALLOC, &shm_buf);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	memcpy(shm_buf->addr, config, sizeof(rk_cipher_config));

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = key_id;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].u.memref.shm = shm_buf;
	param[1].u.memref.size = length;
	param[2].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].u.value.a = src_phys_addr;
	param[2].u.value.b = len;
	param[3].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].u.value.a = dst_phys_addr;

	crypto_flush_cacheline(src_phys_addr, len);
	crypto_flush_cacheline(dst_phys_addr, len);

	ret = invoke_func(CRYPTO_SERVICE_CMD_FW_KEY_PHYS_CIPHER,
				ARRAY_SIZE(param), param);

	crypto_invalidate_cacheline(dst_phys_addr, len);

	tee_shm_free(shm_buf);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_verify_config_ip(char *licence_str)
{
	int rc = 0;
	uint32_t ret;
	uint32_t length;
	struct tee_shm *shm_buf;
	struct tee_param param[1];

	if (!licence_str)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!tee) {
		if (crypto_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	length = strlen(licence_str);
	rc = tee_shm_alloc(tee, length,
			   TEE_SHM_ALLOC, &shm_buf);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	memcpy(shm_buf->addr, licence_str, length);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = shm_buf;
	param[0].u.memref.size = length;

	ret = invoke_func(CRYPTO_SERVICE_CMD_VERIFY_CONFIG_IP,
			  ARRAY_SIZE(param), param);

	tee_shm_free(shm_buf);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}
