// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2018, Linaro Limited
 */

#include <log.h>
#include <malloc.h>
#include <tee.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/types.h>

#include "optee_msg.h"
#include "optee_msg_supplicant.h"
#include "optee_private.h"
#include "optee_smc.h"

static void cmd_shm_alloc(struct udevice *dev, struct optee_msg_arg *arg,
			  void **page_list)
{
	int rc;
	struct tee_shm *shm;
	void *pl;
	u64 ph_ptr;

	arg->ret_origin = TEE_ORIGIN_COMMS;

	if (arg->num_params != 1 ||
	    arg->params[0].attr != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT) {
		arg->ret = TEE_ERROR_BAD_PARAMETERS;
		return;
	}

	if (optee_is_support_dynamic_shm(dev)) {
		rc = __tee_shm_add(dev, 0, NULL, arg->params[0].u.value.b,
				   TEE_SHM_REGISTER | TEE_SHM_ALLOC, &shm);
		if (rc) {
			if (rc == -ENOMEM)
				arg->ret = TEE_ERROR_OUT_OF_MEMORY;
			else
				arg->ret = TEE_ERROR_GENERIC;
			return;
		}

		pl = optee_alloc_and_init_page_list(shm->addr, shm->size, &ph_ptr);
		if (!pl) {
			arg->ret = TEE_ERROR_OUT_OF_MEMORY;
			tee_shm_free(shm);
			return;
		}

		*page_list = pl;
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT |
				      OPTEE_MSG_ATTR_NONCONTIG;
		arg->params[0].u.tmem.buf_ptr = ph_ptr;
		arg->params[0].u.tmem.size = shm->size;
		arg->params[0].u.tmem.shm_ref = (ulong)shm;
	} else {
		rc = __tee_shm_add(dev, 0, NULL, arg->params[0].u.value.b,
				   TEE_SHM_RES_ALLOC, &shm);
		if (rc) {
			if (rc == -ENOMEM)
				arg->ret = TEE_ERROR_OUT_OF_MEMORY;
			else
				arg->ret = TEE_ERROR_GENERIC;
			return;
		}
		arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT;
		arg->params[0].u.tmem.buf_ptr = virt_to_phys(shm->addr);
		arg->params[0].u.tmem.size = shm->size;
		arg->params[0].u.tmem.shm_ref = (ulong)shm;
	}

	arg->ret = TEE_SUCCESS;
}

static void cmd_shm_free(struct optee_msg_arg *arg)
{
	arg->ret_origin = TEE_ORIGIN_COMMS;

	if (arg->num_params != 1 ||
	    arg->params[0].attr != OPTEE_MSG_ATTR_TYPE_VALUE_INPUT) {
		arg->ret = TEE_ERROR_BAD_PARAMETERS;
		return;
	}

	tee_shm_free((struct tee_shm *)(ulong)arg->params[0].u.value.b);
	arg->ret = TEE_SUCCESS;
}

bool tee_supp_param_is_value(struct optee_msg_param *param)
{
	switch (param->attr & TEE_PARAM_ATTR_TYPE_MASK) {
	case OPTEE_MSG_ATTR_TYPE_VALUE_INPUT:
	case OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT:
	case OPTEE_MSG_ATTR_TYPE_VALUE_INOUT:
		return true;
	default:
		return false;
	}
}

void *tee_supp_param_to_va(struct optee_msg_param *param)
{
	struct tee_shm *shm = NULL;

	switch (param->attr & TEE_PARAM_ATTR_TYPE_MASK) {
	case OPTEE_MSG_ATTR_TYPE_RMEM_INPUT:
	case OPTEE_MSG_ATTR_TYPE_RMEM_OUTPUT:
	case OPTEE_MSG_ATTR_TYPE_RMEM_INOUT:
		shm = (struct tee_shm *)(size_t)param->u.rmem.shm_ref;
		if (!shm)
			return NULL;
		return (uint8_t *)shm->addr + param->u.rmem.offs;
	case OPTEE_MSG_ATTR_TYPE_TMEM_INPUT:
	case OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT:
	case OPTEE_MSG_ATTR_TYPE_TMEM_INOUT:
		shm = (struct tee_shm *)(size_t)param->u.tmem.shm_ref;
		if (!shm)
			return NULL;
		return (uint8_t *)shm->addr;
	default:
		return NULL;
	}
}

static bool is_ufs_device(void)
{
	struct blk_desc *desc = plat_bootdev();

	if (desc->uclass_id == UCLASS_SCSI && desc->rawblksz == 4096)//ufs
		return true;
	else
		return false;
}

void optee_suppl_cmd(struct udevice *dev, struct tee_shm *shm_arg,
		     void **page_list)
{
	struct optee_msg_arg *arg = shm_arg->addr;

	switch (arg->cmd) {
	case OPTEE_MSG_RPC_CMD_SHM_ALLOC:
		cmd_shm_alloc(dev, arg, page_list);
		break;
	case OPTEE_MSG_RPC_CMD_SHM_FREE:
		cmd_shm_free(arg);
		break;
	case OPTEE_MSG_RPC_CMD_FS:
#ifdef CONFIG_ARCH_ROCKCHIP
		optee_suppl_cmd_fs(arg);
#else
		debug("REE FS storage isn't available\n");
		arg->ret = TEE_ERROR_STORAGE_NOT_AVAILABLE;
#endif
		break;
	case OPTEE_MSG_RPC_CMD_RPMB:
		if (is_ufs_device())
			optee_suppl_cmd_ufs_rpmb(dev, arg);
		else
			optee_suppl_cmd_rpmb(dev, arg);
		break;
	case OPTEE_MSG_RPC_CMD_I2C_TRANSFER:
		optee_suppl_cmd_i2c_transfer(arg);
		break;
#ifdef CONFIG_ARCH_ROCKCHIP
	case OPTEE_MSG_RPC_CMD_LOAD_TA:
		optee_suppl_cmd_load_ta(arg);
		break;
#endif
	default:
		arg->ret = TEE_ERROR_NOT_IMPLEMENTED;
	}

	arg->ret_origin = TEE_ORIGIN_COMMS;
}
