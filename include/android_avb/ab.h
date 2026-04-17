/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef AB_H_
#define AB_H_

#include <android_avb/libavb.h>
#include <android_avb/libavb_ab.h>
#include <android_avb/libavb_atx.h>
#include <android_avb/libavb_user.h>
#include <android_avb/avb.h>

#ifdef __cplusplus
extern "C" {
#endif

/* rk used */
#define SLOT_NUM			2
#define CURR_SYSTEM_SLOT_SUFFIX		"ab"

/**
 * Get current slot: '_a' or '_b'.
 *
 * @param select_slot  obtain current slot.
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbABFlowResult ab_get_current_slot(char *select_slot);

/**
 * Append current slot to given partition name
 *
 * @param part_name	partition name
 * @param slot		given slot suffix, auto append current slot if NULL
 * @param new_name	partition name with slot suffix appended
 *
 * @return 0 if the command succeeded, -1 if it failed
 */
AvbABFlowResult ab_append_part_slot(const char *part_name, char *new_name);

/**
 * Get the information whether the partition has slot
 *
 * @param the partition name
 *
 * @return 0 if the partition has slot, -1 if not
 */
AvbABFlowResult ab_get_part_has_slot_info(const char *base_name);

/**
 * get current ab slot_suffixs.
 */
AvbABFlowResult ab_slot_select(AvbABOps* ab_ops,char select_slot[]);

/**
 * Get last boot slot
 *
 * @return 0 is slot A; 1 is slot B; -1 is error
 */
AvbABFlowResult ab_get_lastboot(void);

/**
 * Do the device have boot slot
 */
bool ab_have_bootable_slot(void);

/**
 * update rollback index
 */
AvbABFlowResult ab_update_stored_rollback_indexes_for_slot(AvbOps* ops, AvbSlotVerifyData* slot_data);

/**
 * Get slot data
 */
AvbABFlowResult ab_get_slot_data(AvbABData* ab_data);

/**
 * Set a certain slot as active
 */
AvbABFlowResult ab_set_slot_active(unsigned int *slot_number);

/**
 * Init ab metadata
 */
int ab_init_metadata(void);

#ifdef __cplusplus
}
#endif

#endif /* AB_H_ */
