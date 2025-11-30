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

#ifndef DICE_OPS_TRAIT_COSE_H_
#define DICE_OPS_TRAIT_COSE_H_

#include <dice/config.h>
#include <dice/dice.h>
#include <stddef.h>
#include <stdint.h>

// These functions may optionally be implemented by a COSE based integration.
// They aren't directly depended on by the main DICE functions but provide
// extra utilities that can be used as part of the integration.

#ifdef __cplusplus
extern "C" {
#endif

// Encodes a public key into |buffer| as a COSE_Key structure. On success,
// |encoded_size| is set to the number of bytes used. If
// kDiceResultBufferTooSmall is returned |encoded_size| will be set to the
// required size of the buffer.
DiceResult DiceCoseEncodePublicKey(
    void* context, DicePrincipal principal,
    const uint8_t public_key[DICE_PUBLIC_KEY_BUFFER_SIZE], size_t buffer_size,
    uint8_t* buffer, size_t* encoded_size);

// Signs the payload and additional authenticated data, formatting the result
// into a COSE_Sign1 structure. There are no unprotected attributes included in
// the result.
//
// |buffer| is used to hold the intermediate To-Be-Signed (TBS) structure and
// then the final result. On success, |encoded_size| is set to the size of the
// final result in |buffer|. If kDiceResultBufferTooSmall is returned,
// |encoded_size| will be set to the required size of the buffer.
DiceResult DiceCoseSignAndEncodeSign1(
    void* context, const uint8_t* payload, size_t payload_size,
    const uint8_t* aad, size_t aad_size,
    const uint8_t private_key[DICE_PRIVATE_KEY_BUFFER_SIZE], size_t buffer_size,
    uint8_t* buffer, size_t* encoded_size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DICE_OPS_TRAIT_COSE_H_
