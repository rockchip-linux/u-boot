// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#ifndef DICE_ANDROID_H_
#define DICE_ANDROID_H_

#include <stdbool.h>

#include "dice/dice.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DICE_ANDROID_CONFIG_COMPONENT_NAME (1 << 0)
#define DICE_ANDROID_CONFIG_COMPONENT_VERSION (1 << 1)
#define DICE_ANDROID_CONFIG_RESETTABLE (1 << 2)
#define DICE_ANDROID_CONFIG_SECURITY_VERSION (1 << 3)
#define DICE_ANDROID_CONFIG_RKP_VM_MARKER (1 << 4)

// Contains the input values used to construct the Android Profile for DICE
// configuration descriptor. The fields to include in the configuration
// descriptor are selected in the |configs| bitfield.
//
// Fields:
//    configs: A bitfield selecting the config fields to include.
//    component_name: Name of the component.
//    component_version: Version of the component.
//    security_version: Monotonically increasing version of the component.
typedef struct DiceAndroidConfigValues_ {
  uint32_t configs;
  const char* component_name;
  uint64_t component_version;
  uint64_t security_version;
} DiceAndroidConfigValues;

// Formats a configuration descriptor following the Android Profile for DICE
// specification. On success, |actual_size| is set to the number of bytes used.
// If kDiceResultBufferTooSmall is returned |actual_size| will be set to the
// required size of the buffer.
DiceResult DiceAndroidFormatConfigDescriptor(
    const DiceAndroidConfigValues* config_values, size_t buffer_size,
    uint8_t* buffer, size_t* actual_size);

// Executes the main Android DICE flow.
//
// Call this instead of DiceMainFlow when the next certificate should be
// appended to an existing Android DICE chain. However, when using
// the Android DICE handover format, use DiceAndroidHandoverMainFlow instead.
//
// Given the current CDIs, a full set of input values, and the current Android
// DICE chain, computes the next CDIs and the extended DICE chain. On success,
// |actual_size| is set to the number of bytes used. If
// kDiceResultBufferTooSmall is returned |actual_size| will be set to the
// required size of the buffer.
DiceResult DiceAndroidMainFlow(void* context,
                               const uint8_t current_cdi_attest[DICE_CDI_SIZE],
                               const uint8_t current_cdi_seal[DICE_CDI_SIZE],
                               const uint8_t* chain, size_t chain_size,
                               const DiceInputValues* input_values,
                               size_t buffer_size, uint8_t* buffer,
                               size_t* actual_size,
                               uint8_t next_cdi_attest[DICE_CDI_SIZE],
                               uint8_t next_cdi_seal[DICE_CDI_SIZE]);

// Executes the main Android DICE handover flow.
//
// Call this instead of DiceAndroidMainFlow when using the Android DICE handover
// format to combine the Android DICE chain and CDIs in a single CBOR object.
//
// Given a full set of input values and the current Android DICE handover
// object, computes the handover data for the next stage. On success,
// |actual_size| is set to the number of bytes used. If
// kDiceResultBufferTooSmall is returned |actual_size| will be set to the
// required size of the buffer.
//
// Using the Android DICE handover object is one option for passing the values
// between boot stages. Passing the bytes between stages is a problem left to
// the caller.
DiceResult DiceAndroidHandoverMainFlow(void* context, const uint8_t* handover,
                                       size_t handover_size,
                                       const DiceInputValues* input_values,
                                       size_t buffer_size, uint8_t* buffer,
                                       size_t* actual_size);

// Parses an Android DICE handover object to extract the fields.
//
// Given a pointer to an Android DICE handover object, returns pointers to the
// CDIs and DICE chain. If the DICE chain is not included in the handover
// object, the pointer is NULL and the size is 0.
DiceResult DiceAndroidHandoverParse(const uint8_t* handover,
                                    size_t handover_size,
                                    const uint8_t** cdi_attest,
                                    const uint8_t** cdi_seal,
                                    const uint8_t** chain, size_t* chain_size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DICE_ANDROID_H_
