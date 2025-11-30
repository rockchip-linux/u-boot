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

#include "dice/utils.h"

#include <stdint.h>

void DiceHexEncode(const uint8_t* in, size_t num_bytes, void* out,
                   size_t out_size) {
  const uint8_t kHexMap[17] = "0123456789abcdef";
  size_t in_pos = 0;
  size_t out_pos = 0;
  uint8_t* out_bytes = out;
  for (in_pos = 0; in_pos < num_bytes && out_pos < out_size; ++in_pos) {
    out_bytes[out_pos++] = kHexMap[(in[in_pos] >> 4)];
    if (out_pos < out_size) {
      out_bytes[out_pos++] = kHexMap[in[in_pos] & 0xF];
    }
  }
}
