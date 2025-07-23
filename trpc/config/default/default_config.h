//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "trpc/common/config/default_value.h"
#include "trpc/config/codec.h"
#include "trpc/config/provider.h"

namespace trpc::config {

/// @brief A default (KV-type) configuration components provided by the framework,
/// representing a piece of business data. This class represents a piece of business data
/// and is a default (KV-type) configuration components.
/// It indicates the source path of the configuration, the data source provider it comes from, and the decoder used.
class DefaultConfig : public RefCounted<DefaultConfig> {
 public:
  /// @brief Get a uint32_t value from decode_data based on the key.
  /// This function retrieves a uint64_t value from the decoded data based on the specified key.
  /// @param key The key to retrieve the value for.
  /// @param default_value The default value to return if the key is not found.
  /// @return The uint32_t value associated with the key, or the default value if the key is not found.
  uint32_t GetUint32(const std::string& key, const uint32_t default_value) const;

  /// @brief Get an int32_t value from decode_data based on the key.
  /// This function retrieves an int64_t value from the decoded data based on the specified key.
  /// @param key The key to retrieve the value for.
  /// @param default_value The default value to return if the key is not found.
  /// @return The int32_t value associated with the key, or the default value if the key is not found.
  int32_t GetInt32(const std::string& key, const int32_t default_value) const;

  /// @brief Get a uint64_t value from decode_data based on the key.
  /// This function retrieves a uint64_t value from the decoded data based on the specified key.
  /// @param key The key to retrieve the value for.
  /// @param default_value The default value to return if the key is not found.
  /// @return The uint64_t value associated with the key, or the default value if the key is not found.
  uint64_t GetUint64(const std::string& key, const uint64_t default_value) const;

  /// @brief Get an int64_t value from decode_data based on the key.
  /// This function retrieves an int64_t value from the decoded data based on the specified key.
  /// @param key The key to retrieve the value for.
  /// @param default_value The default value to return if the key is not found.
  /// @return The int64_t value associated with the key, or the default value if the key is not found.
  int64_t GetInt64(const std::string& key, const int64_t default_value) const;

  /// @brief Get a float value from decode_data based on the key.
  /// This function retrieves a float value from the decoded data based on the specified key.
  /// @param key The key to retrieve the value for.
  /// @param default_value The default value to return if the key is not found.
  /// @return The float value associated with the key, or the default value if the key is not found.
  float GetFloat(const std::string& key, const float default_value) const;

  /// @brief Get a double value from decode_data based on the key.
  /// This function retrieves a double value from the decoded data based on the specified key.
  /// @param key The key to retrieve the value for.
  /// @param default_value The default value to return if the key is not found.
  /// @return The double value associated with the key, or the default value if the key is not found.
  double GetDouble(const std::string& key, const double default_value) const;

  /// @brief Get a bool value from decode_data based on the key.
  /// This function retrieves a bool value from the decoded data based on the specified key.
  /// @param key The key to retrieve the value for.
  /// @param default_value The default value to return if the key is not found.
  /// @return The bool value associated with the key, or the default value if the key is not found.
  bool GetBool(const std::string& key, const bool default_value) const;

  /// @brief Get a string value from decode_data based on the key.
  /// This function retrieves a string value from the decoded data based on the specified key.
  /// @param key The key to retrieve the value for.
  /// @param defaultValue The default value to return if the key is not found.
  /// @return The string value associated with the key, or the default value if the key is not found.
  std::string GetString(const std::string& key, const std::string& defaultValue) const;

  /// @brief Check if the key exists.
  /// This function checks if the specified key exists in the decoded data.
  /// @param key The key to check for.
  /// @return True if the key exists, false otherwise.
  bool IsSet(const std::string& key) const;

  /// @brief Get the path of the configuration file.
  /// This function returns the path of the configuration file.
  /// @return The path of the configuration file.
  const std::string& GetPath() const { return path_; }

  /// @brief Set the path of the configuration file.
  /// This function sets the path of the configuration file.
  /// @param path The path of the configuration file.
  void SetPath(const std::string& path) { path_ = path; }

  /// @brief Get the data source provider.
  /// This function returns the data source provider.
  /// @return The data source provider.
  config::ProviderPtr GetProvider() const { return data_provider_; }

  /// @brief Set the data source provider.
  /// This function sets the data source provider.
  /// @param provider The data source provider.
  void SetProvider(config::ProviderPtr provider) { data_provider_ = provider; }

  /// @brief Get the decoder.
  /// This function returns the decoder.
  /// @return The decoder.
  config::CodecPtr GetCodec() const { return decoder_; }

  /// @brief Set the decoder.
  /// This function sets the decoder.
  /// @param decoder The decoder.
  void SetCodec(config::CodecPtr decoder) { decoder_ = decoder; }

  /// @brief Get the raw data.
  /// This function returns the raw data.
  /// @return The raw data.
  const std::string& GetRawData() const { return raw_data_; }

  /// @brief Set the raw data.
  /// This function sets the raw data.
  /// @param data The raw data.
  void SetRawData(const std::string& data) { raw_data_ = data; }

  /// @brief Get the decoded data.
  /// This function returns the decoded data.
  /// @return The decoded data.
  std::unordered_map<std::string, std::any> GetDecodeData() { return decode_data_; }

  /// @brief Set the decoded data.
  /// This function sets the decoded data.
  /// @param decode_data The decoded data.
  void SetDecodeData(const std::unordered_map<std::string, std::any>& decode_data) { decode_data_ = decode_data; }

 private:
  // The path of the file in the data source.
  std::string path_;
  // The data source provider.
  config::ProviderPtr data_provider_;
  // The decoder used to decode the data.
  config::CodecPtr decoder_;
  // The raw data obtained from the data source.
  std::string raw_data_;
  // The decoded data in the form of a key-value map.
  std::unordered_map<std::string, std::any> decode_data_;
};

using DefaultConfigPtr = RefPtr<DefaultConfig>;
using OptionFunc = std::function<void(DefaultConfigPtr&)>;

}  // namespace trpc::config
