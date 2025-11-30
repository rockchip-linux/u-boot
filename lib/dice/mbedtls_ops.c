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

// This is an implementation of DiceGenerateCertificate and the crypto
// operations that uses mbedtls. The algorithms used are SHA512, HKDF-SHA512,
// and deterministic ECDSA-P256-SHA256.

#include <stdint.h>
#include <string.h>

#define MBEDTLS_ALLOW_PRIVATE_ACCESS

#include "dice/dice.h"
#include "dice/ops.h"
#include "dice/profile_name.h"
#include "dice/utils.h"
#include "mbedtls/hkdf.h"

DiceResult DiceHash(void* context_not_used, const uint8_t* input,
                    size_t input_size, uint8_t output[DICE_HASH_SIZE]) {
  (void)context_not_used;
  if (0 != mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), input,
                      input_size, output)) {
    return kDiceResultPlatformError;
  }
  return kDiceResultOk;
}

DiceResult DiceKdf(void* context_not_used, size_t length, const uint8_t* ikm,
                   size_t ikm_size, const uint8_t* salt, size_t salt_size,
                   const uint8_t* info, size_t info_size, uint8_t* output) {
  (void)context_not_used;
  if (0 != mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), salt,
                        salt_size, ikm, ikm_size, info, info_size, output,
                        length)) {
    return kDiceResultPlatformError;
  }
  return kDiceResultOk;
}
