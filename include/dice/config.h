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

#ifndef DICE_CONFIG_BORINGSSL_ECDSA_P256_DICE_CONFIG_H_
#define DICE_CONFIG_BORINGSSL_ECDSA_P256_DICE_CONFIG_H_

// ECDSA P256
// From table 1 of RFC 9053
#define DICE_PUBLIC_KEY_BUFFER_SIZE 64
#define DICE_PRIVATE_KEY_BUFFER_SIZE 32
#define DICE_SIGNATURE_BUFFER_SIZE 64

#endif  // DICE_CONFIG_BORINGSSL_ECDSA_P256_DICE_CONFIG_H_
