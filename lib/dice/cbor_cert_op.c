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

// This is a DiceGenerateCertificate implementation that generates a CWT-style
// CBOR certificate. The function DiceCoseEncodePublicKey depends on the
// signature algorithm type, and must be implemented elsewhere.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dice/cbor_writer.h"
#include "dice/config/cose_key_config.h"
#include "dice/dice.h"
#include "dice/ops.h"
#include "dice/ops/trait/cose.h"
#include "dice/profile_name.h"
#include "dice/utils.h"

// Max size of COSE_Key encoding.
#define DICE_MAX_PUBLIC_KEY_SIZE (DICE_PUBLIC_KEY_BUFFER_SIZE + 32)
// Max size of the COSE_Sign1 protected attributes.
#define DICE_MAX_PROTECTED_ATTRIBUTES_SIZE 16

static DiceResult EncodeProtectedAttributes(void* context,
                                            DicePrincipal principal,
                                            size_t buffer_size, uint8_t* buffer,
                                            size_t* encoded_size) {
  // Constants per RFC 8152.
  const int64_t kCoseHeaderAlgLabel = 1;

  struct CborOut out;
  CborOutInit(buffer, buffer_size, &out);
  CborWriteMap(/*num_elements=*/1, &out);
  // Add the algorithm.
  DiceKeyParam key_param;
  DiceResult result = DiceGetKeyParam(context, principal, &key_param);
  if (result != kDiceResultOk) {
    return result;
  }
  CborWriteInt(kCoseHeaderAlgLabel, &out);
  CborWriteInt(key_param.cose_key_algorithm, &out);
  *encoded_size = CborOutSize(&out);
  if (CborOutOverflowed(&out)) {
    return kDiceResultBufferTooSmall;
  }
  return kDiceResultOk;
}

static DiceResult EncodeCoseTbs(const uint8_t* protected_attributes,
                                size_t protected_attributes_size,
                                size_t payload_size, const uint8_t* aad,
                                size_t aad_size, size_t buffer_size,
                                uint8_t* buffer, uint8_t** payload,
                                size_t* encoded_size) {
  struct CborOut out;
  CborOutInit(buffer, buffer_size, &out);
  // TBS is an array of four elements.
  CborWriteArray(/*num_elements=*/4, &out);
  // Context string field.
  CborWriteTstr("Signature1", &out);
  // Protected attributes from COSE_Sign1.
  CborWriteBstr(protected_attributes_size, protected_attributes, &out);
  // Additional authenticated data.
  CborWriteBstr(aad_size, aad, &out);
  // Space for the payload, to be filled in by the caller.
  *payload = CborAllocBstr(payload_size, &out);
  *encoded_size = CborOutSize(&out);
  if (CborOutOverflowed(&out)) {
    return kDiceResultBufferTooSmall;
  }
  return kDiceResultOk;
}

static DiceResult EncodeCoseSign1(
    void* context, const uint8_t* protected_attributes,
    size_t protected_attributes_size, const uint8_t* payload,
    size_t payload_size, bool move_payload,
    const uint8_t signature[DICE_SIGNATURE_BUFFER_SIZE], size_t buffer_size,
    uint8_t* buffer, size_t* encoded_size) {
  struct CborOut out;
  CborOutInit(buffer, buffer_size, &out);
  // COSE_Sign1 is an array of four elements.
  CborWriteArray(/*num_elements=*/4, &out);
  // Protected attributes.
  CborWriteBstr(protected_attributes_size, protected_attributes, &out);
  // Empty map for unprotected attributes.
  CborWriteMap(/*num_pairs=*/0, &out);
  // Payload.
  if (move_payload) {
    // The payload is already present in the buffer, so we can move it into
    // place.
    uint8_t* payload_alloc = CborAllocBstr(payload_size, &out);
    if (payload_alloc) {
      // We're assuming what we've written above is small enough that it doesn't
      // overwrite the payload. Check in case that stops being true.
      if (payload < payload_alloc) {
        return kDiceResultPlatformError;
      }
      memmove(payload_alloc, payload, payload_size);
    }
  } else {
    CborWriteBstr(payload_size, payload, &out);
  }
  DiceKeyParam key_param;
  DiceResult result =
      DiceGetKeyParam(context, kDicePrincipalAuthority, &key_param);
  if (result != kDiceResultOk) {
    return result;
  }
  // Signature.
  CborWriteBstr(/*num_elements=*/key_param.signature_size, signature, &out);
  *encoded_size = CborOutSize(&out);
  if (CborOutOverflowed(&out)) {
    return kDiceResultBufferTooSmall;
  }
  return kDiceResultOk;
}

