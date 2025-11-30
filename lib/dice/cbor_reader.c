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

#include "dice/cbor_reader.h"

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

static bool CborReadWouldOverflow(size_t size, struct CborIn* in) {
  return size > SIZE_MAX - in->cursor || in->cursor + size > in->buffer_size;
}

static enum CborReadResult CborPeekInitialValueAndArgument(struct CborIn* in,
                                                           uint8_t* size,
                                                           enum CborType* type,
                                                           uint64_t* val) {
  uint8_t initial_byte;
  uint8_t additional_information;
  uint64_t value;
  uint8_t bytes = 1;
  if (CborInAtEnd(in)) {
    return CBOR_READ_RESULT_END;
  }
  initial_byte = in->buffer[in->cursor];
  *type = initial_byte >> 5;
  additional_information = initial_byte & 0x1f;
  if (additional_information <= 23) {
    value = additional_information;
  } else if (additional_information <= 27) {
    bytes += 1 << (additional_information - 24);
    if (CborReadWouldOverflow(bytes, in)) {
      return CBOR_READ_RESULT_END;
    }
    value = 0;
    if (bytes == 2) {
      value |= in->buffer[in->cursor + 1];
    } else if (bytes == 3) {
      value |= (uint64_t)in->buffer[in->cursor + 1] << 8;
      value |= (uint64_t)in->buffer[in->cursor + 2];
    } else if (bytes == 5) {
      value |= (uint64_t)in->buffer[in->cursor + 1] << 24;
      value |= (uint64_t)in->buffer[in->cursor + 2] << 16;
      value |= (uint64_t)in->buffer[in->cursor + 3] << 8;
      value |= (uint64_t)in->buffer[in->cursor + 4];
    } else if (bytes == 9) {
      value |= (uint64_t)in->buffer[in->cursor + 1] << 56;
      value |= (uint64_t)in->buffer[in->cursor + 2] << 48;
      value |= (uint64_t)in->buffer[in->cursor + 3] << 40;
      value |= (uint64_t)in->buffer[in->cursor + 4] << 32;
      value |= (uint64_t)in->buffer[in->cursor + 5] << 24;
      value |= (uint64_t)in->buffer[in->cursor + 6] << 16;
      value |= (uint64_t)in->buffer[in->cursor + 7] << 8;
      value |= (uint64_t)in->buffer[in->cursor + 8];
    }
  } else {
    // Indefinite lengths and reserved values are not supported.
    return CBOR_READ_RESULT_MALFORMED;
  }
  *val = value;
  *size = bytes;
  return CBOR_READ_RESULT_OK;
}

static enum CborReadResult CborReadSize(struct CborIn* in, enum CborType type,
                                        size_t* size) {
  uint8_t bytes;
  enum CborType in_type;
  uint64_t raw;
  enum CborReadResult res =
      CborPeekInitialValueAndArgument(in, &bytes, &in_type, &raw);
  if (res != CBOR_READ_RESULT_OK) {
    return res;
  }
  if (in_type != type) {
    return CBOR_READ_RESULT_NOT_FOUND;
  }
  if (raw > SIZE_MAX) {
    return CBOR_READ_RESULT_MALFORMED;
  }
  *size = (size_t)raw;
  in->cursor += bytes;
  return CBOR_READ_RESULT_OK;
}

static enum CborReadResult CborReadStr(struct CborIn* in, enum CborType type,
                                       size_t* data_size,
                                       const uint8_t** data) {
  size_t size;
  struct CborIn peeker = *in;
  enum CborReadResult res = CborReadSize(&peeker, type, &size);
  if (res != CBOR_READ_RESULT_OK) {
    return res;
  }
  if (CborReadWouldOverflow(size, &peeker)) {
    return CBOR_READ_RESULT_END;
  }
  *data_size = size;
  *data = &in->buffer[peeker.cursor];
  in->cursor = peeker.cursor + size;
  return CBOR_READ_RESULT_OK;
}

static enum CborReadResult CborReadSimple(struct CborIn* in, uint8_t val) {
  uint8_t bytes;
  enum CborType type;
  uint64_t raw;
  enum CborReadResult res =
      CborPeekInitialValueAndArgument(in, &bytes, &type, &raw);
  if (res != CBOR_READ_RESULT_OK) {
    return res;
  }
  if (type != CBOR_TYPE_SIMPLE || raw != val) {
    return CBOR_READ_RESULT_NOT_FOUND;
  }
  in->cursor += bytes;
  return CBOR_READ_RESULT_OK;
}

