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

#ifndef DICE_CBOR_READER_H_
#define DICE_CBOR_READER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CborIn {
  const uint8_t* buffer;
  size_t buffer_size;
  size_t cursor;
};

enum CborReadResult {
  CBOR_READ_RESULT_OK,
  // The end of the input was reached before the token was fully read.
  CBOR_READ_RESULT_END,
  // A malformed or unsupported token was found.
  CBOR_READ_RESULT_MALFORMED,
  // The requested token was not found.
  CBOR_READ_RESULT_NOT_FOUND,
};

// Initializes an input stream for reading CBOR tokens.
static inline void CborInInit(const uint8_t* buffer, size_t buffer_size,
                              struct CborIn* in) {
  in->buffer = buffer;
  in->buffer_size = buffer_size;
  in->cursor = 0;
}

// Returns the number of bytes that have been read from the input.
static inline size_t CborInOffset(const struct CborIn* in) {
  return in->cursor;
}

// Returns whether the input stream has been fully consumed.
static inline bool CborInAtEnd(const struct CborIn* in) {
  return in->cursor == in->buffer_size;
}

// These functions read simple CBOR tokens from the input stream. Interpreting
// the greater structure of the data left to the caller and it is expected that
// these functions are just being used to validate and extract data from a known
// structure.
enum CborReadResult CborReadInt(struct CborIn* in, int64_t* val);
enum CborReadResult CborReadUint(struct CborIn* in, uint64_t* val);
enum CborReadResult CborReadBstr(struct CborIn* in, size_t* data_size,
                                 const uint8_t** data);
enum CborReadResult CborReadTstr(struct CborIn* in, size_t* size,
                                 const char** str);
enum CborReadResult CborReadArray(struct CborIn* in, size_t* num_elements);
enum CborReadResult CborReadMap(struct CborIn* in, size_t* num_pairs);
enum CborReadResult CborReadFalse(struct CborIn* in);
enum CborReadResult CborReadTrue(struct CborIn* in);
enum CborReadResult CborReadNull(struct CborIn* in);
// Returns CBOR_READ_RESULT_OK even if the value read does not correspond to
// a valid tag. See https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml
// for a registry of reserved and invalid tag values.
enum CborReadResult CborReadTag(struct CborIn* in, uint64_t* tag);

// Skips over the next CBOR item in the input. The item may contain nested
// items, in the case of an array, map, or tag, and this function will attempt
// to descend and skip all nested items in order to skip the parent item. There
// is a limit on the level of nesting, after which this function will fail with
// CBOR_READ_RESULT_MALFORMED.
#define CBOR_READ_SKIP_STACK_SIZE 10
enum CborReadResult CborReadSkip(struct CborIn* in);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // DICE_CBOR_READER_H_