DiceResult DiceCoseSignAndEncodeSign1(
    void* context, const uint8_t* payload, size_t payload_size,
    const uint8_t* aad, size_t aad_size,
    const uint8_t private_key[DICE_PRIVATE_KEY_BUFFER_SIZE], size_t buffer_size,
    uint8_t* buffer, size_t* encoded_size) {
  DiceResult result;

  *encoded_size = 0;

  // The encoded protected attributes are used in the TBS and the final
  // COSE_Sign1 structure.
  uint8_t protected_attributes[DICE_MAX_PROTECTED_ATTRIBUTES_SIZE];
  size_t protected_attributes_size = 0;
  result = EncodeProtectedAttributes(
      context, kDicePrincipalAuthority, sizeof(protected_attributes),
      protected_attributes, &protected_attributes_size);
  if (result != kDiceResultOk) {
    return kDiceResultPlatformError;
  }

  // Construct a To-Be-Signed (TBS) structure based on the relevant fields of
  // the COSE_Sign1.
  uint8_t* payload_buffer;
  result = EncodeCoseTbs(protected_attributes, protected_attributes_size,
                         payload_size, aad, aad_size, buffer_size, buffer,
                         &payload_buffer, encoded_size);
  if (result != kDiceResultOk) {
    // Check how big the buffer needs to be in total.
    size_t final_encoded_size = 0;
    EncodeCoseSign1(context, protected_attributes, protected_attributes_size,
                    payload, payload_size, /*move_payload=*/false,
                    /*signature=*/NULL, /*buffer_size=*/0, /*buffer=*/NULL,
                    &final_encoded_size);
    if (*encoded_size < final_encoded_size) {
      *encoded_size = final_encoded_size;
    }
    return result;
  }
  memcpy(payload_buffer, payload, payload_size);

  // Sign the TBS with the authority key.
  uint8_t signature[DICE_SIGNATURE_BUFFER_SIZE];
  result = DiceSign(context, buffer, *encoded_size, private_key, signature);
  if (result != kDiceResultOk) {
    return result;
  }

  // The final certificate is an untagged COSE_Sign1 structure.
  return EncodeCoseSign1(context, protected_attributes,
                         protected_attributes_size, payload, payload_size,
                         /*move_payload=*/false, signature, buffer_size, buffer,
                         encoded_size);
}

