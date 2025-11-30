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

#ifndef DICE_UTILS_H_
#define DICE_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include "dice/dice.h"

#ifdef __cplusplus
extern "C" {
#endif

// Converts arbitrary bytes to ascii hex, no NUL terminator is added. Up to
// |num_bytes| from |in| will be converted, and up to |out_size| bytes will be
// written to |out|. If |out_size| is less than |num_bytes| * 2, the output will
// be truncated at |out_size|.
void DiceHexEncode(const uint8_t* in, size_t num_bytes, void* out,
                   size_t out_size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DICE_UTILS_H_
