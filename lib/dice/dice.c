// Copyright 2020 Google LLC
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

#include "dice/dice.h"

#include <string.h>

#include "dice/ops.h"

#define DICE_CODE_SIZE DICE_HASH_SIZE
#define DICE_CONFIG_SIZE DICE_INLINE_CONFIG_SIZE
#define DICE_AUTHORITY_SIZE DICE_HASH_SIZE
#define DICE_MODE_SIZE 1

static const uint8_t kAsymSalt[] = {
    0x63, 0xB6, 0xA0, 0x4D, 0x2C, 0x07, 0x7F, 0xC1, 0x0F, 0x63, 0x9F,
    0x21, 0xDA, 0x79, 0x38, 0x44, 0x35, 0x6C, 0xC2, 0xB0, 0xB4, 0x41,
    0xB3, 0xA7, 0x71, 0x24, 0x03, 0x5C, 0x03, 0xF8, 0xE1, 0xBE, 0x60,
    0x35, 0xD3, 0x1F, 0x28, 0x28, 0x21, 0xA7, 0x45, 0x0A, 0x02, 0x22,
    0x2A, 0xB1, 0xB3, 0xCF, 0xF1, 0x67, 0x9B, 0x05, 0xAB, 0x1C, 0xA5,
    0xD1, 0xAF, 0xFB, 0x78, 0x9C, 0xCD, 0x2B, 0x0B, 0x3B};
static const size_t kAsymSaltSize = 64;

static const uint8_t kIdSalt[] = {
    0xDB, 0xDB, 0xAE, 0xBC, 0x80, 0x20, 0xDA, 0x9F, 0xF0, 0xDD, 0x5A,
    0x24, 0xC8, 0x3A, 0xA5, 0xA5, 0x42, 0x86, 0xDF, 0xC2, 0x63, 0x03,
    0x1E, 0x32, 0x9B, 0x4D, 0xA1, 0x48, 0x43, 0x06, 0x59, 0xFE, 0x62,
    0xCD, 0xB5, 0xB7, 0xE1, 0xE0, 0x0F, 0xC6, 0x80, 0x30, 0x67, 0x11,
    0xEB, 0x44, 0x4A, 0xF7, 0x72, 0x09, 0x35, 0x94, 0x96, 0xFC, 0xFF,
    0x1D, 0xB9, 0x52, 0x0B, 0xA5, 0x1C, 0x7B, 0x29, 0xEA};
static const size_t kIdSaltSize = 64;

DiceResult DiceDeriveCdiPrivateKeySeed(
    void* context, const uint8_t cdi_attest[DICE_CDI_SIZE],
    uint8_t cdi_private_key_seed[DICE_PRIVATE_KEY_SEED_SIZE]) {
  // Use the CDI as input key material, with fixed salt and info.
  return DiceKdf(context, /*length=*/DICE_PRIVATE_KEY_SEED_SIZE, cdi_attest,
                 /*ikm_size=*/DICE_CDI_SIZE, kAsymSalt, kAsymSaltSize,
                 /*info=*/(const uint8_t*)"Key Pair", /*info_size=*/8,
                 cdi_private_key_seed);
}

DiceResult DiceDeriveCdiCertificateId(void* context,
                                      const uint8_t* cdi_public_key,
                                      size_t cdi_public_key_size,
                                      uint8_t id[DICE_ID_SIZE]) {
  // Use the public key as input key material, with fixed salt and info.
  DiceResult result =
      DiceKdf(context, /*length=*/20, cdi_public_key, cdi_public_key_size,
              kIdSalt, kIdSaltSize,
              /*info=*/(const uint8_t*)"ID", /*info_size=*/2, id);
  if (result == kDiceResultOk) {
    // Clear the top bit to keep the integer positive.
    id[0] &= ~0x80;
  }
  return result;
}

