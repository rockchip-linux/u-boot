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

#include "dice/cbor_writer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum CborType {
  CBOR_TYPE_UINT = 0,
  CBOR_TYPE_NINT = 1,
  CBOR_TYPE_BSTR = 2,
  CBOR_TYPE_TSTR = 3,
  CBOR_TYPE_ARRAY = 4,
  CBOR_TYPE_MAP = 5,
  CBOR_TYPE_TAG = 6,
  CBOR_TYPE_SIMPLE = 7,
};

static bool CborWriteWouldOverflowCursor(size_t size, struct CborOut* out) {
  return size > SIZE_MAX - out->cursor;
}

static bool CborWriteFitsInBuffer(size_t size, struct CborOut* out) {
  return out->cursor <= out->buffer_size &&
         size <= out->buffer_size - out->cursor;
}

static void CborWriteType(enum CborType type, uint64_t val,
                          struct CborOut* out) {
  size_t size;
  if (val <= 23) {
    size = 1;
  } else if (val <= 0xff) {
    size = 2;
  } else if (val <= 0xffff) {
    size = 3;
  } else if (val <= 0xffffffff) {
    size = 5;
  } else {
    size = 9;
  }
  if (CborWriteWouldOverflowCursor(size, out)) {
    out->cursor = SIZE_MAX;
    return;
  }
  if (CborWriteFitsInBuffer(size, out)) {
    if (size == 1) {
      out->buffer[out->cursor] = (uint8_t)((type << 5) | val);
    } else if (size == 2) {
      out->buffer[out->cursor] = (uint8_t)((type << 5) | 24);
      out->buffer[out->cursor + 1] = val & 0xff;
    } else if (size == 3) {
      out->buffer[out->cursor] = (uint8_t)((type << 5) | 25);
      out->buffer[out->cursor + 1] = (val >> 8) & 0xff;
      out->buffer[out->cursor + 2] = val & 0xff;
    } else if (size == 5) {
      out->buffer[out->cursor] = (uint8_t)((type << 5) | 26);
      out->buffer[out->cursor + 1] = (val >> 24) & 0xff;
      out->buffer[out->cursor + 2] = (val >> 16) & 0xff;
      out->buffer[out->cursor + 3] = (val >> 8) & 0xff;
      out->buffer[out->cursor + 4] = val & 0xff;
    } else if (size == 9) {
      out->buffer[out->cursor] = (uint8_t)((type << 5) | 27);
      out->buffer[out->cursor + 1] = (val >> 56) & 0xff;
      out->buffer[out->cursor + 2] = (val >> 48) & 0xff;
      out->buffer[out->cursor + 3] = (val >> 40) & 0xff;
      out->buffer[out->cursor + 4] = (val >> 32) & 0xff;
      out->buffer[out->cursor + 5] = (val >> 24) & 0xff;
      out->buffer[out->cursor + 6] = (val >> 16) & 0xff;
      out->buffer[out->cursor + 7] = (val >> 8) & 0xff;
      out->buffer[out->cursor + 8] = val & 0xff;
    }
  }
  out->cursor += size;
}

static void* CborAllocStr(enum CborType type, size_t data_size,
                          struct CborOut* out) {
  CborWriteType(type, data_size, out);
  bool overflow = CborWriteWouldOverflowCursor(data_size, out);
  bool fit = CborWriteFitsInBuffer(data_size, out);
  void* ptr = (overflow || !fit) ? NULL : &out->buffer[out->cursor];
  out->cursor = overflow ? SIZE_MAX : out->cursor + data_size;
  return ptr;
}

static void CborWriteStr(enum CborType type, size_t data_size, const void* data,
                         struct CborOut* out) {
  uint8_t* ptr = CborAllocStr(type, data_size, out);
  if (ptr && data_size) {
    memcpy(ptr, data, data_size);
  }
}

void CborWriteInt(int64_t val, struct CborOut* out) {
  if (val < 0) {
    CborWriteType(CBOR_TYPE_NINT, (uint64_t)(-1 - val), out);
  } else {
    CborWriteType(CBOR_TYPE_UINT, (uint64_t)val, out);
  }
}

void CborWriteUint(uint64_t val, struct CborOut* out) {
  CborWriteType(CBOR_TYPE_UINT, val, out);
}

void CborWriteBstr(size_t data_size, const uint8_t* data, struct CborOut* out) {
  CborWriteStr(CBOR_TYPE_BSTR, data_size, data, out);
}

uint8_t* CborAllocBstr(size_t data_size, struct CborOut* out) {
  return CborAllocStr(CBOR_TYPE_BSTR, data_size, out);
}

void CborWriteTstr(const char* str, struct CborOut* out) {
  CborWriteStr(CBOR_TYPE_TSTR, strlen(str), str, out);
}

char* CborAllocTstr(size_t size, struct CborOut* out) {
  return CborAllocStr(CBOR_TYPE_TSTR, size, out);
}

void CborWriteArray(size_t num_elements, struct CborOut* out) {
  CborWriteType(CBOR_TYPE_ARRAY, num_elements, out);
}

void CborWriteMap(size_t num_pairs, struct CborOut* out) {
  CborWriteType(CBOR_TYPE_MAP, num_pairs, out);
}

void CborWriteTag(uint64_t tag, struct CborOut* out) {
  CborWriteType(CBOR_TYPE_TAG, tag, out);
}

void CborWriteFalse(struct CborOut* out) {
  CborWriteType(CBOR_TYPE_SIMPLE, /*val=*/20, out);
}

void CborWriteTrue(struct CborOut* out) {
  CborWriteType(CBOR_TYPE_SIMPLE, /*val=*/21, out);
}

void CborWriteNull(struct CborOut* out) {
  CborWriteType(CBOR_TYPE_SIMPLE, /*val=*/22, out);
}