// Encodes a CBOR Web Token (CWT) with an issuer, subject, and additional
// fields.
static DiceResult EncodeCwt(void* context, const DiceInputValues* input_values,
                            const char* authority_id_hex,
                            const char* subject_id_hex,
                            const uint8_t* encoded_public_key,
                            size_t encoded_public_key_size, size_t buffer_size,
                            uint8_t* buffer, size_t* encoded_size) {
  // Constants per RFC 8392.
  const int64_t kCwtIssuerLabel = 1;
  const int64_t kCwtSubjectLabel = 2;
  // Constants per the Open Profile for DICE specification.
  const int64_t kCodeHashLabel = -4670545;
  const int64_t kCodeDescriptorLabel = -4670546;
  const int64_t kConfigHashLabel = -4670547;
  const int64_t kConfigDescriptorLabel = -4670548;
  const int64_t kAuthorityHashLabel = -4670549;
  const int64_t kAuthorityDescriptorLabel = -4670550;
  const int64_t kModeLabel = -4670551;
  const int64_t kSubjectPublicKeyLabel = -4670552;
  const int64_t kKeyUsageLabel = -4670553;
  const int64_t kProfileNameLabel = -4670554;
  // Key usage constant per RFC 5280.
  const uint8_t kKeyUsageCertSign = 32;

  // Count the number of entries.
  uint32_t map_pairs = 7;
  if (input_values->code_descriptor_size > 0) {
    map_pairs += 1;
  }
  if (input_values->config_type == kDiceConfigTypeDescriptor) {
    map_pairs += 2;
  } else {
    map_pairs += 1;
  }
  if (input_values->authority_descriptor_size > 0) {
    map_pairs += 1;
  }

  DiceKeyParam key_param;
  DiceResult result =
      DiceGetKeyParam(context, kDicePrincipalSubject, &key_param);
  if (result != kDiceResultOk) {
    return result;
  }
  if (DICE_PROFILE_NAME) {
    map_pairs += 1;
  }

  struct CborOut out;
  CborOutInit(buffer, buffer_size, &out);
  CborWriteMap(map_pairs, &out);
  // Add the issuer.
  CborWriteInt(kCwtIssuerLabel, &out);
  CborWriteTstr(authority_id_hex, &out);
  // Add the subject.
  CborWriteInt(kCwtSubjectLabel, &out);
  CborWriteTstr(subject_id_hex, &out);
  // Add the code hash.
  CborWriteInt(kCodeHashLabel, &out);
  CborWriteBstr(DICE_HASH_SIZE, input_values->code_hash, &out);
  // Add the code descriptor, if provided.
  if (input_values->code_descriptor_size > 0) {
    CborWriteInt(kCodeDescriptorLabel, &out);
    CborWriteBstr(input_values->code_descriptor_size,
                  input_values->code_descriptor, &out);
  }
  // Add the config inputs.
  if (input_values->config_type == kDiceConfigTypeDescriptor) {
    uint8_t config_descriptor_hash[DICE_HASH_SIZE];
    // Skip hashing if we're not going to use the answer.
    if (!CborOutOverflowed(&out)) {
      result = DiceHash(context, input_values->config_descriptor,
                        input_values->config_descriptor_size,
                        config_descriptor_hash);
      if (result != kDiceResultOk) {
        return result;
      }
    }
    // Add the config descriptor.
    CborWriteInt(kConfigDescriptorLabel, &out);
    CborWriteBstr(input_values->config_descriptor_size,
                  input_values->config_descriptor, &out);
    // Add the Config hash.
    CborWriteInt(kConfigHashLabel, &out);
    CborWriteBstr(DICE_HASH_SIZE, config_descriptor_hash, &out);
  } else if (input_values->config_type == kDiceConfigTypeInline) {
    // Add the inline config.
    CborWriteInt(kConfigDescriptorLabel, &out);
    CborWriteBstr(DICE_INLINE_CONFIG_SIZE, input_values->config_value, &out);
  }
  // Add the authority inputs.
  CborWriteInt(kAuthorityHashLabel, &out);
  CborWriteBstr(DICE_HASH_SIZE, input_values->authority_hash, &out);
  if (input_values->authority_descriptor_size > 0) {
    CborWriteInt(kAuthorityDescriptorLabel, &out);
    CborWriteBstr(input_values->authority_descriptor_size,
                  input_values->authority_descriptor, &out);
  }
  uint8_t mode_byte = input_values->mode;
  uint8_t key_usage = kKeyUsageCertSign;
  // Add the mode input.
  CborWriteInt(kModeLabel, &out);
  CborWriteBstr(/*data_sisze=*/1, &mode_byte, &out);
  // Add the subject public key.
  CborWriteInt(kSubjectPublicKeyLabel, &out);
  CborWriteBstr(encoded_public_key_size, encoded_public_key, &out);
  // Add the key usage.
  CborWriteInt(kKeyUsageLabel, &out);
  CborWriteBstr(/*data_size=*/1, &key_usage, &out);
  // Add the profile name
  if (DICE_PROFILE_NAME) {
    CborWriteInt(kProfileNameLabel, &out);
    CborWriteTstr(DICE_PROFILE_NAME, &out);
  }
  *encoded_size = CborOutSize(&out);
  if (CborOutOverflowed(&out)) {
    return kDiceResultBufferTooSmall;
  }
  return kDiceResultOk;
}

