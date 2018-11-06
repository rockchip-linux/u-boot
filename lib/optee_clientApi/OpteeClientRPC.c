/*
 * Copyright 2017, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <stdlib.h>
#include <command.h>
#include <mmc.h>
#include <optee_include/OpteeClientMem.h>
#include <optee_include/OpteeClientRPC.h>
#include <optee_include/teesmc.h>
#include <optee_include/teesmc_v2.h>
#include <optee_include/teesmc_optee.h>
#include <optee_include/tee_rpc_types.h>
#include <optee_include/tee_rpc.h>
#include <optee_include/OpteeClientRkFs.h>

/*
 * Memory allocation.
 * Currently treated the same for both arguments & payloads.
 */
TEEC_Result OpteeRpcAlloc(uint32_t Size, uint32_t *Address)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;
	size_t AllocAddress;

	*Address = 0;

	if (Size != 0) {
		AllocAddress = (size_t) OpteeClientMemAlloc(Size);

		if (AllocAddress == 0)
			TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
		else
			*Address = AllocAddress;
	}
	return TeecResult;
}

/*
 * Memory free.
 * Currently treated the same for both arguments & payloads.
 */
TEEC_Result OpteeRpcFree(uint32_t Address)
{
	OpteeClientMemFree((void *)(size_t)Address);
	return TEEC_SUCCESS;
}

int is_uuid_equal(TEE_UUID uuid1, TEEC_UUID uuid2)
{
	bool a, b, c;

	a = (uuid1.timeLow == uuid2.timeLow);
	b = (uuid1.timeMid == uuid2.timeMid);
	c = (uuid1.timeHiAndVersion == uuid2.timeHiAndVersion);
	if ((a & b & c) == 0) {
		return 0;
	} else {
		if (memcmp(uuid1.clockSeqAndNode,
			   uuid2.clockSeqAndNode, 8) == 0)
			return 1;
		else
			return 0;
	}
}

/*
 * Load a TA from storage into memory and provide it back to OpTEE.
 * Param[0] = IN: struct tee_rpc_load_ta_cmd
 * Param[1] = IN: all-zero OUT: TA Image allocated
 */
TEEC_Result OpteeRpcCmdLoadTa(t_teesmc32_arg *TeeSmc32Arg)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;
	t_teesmc32_param *TeeSmc32Param = NULL;
	struct tee_rpc_load_ta_cmd *TeeLoadTaCmd = NULL;
	uint32_t TeeLoadTaCmdSize = 0;
	TEEC_UUID TA_RK_KEYMASTER_UUID = {0x258be795, 0xf9ca, 0x40e6,
			{0xa8, 0x69, 0x9c, 0xe6, 0x88, 0x6c, 0x5d, 0x5d}};

	if (TeeSmc32Arg->num_params != 2) {
		TeecResult = TEEC_ERROR_BAD_PARAMETERS;
		goto Exit;
	}

	TeeSmc32Param = TEESMC32_GET_PARAMS(TeeSmc32Arg);
	TeeLoadTaCmd = (struct tee_rpc_load_ta_cmd *)
					(size_t)TeeSmc32Param[0].u.memref.buf_ptr;
	TeeLoadTaCmdSize = TeeSmc32Param[0].u.memref.size;

	if ((TeeLoadTaCmd == NULL) ||
		(TeeLoadTaCmdSize != sizeof(*TeeLoadTaCmd))) {
		TeecResult = TEEC_ERROR_BAD_PARAMETERS;
		goto Exit;
	}

	TEEC_Result Status = 0;
	void *ImageData = NULL;
	uint32_t ImageSize = 0;
	size_t AllocAddress = 0;

	if (is_uuid_equal(TeeLoadTaCmd->uuid, TA_RK_KEYMASTER_UUID)) {
		ImageData = (void *)0;
		ImageSize = 0;
	} else {
		ImageData = (void *)0;
		ImageSize = 0;
	}

	if (Status != 0) {
		TeecResult = TEEC_ERROR_ITEM_NOT_FOUND;
		goto Exit;
	}

	AllocAddress = (size_t) OpteeClientMemAlloc(ImageSize);

	if (AllocAddress == 0) {
		TeecResult = TEEC_ERROR_OUT_OF_MEMORY;
		goto Exit;
	}

	memcpy((void *)AllocAddress, ImageData, ImageSize);

	debug("...TA loaded at 0x%zu of size 0x%X bytes\n",
		AllocAddress, ImageSize);
	debug("...AllocAddress[0] 0x%X ; AllocAddress[1] 0x%X bytes\n",
		*(char *)AllocAddress, *(char *)(AllocAddress+1));

	TeeLoadTaCmd->va = AllocAddress;

	TeeSmc32Param[1].u.memref.buf_ptr = AllocAddress;
	TeeSmc32Param[1].u.memref.size = ImageSize;

