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

// For more information on the Android Profile for DICE, see docs/android.md.

#include "dice/android.h"

#include <string.h>

#include "dice/cbor_reader.h"
#include "dice/cbor_writer.h"
#include "dice/dice.h"
#include "dice/ops.h"
#include "dice/ops/trait/cose.h"

// Completely gratuitous bit twiddling.
static size_t PopulationCount(uint32_t n) {
  n = n - ((n >> 1) & 0x55555555);
  n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
  return (((n + (n >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

DiceResult DiceAndroidFormatConfigDescriptor(
    const DiceAndroidConfigValues* config_values, size_t buffer_size,
    uint8_t* buffer, size_t* actual_size) {
  static const int64_t kComponentNameLabel = -70002;
  static const int64_t kComponentVersionLabel = -70003;
  static const int64_t kResettableLabel = -70004;
  static const int64_t kSecurityVersionLabel = -70005;
  static const int64_t kRkpVmMarkerLabel = -70006;

  // AndroidConfigDescriptor = {
  //   ? -70002 : tstr,     ; Component name
  //   ? -70003 : int,      ; Component version
  //   ? -70004 : null,     ; Resettable
  // }
  struct CborOut out;
  CborOutInit(buffer, buffer_size, &out);
  CborWriteMap(PopulationCount(config_values->configs), &out);
  if (config_values->configs & DICE_ANDROID_CONFIG_COMPONENT_NAME &&
      config_values->component_name) {
    CborWriteInt(kComponentNameLabel, &out);
    CborWriteTstr(config_values->component_name, &out);
  }
  if (config_values->configs & DICE_ANDROID_CONFIG_COMPONENT_VERSION) {
    CborWriteInt(kComponentVersionLabel, &out);
    CborWriteUint(config_values->component_version, &out);
  }
  if (config_values->configs & DICE_ANDROID_CONFIG_RESETTABLE) {
    CborWriteInt(kResettableLabel, &out);
    CborWriteNull(&out);
  }
  if (config_values->configs & DICE_ANDROID_CONFIG_SECURITY_VERSION) {
    CborWriteInt(kSecurityVersionLabel, &out);
    CborWriteUint(config_values->security_version, &out);
  }
  if (config_values->configs & DICE_ANDROID_CONFIG_RKP_VM_MARKER) {
    CborWriteInt(kRkpVmMarkerLabel, &out);
    CborWriteNull(&out);
  }
  *actual_size = CborOutSize(&out);
  if (CborOutOverflowed(&out)) {
    return kDiceResultBufferTooSmall;
  }
  return kDiceResultOk;
}

DiceResult DiceAndroidMainFlow(void* context,
                               const uint8_t current_cdi_attest[DICE_CDI_SIZE],
                               const uint8_t current_cdi_seal[DICE_CDI_SIZE],
                               const uint8_t* chain, size_t chain_size,
                               const DiceInputValues* input_values,
                               size_t buffer_size, uint8_t* buffer,
                               size_t* actual_size,
                               uint8_t next_cdi_attest[DICE_CDI_SIZE],
                               uint8_t next_cdi_seal[DICE_CDI_SIZE]) {
  DiceResult result;
  enum CborReadResult res;
  struct CborIn in;
  size_t chain_item_count;

  // The Android DICE chain has a more detailed internal structure, but those
  // details aren't relevant to the work of this function.
  //
  // DiceCertChain = [
  //   COSE_Key,         ; Root public key
  //   + COSE_Sign1,     ; DICE chain entries
  // ]
  CborInInit(chain, chain_size, &in);
  res = CborReadArray(&in, &chain_item_count);
  if (res != CBOR_READ_RESULT_OK) {
    return kDiceResultInvalidInput;
  }

  if (chain_item_count < 2 || chain_item_count == SIZE_MAX) {
    // There should at least be the public key and one entry.
    return kDiceResultInvalidInput;
  }

  // Measure the existing chain entries.
  size_t chain_items_offset = CborInOffset(&in);
  for (size_t chain_pos = 0; chain_pos < chain_item_count; ++chain_pos) {
    res = CborReadSkip(&in);
    if (res != CBOR_READ_RESULT_OK) {
      return kDiceResultInvalidInput;
    }
  }
  size_t chain_items_size = CborInOffset(&in) - chain_items_offset;

  // Copy to the new buffer, with space in the chain for one more entry.
  struct CborOut out;
  CborOutInit(buffer, buffer_size, &out);
  CborWriteArray(chain_item_count + 1, &out);
  size_t new_chain_prefix_size = CborOutSize(&out);
  if (CborOutOverflowed(&out) ||
      chain_items_size > buffer_size - new_chain_prefix_size) {
    // Continue with an empty buffer to measure the required size.
    buffer_size = 0;
  } else {
    memcpy(buffer + new_chain_prefix_size, chain + chain_items_offset,
           chain_items_size);
    buffer += new_chain_prefix_size + chain_items_size;
    buffer_size -= new_chain_prefix_size + chain_items_size;
  }

  size_t certificate_size = 0;
  result = DiceMainFlow(context, current_cdi_attest, current_cdi_seal,
                        input_values, buffer_size, buffer, &certificate_size,
                        next_cdi_attest, next_cdi_seal);
  *actual_size = new_chain_prefix_size + chain_items_size + certificate_size;
  return result;
}

static DiceResult DiceAndroidMainFlowWithNewDiceChain(
    void* context, const uint8_t current_cdi_attest[DICE_CDI_SIZE],
    const uint8_t current_cdi_seal[DICE_CDI_SIZE],
    const DiceInputValues* input_values, size_t buffer_size, uint8_t* buffer,
    size_t* chain_size, uint8_t next_cdi_attest[DICE_CDI_SIZE],
    uint8_t next_cdi_seal[DICE_CDI_SIZE]) {
  uint8_t current_cdi_private_key_seed[DICE_PRIVATE_KEY_SEED_SIZE];
  uint8_t attestation_public_key[DICE_PUBLIC_KEY_BUFFER_SIZE];
  uint8_t attestation_private_key[DICE_PRIVATE_KEY_BUFFER_SIZE];
  // Derive an asymmetric private key seed from the current attestation CDI
  // value.
  DiceResult result = DiceDeriveCdiPrivateKeySeed(context, current_cdi_attest,
                                                  current_cdi_private_key_seed);
  if (result != kDiceResultOk) {
    goto out;
  }
  // Derive attestation key pair.
  result = DiceKeypairFromSeed(context, kDicePrincipalAuthority,
                               current_cdi_private_key_seed,
                               attestation_public_key, attestation_private_key);
  if (result != kDiceResultOk) {
    goto out;
  }

  // Consruct the DICE chain from the attestation public key and the next CDI
  // certificate.
  struct CborOut out;
  CborOutInit(buffer, buffer_size, &out);
  CborWriteArray(2, &out);
  size_t encoded_size_used = CborOutSize(&out);
  if (CborOutOverflowed(&out)) {
    // Continue with an empty buffer to measure the required size.
    buffer_size = 0;
  } else {
    buffer += encoded_size_used;
    buffer_size -= encoded_size_used;
  }

  size_t encoded_pub_key_size = 0;
  result = DiceCoseEncodePublicKey(context, kDicePrincipalAuthority,
                                   attestation_public_key, buffer_size, buffer,
                                   &encoded_pub_key_size);
  if (result == kDiceResultOk) {
    buffer += encoded_pub_key_size;
    buffer_size -= encoded_pub_key_size;
  } else if (result == kDiceResultBufferTooSmall) {
    // Continue with an empty buffer to measure the required size.
    buffer_size = 0;
  } else {
    goto out;
  }

  result = DiceMainFlow(context, current_cdi_attest, current_cdi_seal,
                        input_values, buffer_size, buffer, chain_size,
                        next_cdi_attest, next_cdi_seal);
  *chain_size += encoded_size_used + encoded_pub_key_size;
  if (result != kDiceResultOk) {
    return result;
  }

out:
  DiceClearMemory(context, sizeof(current_cdi_private_key_seed),
                  current_cdi_private_key_seed);
  DiceClearMemory(context, sizeof(attestation_private_key),
                  attestation_private_key);

  return result;
}

// AndroidDiceHandover = {
//   1 : bstr .size 32,     ; CDI_Attest
//   2 : bstr .size 32,     ; CDI_Seal
//   ? 3 : DiceCertChain,   ; Android DICE chain
// }
static const int64_t kCdiAttestLabel = 1;
static const int64_t kCdiSealLabel = 2;
static const int64_t kDiceChainLabel = 3;

DiceResult DiceAndroidHandoverMainFlow(void* context, const uint8_t* handover,
                                       size_t handover_size,
                                       const DiceInputValues* input_values,
                                       size_t buffer_size, uint8_t* buffer,
                                       size_t* actual_size) {
  DiceResult result;
  const uint8_t* current_cdi_attest;
  const uint8_t* current_cdi_seal;
  const uint8_t* chain;
  size_t chain_size;

  result =
      DiceAndroidHandoverParse(handover, handover_size, &current_cdi_attest,
                               &current_cdi_seal, &chain, &chain_size);
  if (result != kDiceResultOk) {
    return kDiceResultInvalidInput;
  }

  // Write the new handover data.
  struct CborOut out;
  CborOutInit(buffer, buffer_size, &out);
  CborWriteMap(/*num_pairs=*/3, &out);
  CborWriteInt(kCdiAttestLabel, &out);
  uint8_t* next_cdi_attest = CborAllocBstr(DICE_CDI_SIZE, &out);
  CborWriteInt(kCdiSealLabel, &out);
  uint8_t* next_cdi_seal = CborAllocBstr(DICE_CDI_SIZE, &out);
  CborWriteInt(kDiceChainLabel, &out);

  uint8_t ignored_cdi_attest[DICE_CDI_SIZE];
  uint8_t ignored_cdi_seal[DICE_CDI_SIZE];
  if (CborOutOverflowed(&out)) {
    // Continue with an empty buffer and placeholders for the output CDIs to
    // measure the required size.
    buffer_size = 0;
    next_cdi_attest = ignored_cdi_attest;
    next_cdi_seal = ignored_cdi_seal;
  } else {
    buffer += CborOutSize(&out);
    buffer_size -= CborOutSize(&out);
  }

  if (chain_size != 0) {
    // If the DICE chain is present in the handover, append the next certificate
    // to the existing DICE chain.
    result = DiceAndroidMainFlow(context, current_cdi_attest, current_cdi_seal,
                                 chain, chain_size, input_values, buffer_size,
                                 buffer, &chain_size, next_cdi_attest,
                                 next_cdi_seal);
  } else {
    // If DICE chain is not present in the handover, construct the DICE chain
    // from the public key derived from the current CDI attest and the next CDI
    // certificate.
    result = DiceAndroidMainFlowWithNewDiceChain(
        context, current_cdi_attest, current_cdi_seal, input_values,
        buffer_size, buffer, &chain_size, next_cdi_attest, next_cdi_seal);
  }
  *actual_size = CborOutSize(&out) + chain_size;
  return result;
}

DiceResult DiceAndroidHandoverParse(const uint8_t* handover,
                                    size_t handover_size,
                                    const uint8_t** cdi_attest,
                                    const uint8_t** cdi_seal,
                                    const uint8_t** chain, size_t* chain_size) {
  // Extract details from the handover data.
  struct CborIn in;
  int64_t label;
  size_t num_pairs;
  size_t item_size;
  CborInInit(handover, handover_size, &in);
  if (CborReadMap(&in, &num_pairs) != CBOR_READ_RESULT_OK || num_pairs < 2 ||
      // Read the attestation CDI.
      CborReadInt(&in, &label) != CBOR_READ_RESULT_OK ||
      label != kCdiAttestLabel ||
      CborReadBstr(&in, &item_size, cdi_attest) != CBOR_READ_RESULT_OK ||
      item_size != DICE_CDI_SIZE ||
      // Read the sealing CDI.
      CborReadInt(&in, &label) != CBOR_READ_RESULT_OK ||
      label != kCdiSealLabel ||
      CborReadBstr(&in, &item_size, cdi_seal) != CBOR_READ_RESULT_OK ||
      item_size != DICE_CDI_SIZE) {
    return kDiceResultInvalidInput;
  }

  *chain = NULL;
  *chain_size = 0;
  if (num_pairs >= 3 && CborReadInt(&in, &label) == CBOR_READ_RESULT_OK) {
    if (label == kDiceChainLabel) {
      // Calculate the DICE chain size, if it is present in the handover object.
      size_t chain_start = CborInOffset(&in);
      if (CborReadSkip(&in) != CBOR_READ_RESULT_OK) {
        return kDiceResultInvalidInput;
      }
      *chain = handover + chain_start;
      *chain_size = CborInOffset(&in) - chain_start;
    }
  }

  return kDiceResultOk;
}
