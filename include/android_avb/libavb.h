/* SPDX-License-Identifier: MIT */
/*
 * Copyright (C) 2016 The Android Open Source Project
 */

#ifndef LIBAVB_H_
#define LIBAVB_H_

/* The AVB_INSIDE_LIBAVB_H preprocessor symbol is used to enforce
 * library users to include only this file. All public interfaces, and
 * only public interfaces, must be included here.
 */

#define AVB_INSIDE_LIBAVB_H
/* avb_cmdline.h, avb_rsa.h, avb_sha.h can't be included in public libavb.h */
#include <android_avb/avb_chain_partition_descriptor.h>
#include <android_avb/avb_crypto.h>
#include <android_avb/avb_descriptor.h>
#include <android_avb/avb_footer.h>
#include <android_avb/avb_hash_descriptor.h>
#include <android_avb/avb_hashtree_descriptor.h>
#include <android_avb/avb_kernel_cmdline_descriptor.h>
#include <android_avb/avb_ops.h>
#include <android_avb/avb_property_descriptor.h>
#include <android_avb/avb_slot_verify.h>
#include <android_avb/avb_sysdeps.h>
#include <android_avb/avb_util.h>
#include <android_avb/avb_vbmeta_image.h>
#include <android_avb/avb_version.h>
#undef AVB_INSIDE_LIBAVB_H

#endif /* LIBAVB_H_ */