Exit:
	TeeSmc32Arg->ret = TeecResult;
	TeeSmc32Arg->ret_origin = TEEC_ORIGIN_API;

	debug("OpteeRpcCmdLoadTa Exit : TeecResult=0x%X\n", TeecResult);

	return TeecResult;
}

#if defined CONFIG_OPTEE_V2
TEEC_Result OpteeRpcCmdLoadV2Ta(t_teesmc32_arg *TeeSmc32Arg)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;
	t_teesmc32_param *TeeSmc32Param = NULL;
	uint8_t uuid[16];
	int i;

	if (TeeSmc32Arg->num_params != 2) {
		TeecResult = TEEC_ERROR_BAD_PARAMETERS;
		goto Exit;
	}

	TeeSmc32Param = TEESMC32_GET_PARAMS(TeeSmc32Arg);

	memcpy(uuid, (void *)&TeeSmc32Param[0].u.value, 16);
	for (i = 0; i < 16; i++)
		printf("uuid 0x%x", uuid[i]);

	if (TeeSmc32Param[1].u.memref.buf_ptr == 0) {
		printf("return size of TA, keymaster_size = 0");
		TeeSmc32Param[1].u.memref.size = 0;
	} else {
		/*memcpy((void *)TeeSmc32Param[1].u.memref.buf_ptr,
			(void *)keymaster_data, TeeSmc32Param[1].u.memref.size);*/
		printf("memref.buf_ptr = 0x%llx; memref.size = 0x%llx",
			TeeSmc32Param[1].u.memref.buf_ptr,
			TeeSmc32Param[1].u.memref.size);
	}

Exit:
	TeeSmc32Arg->ret = TeecResult;
	TeeSmc32Arg->ret_origin = TEEC_ORIGIN_API;

	debug("OpteeRpcCmdLoadTa Exit : TeecResult=0x%X\n", TeecResult);

	return TeecResult;
}
#endif

/*
 * Free a previously loaded TA and release the memory
 * Param[0] = IN: TA Image to free
 *
 * Um, why is OpTEE holding on to this memory? The OS code suggests that OpTEE
 * is using the binary in place out of shared memory but I don't understand how
 * runtime modifications of the binary are being prevented if that's the case?
 */
TEEC_Result OpteeRpcCmdFreeTa(t_teesmc32_arg *TeeSmc32Arg)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;
	t_teesmc32_param *TeeSmc32Param = NULL;
	uint32_t ImageSize = 0;
	size_t AllocAddress = 0;

	if (TeeSmc32Arg->num_params != 1) {
		TeecResult = TEEC_ERROR_BAD_PARAMETERS;
		goto Exit;
	}

	TeeSmc32Param = TEESMC32_GET_PARAMS(TeeSmc32Arg);

	AllocAddress = TeeSmc32Param[0].u.memref.buf_ptr;
	ImageSize = TeeSmc32Param[0].u.memref.size;

	debug("OpteeRpcCmdFreeTa Enter: AllocAddress=0x%X, ImageSize=0x%X\n",
			(uint32_t) AllocAddress, (uint32_t) ImageSize);

	if (AllocAddress == 0) {
		TeecResult = TEEC_ERROR_BAD_PARAMETERS;
		goto Exit;
	}

	OpteeClientMemFree((void *)AllocAddress);

Exit:
	TeeSmc32Arg->ret = TeecResult;
	TeeSmc32Arg->ret_origin = TEEC_ORIGIN_API;

	debug("OpteeRpcCmdFreeTa Exit : TeecResult=0x%X\n", TeecResult);

	return TeecResult;
}

