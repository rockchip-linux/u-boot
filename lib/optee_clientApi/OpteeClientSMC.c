/*
 * Copyright 2017, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <optee_include/OpteeClientMem.h>
#include <optee_include/OpteeClientSMC.h>
#include <optee_include/OpteeClientRPC.h>
#include <optee_include/teesmc.h>

#define TEEC_SMC_DEFAULT_CACHE_ATTRIBUTES \
	(TEESMC_ATTR_CACHE_DEFAULT << TEESMC_ATTR_CACHE_SHIFT);

static void SetTeeSmc32Params(TEEC_Operation *operation,
	t_teesmc32_param *TeeSmc32Param);
static void GetTeeSmc32Params(t_teesmc32_param *TeeSmc32Param,
	TEEC_Operation *operation);
static TEEC_Result OpteeSmcCall(t_teesmc32_arg *TeeSmc32Arg);

/*
 * This function opens a new Session between the Client application and the
 * specified TEE application.
 *
 * Only connection_method == TEEC_LOGIN_PUBLIC is supported connection_data and
 * operation shall be set to NULL.
 */
TEEC_Result TEEC_SMC_OpenSession(TEEC_Context *context,
				TEEC_Session *session,
				const TEEC_UUID *destination,
				TEEC_Operation  *operation,
				uint32_t *error_origin)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;
	uint32_t TeeSmc32ArgLength;
	uint32_t TeeSmcMetaSessionLength;

	t_teesmc32_arg *TeeSmc32Arg = NULL;
	t_teesmc32_param *TeeSmc32Param = NULL;
	t_teesmc_meta_open_session *TeeSmcMetaSession = NULL;
	static const uint32_t MetaNum = 1;

	*error_origin = TEEC_ORIGIN_API;

	TeeSmc32ArgLength =
		TEESMC32_GET_ARG_SIZE(TEEC_CONFIG_PAYLOAD_REF_COUNT + MetaNum);

	TeeSmc32Arg = (t_teesmc32_arg *)OpteeClientMemAlloc(TeeSmc32ArgLength);

	if (TeeSmc32Arg == NULL) {
		TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
		goto Exit;
	}

	memset(TeeSmc32Arg, 0, TeeSmc32ArgLength);

	TeeSmcMetaSessionLength = sizeof(*TeeSmcMetaSession);

	TeeSmcMetaSession = (t_teesmc_meta_open_session *)
		OpteeClientMemAlloc(TeeSmcMetaSessionLength);

	if (TeeSmcMetaSession == NULL) {
		TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
		goto Exit;
	}

	memset(TeeSmcMetaSession, 0, TeeSmcMetaSessionLength);

	TeeSmc32Arg->cmd = TEESMC_CMD_OPEN_SESSION;
	TeeSmc32Arg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT + MetaNum;

	TeeSmc32Param = TEESMC32_GET_PARAMS(TeeSmc32Arg);

	memcpy(&TeeSmcMetaSession->uuid,
		destination,
		sizeof(TeeSmcMetaSession->uuid));
	TeeSmcMetaSession->clnt_login = TEEC_LOGIN_PUBLIC;

	TeeSmc32Param[0].u.memref.buf_ptr = (uint32_t) TeeSmcMetaSession;
	TeeSmc32Param[0].u.memref.size = sizeof(*TeeSmcMetaSession);

#ifdef CONFIG_ROCKCHIP_RK3328
	TeeSmc32Param[0].attr = TEESMC_ATTR_TYPE_MEMREF_INPUT |
				TEESMC_ATTR_META              |
				TEEC_SMC_DEFAULT_CACHE_ATTRIBUTES;
#endif

#ifdef CONFIG_ROCKCHIP_RK322X
	TeeSmc32Param[0].attr = TEESMC_ATTR_TYPE_MEMREF_INPUT |
				TEESMC_ATTR_META;
#endif

#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
	TeeSmc32Param[0].attr = TEESMC_ATTR_TYPE_MEMREF_INPUT |
				TEESMC_ATTR_META;
