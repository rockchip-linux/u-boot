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

#define TA_UBOOT_STORAGE_UUID	{ 0x1b484ea5, 0x698b, 0x4142, \
		{ 0x82, 0xb8, 0x3a, 0xcf, 0x16, 0xe9, 0x9e, 0x2a } }

#define STORAGE_CMD_READ_OBJ		0
#define STORAGE_CMD_WRITE_OBJ		1
#define STORAGE_CMD_UBOOT_END		2
#define STORAGE_CMD_SET_SECURITY	9

#define USE_RPMB		1
#define USE_SECURITY		0

static struct udevice *tee;
static uint32_t session;

static bool is_use_rpmb(void)
{
	struct mmc *mmc;
	int mmc_num;
	static bool found = false;
	static bool result = false;

	if (found == true) {
		return result;
	}

	mmc_num = get_mmc_num();

	if (mmc_num > 0) {
		for (int dev_id = 0; dev_id < mmc_num; dev_id++) {
			mmc = find_mmc_device(dev_id);
			if (mmc && !IS_SD(mmc)) {
				debug("find emmc device, use rpmb default!\n");
				result = true;
				break;
			}
		}
	}
	found = true;
	return result;
}

static uint32_t storage_ta_open_session(void)
{
	const struct tee_optee_ta_uuid uuid = TA_UBOOT_STORAGE_UUID;
	struct tee_open_session_arg arg;
	struct tee_param param[1];
	int rc;

	tee = tee_find_device(tee, NULL, NULL, NULL);
	if (!tee)
		return TEE_ERROR_CANCEL;

	memset(&arg, 0, sizeof(arg));
	tee_optee_ta_uuid_to_octets(arg.uuid, &uuid);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;

	if (is_use_rpmb())
		param[0].u.value.a = USE_RPMB;
	else
		param[0].u.value.a = USE_SECURITY;

#if defined(CONFIG_OPTEE_ALWAYS_USE_SECURITY_PARTITION)
	param[0].u.value.a = USE_SECURITY;
#endif

	rc = tee_open_session(tee, &arg, ARRAY_SIZE(param), param);
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

static uint32_t optee_base_storage(uint32_t cmd,
				   char *filename,
				   uint32_t name_size,
				   uint8_t *data,
				   uint32_t data_size)
{
	int rc = 0;
	uint32_t ret;
	struct tee_shm *shm_name;
	struct tee_shm *shm_buf;
	struct tee_param param[2];

	if (!filename || !data)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!name_size || !data_size)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!tee) {
		if (storage_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	rc = tee_shm_alloc(tee, name_size,
			   TEE_SHM_ALLOC, &shm_name);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	rc = tee_shm_alloc(tee, data_size,
			   TEE_SHM_ALLOC, &shm_buf);
	if (rc) {
		ret = TEE_ERROR_OUT_OF_MEMORY;
		goto free_name;
	}

	memcpy(shm_name->addr, filename, name_size);
	memcpy(shm_buf->addr, data, data_size);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = shm_name;
	param[0].u.memref.size = name_size;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[1].u.memref.shm = shm_buf;
	param[1].u.memref.size = data_size;

	ret = invoke_func(cmd, ARRAY_SIZE(param), param);
	if (cmd == STORAGE_CMD_READ_OBJ && ret == TEE_SUCCESS)
		memcpy(data, shm_buf->addr, data_size);

	tee_shm_free(shm_buf);
free_name:
	tee_shm_free(shm_name);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

static uint32_t optee_base_write_security_data(char *filename,
					       uint32_t name_size,
					       uint8_t *data,
					       uint32_t data_size)
{
	return optee_base_storage(STORAGE_CMD_WRITE_OBJ,
				  filename, name_size,
				  data, data_size);
}

static uint32_t optee_base_read_security_data(char *filename,
					      uint32_t name_size,
					      uint8_t *data,
					      uint32_t data_size)
{
	return optee_base_storage(STORAGE_CMD_READ_OBJ,
				  filename, name_size,
				  data, data_size);
}

static uint8_t b2hs_add_base(uint8_t in)
{
	if (in > 9)
		return in + 55;
	else
		return in + 48;
}

static uint32_t b2hs(uint8_t *b, uint8_t *hs, uint32_t blen, uint32_t hslen)
{
	uint32_t i = 0;

	if (blen * 2 + 1 > hslen)
		return 0;

	for (; i < blen; i++) {
		hs[i * 2 + 1] = b2hs_add_base(b[i] & 0xf);
		hs[i * 2] = b2hs_add_base(b[i] >> 4);
	}
	hs[blen * 2] = 0;

	return blen * 2;
}

uint32_t optee_read_rollback_index(uint32_t slot, uint64_t *value)
{
	char hs[9];

	b2hs((uint8_t *)&slot, (uint8_t *)hs, 4, 9);

	return optee_base_read_security_data(hs, 8, (uint8_t *)value, 8);
}

uint32_t optee_write_rollback_index(uint32_t slot, uint64_t value)
{
	char hs[9];

	b2hs((uint8_t *)&slot, (uint8_t *)hs, 4, 9);

	return optee_base_write_security_data(hs, 8, (uint8_t *)&value, 8);
}

uint32_t optee_read_permanent_attributes(uint8_t *attributes, uint32_t size)
{
	return optee_base_read_security_data("attributes",
					     sizeof("attributes"),
					     attributes, size);
}

uint32_t optee_write_permanent_attributes(uint8_t *attributes, uint32_t size)
{
	return optee_base_write_security_data("attributes",
					      sizeof("attributes"),
					      attributes, size);
}

uint32_t optee_read_permanent_attributes_flag(uint8_t *attributes)
{
	return optee_base_read_security_data("attributes_flag",
					     sizeof("attributes_flag"),
					     attributes, 1);
}

uint32_t optee_write_permanent_attributes_flag(uint8_t attributes)
{
	return optee_base_write_security_data("attributes_flag",
					      sizeof("attributes_flag"),
					      &attributes, 1);
}

uint32_t optee_read_permanent_attributes_cer(uint8_t *attributes,
					     uint32_t size)
{
	return optee_base_read_security_data("rsacer",
					     sizeof("rsacer"),
					     attributes, size);
}

uint32_t optee_write_permanent_attributes_cer(uint8_t *attributes,
					      uint32_t size)
{
	return optee_base_write_security_data("rsacer",
					      sizeof("rsacer"),
					      attributes, size);
}

uint32_t optee_read_lock_state(uint8_t *lock_state)
{
	return optee_base_read_security_data("lock_state",
					     sizeof("lock_state"),
					     lock_state, 1);
}

uint32_t optee_write_lock_state(uint8_t lock_state)
{
	return optee_base_write_security_data("lock_state",
					      sizeof("lock_state"),
					      &lock_state, 1);
}

uint32_t optee_read_flash_lock_state(uint8_t *flash_lock_state)
{
	return optee_base_read_security_data("flash_lock_state",
					     sizeof("flash_lock_state"),
					     flash_lock_state, 1);
}

uint32_t optee_write_flash_lock_state(uint8_t flash_lock_state)
{
	return optee_base_write_security_data("flash_lock_state",
					      sizeof("flash_lock_state"),
					      &flash_lock_state, 1);
}

static void optee_notify_always_use_security(void)
{
#if defined(CONFIG_ROCKCHIP_OPTEE_V2) && defined(CONFIG_OPTEE_ALWAYS_USE_SECURITY_PARTITION)
	uint32_t ret;

	if (!tee) {
		if (storage_ta_open_session())
			return;
	}

	ret = invoke_func(STORAGE_CMD_SET_SECURITY, 0, NULL);

	tee_close_session(tee, session);
	tee = NULL;

	return;
#endif
}

void optee_client_init(void)
{
	optee_select_security_level();
	optee_notify_always_use_security();
}

static uint32_t optee_base_finish_storage(void)
{
	uint32_t ret;

	if (!tee) {
		if (storage_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	ret = invoke_func(STORAGE_CMD_UBOOT_END, 0, NULL);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_notify_uboot_end(void)
{
	uint32_t res;

	res = optee_base_finish_storage();
	res |= optee_base_finish_otp();
	return res;
}