/*
 * Execute an RPMB storage operation.
 */

uint16_t global_block_count;
TEEC_Result OpteeRpcCmdRpmb(t_teesmc32_arg *TeeSmc32Arg)
{
	struct tee_rpc_rpmb_dev_info *DevInfo;
	TEEC_Result EfiStatus;
	uint16_t RequestMsgType, i;
	EFI_RK_RPMB_DATA_PACKET *RequestPackets;
	EFI_RK_RPMB_DATA_PACKET *ResponsePackets;
	EFI_RK_RPMB_DATA_PACKET *tempPackets;
	EFI_RK_RPMB_DATA_PACKET_BACK *RequestPackets_back;
	EFI_RK_RPMB_DATA_PACKET_BACK *tempPackets_back;
	struct tee_rpc_rpmb_cmd *RpmbRequest;
	TEEC_Result TeecResult = TEEC_SUCCESS;
	t_teesmc32_param *TeeSmc32Param;
	struct mmc *mmc;

	debug("Entered RPMB RPC\n");

	if (TeeSmc32Arg->num_params != 2) {
		TeecResult = TEEC_ERROR_BAD_PARAMETERS;
		goto Exit;
	}

	TeeSmc32Param = TEESMC32_GET_PARAMS(TeeSmc32Arg);
	RpmbRequest = (struct tee_rpc_rpmb_cmd *)(size_t)
		TeeSmc32Param[0].u.memref.buf_ptr;
	switch (RpmbRequest->cmd) {
	case TEE_RPC_RPMB_CMD_DATA_REQ: {
		RequestPackets = (EFI_RK_RPMB_DATA_PACKET *)(RpmbRequest + 1);
		ResponsePackets = (EFI_RK_RPMB_DATA_PACKET *)(size_t)
		TeeSmc32Param[1].u.memref.buf_ptr;

		global_block_count =
			(RpmbRequest->block_count == 0 ?
			1 : RpmbRequest->block_count);
		RequestPackets_back =
			memalign(ARCH_DMA_MINALIGN, sizeof(EFI_RK_RPMB_DATA_PACKET_BACK)
			* global_block_count);
		memcpy(RequestPackets_back->stuff,
			RequestPackets->stuff_bytes,
			RPMB_STUFF_DATA_SIZE);
		memcpy(RequestPackets_back->mac,
			RequestPackets->key_mac,
			RPMB_KEY_MAC_SIZE);
		memcpy(RequestPackets_back->data,
			RequestPackets->data,
			RPMB_DATA_SIZE);
		memcpy(RequestPackets_back->nonce,
			RequestPackets->nonce,
			RPMB_NONCE_SIZE);
		RequestPackets_back->write_counter =
			((RequestPackets->write_counter[3]) << 24) +
			((RequestPackets->write_counter[2]) << 16) +
			((RequestPackets->write_counter[1]) << 8) +
			(RequestPackets->write_counter[0]);
		RequestPackets_back->address =
			((RequestPackets->address[1]) << 8) +
			(RequestPackets->address[0]);
		RequestPackets_back->block_count =
			((RequestPackets->block_count[1]) << 8) +
			(RequestPackets->block_count[0]);
		RequestPackets_back->result =
			((RequestPackets->op_result[1]) << 8) +
			(RequestPackets->op_result[0]);
		RequestPackets_back->request =
			((RequestPackets->msg_type[1]) << 8) +
			(RequestPackets->msg_type[0]);

		RequestMsgType = RPMB_PACKET_DATA_TO_UINT16(
				RequestPackets->msg_type);

		debug("RPMB Data request %d\n", RequestMsgType);

		switch (RequestMsgType) {
		case TEE_RPC_RPMB_MSG_TYPE_REQ_AUTH_KEY_PROGRAM: {
			EfiStatus = init_rpmb();
			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			EfiStatus = do_programkey((struct s_rpmb *)
				RequestPackets_back);

			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			EfiStatus = finish_rpmb();
			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			break;
		}

		case TEE_RPC_RPMB_MSG_TYPE_REQ_WRITE_COUNTER_VAL_READ: {
			EfiStatus = init_rpmb();
			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			EfiStatus = do_readcounter((struct s_rpmb *)
				RequestPackets_back);
			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			EfiStatus = finish_rpmb();
			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			break;
		}

		case TEE_RPC_RPMB_MSG_TYPE_REQ_AUTH_DATA_WRITE: {
			EfiStatus = init_rpmb();
			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			EfiStatus = do_authenticatedwrite((struct s_rpmb *)
				RequestPackets_back);
			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			EfiStatus = finish_rpmb();

			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			break;
		}

		case TEE_RPC_RPMB_MSG_TYPE_REQ_AUTH_DATA_READ: {
			EfiStatus = init_rpmb();
			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			EfiStatus = do_authenticatedread((struct s_rpmb *)
				RequestPackets_back, global_block_count);
			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			EfiStatus = finish_rpmb();

			if (EfiStatus != 0) {
				TeecResult = TEEC_ERROR_GENERIC;
				break;
			}

			break;
		}

		default:
			TeecResult = TEEC_ERROR_BAD_PARAMETERS;
			break;
		}
		debug("RPMB TeecResult %d\n", TeecResult);
		break;
	}

	case TEE_RPC_RPMB_CMD_GET_DEV_INFO: {
		mmc = do_returnmmc();

		DevInfo = (struct tee_rpc_rpmb_dev_info *)(size_t)
		TeeSmc32Param[1].u.memref.buf_ptr;

		DevInfo->cid[0] = (mmc->cid[0]) >> 24 & 0xff;
		DevInfo->cid[1] = (mmc->cid[0]) >> 16 & 0xff;
		DevInfo->cid[2] = (mmc->cid[0]) >> 8 & 0xff;
		DevInfo->cid[3] = (mmc->cid[0]) & 0xff;
		DevInfo->cid[4] = (mmc->cid[1]) >> 24 & 0xff;
		DevInfo->cid[5] = (mmc->cid[1]) >> 16 & 0xff;
		DevInfo->cid[6] = (mmc->cid[1]) >> 8 & 0xff;
		DevInfo->cid[7] = (mmc->cid[1]) & 0xff;
		DevInfo->cid[8] = (mmc->cid[2]) >> 24 & 0xff;
		DevInfo->cid[9] = (mmc->cid[2]) >> 16 & 0xff;
		DevInfo->cid[10] = (mmc->cid[2]) >> 8 & 0xff;
		DevInfo->cid[11] = (mmc->cid[2]) & 0xff;
		DevInfo->cid[12] = (mmc->cid[3]) >> 24 & 0xff;
		DevInfo->cid[13] = (mmc->cid[3]) >> 16 & 0xff;
		DevInfo->cid[14] = (mmc->cid[3]) >> 8 & 0xff;
		DevInfo->cid[15] = (mmc->cid[3]) & 0xff;
		DevInfo->rel_wr_sec_c = 1;
		DevInfo->rpmb_size_mult =
			(uint8_t)(mmc->capacity_rpmb / (128 * 1024));
		DevInfo->ret_code = 0;

		goto Exit;
	}

	default:
		TeecResult = TEEC_ERROR_BAD_PARAMETERS;

		goto Exit;
	}

	tempPackets = ResponsePackets;
	tempPackets_back = RequestPackets_back;

	for (i = 0; i < global_block_count; i++) {
		memcpy(tempPackets->stuff_bytes,
			tempPackets_back->stuff,
			RPMB_STUFF_DATA_SIZE);
		memcpy(tempPackets->key_mac,
			tempPackets_back->mac,
			RPMB_KEY_MAC_SIZE);
		memcpy(tempPackets->data,
			tempPackets_back->data,
			RPMB_DATA_SIZE);
		memcpy(tempPackets->nonce,
			tempPackets_back->nonce,
			RPMB_NONCE_SIZE);
		tempPackets->write_counter[3] =
			((tempPackets_back->write_counter) >> 24) & 0xFF;
		tempPackets->write_counter[2] =
			((tempPackets_back->write_counter) >> 16) & 0xFF;
		tempPackets->write_counter[1] =
			((tempPackets_back->write_counter) >> 8) & 0xFF;
		tempPackets->write_counter[0] =
			(tempPackets_back->write_counter) & 0xFF;
		tempPackets->address[1] =
			((tempPackets_back->address) >> 8) & 0xFF;
		tempPackets->address[0] =
			(tempPackets_back->address) & 0xFF;
		tempPackets->block_count[1] =
			((tempPackets_back->block_count) >> 8) & 0xFF;
		tempPackets->block_count[0] =
			(tempPackets_back->block_count) & 0xFF;
		tempPackets->op_result[1] =
			((tempPackets_back->result) >> 8) & 0xFF;
		tempPackets->op_result[0] =
			(tempPackets_back->result) & 0xFF;
		tempPackets->msg_type[1] =
			((tempPackets_back->request) >> 8) & 0xFF;
		tempPackets->msg_type[0] =
			(tempPackets_back->request) & 0xFF;
		tempPackets++;
		tempPackets_back++;
	}

	free(RequestPackets_back);

Exit:
	TeeSmc32Arg->ret = TeecResult;
	TeeSmc32Arg->ret_origin = TEEC_ORIGIN_API;

	return TeecResult;
}

