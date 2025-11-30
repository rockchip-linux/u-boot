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

#ifndef DICE_OPS_CLEAR_MEMORY_H_
#define DICE_OPS_CLEAR_MEMORY_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Securely clears |size| bytes at |address|. This project contains a basic
// implementation. OPENSSL_cleanse from boringssl, SecureZeroMemory from
// Windows and memset_s from C11 could also be used as an implementation but a
// particular target platform or toolchain may have a better implementation
// available that can be plugged in here. Care may be needed to ensure sensitive
// data does not leak due to features such as caches.
void DiceClearMemory(void* context, size_t size, void* address);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DICE_OPS_CLEAR_MEMORY_H_
