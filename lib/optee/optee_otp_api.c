// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 */

#include <common.h>
#include <log.h>
#include <malloc.h>
#include <mmc.h>
#include <tee.h>
#include <linux/ioport.h>
#include <tee/optee.h>

#define TA_UBOOT_OTP_UUID	{ 0x2d26d8a8, 0x5134, 0x4dd8, \
		{ 0xb3, 0x2f, 0xb3, 0x4b, 0xce, 0xeb, 0xc4, 0x71 } }

#define STORAGE_CMD_READ_ATTRIBUTE_HASH			0
#define STORAGE_CMD_WRITE_ATTRIBUTE_HASH		1
#define STORAGE_CMD_UBOOT_END_OTP			2
#define STORAGE_CMD_READ_VBOOTKEY_HASH			3
#define STORAGE_CMD_WRITE_VBOOTKEY_HASH			4
#define STORAGE_CMD_READ_ENABLE_FLAG			5
#define STORAGE_CMD_WRITE_TA_ENCRYPTION_KEY		9
#define STORAGE_CMD_CHECK_SECURITY_LEVEL_FLAG		10
#define STORAGE_CMD_WRITE_OEM_HUK			11
#define STORAGE_CMD_WRITE_OEM_NS_OTP			12
#define STORAGE_CMD_READ_OEM_NS_OTP			13
#define STORAGE_CMD_WRITE_OEM_OTP_KEY			14
#define STORAGE_CMD_SET_OEM_HR_OTP_READ_LOCK		15
#define STORAGE_CMD_OEM_OTP_KEY_IS_WRITTEN		16
#define STORAGE_CMD_TA_ENCRYPTION_KEY_IS_WRITTEN	20
#define STORAGE_CMD_WRITE_OEM_HDCP_KEY			21
#define STORAGE_CMD_OEM_HDCP_KEY_IS_WRITTEN		22
#define STORAGE_CMD_SET_OEM_HDCP_KEY_MASK		23

static struct udevice *tee;
static uint32_t session;

