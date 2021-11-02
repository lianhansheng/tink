// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TINK_AEAD_KMS_AEAD_KEY_MANAGER_H_
#define TINK_AEAD_KMS_AEAD_KEY_MANAGER_H_

#include <string>

#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "tink/aead.h"
#include "tink/core/key_type_manager.h"
#include "tink/kms_clients.h"
#include "tink/util/constants.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"
#include "tink/util/validation.h"
#include "proto/kms_aead.pb.h"

namespace crypto {
namespace tink {

class KmsAeadKeyManager
    : public KeyTypeManager<google::crypto::tink::KmsAeadKey,
                            google::crypto::tink::KmsAeadKeyFormat,
                            List<Aead>> {
 public:
  class AeadFactory : public PrimitiveFactory<Aead> {
    crypto::tink::util::StatusOr<std::unique_ptr<Aead>> Create(
        const google::crypto::tink::KmsAeadKey& kms_aead_key) const override {
      const auto& key_uri = kms_aead_key.params().key_uri();
      auto kms_client_result = KmsClients::Get(key_uri);
      if (!kms_client_result.ok()) return kms_client_result.status();
      return kms_client_result.ValueOrDie()->GetAead(key_uri);
    }
  };

  KmsAeadKeyManager() : KeyTypeManager(absl::make_unique<AeadFactory>()) {}

  uint32_t get_version() const override { return 0; }

  google::crypto::tink::KeyData::KeyMaterialType key_material_type()
      const override {
    return google::crypto::tink::KeyData::REMOTE;
  }

  const std::string& get_key_type() const override { return key_type_; }

  crypto::tink::util::Status ValidateKey(
      const google::crypto::tink::KmsAeadKey& key) const override {
    crypto::tink::util::Status status =
        ValidateVersion(key.version(), get_version());
    if (!status.ok()) return status;
    return ValidateKeyFormat(key.params());
  }

  crypto::tink::util::Status ValidateKeyFormat(
      const google::crypto::tink::KmsAeadKeyFormat& key_format) const override {
    if (key_format.key_uri().empty()) {
      return crypto::tink::util::Status(absl::StatusCode::kInvalidArgument,
                                        "Missing key_uri.");
    }
    return util::OkStatus();
  }

  crypto::tink::util::StatusOr<google::crypto::tink::KmsAeadKey> CreateKey(
      const google::crypto::tink::KmsAeadKeyFormat& key_format) const override {
    google::crypto::tink::KmsAeadKey kms_aead_key;
    kms_aead_key.set_version(get_version());
    *(kms_aead_key.mutable_params()) = key_format;
    return kms_aead_key;
  }

 private:
  const std::string key_type_ = absl::StrCat(
      kTypeGoogleapisCom, google::crypto::tink::KmsAeadKey().GetTypeName());
};

}  // namespace tink
}  // namespace crypto

#endif  // TINK_AEAD_KMS_AEAD_KEY_MANAGER_H_
