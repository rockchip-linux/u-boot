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

// This is a basic, standalone implementation of DiceClearMemory that aims to
// write zeros to the memory without the compiler optimizing it away by using a
// volatile data pointer. Attention has not been given to performance, clearing
// caches or other potential side channels.

#include "dice/ops/clear_memory.h"

#include <stdint.h>
#include <linux/types.h>

void DiceClearMemory(void* context, size_t size, void* address) {
  (void)context;
  volatile uint8_t* p = address;
  for (size_t i = 0; i < size; i++) {
    p[i] = 0;
  }
}