static uint32_t otp_ta_open_session(void)
{
	const struct tee_optee_ta_uuid uuid = TA_UBOOT_OTP_UUID;
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

static uint32_t optee_base_otp_operation(uint32_t cmd,
					 uint8_t is_write,
					 uint32_t *buf,
					 uint32_t word_len)
{
	int rc = 0;
	uint32_t ret;
	uint32_t length = word_len * sizeof(uint32_t);
	struct tee_shm *shm_buf;
	struct tee_param param[1];

	if (!buf || !length)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	rc = tee_shm_alloc(tee, length,
			   TEE_SHM_ALLOC, &shm_buf);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	memset(param, 0, sizeof(param));
	if (is_write) {
		memcpy(shm_buf->addr, buf, length);
		param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
		param[0].u.memref.shm = shm_buf;
		param[0].u.memref.size = length;
	} else {
		param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
		param[0].u.memref.shm = shm_buf;
		param[0].u.memref.size = length;
	}

	ret = invoke_func(cmd, ARRAY_SIZE(param), param);
	if (ret != TEE_SUCCESS)
		goto exit;

	if (!is_write)
		memcpy(buf, shm_buf->addr, length);

exit:
	tee_shm_free(shm_buf);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_read_attribute_hash(uint32_t *buf, uint32_t length)
{
	return optee_base_otp_operation(STORAGE_CMD_READ_ATTRIBUTE_HASH,
					false, buf, length);
}

uint32_t optee_write_attribute_hash(uint32_t *buf, uint32_t length)
{
	return optee_base_otp_operation(STORAGE_CMD_WRITE_ATTRIBUTE_HASH,
					true, buf, length);
}

uint32_t optee_read_vbootkey_hash(uint32_t *buf, uint32_t length)
{
	return optee_base_otp_operation(STORAGE_CMD_READ_VBOOTKEY_HASH,
					false, buf, length);
}

uint32_t optee_write_vbootkey_hash(uint32_t *buf, uint32_t length)
{
	return optee_base_otp_operation(STORAGE_CMD_WRITE_VBOOTKEY_HASH,
					true, buf, length);
}

uint32_t optee_read_vbootkey_enable_flag(uint8_t *flag)
{
	uint32_t bootflag;
	uint32_t ret;

	*flag = 0;

	ret = optee_base_otp_operation(STORAGE_CMD_READ_ENABLE_FLAG,
				       false, &bootflag, 1);

	if (ret == TEE_SUCCESS) {
#if defined(CONFIG_ROCKCHIP_RK3288)
		if (bootflag == 0x00000001)
			*flag = 1;
#else
		if (bootflag == 0x000000FF)
			*flag = 1;
#endif
	}
	return ret;
}

uint32_t optee_check_security_level_flag(uint8_t flag)
{
	uint32_t levelflag;

	levelflag = flag;
	return optee_base_otp_operation(STORAGE_CMD_CHECK_SECURITY_LEVEL_FLAG,
					true, &levelflag, 1);
}

uint32_t optee_write_oem_huk(uint32_t *buf, uint32_t length)
{
	return optee_base_otp_operation(STORAGE_CMD_WRITE_OEM_HUK,
					true, buf, length);
}

uint32_t optee_write_ta_encryption_key(uint32_t *buf, uint32_t length)
{
	return optee_base_otp_operation(STORAGE_CMD_WRITE_TA_ENCRYPTION_KEY,
					true, buf, length);
}

uint32_t optee_ta_encryption_key_is_written(uint8_t *value)
{
	uint32_t ret;
	struct tee_param param[1];

	if (!value)
		return TEE_ERROR_BAD_PARAMETERS;

	*value = 0;

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	ret = invoke_func(STORAGE_CMD_TA_ENCRYPTION_KEY_IS_WRITTEN,
			  ARRAY_SIZE(param), param);
	if (ret == TEE_SUCCESS)
		*value = param[0].u.value.a;

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_write_oem_ns_otp(uint32_t byte_off, uint8_t *byte_buf, uint32_t byte_len)
{
	int rc = 0;
	uint32_t ret;
	uint32_t length = byte_len;
	struct tee_shm *shm_buf;
	struct tee_param param[2];

	if (!byte_buf || !length)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	rc = tee_shm_alloc(tee, length,
			   TEE_SHM_ALLOC, &shm_buf);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	memcpy(shm_buf->addr, byte_buf, length);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = byte_off;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].u.memref.shm = shm_buf;
	param[1].u.memref.size = length;

	ret = invoke_func(STORAGE_CMD_WRITE_OEM_NS_OTP,
			  ARRAY_SIZE(param), param);
	if (ret != TEE_SUCCESS)
		goto exit;

exit:
	tee_shm_free(shm_buf);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_read_oem_ns_otp(uint32_t byte_off, uint8_t *byte_buf, uint32_t byte_len)
{
	int rc = 0;
	uint32_t ret;
	uint32_t length = byte_len;
	struct tee_shm *shm_buf;
	struct tee_param param[2];

	if (!byte_buf || !length)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	rc = tee_shm_alloc(tee, length,
			   TEE_SHM_ALLOC, &shm_buf);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = byte_off;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	param[1].u.memref.shm = shm_buf;
	param[1].u.memref.size = length;

	ret = invoke_func(STORAGE_CMD_READ_OEM_NS_OTP,
			  ARRAY_SIZE(param), param);
	if (ret != TEE_SUCCESS)
		goto exit;

	memcpy(byte_buf, shm_buf->addr, length);

exit:
	tee_shm_free(shm_buf);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_write_oem_otp_key(enum RK_OEM_OTP_KEYID key_id,
				 uint8_t *byte_buf, uint32_t byte_len)
{
	int rc = 0;
	uint32_t ret;
	uint32_t length = byte_len;
	struct tee_shm *shm_buf;
	struct tee_param param[2];

	if (!byte_buf || !length)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	rc = tee_shm_alloc(tee, length,
			   TEE_SHM_ALLOC, &shm_buf);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	memcpy(shm_buf->addr, byte_buf, length);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = key_id;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].u.memref.shm = shm_buf;
	param[1].u.memref.size = length;

	ret = invoke_func(STORAGE_CMD_WRITE_OEM_OTP_KEY,
			  ARRAY_SIZE(param), param);
	if (ret != TEE_SUCCESS)
		goto exit;

exit:
	tee_shm_free(shm_buf);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_oem_otp_key_is_written(enum RK_OEM_OTP_KEYID key_id, uint8_t *value)
{
	uint32_t ret;
	struct tee_param param[1];

	if (!value)
		return TEE_ERROR_BAD_PARAMETERS;

	*value = 0xFF;

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].u.value.a = key_id;
	ret = invoke_func(STORAGE_CMD_OEM_OTP_KEY_IS_WRITTEN,
			  ARRAY_SIZE(param), param);
	if (ret == TEE_SUCCESS)
		*value = param[0].u.value.b;

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_set_oem_hr_otp_read_lock(enum RK_OEM_OTP_KEYID key_id)
{
	uint32_t ret;
	struct tee_param param[1];

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = key_id;
	ret = invoke_func(STORAGE_CMD_SET_OEM_HR_OTP_READ_LOCK,
			  ARRAY_SIZE(param), param);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_write_oem_hdcp_key(enum RK_HDCP_KEYID key_id,
				  uint8_t *byte_buf, uint32_t byte_len)
{
	int rc = 0;
	uint32_t ret;
	uint32_t length = byte_len;
	struct tee_shm *shm_buf;
	struct tee_param param[2];

	if (!byte_buf || !length)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	rc = tee_shm_alloc(tee, length,
			   TEE_SHM_ALLOC, &shm_buf);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	memcpy(shm_buf->addr, byte_buf, length);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = key_id;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].u.memref.shm = shm_buf;
	param[1].u.memref.size = length;

	ret = invoke_func(STORAGE_CMD_WRITE_OEM_HDCP_KEY,
			  ARRAY_SIZE(param), param);
	if (ret != TEE_SUCCESS)
		goto exit;

exit:
	tee_shm_free(shm_buf);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_oem_hdcp_key_is_written(enum RK_HDCP_KEYID key_id, uint8_t *value)
{
	uint32_t ret;
	struct tee_param param[1];

	if (!value)
		return TEE_ERROR_BAD_PARAMETERS;

	*value = 0xFF;

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].u.value.a = key_id;
	ret = invoke_func(STORAGE_CMD_OEM_HDCP_KEY_IS_WRITTEN,
			  ARRAY_SIZE(param), param);
	if (ret == TEE_SUCCESS)
		*value = param[0].u.value.b;

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_set_oem_hdcp_key_mask(enum RK_HDCP_KEYID key_id)
{
	uint32_t ret;
	struct tee_param param[1];

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = key_id;
	ret = invoke_func(STORAGE_CMD_SET_OEM_HDCP_KEY_MASK,
			  ARRAY_SIZE(param), param);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

void optee_select_security_level(void)
{
#if (CONFIG_OPTEE_SECURITY_LEVEL > 0)
	uint32_t res;

	res = optee_check_security_level_flag(CONFIG_OPTEE_SECURITY_LEVEL);
	if (res == TEE_ERROR_CANCEL) {
		run_command("download", 0);
		return;
	}

	if (res == TEE_SUCCESS)
		debug("optee select security level success!");
	else
		panic("optee select security level fail!");

	return;
#endif
}

uint32_t optee_base_finish_otp(void)
{
	uint32_t ret;

	if (!tee) {
		if (otp_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	ret = invoke_func(STORAGE_CMD_UBOOT_END_OTP, 0, NULL);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

