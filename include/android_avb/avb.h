/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef AVB_H_
#define AVB_H_

#include <android_avb/libavb.h>
#include <android_avb/libavb_ab.h>
#include <android_avb/libavb_atx.h>
#include <android_avb/libavb_user.h>

#ifdef __cplusplus
extern "C" {
#endif

/* rk used */
#define PERM_ATTR_DIGEST_SIZE		32
#define PERM_ATTR_TOTAL_SIZE		1052
#define VBOOT_KEY_HASH_SIZE		32
#define VBMETA_MAX_SIZE			65536
#define ROLLBACK_MAX_SIZE		20
#define LOCK_MASK			(1 << 0)
#define UNLOCK_DISABLE_MASK		(1 << 1)
#define AVB_STATE_SIZE		1000
#define PERM_ATTR_SUCCESS_FLAG		1
/* bootloader vboot key length */
#ifndef CONFIG_FIT_ENABLE_RSA4096_SUPPORT
#define VBOOT_KEY_SIZE			256
#else
#define VBOOT_KEY_SIZE			512
#endif
#define RPMB_BASE_ADDR			(64*1024/256)
#define UBOOT_RB_INDEX_OFFSET		24
#define TRUST_RB_INDEX_OFFSET		28
#ifndef CONFIG_FIT_ENABLE_RSA4096_SUPPORT
#define ROCKCHIP_RSA_PARAMETER_SIZE	64
#else
#define ROCKCHIP_RSA_PARAMETER_SIZE	128
#endif
#define RK_AVB_RSA_NUM_BYTES		(ROCKCHIP_RSA_PARAMETER_SIZE * sizeof(u_int32_t))
#define RK_AVB_PERM_ATTR_CER_SIZE	RK_AVB_RSA_NUM_BYTES
/* write/read permanent attributes all use. */
#define AT_PERM_ATTR_FUSE		1
#define AT_PERM_ATTR_CER_FUSE		2
#define AT_LOCK_VBOOT			3

struct rk_pub_key {
	u_int32_t rsa_n[ROCKCHIP_RSA_PARAMETER_SIZE];
	u_int32_t rsa_e[ROCKCHIP_RSA_PARAMETER_SIZE];
	u_int32_t rsa_c[ROCKCHIP_RSA_PARAMETER_SIZE];
};

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
AvbIOResult avb_read_permanent_attributes_flag(uint8_t *flag);

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
AvbIOResult avb_write_permanent_attributes_flag(uint8_t flag);

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
AvbIOResult avb_write_permanent_attributes_hash(uint8_t *buf, uint8_t length);

/**
 * Get the avb state
 *
 * @param buf    store avb state.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbIOResult avb_get_state(char *buf);

/**
 * Get permanent attribute certificate
 *
 * @param cer: certificate data
 *
 * @param size: certificate size
 */
AvbIOResult avb_get_permanent_attributes_cer(uint8_t *cer, uint32_t size);

/**
 * Set permanent attribute certificate
 *
 * @param cer: certificate data
 *
 * @param size: certificate size
 */
AvbIOResult avb_set_permanent_attributes_cer(uint8_t *cer, uint32_t size);

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

/**
 * Generate unlock challenge
 *
 * @param buffer: unlock challenge buffer
 * @param challenge_len: challenge length
 */
int avb_generate_unlock_challenge(void *buffer, uint32_t *challenge_len);

/**
 * AVB auth unlock
 *
 * @param buffer: unlock challenge buffer
 * @param out_is_trusted: unlock result
 */
int avb_auth_unlock(void *buffer, char *out_is_trusted);

/**
 * AVB write permanent attributes set
 *
 * @param id: operation id
 * @param pbuf: permanent attributes buffer
 * @param size: permanent attributes size
 */
int avb_write_permanent_attributes_all(u16 id, void *pbuf, u16 size);

/**
 * AVB read permanent attributes set
 *
 * @param id: operation id
 * @param pbuf: permanent attributes buffer
 * @param size: permanent attributes size
 */
int avb_read_permanent_attributes_all(u16 id, void *pbuf, u16 size);

#ifdef __cplusplus
}
#endif

#endif /* AVB_H_ */
