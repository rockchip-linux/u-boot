// Copyright 2024 Google LLC
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

#ifndef DICE_TYPES_H_
#define DICE_TYPES_H_

#include <stddef.h>
#include <stdint.h>
#include <linux/types.h>

typedef enum {
  kDiceResultOk,
  kDiceResultInvalidInput,
  kDiceResultBufferTooSmall,
  kDiceResultPlatformError,
} DiceResult;

typedef enum {
  kDicePrincipalAuthority,
  kDicePrincipalSubject,
} DicePrincipal;

typedef enum {
  kDiceModeNotInitialized,
  kDiceModeNormal,
  kDiceModeDebug,
  kDiceModeMaintenance,
} DiceMode;

typedef enum {
  kDiceConfigTypeInline,
  kDiceConfigTypeDescriptor,
} DiceConfigType;

// Parameters related to the DICE key operations.
//
// Fields:
//   public_key_size: Actual size of the public key.
//   signature_size: Actual size of the signature.
//   cose_key_type: Key type that is represented as the 'kty' member of the
//    COSE_Key object as per RFC 8152.
//   cose_key_algorithm: COSE algorithm identifier for the key.
//   cose_key_curve: COSE curve identifier for the key.
typedef struct DiceKeyParam_ {
  size_t public_key_size;
  size_t signature_size;
  int64_t cose_key_type;
  int64_t cose_key_algorithm;
  int64_t cose_key_curve;
} DiceKeyParam;

#endif  // DICE_TYPES_H_