#endif

	SetTeeSmc32Params(operation, TeeSmc32Param + MetaNum);

	*error_origin = TEEC_ORIGIN_COMMS;

	TeecResult = OpteeSmcCall(TeeSmc32Arg);
	if (TeecResult != TEEC_SUCCESS)
		goto Exit;

	session->id = TeeSmc32Arg->session;
	TeecResult = TeeSmc32Arg->ret;
	*error_origin = TeeSmc32Arg->ret_origin;

	GetTeeSmc32Params(TeeSmc32Param + MetaNum, operation);

Exit:
	if (TeeSmc32Arg != NULL)
		OpteeClientMemFree(TeeSmc32Arg);

	if (TeeSmcMetaSession != NULL)
		OpteeClientMemFree(TeeSmcMetaSession);

	return TeecResult;
}

/*
 * This function closes a session which has been opened with a TEE
 * application.
 *
 * Note that the GP specification does not allow for this API to fail and return
 * a failure code however we'll support this at the SMC level so we can get
 * see debug information about such failures.
 */
TEEC_Result TEEC_SMC_CloseSession(TEEC_Session *session,
				uint32_t *error_origin)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;
	uint32_t TeeSmc32ArgLength;
	t_teesmc32_arg *TeeSmc32Arg = NULL;

	*error_origin = TEEC_ORIGIN_API;

	TeeSmc32ArgLength =
		TEESMC32_GET_ARG_SIZE(TEEC_CONFIG_PAYLOAD_REF_COUNT);

	TeeSmc32Arg = (t_teesmc32_arg *)OpteeClientMemAlloc(TeeSmc32ArgLength);

	if (TeeSmc32Arg == NULL) {
		TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
		goto Exit;
	}

	memset(TeeSmc32Arg, 0, TeeSmc32ArgLength);

	TeeSmc32Arg->cmd = TEESMC_CMD_CLOSE_SESSION;
	TeeSmc32Arg->session = session->id;

	*error_origin = TEEC_ORIGIN_COMMS;

	TeecResult = OpteeSmcCall(TeeSmc32Arg);

	if (TeecResult != TEEC_SUCCESS)
		goto Exit;

	TeecResult = TeeSmc32Arg->ret;
	*error_origin = TeeSmc32Arg->ret_origin;

Exit:
	if (TeeSmc32Arg != NULL)
		OpteeClientMemFree(TeeSmc32Arg);

	return TeecResult;
}

/*
 * Invokes a TEE command (secure service, sub-PA or whatever).
 */
TEEC_Result TEEC_SMC_InvokeCommand(TEEC_Session *session,
				uint32_t cmd_id,
				TEEC_Operation *operation,
				uint32_t *error_origin)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;
	uint32_t TeeSmc32ArgLength;
	t_teesmc32_arg *TeeSmc32Arg = NULL;
	t_teesmc32_param *TeeSmc32Param = NULL;

	*error_origin = TEEC_ORIGIN_API;

	TeeSmc32ArgLength =
		TEESMC32_GET_ARG_SIZE(TEEC_CONFIG_PAYLOAD_REF_COUNT);

	TeeSmc32Arg = (t_teesmc32_arg *)OpteeClientMemAlloc(TeeSmc32ArgLength);

	if (TeeSmc32Arg == NULL) {
		TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
		goto Exit;
	}

	memset(TeeSmc32Arg, 0, TeeSmc32ArgLength);

	TeeSmc32Arg->cmd = TEESMC_CMD_INVOKE_COMMAND;
	TeeSmc32Arg->ta_func = cmd_id;
	TeeSmc32Arg->session = session->id;
	TeeSmc32Arg->num_params = TEEC_CONFIG_PAYLOAD_REF_COUNT;

	TeeSmc32Param = TEESMC32_GET_PARAMS(TeeSmc32Arg);

	SetTeeSmc32Params(operation, TeeSmc32Param);

	*error_origin = TEEC_ORIGIN_COMMS;

	TeecResult = OpteeSmcCall(TeeSmc32Arg);
	if (TeecResult != TEEC_SUCCESS)
		goto Exit;

	TeecResult = TeeSmc32Arg->ret;
	*error_origin = TeeSmc32Arg->ret_origin;

	GetTeeSmc32Params(TeeSmc32Param, operation);

Exit:
	if (TeeSmc32Arg != NULL)
		OpteeClientMemFree(TeeSmc32Arg);


	return TeecResult;
}

