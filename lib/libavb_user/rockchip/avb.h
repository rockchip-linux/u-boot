/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef AVB_H_
#define AVB_H_

#include "../../libavb/libavb.h"
#include "../../libavb_ab/libavb_ab.h"
#include "../../libavb_atx/libavb_atx.h"
#include "../../libavb_user/libavb_user.h"

#ifdef __cplusplus
extern "C" {
#endif

/* rk used */
#define PERM_ATTR_DIGEST_SIZE		32
#define PERM_ATTR_TOTAL_SIZE		1052
#define VBOOT_KEY_HASH_SIZE		32
#define ANDROID_VBOOT_LOCK		0
#define ANDROID_VBOOT_UNLOCK		1
#define VBMETA_MAX_SIZE			65536
#define ROLLBACK_MAX_SIZE		20
#define LOCK_MASK			(1 << 0)
#define UNLOCK_DISABLE_MASK		(1 << 1)
#define VBOOT_STATE_SIZE		1000
#define PERM_ATTR_SUCCESS_FLAG		1
/* soc-v use the rsa2048 */
#define VBOOT_KEY_SIZE			256
#define RPMB_BASE_ADDR			(64*1024/256)
#define UBOOT_RB_INDEX_OFFSET		24
#define TRUST_RB_INDEX_OFFSET		28
#define ROCHCHIP_RSA_PARAMETER_SIZE	64

struct rk_pub_key {
	u_int32_t rsa_n[ROCHCHIP_RSA_PARAMETER_SIZE];
	u_int32_t rsa_e[ROCHCHIP_RSA_PARAMETER_SIZE];
	u_int32_t rsa_c[ROCHCHIP_RSA_PARAMETER_SIZE];
};

/**
 * The android things defines permanent attributes to
 * store PSK_public, product id. We can use this function
 * to write them.
 *
 * @param attributes  PSK_public, product id....
 *
 * @param size        The size of attributes.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_write_permanent_attributes(uint8_t *attributes, uint32_t size);

/**
 * The funtion can be use to read the device state to judge
 * whether the device can be flash.
 *
 * @param flash_lock_state  A flag indicate the device flash state.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_read_flash_lock_state(uint8_t *flash_lock_state);

/**
 * The function is provided to write device flash state.
 *
 * @param flash_lock_state   A flag indicate the device flash state.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_write_flash_lock_state(uint8_t flash_lock_state);

/**
 * The android things use the flag of lock state to indicate
 * whether the device can be booted when verified error.
 *
 * @param lock_state  A flag indicate the device lock state.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_read_lock_state(uint8_t *lock_state);

/**
 * The android things use the flag of lock state to indicate
 * whether the device can be booted when verified error.
 *
 * @param lock_state   A flag indicate the device lock state.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_write_lock_state(uint8_t lock_state);

/**
 * The android things uses fastboot to flash the permanent attributes.
 * And if them were written, there must have a flag to indicate.
 *
 * @param flag   indicate the permanent attributes have been written
 *               or not.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_read_perm_attr_flag(uint8_t *flag);

/**
 * The android things uses fastboot to flash the permanent attributes.
 * And if them were written, there must have a flag to indicate.
 *
 * @param flag   We can call this function to write the flag '1'
 *               to indicate the permanent attributes has been
 *               written.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_write_perm_attr_flag(uint8_t flag);

/**
 * The android things require the soc-v key hash to be flashed
 * using the fastboot. So the function can be used in fastboot
 * to flash the key hash.
 *
 * @param buf    The vboot key hash data.
 *
 * @param length The length of key hash.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_read_vbootkey_hash(uint8_t *buf, uint8_t length);

/**
 * The android things require the soc-v key hash to be flashed
 * using the fastboot. So the function can be used in fastboot
 * to flash the key hash.
 *
 * @param buf    The vboot key hash data.
 *
 * @param length The length of key hash.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_write_vbootkey_hash(uint8_t *buf, uint8_t length);

/**
 * U-boot close the optee client when start kernel
 * to prevent the optee client being invoking by other
 * program.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_close_optee_client(void);

/**
 * Write the permanent attributes hash.
 *
 * @param buf    The permanent attributes hash data.
 *
 * @param length The length of permanent attributes hash.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_write_attribute_hash(uint8_t *buf, uint8_t length);

/**
 * Get the avb vboot state
 *
 * @param buf    store the vboot state.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_get_at_vboot_state(char *buf);

/**
 * Get permanent attribute certificate
 *
 * @param cer: certificate data
 *
 * @param size: certificate size
 */
AvbIOResult avb_get_perm_attr_cer(uint8_t *cer, uint32_t size);

/**
 * Set permanent attribute certificate
 *
 * @param cer: certificate data
 *
 * @param size: certificate size
 */
AvbIOResult avb_set_perm_attr_cer(uint8_t *cer, uint32_t size);

/**
 * Get public key
 *
 * @param pub_key: public key data
 */
AvbIOResult avb_get_pub_key(struct rk_pub_key *pub_key);

/**
 * Get rollback index
 *
 * @param buffer: rollback index location
 */
AvbIOResult avb_read_all_rollback_index(char *buffer);

#ifdef __cplusplus
}
#endif

#endif /* AVB_H_ */
