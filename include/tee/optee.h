/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * OP-TEE related definitions
 *
 * (C) Copyright 2016 Linaro Limited
 * Andrew F. Davis <andrew.davis@linaro.org>
 */

#ifndef	_OPTEE_H
#define _OPTEE_H

#include <linux/errno.h>
#include <image.h>

#define OPTEE_MAGIC             0x4554504f
#define OPTEE_VERSION           1
#define OPTEE_ARCH_ARM32        0
#define OPTEE_ARCH_ARM64        1

struct optee_header {
	uint32_t magic;
	uint8_t version;
	uint8_t arch;
	uint16_t flags;
	uint32_t init_size;
	uint32_t init_load_addr_hi;
	uint32_t init_load_addr_lo;
	uint32_t init_mem_usage;
	uint32_t paged_size;
};

static inline uint32_t
optee_image_get_entry_point(const struct legacy_img_hdr *hdr)
{
	struct optee_header *optee_hdr = (struct optee_header *)(hdr + 1);

	return optee_hdr->init_load_addr_lo;
}

static inline uint32_t
optee_image_get_load_addr(const struct legacy_img_hdr *hdr)
{
	return optee_image_get_entry_point(hdr) - sizeof(struct optee_header);
}

#if defined(CONFIG_OPTEE_IMAGE)
int optee_verify_bootm_image(unsigned long image_addr,
			     unsigned long image_load_addr,
			     unsigned long image_len);
#else
static inline int optee_verify_bootm_image(unsigned long image_addr,
					   unsigned long image_load_addr,
					   unsigned long image_len)
{
	return -EPERM;
}
#endif

#if defined(CONFIG_OPTEE_LIB) && defined(CONFIG_OF_LIBFDT)
int optee_copy_fdt_nodes(void *new_blob);
#else
static inline int optee_copy_fdt_nodes(void *new_blob)
{
	return 0;
}
#endif

/* rockchip struct for optee api */
enum RK_OEM_OTP_KEYID {
	RK_OEM_OTP_KEY0 = 0,
	RK_OEM_OTP_KEY1 = 1,
	RK_OEM_OTP_KEY2 = 2,
	RK_OEM_OTP_KEY3 = 3,
	RK_OEM_OTP_KEY_FW = 10,	//keyid of fw_encryption_key
	RK_OEM_OTP_KEYMAX
};

enum RK_HDCP_KEYID {
	RK_HDCP_KEY0 = 0,
	RK_HDCP_KEY1 = 1,
	RK_HDCP_KEYMAX
};

typedef struct {
	uint32_t	algo;
	uint32_t	mode;
	uint32_t	operation;
	uint8_t		key[64];
	uint32_t	key_len;
	uint8_t		iv[16];
	void		*reserved;
} rk_cipher_config;

enum RK_CIPIHER_MODE {
	RK_CIPHER_MODE_ECB = 0,
	RK_CIPHER_MODE_CBC = 1,
	RK_CIPHER_MODE_CTS = 2,
	RK_CIPHER_MODE_CTR = 3,
	RK_CIPHER_MODE_CFB = 4,
	RK_CIPHER_MODE_OFB = 5,
	RK_CIPHER_MODE_XTS = 6,
	RK_CIPHER_MODE_CCM = 7,
	RK_CIPHER_MODE_GCM = 8,
	RK_CIPHER_MODE_CMAC = 9,
	RK_CIPHER_MODE_CBC_MAC = 10,
	RK_CIPHER_MODE_MAX
};

enum RK_CRYPTO_ALGO {
	RK_ALGO_AES = 1,
	RK_ALGO_DES,
	RK_ALGO_TDES,
	RK_ALGO_SM4,
	RK_ALGO_ALGO_MAX
};

#define RK_MODE_ENCRYPT			1
#define RK_MODE_DECRYPT			0

#define AES_BLOCK_SIZE			16

/* rockchip optee api for storage */
uint32_t optee_read_rollback_index(uint32_t slot, uint64_t *value);
uint32_t optee_write_rollback_index(uint32_t slot, uint64_t value);
uint32_t optee_read_permanent_attributes(uint8_t *attributes, uint32_t size);
uint32_t optee_write_permanent_attributes(uint8_t *attributes, uint32_t size);
uint32_t optee_read_permanent_attributes_flag(uint8_t *attributes);
uint32_t optee_write_permanent_attributes_flag(uint8_t attributes);
uint32_t optee_read_permanent_attributes_cer(uint8_t *attributes, uint32_t size);
uint32_t optee_write_permanent_attributes_cer(uint8_t *attributes, uint32_t size);
uint32_t optee_read_lock_state(uint8_t *lock_state);
uint32_t optee_write_lock_state(uint8_t lock_state);
uint32_t optee_read_flash_lock_state(uint8_t *flash_lock_state);
uint32_t optee_write_flash_lock_state(uint8_t flash_lock_state);
void optee_client_init(void);
uint32_t optee_notify_uboot_end(void);

/* rockchip optee api for otp */
uint32_t optee_read_attribute_hash(uint32_t *buf, uint32_t length);
uint32_t optee_write_attribute_hash(uint32_t *buf, uint32_t length);
uint32_t optee_read_vbootkey_hash(uint32_t *buf, uint32_t length);
uint32_t optee_write_vbootkey_hash(uint32_t *buf, uint32_t length);
uint32_t optee_read_vbootkey_enable_flag(uint8_t *flag);
uint32_t optee_check_security_level_flag(uint8_t flag);
uint32_t optee_write_oem_huk(uint32_t *buf, uint32_t length);
uint32_t optee_write_ta_encryption_key(uint32_t *buf, uint32_t length);
uint32_t optee_ta_encryption_key_is_written(uint8_t *value);
uint32_t optee_write_oem_encrypt_data(uint32_t *buf, uint32_t length);
uint32_t optee_write_oem_ns_otp(uint32_t byte_off, uint8_t *byte_buf, uint32_t byte_len);
uint32_t optee_read_oem_ns_otp(uint32_t byte_off, uint8_t *byte_buf, uint32_t byte_len);
uint32_t optee_write_oem_otp_key(enum RK_OEM_OTP_KEYID key_id,
				 uint8_t *byte_buf, uint32_t byte_len);
uint32_t optee_oem_otp_key_is_written(enum RK_OEM_OTP_KEYID key_id, uint8_t *value);
uint32_t optee_set_oem_hr_otp_read_lock(enum RK_OEM_OTP_KEYID key_id);
uint32_t optee_write_oem_hdcp_key(enum RK_HDCP_KEYID key_id,
				  uint8_t *byte_buf, uint32_t byte_len);
uint32_t optee_oem_hdcp_key_is_written(enum RK_HDCP_KEYID key_id, uint8_t *value);
uint32_t optee_set_oem_hdcp_key_mask(enum RK_HDCP_KEYID key_id);
void optee_select_security_level(void);
uint32_t optee_base_finish_otp(void);

/* rockchip optee api for crypto */
uint32_t optee_oem_otp_key_cipher(enum RK_OEM_OTP_KEYID key_id, rk_cipher_config *config,
				  uint32_t src_phys_addr, uint32_t dst_phys_addr, uint32_t len);

/* rockchip optee api for user ta */
uint32_t optee_oem_user_ta_transfer(void);
uint32_t optee_oem_user_ta_storage(void);

#endif /* _OPTEE_H */
