//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/config/default/default_config.h"

#include <any>
#include <stdexcept>
#include <string>

#include "trpc/util/log/logging.h"

namespace trpc::config {

uint32_t DefaultConfig::GetUint32(const std::string& key, const uint32_t default_value) const {
  auto it = decode_data_.find(key);
  if (it != decode_data_.end()) {
    try {
      if (it->second.type() == typeid(uint32_t)) {
        return std::any_cast<uint32_t>(it->second);
      } else if (it->second.type() == typeid(int32_t)) {
        int32_t value_int32 = std::any_cast<int32_t>(it->second);
        if (value_int32 >= 0) {
          return static_cast<uint32_t>(value_int32);
        }
      } else if (it->second.type() == typeid(uint64_t)) {
        uint64_t value_uint64 = std::any_cast<uint64_t>(it->second);
        if (value_uint64 <= std::numeric_limits<uint32_t>::max()) {
          return static_cast<uint32_t>(value_uint64);
        }
      } else if (it->second.type() == typeid(int64_t)) {
        int64_t value_int64 = std::any_cast<int64_t>(it->second);
        if (value_int64 >= 0 && value_int64 <= std::numeric_limits<uint32_t>::max()) {
          return static_cast<uint32_t>(value_int64);
        }
      }
    } catch (const std::bad_any_cast& e) {
      TRPC_FMT_ERROR("Failed to get uint32 value for key '{}': {}", key, e.what());
      return default_value;
    }
  }
  return default_value;
}

int32_t DefaultConfig::GetInt32(const std::string& key, const int32_t default_value) const {
  auto it = decode_data_.find(key);
  if (it != decode_data_.end()) {
    try {
      if (it->second.type() == typeid(int32_t)) {
        return std::any_cast<int32_t>(it->second);
      } else if (it->second.type() == typeid(uint32_t)) {
        uint32_t value_uint32 = std::any_cast<uint32_t>(it->second);
        if (value_uint32 <= static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
          return static_cast<int32_t>(value_uint32);
        }
      } else if (it->second.type() == typeid(int64_t)) {
        int64_t value_int64 = std::any_cast<int64_t>(it->second);
        if (value_int64 >= std::numeric_limits<int32_t>::min() && value_int64 <= std::numeric_limits<int32_t>::max()) {
          return static_cast<int32_t>(value_int64);
        }
      } else if (it->second.type() == typeid(uint64_t)) {
        uint64_t value_uint64 = std::any_cast<uint64_t>(it->second);
        if (value_uint64 <= static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
          return static_cast<int32_t>(value_uint64);
        }
      }
    } catch (const std::bad_any_cast& e) {
      TRPC_FMT_ERROR("Failed to get int32 value for key '{}': {}", key, e.what());
      return default_value;
    }
  }
  return default_value;
}

uint64_t DefaultConfig::GetUint64(const std::string& key, const uint64_t default_value) const {
  auto it = decode_data_.find(key);
  if (it != decode_data_.end()) {
    try {
      if (it->second.type() == typeid(uint64_t)) {
        return std::any_cast<uint64_t>(it->second);
      } else if (it->second.type() == typeid(int64_t)) {
        int64_t value_int64 = std::any_cast<int64_t>(it->second);
        if (value_int64 >= 0) {
          return static_cast<uint64_t>(value_int64);
        }
      } else if (it->second.type() == typeid(uint32_t)) {
        return static_cast<uint64_t>(std::any_cast<uint32_t>(it->second));
      } else if (it->second.type() == typeid(int32_t)) {
        int32_t value_int32 = std::any_cast<int32_t>(it->second);
        if (value_int32 >= 0) {
          return static_cast<uint64_t>(value_int32);
        }
      }
    } catch (const std::bad_any_cast& e) {
      TRPC_FMT_ERROR("Failed to get uint64 value for key '{}': {}", key, e.what());
      return default_value;
    }
  }
  return default_value;
}

int64_t DefaultConfig::GetInt64(const std::string& key, const int64_t default_value) const {
  auto it = decode_data_.find(key);
  if (it != decode_data_.end()) {
    try {
      if (it->second.type() == typeid(int64_t)) {
        return std::any_cast<int64_t>(it->second);
      } else if (it->second.type() == typeid(uint64_t)) {
        uint64_t value_uint64 = std::any_cast<uint64_t>(it->second);
        if (value_uint64 <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
          return static_cast<int64_t>(value_uint64);
        }
      } else if (it->second.type() == typeid(int32_t)) {
        return static_cast<int64_t>(std::any_cast<int32_t>(it->second));
      } else if (it->second.type() == typeid(uint32_t)) {
        return static_cast<int64_t>(std::any_cast<uint32_t>(it->second));
      }
    } catch (const std::bad_any_cast& e) {
      TRPC_FMT_ERROR("Failed to get int64 value for key '{}': {}", key, e.what());
      return default_value;
    }
  }
  return default_value;
}

float DefaultConfig::GetFloat(const std::string& key, const float default_value) const {
  auto it = decode_data_.find(key);
  if (it != decode_data_.end()) {
    try {
      const std::any& value = it->second;
      if (value.type() == typeid(double)) {
        return static_cast<float>(std::any_cast<double>(value));
      }
      return std::any_cast<float>(value);
    } catch (const std::bad_any_cast& e) {
      TRPC_FMT_ERROR("Failed to get float value for key '{}': {}", key, e.what());
      return default_value;
    }
  }
  return default_value;
}

double DefaultConfig::GetDouble(const std::string& key, const double default_value) const {
  auto it = decode_data_.find(key);
  if (it != decode_data_.end()) {
    try {
      const std::any& value = it->second;
      if (value.type() == typeid(float)) {
        return static_cast<double>(std::any_cast<float>(value));
      }
      return std::any_cast<double>(value);
    } catch (const std::bad_any_cast& e) {
      TRPC_FMT_ERROR("Failed to get double value for key '{}': {}", key, e.what());
      return default_value;
    }
  }
  return default_value;
}

bool DefaultConfig::GetBool(const std::string& key, const bool default_value) const {
  auto it = decode_data_.find(key);
  if (it != decode_data_.end()) {
    try {
      return std::any_cast<bool>(it->second);
    } catch (const std::bad_any_cast& e) {
      TRPC_FMT_ERROR("Failed to get bool value for key '{}': {}", key, e.what());
      return default_value;
    }
  }
  return default_value;
}

std::string DefaultConfig::GetString(const std::string& key, const std::string& default_value) const {
  auto it = decode_data_.find(key);
  if (it != decode_data_.end()) {
    try {
      return std::any_cast<std::string>(it->second);
    } catch (const std::bad_any_cast& e) {
      TRPC_FMT_ERROR("Failed to get string value for key '{}': {}", key, e.what());
      return default_value;
    }
  }
  return default_value;
}

bool DefaultConfig::IsSet(const std::string& key) const { return decode_data_.count(key) > 0; }

}  // namespace trpc::config