enum CborReadResult CborReadInt(struct CborIn* in, int64_t* val) {
  uint8_t bytes;
  enum CborType type;
  uint64_t raw;
  enum CborReadResult res =
      CborPeekInitialValueAndArgument(in, &bytes, &type, &raw);
  if (res != CBOR_READ_RESULT_OK) {
    return res;
  }
  if (type != CBOR_TYPE_UINT && type != CBOR_TYPE_NINT) {
    return CBOR_READ_RESULT_NOT_FOUND;
  }
  if (raw > INT64_MAX) {
    return CBOR_READ_RESULT_MALFORMED;
  }
  *val = (type == CBOR_TYPE_NINT) ? (-1 - (int64_t)raw) : (int64_t)raw;
  in->cursor += bytes;
  return CBOR_READ_RESULT_OK;
}

enum CborReadResult CborReadUint(struct CborIn* in, uint64_t* val) {
  uint8_t bytes;
  enum CborType type;
  enum CborReadResult res =
      CborPeekInitialValueAndArgument(in, &bytes, &type, val);
  if (res != CBOR_READ_RESULT_OK) {
    return res;
  }
  if (type != CBOR_TYPE_UINT) {
    return CBOR_READ_RESULT_NOT_FOUND;
  }
  in->cursor += bytes;
  return CBOR_READ_RESULT_OK;
}

enum CborReadResult CborReadBstr(struct CborIn* in, size_t* data_size,
                                 const uint8_t** data) {
  return CborReadStr(in, CBOR_TYPE_BSTR, data_size, data);
}

enum CborReadResult CborReadTstr(struct CborIn* in, size_t* size,
                                 const char** str) {
  return CborReadStr(in, CBOR_TYPE_TSTR, size, (const uint8_t**)str);
}

enum CborReadResult CborReadArray(struct CborIn* in, size_t* num_elements) {
  return CborReadSize(in, CBOR_TYPE_ARRAY, num_elements);
}

enum CborReadResult CborReadMap(struct CborIn* in, size_t* num_pairs) {
  return CborReadSize(in, CBOR_TYPE_MAP, num_pairs);
}

enum CborReadResult CborReadTag(struct CborIn* in, uint64_t* tag) {
  uint8_t bytes;
  enum CborType type;
  enum CborReadResult res =
      CborPeekInitialValueAndArgument(in, &bytes, &type, tag);
  if (res != CBOR_READ_RESULT_OK) {
    return res;
  }
  if (type != CBOR_TYPE_TAG) {
    return CBOR_READ_RESULT_NOT_FOUND;
  }
  in->cursor += bytes;
  return CBOR_READ_RESULT_OK;
}

enum CborReadResult CborReadFalse(struct CborIn* in) {
  return CborReadSimple(in, /*val=*/20);
}

enum CborReadResult CborReadTrue(struct CborIn* in) {
  return CborReadSimple(in, /*val=*/21);
}

enum CborReadResult CborReadNull(struct CborIn* in) {
  return CborReadSimple(in, /*val=*/22);
}

enum CborReadResult CborReadSkip(struct CborIn* in) {
  struct CborIn peeker = *in;
  size_t size_stack[CBOR_READ_SKIP_STACK_SIZE];
  size_t stack_size = 0;

  size_stack[stack_size++] = 1;

  while (stack_size > 0) {
    // Get the type
    uint8_t bytes;
    enum CborType type;
    uint64_t val;
    enum CborReadResult res;

    res = CborPeekInitialValueAndArgument(&peeker, &bytes, &type, &val);
    if (res != CBOR_READ_RESULT_OK) {
      return res;
    }

    if (CborReadWouldOverflow(bytes, &peeker)) {
      return CBOR_READ_RESULT_END;
    }
    peeker.cursor += bytes;

    if (--size_stack[stack_size - 1] == 0) {
      --stack_size;
    }

    switch (type) {
      case CBOR_TYPE_UINT:
      case CBOR_TYPE_NINT:
      case CBOR_TYPE_SIMPLE:
        continue;
      case CBOR_TYPE_BSTR:
      case CBOR_TYPE_TSTR:
        if (val > SIZE_MAX || CborReadWouldOverflow((size_t)val, &peeker)) {
          return CBOR_READ_RESULT_END;
        }
        peeker.cursor += val;
        continue;
      case CBOR_TYPE_MAP:
        if (val > UINT64_MAX / 2) {
          return CBOR_READ_RESULT_END;
        }
        val *= 2;
        break;
      case CBOR_TYPE_TAG:
        val = 1;
        break;
      case CBOR_TYPE_ARRAY:
        break;
      default:
        return CBOR_READ_RESULT_MALFORMED;
    }

    // Push a new level of nesting to the stack.
    if (val == 0) {
      continue;
    }
    if (stack_size == CBOR_READ_SKIP_STACK_SIZE) {
      return CBOR_READ_RESULT_MALFORMED;
    }
    if (val > SIZE_MAX) {
      return CBOR_READ_RESULT_END;
    }
    size_stack[stack_size++] = (size_t)val;
  }

  in->cursor = peeker.cursor;
  return CBOR_READ_RESULT_OK;
}
