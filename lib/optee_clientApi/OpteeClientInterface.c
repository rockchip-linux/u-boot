/*
 * Copyright 2017, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <optee_include/OpteeClientApiLib.h>
#include <optee_include/tee_client_api.h>

void test_optee(void)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x1b484ea5, 0x698b, 0x4142,
		{ 0x82, 0xb8, 0x3a, 0xcf, 0x16, 0xe9, 0x9e, 0x2a } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = sizeof("filename_test");
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, "filename_test", SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = 32;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	memset(SharedMem1.buffer, 'a', SharedMem1.size);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;


	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					1,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);

	TEEC_CloseSession(&TeecSession);

	TEEC_FinalizeContext(&TeecContext);

	debug("testmm end\n");
	debug("TeecResult %x\n", TeecResult);
}

static uint8_t b2hs_add_base(uint8_t in)
{
	if (in > 9)
		return in + 55;
	else
		return in + 48;
}

uint32_t b2hs(uint8_t *b, uint8_t *hs, uint32_t blen, uint32_t hslen)
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


uint32_t trusty_read_rollback_index(uint32_t slot, uint64_t *value)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x1b484ea5, 0x698b, 0x4142,
			{ 0x82, 0xb8, 0x3a, 0xcf, 0x16, 0xe9, 0x9e, 0x2a } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};
	uint8_t hs[9];

	b2hs((uint8_t *)&slot, hs, 4, 9);
	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = 8;
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, hs, SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = 8;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					0,
					&TeecOperation,
					&ErrorOrigin);
	if (TeecResult == TEEC_SUCCESS)
		memcpy((char *)value, SharedMem1.buffer, SharedMem1.size);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);

	TEEC_CloseSession(&TeecSession);

	TEEC_FinalizeContext(&TeecContext);

	debug("testmm end\n");
	return TeecResult;
}

uint32_t trusty_write_rollback_index(uint32_t slot, uint64_t value)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x1b484ea5, 0x698b, 0x4142,
		{ 0x82, 0xb8, 0x3a, 0xcf, 0x16, 0xe9, 0x9e, 0x2a } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};
	uint8_t hs[9];
	b2hs((uint8_t *)&slot, hs, 4, 9);
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = 8;
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, hs, SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = 8;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	memcpy(SharedMem1.buffer, (char *)&value, SharedMem1.size);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;


	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					1,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);

	TEEC_CloseSession(&TeecSession);

	TEEC_FinalizeContext(&TeecContext);

	debug("testmm end\n");

	return TeecResult;
}

uint32_t trusty_read_permanent_attributes(uint8_t *attributes, uint32_t size)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x258be795, 0xf9ca, 0x40e6,
		{ 0xa8, 0x69, 0x9c, 0xe6, 0x88, 0x6c, 0x5d, 0x5d } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = sizeof("attributes");
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, "attributes", SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = size;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;


	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					142,
					&TeecOperation,
					&ErrorOrigin);
	if (TeecResult == TEEC_SUCCESS)
		memcpy(attributes, SharedMem1.buffer, SharedMem1.size);
	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);
	debug("testmm end\n");

	return TeecResult;
}

uint32_t trusty_write_permanent_attributes(uint8_t *attributes, uint32_t size)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x258be795, 0xf9ca, 0x40e6,
		{ 0xa8, 0x69, 0x9c, 0xe6, 0x88, 0x6c, 0x5d, 0x5d } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = sizeof("attributes");
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, "attributes", SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = size;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	memcpy(SharedMem1.buffer, attributes, SharedMem1.size);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;


	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					141,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);
	debug("testmm end\n");

	return TeecResult;
}

uint32_t trusty_read_lock_state(uint8_t *lock_state)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x1b484ea5, 0x698b, 0x4142,
		{ 0x82, 0xb8, 0x3a, 0xcf, 0x16, 0xe9, 0x9e, 0x2a } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);


	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = sizeof("lock_state");
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, "lock_state", SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = 1;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;


	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					0,
					&TeecOperation,
					&ErrorOrigin);
	if (TeecResult == TEEC_SUCCESS)
		memcpy(lock_state, SharedMem1.buffer, SharedMem1.size);
	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);
	debug("testmm end\n");

	return TeecResult;
}

uint32_t trusty_write_lock_state(uint8_t lock_state)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID  tempuuid = { 0x1b484ea5, 0x698b, 0x4142,
		{ 0x82, 0xb8, 0x3a, 0xcf, 0x16, 0xe9, 0x9e, 0x2a } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = sizeof("lock_state");
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, "lock_state", SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = 1;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	memcpy(SharedMem1.buffer, &lock_state, SharedMem1.size);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;


	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					1,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);
	debug("testmm end\n");

	return TeecResult;
}

uint32_t trusty_read_flash_lock_state(uint8_t *flash_lock_state)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x1b484ea5, 0x698b, 0x4142,
		{ 0x82, 0xb8, 0x3a, 0xcf, 0x16, 0xe9, 0x9e, 0x2a } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);


	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = sizeof("flash_lock_state");
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, "flash_lock_state", SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = 1;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;


	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					0,
					&TeecOperation,
					&ErrorOrigin);
	if (TeecResult == TEEC_SUCCESS)
		memcpy(flash_lock_state, SharedMem1.buffer, SharedMem1.size);
	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);
	debug("testmm end\n");

	return TeecResult;
}

uint32_t trusty_write_flash_lock_state(uint8_t flash_lock_state)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID  tempuuid = { 0x1b484ea5, 0x698b, 0x4142,
		{ 0x82, 0xb8, 0x3a, 0xcf, 0x16, 0xe9, 0x9e, 0x2a } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = sizeof("flash_lock_state");
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, "flash_lock_state", SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = 1;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	memcpy(SharedMem1.buffer, &flash_lock_state, SharedMem1.size);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;


	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					1,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);
	debug("testmm end\n");

	return TeecResult;
}

uint32_t write_to_keymaster(uint8_t *filename,
		uint32_t filename_size,
		uint8_t *data,
		uint32_t data_size)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;

	TEEC_UUID tempuuid = { 0x258be795, 0xf9ca, 0x40e6,
		{ 0xa8, 0x69, 0x9c, 0xe6, 0x88, 0x6c, 0x5d, 0x5d } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("write_to_keymaster\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = filename_size;
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, filename, SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = data_size;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	memcpy(SharedMem1.buffer, data, SharedMem1.size);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;


	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					141,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);
	debug("testmm end\n");
	debug("TeecResult %x\n", TeecResult);

	return TeecResult;
}

uint32_t trusty_read_attribute_hash(uint32_t *buf, uint32_t length)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;

	TEEC_UUID tempuuid = { 0x2d26d8a8, 0x5134, 0x4dd8, \
			{ 0xb3, 0x2f, 0xb3, 0x4b, 0xce, 0xeb, 0xc4, 0x71 } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = length * sizeof(uint32_t);
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					0,
					&TeecOperation,
					&ErrorOrigin);

	if (TeecResult == TEEC_SUCCESS)
		memcpy(buf, SharedMem0.buffer, SharedMem0.size);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);

	return TeecResult;
}

uint32_t trusty_write_attribute_hash(uint32_t *buf, uint32_t length)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;

	TEEC_UUID tempuuid = { 0x2d26d8a8, 0x5134, 0x4dd8, \
			{ 0xb3, 0x2f, 0xb3, 0x4b, 0xce, 0xeb, 0xc4, 0x71 } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = length * sizeof(uint32_t);
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, buf, SharedMem0.size);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					1,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);

	return TeecResult;
}

uint32_t notify_optee_rpmb_ta(void)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID  tempuuid = { 0x1b484ea5, 0x698b, 0x4142,
		{ 0x82, 0xb8, 0x3a, 0xcf, 0x16, 0xe9, 0x9e, 0x2a } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					2,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);

	return TeecResult;
}

uint32_t notify_optee_efuse_ta(void)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x2d26d8a8, 0x5134, 0x4dd8, \
			{ 0xb3, 0x2f, 0xb3, 0x4b, 0xce, 0xeb, 0xc4, 0x71 } };

	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					2,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);

	return TeecResult;
}

uint32_t trusty_notify_optee_uboot_end(void)
{
	TEEC_Result res;
	res = notify_optee_rpmb_ta();
	res |= notify_optee_efuse_ta();
	return res;
}

uint32_t trusty_read_vbootkey_hash(uint32_t *buf, uint32_t length)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;

	TEEC_UUID tempuuid = { 0x2d26d8a8, 0x5134, 0x4dd8, \
			{ 0xb3, 0x2f, 0xb3, 0x4b, 0xce, 0xeb, 0xc4, 0x71 } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = length * sizeof(uint32_t);
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					3,
					&TeecOperation,
					&ErrorOrigin);

	if (TeecResult == TEEC_SUCCESS)
		memcpy(buf, SharedMem0.buffer, SharedMem0.size);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);

	return TeecResult;
}
uint32_t trusty_write_vbootkey_hash(uint32_t *buf, uint32_t length)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;

	TEEC_UUID tempuuid = { 0x2d26d8a8, 0x5134, 0x4dd8, \
			{ 0xb3, 0x2f, 0xb3, 0x4b, 0xce, 0xeb, 0xc4, 0x71 } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = length * sizeof(uint32_t);
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, buf, SharedMem0.size);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					4,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);

	return TeecResult;
}

uint32_t trusty_read_vbootkey_enable_flag(uint8_t *flag)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	uint32_t bootflag;

	TEEC_UUID tempuuid = { 0x2d26d8a8, 0x5134, 0x4dd8, \
			{ 0xb3, 0x2f, 0xb3, 0x4b, 0xce, 0xeb, 0xc4, 0x71 } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = 1 * sizeof(uint32_t);
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
						TEEC_NONE,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					5,
					&TeecOperation,
					&ErrorOrigin);

	if (TeecResult == TEEC_SUCCESS) {
		memcpy(&bootflag, SharedMem0.buffer, SharedMem0.size);
		if (bootflag == 0x000000FF)
			*flag = 1;
	}

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);

	return TeecResult;
}

uint32_t trusty_read_permanent_attributes_flag(uint8_t *attributes)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x258be795, 0xf9ca, 0x40e6,
		{ 0xa8, 0x69, 0x9c, 0xe6, 0x88, 0x6c, 0x5d, 0x5d } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = sizeof("attributes_flag");
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, "attributes_flag", SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = 1;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					142,
					&TeecOperation,
					&ErrorOrigin);
	if (TeecResult == TEEC_SUCCESS)
		memcpy(attributes, SharedMem1.buffer, SharedMem1.size);
	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);
	debug("testmm end\n");

	return TeecResult;
}

uint32_t trusty_write_permanent_attributes_flag(uint8_t attributes)
{
	TEEC_Result TeecResult;
	TEEC_Context TeecContext;
	TEEC_Session TeecSession;
	uint32_t ErrorOrigin;
	TEEC_UUID tempuuid = { 0x258be795, 0xf9ca, 0x40e6,
		{ 0xa8, 0x69, 0x9c, 0xe6, 0x88, 0x6c, 0x5d, 0x5d } };
	TEEC_UUID *TeecUuid = &tempuuid;
	TEEC_Operation TeecOperation = {0};

	debug("testmm start\n");
	OpteeClientApiLibInitialize();

	TeecResult = TEEC_InitializeContext(NULL, &TeecContext);

	TeecResult = TEEC_OpenSession(&TeecContext,
				&TeecSession,
				TeecUuid,
				TEEC_LOGIN_PUBLIC,
				NULL,
				NULL,
				&ErrorOrigin);

	TEEC_SharedMemory SharedMem0 = {0};

	SharedMem0.size = sizeof("attributes_flag");
	SharedMem0.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem0);

	memcpy(SharedMem0.buffer, "attributes_flag", SharedMem0.size);

	TEEC_SharedMemory SharedMem1 = {0};

	SharedMem1.size = 1;
	SharedMem1.flags = 0;

	TeecResult = TEEC_AllocateSharedMemory(&TeecContext, &SharedMem1);

	memcpy(SharedMem1.buffer, (char *)&attributes, SharedMem1.size);

	TeecOperation.params[0].tmpref.buffer = SharedMem0.buffer;
	TeecOperation.params[0].tmpref.size = SharedMem0.size;

	TeecOperation.params[1].tmpref.buffer = SharedMem1.buffer;
	TeecOperation.params[1].tmpref.size = SharedMem1.size;

	TeecOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
						TEEC_MEMREF_TEMP_INOUT,
						TEEC_NONE,
						TEEC_NONE);

	TeecResult = TEEC_InvokeCommand(&TeecSession,
					141,
					&TeecOperation,
					&ErrorOrigin);

	TEEC_ReleaseSharedMemory(&SharedMem0);
	TEEC_ReleaseSharedMemory(&SharedMem1);
	TEEC_CloseSession(&TeecSession);
	TEEC_FinalizeContext(&TeecContext);
	debug("testmm end\n");

	return TeecResult;
}
