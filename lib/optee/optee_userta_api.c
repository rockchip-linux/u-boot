// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025, Rockchip Electronics Co., Ltd
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

#define RKTEST_TA_UUID	{ 0x1db57234, 0xdacd, 0x462d, \
		{ 0x9b, 0xb1, 0xae, 0x79, 0xde, 0x44, 0xe2, 0xa5 } }

#define RKTEST_TA_CMD_TRANSFER		102
#define RKTEST_TA_CMD_STORAGE		103

static struct udevice *tee;
static uint32_t session;

static uint32_t user_ta_open_session(void)
{
	const struct tee_optee_ta_uuid uuid = RKTEST_TA_UUID;
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

uint32_t optee_oem_user_ta_transfer(void)
{
	int rc = 0;
	uint32_t ret;
	struct tee_shm *shm_in;
	struct tee_shm *shm_out;
	struct tee_param param[3];
	const uint8_t transfer_inout[] = "Transfer data test.";
	uint32_t data_size = sizeof(transfer_inout);

	if (!tee) {
		if (user_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	rc = tee_shm_alloc(tee, data_size,
			   TEE_SHM_ALLOC, &shm_in);
	if (rc)
		return TEE_ERROR_OUT_OF_MEMORY;

	rc = tee_shm_alloc(tee, data_size,
			   TEE_SHM_ALLOC, &shm_out);
	if (rc) {
		ret = TEE_ERROR_OUT_OF_MEMORY;
		goto free;
	}

	memcpy(shm_in->addr, transfer_inout, data_size);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].u.value.a = 66;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].u.memref.shm = shm_in;
	param[1].u.memref.size = data_size;
	param[2].attr = TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	param[2].u.memref.shm = shm_out;
	param[2].u.memref.size = data_size;

	ret = invoke_func(RKTEST_TA_CMD_TRANSFER, ARRAY_SIZE(param), param);
	if (ret != TEE_SUCCESS)
		goto exit;

	//Check the result
	if (param[0].u.value.a == 66 + 1 &&
	    param[0].u.value.b == param[0].u.value.a)
		printf("test value : Pass!\n");
	else
		printf("test value : Fail! (mismatch values)\n");

	if (memcmp(shm_in->addr, shm_out->addr, data_size) == 0)
		printf("test buffer : Pass!\n");
	else
		printf("test buffer : Fail! (mismatch buffer)\n");

exit:
	tee_shm_free(shm_out);
free:
	tee_shm_free(shm_in);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}

uint32_t optee_oem_user_ta_storage(void)
{
	uint32_t ret;

	if (!tee) {
		if (user_ta_open_session())
			return TEE_ERROR_CANCEL;
	}

	ret = invoke_func(RKTEST_TA_CMD_STORAGE, 0, NULL);

	tee_close_session(tee, session);
	tee = NULL;

	return ret;
}
