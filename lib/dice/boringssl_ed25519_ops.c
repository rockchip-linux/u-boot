// Copyright 2022 Google LLC
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

// An implementation of the ed25519 signature operations using boringssl.

#include <stdint.h>

#include "dice/config/cose_key_config.h"
#include "dice/dice.h"
#include "dice/ops.h"
#include "openssl/curve25519.h"

#if DICE_PRIVATE_KEY_SEED_SIZE != 32
#error "Private key seed is expected to be 32 bytes."
#endif
#if DICE_PUBLIC_KEY_BUFFER_SIZE != 32
#error "Ed25519 needs 32 bytes to store the public key."
#endif
#if DICE_PRIVATE_KEY_BUFFER_SIZE != 64
#error "This Ed25519 implementation needs 64 bytes for the private key."
#endif
#if DICE_SIGNATURE_BUFFER_SIZE != 64
#error "Ed25519 needs 64 bytes to store the signature."
#endif

DiceResult DiceGetKeyParam(void* context_not_used,
                           DicePrincipal principal_not_used,
                           DiceKeyParam* key_param) {
  (void)context_not_used;
  (void)principal_not_used;
  key_param->public_key_size = DICE_PUBLIC_KEY_BUFFER_SIZE;
  key_param->signature_size = DICE_SIGNATURE_BUFFER_SIZE;

  key_param->cose_key_type = kCoseKeyKtyOkp;
  key_param->cose_key_algorithm = kCoseAlgEdDsa;
  key_param->cose_key_curve = kCoseCrvEd25519;
  return kDiceResultOk;
}

DiceResult DiceKeypairFromSeed(
    void* context_not_used, DicePrincipal principal_not_used,
    const uint8_t seed[DICE_PRIVATE_KEY_SEED_SIZE],
    uint8_t public_key[DICE_PUBLIC_KEY_BUFFER_SIZE],
    uint8_t private_key[DICE_PRIVATE_KEY_BUFFER_SIZE]) {
  (void)context_not_used;
  (void)principal_not_used;
  ED25519_keypair_from_seed(public_key, private_key, seed);
  return kDiceResultOk;
}

DiceResult DiceSign(void* context_not_used, const uint8_t* message,
                    size_t message_size,
                    const uint8_t private_key[DICE_PRIVATE_KEY_BUFFER_SIZE],
                    uint8_t signature[DICE_SIGNATURE_BUFFER_SIZE]) {
  (void)context_not_used;
  if (1 != ED25519_sign(signature, message, message_size, private_key)) {
    return kDiceResultPlatformError;
  }
  return kDiceResultOk;
}

DiceResult DiceVerify(void* context_not_used, const uint8_t* message,
                      size_t message_size,
                      const uint8_t signature[DICE_SIGNATURE_BUFFER_SIZE],
                      const uint8_t public_key[DICE_PUBLIC_KEY_BUFFER_SIZE]) {
  (void)context_not_used;
  if (1 != ED25519_verify(message, message_size, signature, public_key)) {
    return kDiceResultPlatformError;
  }
  return kDiceResultOk;
}
