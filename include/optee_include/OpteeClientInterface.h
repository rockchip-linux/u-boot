/*
 * Copyright 2017, Rockchip Electronics Co., Ltd
 * hisping lin, <hisping.lin@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _OPTEECLIENTTEST_H_
#define _OPTEECLIENTTEST_H_

void test_optee(void);
uint32_t trusty_read_rollback_index(uint32_t slot, uint64_t *value);
uint32_t trusty_write_rollback_index(uint32_t slot, uint64_t value);
uint32_t trusty_read_permanent_attributes(uint8_t *attributes, uint32_t size);
uint32_t trusty_write_permanent_attributes(uint8_t *attributes, uint32_t size);
uint32_t trusty_read_lock_state(uint8_t *lock_state);
uint32_t trusty_write_lock_state(uint8_t lock_state);
uint32_t trusty_read_flash_lock_state(uint8_t *flash_lock_state);
uint32_t trusty_write_flash_lock_state(uint8_t flash_lock_state);
uint32_t write_to_keymaster
	(uint8_t *filename, uint32_t filename_size,
	uint8_t *data, uint32_t data_size);
uint32_t trusty_read_attribute_hash(uint32_t *buf, uint32_t length);
uint32_t trusty_write_attribute_hash(uint32_t *buf, uint32_t length);
uint32_t trusty_notify_optee_uboot_end(void);
uint32_t trusty_read_vbootkey_hash(uint32_t *buf, uint32_t length);
uint32_t trusty_write_vbootkey_hash(uint32_t *buf, uint32_t length);
uint32_t trusty_read_vbootkey_enable_flag(uint8_t *flag);
uint32_t trusty_read_permanent_attributes_flag(uint8_t *attributes);
uint32_t trusty_write_permanent_attributes_flag(uint8_t attributes);

#endif