DiceResult DiceMainFlow(void* context,
                        const uint8_t current_cdi_attest[DICE_CDI_SIZE],
                        const uint8_t current_cdi_seal[DICE_CDI_SIZE],
                        const DiceInputValues* input_values,
                        size_t next_cdi_certificate_buffer_size,
                        uint8_t* next_cdi_certificate,
                        size_t* next_cdi_certificate_actual_size,
                        uint8_t next_cdi_attest[DICE_CDI_SIZE],
                        uint8_t next_cdi_seal[DICE_CDI_SIZE]) {
  // This implementation serializes the inputs for a one-shot hash. On some
  // platforms, using a multi-part hash operation may be more optimal. The
  // combined input buffer has this layout:
  // ---------------------------------------------------------------------------
  // | Code Input | Config Input | Authority Input | Mode Input | Hidden Input |
  // ---------------------------------------------------------------------------
  const size_t kCodeOffset = 0;
  const size_t kConfigOffset = kCodeOffset + DICE_CODE_SIZE;
  const size_t kAuthorityOffset = kConfigOffset + DICE_CONFIG_SIZE;
  const size_t kModeOffset = kAuthorityOffset + DICE_AUTHORITY_SIZE;
  const size_t kHiddenOffset = kModeOffset + DICE_MODE_SIZE;

  DiceResult result = kDiceResultOk;

  // Declare buffers that get cleaned up on 'goto out'.
  uint8_t input_buffer[DICE_CODE_SIZE + DICE_CONFIG_SIZE + DICE_AUTHORITY_SIZE +
                       DICE_MODE_SIZE + DICE_HIDDEN_SIZE];
  uint8_t attest_input_hash[DICE_HASH_SIZE];
  uint8_t seal_input_hash[DICE_HASH_SIZE];
  uint8_t current_cdi_private_key_seed[DICE_PRIVATE_KEY_SEED_SIZE];
  uint8_t next_cdi_private_key_seed[DICE_PRIVATE_KEY_SEED_SIZE];

  // Assemble the input buffer.
  memcpy(&input_buffer[kCodeOffset], input_values->code_hash, DICE_CODE_SIZE);
  if (input_values->config_type == kDiceConfigTypeInline) {
    memcpy(&input_buffer[kConfigOffset], input_values->config_value,
           DICE_CONFIG_SIZE);
  } else if (!input_values->config_descriptor) {
    result = kDiceResultInvalidInput;
    goto out;
  } else {
    result = DiceHash(context, input_values->config_descriptor,
                      input_values->config_descriptor_size,
                      &input_buffer[kConfigOffset]);
    if (result != kDiceResultOk) {
      goto out;
    }
  }
  memcpy(&input_buffer[kAuthorityOffset], input_values->authority_hash,
         DICE_AUTHORITY_SIZE);
  input_buffer[kModeOffset] = input_values->mode;
  memcpy(&input_buffer[kHiddenOffset], input_values->hidden, DICE_HIDDEN_SIZE);

  // Hash the appropriate input values for both attestation and sealing. For
  // attestation all the inputs are used, and for sealing only the authority,
  // mode, and hidden inputs are used.
  result =
      DiceHash(context, input_buffer, sizeof(input_buffer), attest_input_hash);
  if (result != kDiceResultOk) {
    goto out;
  }
  result = DiceHash(context, &input_buffer[kAuthorityOffset],
                    DICE_AUTHORITY_SIZE + DICE_MODE_SIZE + DICE_HIDDEN_SIZE,
                    seal_input_hash);
  if (result != kDiceResultOk) {
    goto out;
  }

  // Compute the next CDI values. For each of these the current CDI value is
  // used as input key material and the input hash is used as salt.
  result = DiceKdf(context, /*length=*/DICE_CDI_SIZE, current_cdi_attest,
                   /*ikm_size=*/DICE_CDI_SIZE, attest_input_hash,
                   /*salt_size=*/DICE_HASH_SIZE,
                   /*info=*/(const uint8_t*)"CDI_Attest", /*info_size=*/10,
                   next_cdi_attest);
  if (result != kDiceResultOk) {
    goto out;
  }
  result = DiceKdf(
      context, /*length=*/DICE_CDI_SIZE, current_cdi_seal,
      /*ikm_size=*/DICE_CDI_SIZE, seal_input_hash, /*salt_size=*/DICE_HASH_SIZE,
      /*info=*/(const uint8_t*)"CDI_Seal", /*info_size=*/8, next_cdi_seal);
  if (result != kDiceResultOk) {
    goto out;
  }

  // Create the CDI certificate only if it is required (i.e. non-null/non-zero
  // values are provided for the next CDI certificate parameters).
  if (next_cdi_certificate == NULL &&
      next_cdi_certificate_actual_size == NULL &&
      next_cdi_certificate_buffer_size == 0) {
    goto out;
  }

  // Derive asymmetric private key seeds from the attestation CDI values.
  result = DiceDeriveCdiPrivateKeySeed(context, current_cdi_attest,
                                       current_cdi_private_key_seed);
  if (result != kDiceResultOk) {
    goto out;
  }
  result = DiceDeriveCdiPrivateKeySeed(context, next_cdi_attest,
                                       next_cdi_private_key_seed);
  if (result != kDiceResultOk) {
    goto out;
  }

  // Generate a certificate for |next_cdi_private_key_seed| with
  // |current_cdi_private_key_seed| as the authority.
  result = DiceGenerateCertificate(
      context, next_cdi_private_key_seed, current_cdi_private_key_seed,
      input_values, next_cdi_certificate_buffer_size, next_cdi_certificate,
      next_cdi_certificate_actual_size);

out:
  // Clear sensitive memory.
  DiceClearMemory(context, sizeof(input_buffer), input_buffer);
  DiceClearMemory(context, sizeof(attest_input_hash), attest_input_hash);
  DiceClearMemory(context, sizeof(seal_input_hash), seal_input_hash);
  DiceClearMemory(context, sizeof(current_cdi_private_key_seed),
                  current_cdi_private_key_seed);
  DiceClearMemory(context, sizeof(next_cdi_private_key_seed),
                  next_cdi_private_key_seed);
  return result;
}