DiceResult DiceGenerateCertificate(
    void* context,
    const uint8_t subject_private_key_seed[DICE_PRIVATE_KEY_SEED_SIZE],
    const uint8_t authority_private_key_seed[DICE_PRIVATE_KEY_SEED_SIZE],
    const DiceInputValues* input_values, size_t certificate_buffer_size,
    uint8_t* certificate, size_t* certificate_actual_size) {
  DiceResult result = kDiceResultOk;

  *certificate_actual_size = 0;
  if (input_values->config_type != kDiceConfigTypeDescriptor &&
      input_values->config_type != kDiceConfigTypeInline) {
    return kDiceResultInvalidInput;
  }

  // Declare buffers which are cleared on 'goto out'.
  uint8_t subject_private_key[DICE_PRIVATE_KEY_BUFFER_SIZE];
  uint8_t authority_private_key[DICE_PRIVATE_KEY_BUFFER_SIZE];

  // Derive keys and IDs from the private key seeds.
  uint8_t subject_public_key[DICE_PUBLIC_KEY_BUFFER_SIZE];
  result = DiceKeypairFromSeed(context, kDicePrincipalSubject,
                               subject_private_key_seed, subject_public_key,
                               subject_private_key);
  if (result != kDiceResultOk) {
    goto out;
  }

  DiceKeyParam subject_key_param;
  DiceKeyParam authority_key_param;
  result = DiceGetKeyParam(context, kDicePrincipalSubject, &subject_key_param);
  if (result != kDiceResultOk) {
    goto out;
  }
  result =
      DiceGetKeyParam(context, kDicePrincipalAuthority, &authority_key_param);
  if (result != kDiceResultOk) {
    goto out;
  }

  uint8_t subject_id[DICE_ID_SIZE];
  result =
      DiceDeriveCdiCertificateId(context, subject_public_key,
                                 subject_key_param.public_key_size, subject_id);
  if (result != kDiceResultOk) {
    goto out;
  }
  char subject_id_hex[41];
  DiceHexEncode(subject_id, sizeof(subject_id), subject_id_hex,
                sizeof(subject_id_hex));
  subject_id_hex[sizeof(subject_id_hex) - 1] = '\0';

  uint8_t authority_public_key[DICE_PUBLIC_KEY_BUFFER_SIZE];
  result = DiceKeypairFromSeed(context, kDicePrincipalAuthority,
                               authority_private_key_seed, authority_public_key,
                               authority_private_key);
  if (result != kDiceResultOk) {
    goto out;
  }

  uint8_t authority_id[DICE_ID_SIZE];
  result = DiceDeriveCdiCertificateId(context, authority_public_key,
                                      authority_key_param.public_key_size,
                                      authority_id);
  if (result != kDiceResultOk) {
    goto out;
  }
  char authority_id_hex[41];
  DiceHexEncode(authority_id, sizeof(authority_id), authority_id_hex,
                sizeof(authority_id_hex));
  authority_id_hex[sizeof(authority_id_hex) - 1] = '\0';

  // The public key encoded as a COSE_Key structure is embedded in the CWT.
  uint8_t encoded_public_key[DICE_MAX_PUBLIC_KEY_SIZE];
  size_t encoded_public_key_size = 0;
  result = DiceCoseEncodePublicKey(
      context, kDicePrincipalSubject, subject_public_key,
      sizeof(encoded_public_key), encoded_public_key, &encoded_public_key_size);
  if (result != kDiceResultOk) {
    result = kDiceResultPlatformError;
    goto out;
  }

  // The encoded protected attributes are used in the TBS and the final
  // COSE_Sign1 structure.
  uint8_t protected_attributes[DICE_MAX_PROTECTED_ATTRIBUTES_SIZE];
  size_t protected_attributes_size = 0;
  result = EncodeProtectedAttributes(
      context, kDicePrincipalAuthority, sizeof(protected_attributes),
      protected_attributes, &protected_attributes_size);
  if (result != kDiceResultOk) {
    result = kDiceResultPlatformError;
    goto out;
  }

  // Find out how big the CWT will be.
  size_t cwt_size;
  EncodeCwt(context, input_values, authority_id_hex, subject_id_hex,
            encoded_public_key, encoded_public_key_size, /*buffer_size=*/0,
            /*buffer=*/NULL, &cwt_size);

  // We need space to assemble the TBS. The size of the buffer needed depends on
  // the size of the CWT, which is outside our control (e.g. it might have a
  // very large config descriptor). So we use the certificate buffer as
  // temporary storage; if we run out of space we will make sure the caller
  // knows the size we actually need for this.
  // Encode the TBS, leaving space for the final payload (the CWT).
  uint8_t* cwt_ptr;
  size_t tbs_size;
  result =
      EncodeCoseTbs(protected_attributes, protected_attributes_size, cwt_size,
                    /*aad=*/NULL, /*aad_size=*/0, certificate_buffer_size,
                    certificate, &cwt_ptr, &tbs_size);

  if (result != kDiceResultOk) {
    // There wasn't enough space to put together the TBS. The total buffer size
    // we need is either the amount needed for the TBS, or the amount needed for
    // encoded payload and signature.
    size_t final_encoded_size = 0;
    EncodeCoseSign1(context, protected_attributes, protected_attributes_size,
                    cwt_ptr, cwt_size, /*move_payload=*/false,
                    /*signature=*/NULL, /*buffer_size=*/0, /*buffer=*/NULL,
                    &final_encoded_size);
    *certificate_actual_size =
        final_encoded_size > tbs_size ? final_encoded_size : tbs_size;
    result = kDiceResultBufferTooSmall;
    goto out;
  }

  // Now we can encode the payload directly into the allocated BSTR in the TBS.
  size_t final_cwt_size;
  result = EncodeCwt(context, input_values, authority_id_hex, subject_id_hex,
                     encoded_public_key, encoded_public_key_size, cwt_size,
                     cwt_ptr, &final_cwt_size);
  if (result == kDiceResultBufferTooSmall || final_cwt_size != cwt_size) {
    result = kDiceResultPlatformError;
  }
  if (result != kDiceResultOk) {
    goto out;
  }

  // Sign the now-complete TBS.
  uint8_t signature[DICE_SIGNATURE_BUFFER_SIZE];
  result = DiceSign(context, certificate, tbs_size, authority_private_key,
                    signature);
  if (result != kDiceResultOk) {
    goto out;
  }

  // And now we can produce the complete CoseSign1, including the signature, and
  // moving the payload into place as we do it.
  result = EncodeCoseSign1(
      context, protected_attributes, protected_attributes_size, cwt_ptr,
      cwt_size, /*move_payload=*/true, signature, certificate_buffer_size,
      certificate, certificate_actual_size);

out:
  DiceClearMemory(context, sizeof(subject_private_key), subject_private_key);
  DiceClearMemory(context, sizeof(authority_private_key),
                  authority_private_key);

  return result;
}