/*
 * Execute a normal world local file system operation.
 */
TEEC_Result OpteeRpcCmdFs(t_teesmc32_arg *TeeSmc32Arg)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;
	t_teesmc32_param *TeeSmc32Param;

	TeeSmc32Param = TEESMC32_GET_PARAMS(TeeSmc32Arg);
	TeecResult = tee_supp_rk_fs_process((void *)(size_t)TeeSmc32Param[0].u.memref.buf_ptr,
							TeeSmc32Param[0].u.memref.size);

	return TeecResult;
}

/*
 * TBD.
 */
TEEC_Result OpteeRpcCmdGetTime(t_teesmc32_arg *TeeSmc32Arg)
{
	return TEEC_ERROR_NOT_IMPLEMENTED;
}

/*
 * TBD.
 */
TEEC_Result OpteeRpcCmdWaitMutex(t_teesmc32_arg *TeeSmc32Arg)
{
	return TEEC_ERROR_NOT_IMPLEMENTED;
}

/*
 * Handle the callback from secure world.
 */
TEEC_Result OpteeRpcCallback(ARM_SMC_ARGS *ArmSmcArgs)
{
	TEEC_Result TeecResult = TEEC_SUCCESS;

	//printf("OpteeRpcCallback Enter: Arg0=0x%X, Arg1=0x%X, Arg2=0x%X\n",
		//ArmSmcArgs->Arg0, ArmSmcArgs->Arg1, ArmSmcArgs->Arg2);

	switch (TEESMC_RETURN_GET_RPC_FUNC(ArmSmcArgs->Arg0)) {
	case TEESMC_RPC_FUNC_ALLOC_ARG: {
#if defined CONFIG_OPTEE_V1
		TeecResult = OpteeRpcAlloc(ArmSmcArgs->Arg1, &ArmSmcArgs->Arg1);
#endif
#if defined CONFIG_OPTEE_V2
		printf("ArmSmcArgs->Arg1 = 0x%x", ArmSmcArgs->Arg1);
		TeecResult = OpteeRpcAlloc(ArmSmcArgs->Arg1, &ArmSmcArgs->Arg2);
		ArmSmcArgs->Arg5 = ArmSmcArgs->Arg2;
		ArmSmcArgs->Arg1 = 0;
		ArmSmcArgs->Arg4 = 0;
#endif
		break;
	}

	case TEESMC_RPC_FUNC_ALLOC_PAYLOAD: {
		TeecResult = OpteeRpcAlloc(ArmSmcArgs->Arg1, &ArmSmcArgs->Arg1);
		break;
	}

	case TEESMC_RPC_FUNC_FREE_ARG: {
#if defined CONFIG_OPTEE_V1
		TeecResult = OpteeRpcFree(ArmSmcArgs->Arg1);
#endif
#if defined CONFIG_OPTEE_V2
		TeecResult = OpteeRpcFree(ArmSmcArgs->Arg2);
#endif
		break;
	}

	case TEESMC_RPC_FUNC_FREE_PAYLOAD: {
		TeecResult = OpteeRpcFree(ArmSmcArgs->Arg1);
		break;
	}

	case TEESMC_RPC_FUNC_IRQ: {
		break;
	}

	case TEESMC_RPC_FUNC_CMD: {
#if defined CONFIG_OPTEE_V1
		t_teesmc32_arg *TeeSmc32Arg =
			(t_teesmc32_arg *)(size_t)ArmSmcArgs->Arg1;
#endif
#if defined CONFIG_OPTEE_V2
		t_teesmc32_arg *TeeSmc32Arg =
			(t_teesmc32_arg *)((size_t)ArmSmcArgs->Arg1 << 32 | ArmSmcArgs->Arg2);
		printf("TeeSmc32Arg->cmd = 0x%x", TeeSmc32Arg->cmd);
#endif
		switch (TeeSmc32Arg->cmd) {
#if defined CONFIG_OPTEE_V1
		case TEE_RPC_LOAD_TA: {
			TeecResult = OpteeRpcCmdLoadTa(TeeSmc32Arg);
			break;
		}

		case TEE_RPC_FREE_TA: {
			TeecResult = OpteeRpcCmdFreeTa(TeeSmc32Arg);
			break;
		}

		case TEE_RPC_RPMB_CMD: {
			TeecResult = OpteeRpcCmdRpmb(TeeSmc32Arg);
			break;
		}

		case TEE_RPC_FS: {
			TeecResult = OpteeRpcCmdFs(TeeSmc32Arg);
			TeeSmc32Arg->ret = TEEC_SUCCESS;
			break;
		}

		case TEE_RPC_GET_TIME: {
			TeecResult = OpteeRpcCmdGetTime(TeeSmc32Arg);
			break;
		}

		case TEE_RPC_WAIT_MUTEX: {
			TeecResult = OpteeRpcCmdWaitMutex(TeeSmc32Arg);
			break;
		}
#endif
#if defined CONFIG_OPTEE_V2
		case OPTEE_MSG_RPC_CMD_SHM_ALLOC_V2: {
			uint32_t tempaddr;
			uint32_t allocsize = TeeSmc32Arg->params[0].u.value.b;
			TeecResult = OpteeRpcAlloc(allocsize, &tempaddr);
			printf("allocsize = 0x%x tempaddr = 0x%x", allocsize, tempaddr);
			TeeSmc32Arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT_V2;
			TeeSmc32Arg->params[0].u.memref.buf_ptr = tempaddr;
			TeeSmc32Arg->params[0].u.memref.size = allocsize;
			TeeSmc32Arg->params[0].u.memref.shm_ref = tempaddr;
			TeeSmc32Arg->ret = TEE_SUCCESS;
			break;
		}
		case OPTEE_MSG_RPC_CMD_SHM_FREE_V2: {
			uint32_t tempaddr = TeeSmc32Arg->params[0].u.value.b;
			TeecResult = OpteeRpcFree(tempaddr);
			break;

		}
		case OPTEE_MSG_RPC_CMD_RPMB_V2: {
			TeecResult = OpteeRpcCmdRpmb(TeeSmc32Arg);
			break;
		}
		case OPTEE_MSG_RPC_CMD_LOAD_TA_V2: {
			TeecResult = OpteeRpcCmdLoadV2Ta(TeeSmc32Arg);
			break;
		}
#endif

		default: {
			printf("...unsupported RPC CMD: cmd=0x%X\n",
				TeeSmc32Arg->cmd);
			TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
			break;
		}
	}

		break;
	}

	case TEESMC_OPTEE_RPC_FUNC_ALLOC_PAYLOAD: {
		TeecResult = OpteeRpcAlloc(ArmSmcArgs->Arg1, &ArmSmcArgs->Arg1);
		ArmSmcArgs->Arg2 = ArmSmcArgs->Arg1;
		break;
	}

	case TEESMC_OPTEE_RPC_FUNC_FREE_PAYLOAD: {
		TeecResult = OpteeRpcFree(ArmSmcArgs->Arg1);
		break;
	}

	default: {
		printf("...unsupported RPC : Arg0=0x%X\n", ArmSmcArgs->Arg0);
		TeecResult = TEEC_ERROR_NOT_IMPLEMENTED;
		break;
	}
	}

	ArmSmcArgs->Arg0 = TEESMC32_CALL_RETURN_FROM_RPC;
	debug("OpteeRpcCallback Exit : TeecResult=0x%X\n", TeecResult);

	return TeecResult;
}