/*
 * Request a cancellation of a in-progress operation (best effort)
 *
 * Note that the GP specification does not allow for this API to fail and return
 * a failure code however we'll support this at the SMC level so we can get
 * see debug information about such failures.
 */
TEEC_Result TEEC_SMC_RequestCancellation(TEEC_Operation *operation,
					uint32_t *error_origin)
{
	return TEEC_ERROR_NOT_IMPLEMENTED;
}

/*
 * Set the call parameter blocks in the
 * SMC call based on the TEEC parameter supplied.
 * This only handles the parameters supplied in
 * the originating call and not those
 * considered internal meta parameters and is
 * thus constrained by the build
 * constants exposed to callers.
 */
void SetTeeSmc32Params(TEEC_Operation *operation,
						t_teesmc32_param *TeeSmc32Param)
{
	uint32_t ParamCount;

	for (ParamCount = 0;
		ParamCount < TEEC_CONFIG_PAYLOAD_REF_COUNT;
		ParamCount++) {
		uint32_t attr =
			TEEC_PARAM_TYPE_GET(operation->paramTypes, ParamCount);

		if (attr == TEEC_MEMREF_TEMP_INPUT ||
			attr == TEEC_MEMREF_TEMP_OUTPUT ||
			attr == TEEC_MEMREF_TEMP_INOUT) {
#ifdef CONFIG_ROCKCHIP_RK3328
			attr |= TEEC_SMC_DEFAULT_CACHE_ATTRIBUTES;
			debug(" 3328 attr %x\n", attr);
#endif
#ifdef CONFIG_ROCKCHIP_RK322X
			debug(" 322X attr %x\n", attr);
#endif
#if defined(CONFIG_RKCHIP_RK3126) || defined(CONFIG_RKCHIP_RK3128)
			debug(" 312X attr %x\n", attr);
#endif

			TeeSmc32Param[ParamCount].attr = attr;
			TeeSmc32Param[ParamCount].u.memref.buf_ptr =
			(uint32_t)operation->params[ParamCount].tmpref.buffer;
			TeeSmc32Param[ParamCount].u.memref.size =
				operation->params[ParamCount].tmpref.size;
		} else {
			TeeSmc32Param[ParamCount].attr = attr;
			TeeSmc32Param[ParamCount].u.value.a =
				operation->params[ParamCount].value.a;
			TeeSmc32Param[ParamCount].u.value.b =
				operation->params[ParamCount].value.b;
		}
	}
}

/*
 * Get the return parameter blocks from
 * the SMC call into the TEEC parameter supplied.
 * This only handles the parameters supplied
 * in the originating call and not those
 * considered internal meta parameters and
 * is thus constrained by the build
 * constants exposed to callers.
 */
void GetTeeSmc32Params(t_teesmc32_param *TeeSmc32Param,
				TEEC_Operation *operation)
{
	uint32_t ParamCount;

	for (ParamCount = 0;
	ParamCount < TEEC_CONFIG_PAYLOAD_REF_COUNT;
	ParamCount++) {
		operation->params[ParamCount].value.a =
			TeeSmc32Param[ParamCount].u.value.a;
		operation->params[ParamCount].value.b =
			TeeSmc32Param[ParamCount].u.value.b;
	}
}

/*
 * Populate the SMC registers and make
 * the call with OpTEE specific handling.
 */
TEEC_Result OpteeSmcCall(t_teesmc32_arg *TeeSmc32Arg)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;
	ARM_SMC_ARGS ArmSmcArgs = {0};

	ArmSmcArgs.Arg0 = TEESMC32_CALL_WITH_ARG;
	ArmSmcArgs.Arg1 = (uint32_t) TeeSmc32Arg;

	while (1) {
		tee_smc_call(&ArmSmcArgs);
		if (TEESMC_RETURN_IS_RPC(ArmSmcArgs.Arg0)) {
			(void) OpteeRpcCallback(&ArmSmcArgs);
		} else if (ArmSmcArgs.Arg0 == TEESMC_RETURN_UNKNOWN_FUNCTION) {
			TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
			break;
		} else if (ArmSmcArgs.Arg0 != TEESMC_RETURN_OK) {
			TeecResult = TEEC_ERROR_COMMUNICATION;
			break;
		} else {
			TeecResult = TEEC_SUCCESS;
			break;
		}
	}

	return TeecResult;
}