DiceResult DiceCoseEncodePublicKey(
    void* context, DicePrincipal principal,
    const uint8_t public_key[DICE_PUBLIC_KEY_BUFFER_SIZE], size_t buffer_size,
    uint8_t* buffer, size_t* encoded_size) {
  DiceKeyParam key_param;
  DiceResult result = DiceGetKeyParam(context, principal, &key_param);
  if (result != kDiceResultOk) {
    return result;
  }
  struct CborOut out;
  CborOutInit(buffer, buffer_size, &out);
  if (key_param.cose_key_type == kCoseKeyKtyOkp) {
    CborWriteMap(/*num_pairs=*/5, &out);
  } else if (key_param.cose_key_type == kCoseKeyKtyEc2) {
    CborWriteMap(/*num_pairs=*/6, &out);
  } else {
    return kDiceResultInvalidInput;
  }
  // Add the key type.
  CborWriteInt(kCoseKeyKtyLabel, &out);
  CborWriteInt(key_param.cose_key_type, &out);
  // Add the algorithm.
  CborWriteInt(kCoseKeyAlgLabel, &out);
  CborWriteInt(key_param.cose_key_algorithm, &out);
  // Add the KeyOps.
  CborWriteInt(kCoseKeyOpsLabel, &out);
  CborWriteArray(/*num_elements=*/1, &out);
  CborWriteInt(kCoseKeyOpsVerify, &out);
  // Add the curve.
  CborWriteInt(kCoseKeyCrvLabel, &out);
  CborWriteInt(key_param.cose_key_curve, &out);

  // Add the public key.
  if (key_param.cose_key_type == kCoseKeyKtyOkp) {
    CborWriteInt(kCoseKeyXLabel, &out);
    CborWriteBstr(key_param.public_key_size, public_key, &out);
  } else if (key_param.cose_key_type == kCoseKeyKtyEc2) {
    // Add the subject public key x and y coordinates
    int xy_param_size = key_param.public_key_size / 2;
    CborWriteInt(kCoseKeyXLabel, &out);
    CborWriteBstr(xy_param_size, &public_key[0], &out);
    CborWriteInt(kCoseKeyYLabel, &out);
    CborWriteBstr(xy_param_size, &public_key[xy_param_size], &out);
  }

  *encoded_size = CborOutSize(&out);
  if (CborOutOverflowed(&out)) {
    return kDiceResultBufferTooSmall;
  }
  return kDiceResultOk;
}
