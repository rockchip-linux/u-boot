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

#ifndef DICE_CBOR_WRITER_H_
#define DICE_CBOR_WRITER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CborOut {
  uint8_t* buffer;
  size_t buffer_size;
  size_t cursor;
};

// Initializes an output stream for writing CBOR tokens.
static inline void CborOutInit(uint8_t* buffer, size_t buffer_size,
                               struct CborOut* out) {
  out->buffer = buffer;
  out->buffer_size = buffer_size;
  out->cursor = 0;
}

// Returns the number of bytes of encoded data. If |CborOutOverflowed()|
// returns false, this number of bytes have been written, otherwise, this is the
// number of bytes that that would have been written had there been space.
static inline size_t CborOutSize(const struct CborOut* out) {
  return out->cursor;
}

// Returns whether the |out| buffer contains the encoded tokens written to it or
// whether the encoded tokens did not fit and the contents of the buffer should
// be considered invalid.
static inline bool CborOutOverflowed(const struct CborOut* out) {
  return out->cursor == SIZE_MAX || out->cursor > out->buffer_size;
}

// These functions write simple deterministically encoded CBOR tokens to an
// output buffer. The offset is always increased, even if there is not enough
// space in the output buffer to allow for measurement of the encoded data.
// Use |CborOutOverflowed()| to check whether or not the buffer successfully
// contains all of the of the encoded data.
//
// Complex types are constructed from these simple types, see RFC 8949. The
// caller is responsible for correct and deterministic encoding of complex
// types.
void CborWriteInt(int64_t val, struct CborOut* out);
void CborWriteUint(uint64_t val, struct CborOut* out);
void CborWriteBstr(size_t data_size, const uint8_t* data, struct CborOut* out);
void CborWriteTstr(const char* str, struct CborOut* out);
void CborWriteArray(size_t num_elements, struct CborOut* out);
void CborWriteMap(size_t num_pairs, struct CborOut* out);
void CborWriteTag(uint64_t tag, struct CborOut* out);
void CborWriteFalse(struct CborOut* out);
void CborWriteTrue(struct CborOut* out);
void CborWriteNull(struct CborOut* out);

// These functions write the type header and reserve space for the caller to
// populate. The reserved space is left uninitialized. Returns NULL if space
// could not be reserved in the output buffer.
uint8_t* CborAllocBstr(size_t data_size, struct CborOut* out);
char* CborAllocTstr(size_t size, struct CborOut* out);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DICE_CBOR_WRITER_H_
