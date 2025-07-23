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

#include <string>
#include <unordered_map>

#include "trpc/config/codec.h"
#include "trpc/util/log/logging.h"

namespace trpc::config {

/// @brief The CodecFactory class is a singleton class that manages codecs.
class CodecFactory {
 public:
  /// @brief Gets the unique instance of CodecFactory.
  /// @return The unique instance of CodecFactory.
  static CodecFactory* GetInstance() {
    static CodecFactory instance;
    return &instance;
  }

  CodecFactory(const CodecFactory&) = delete;
  CodecFactory& operator=(const CodecFactory&) = delete;

  /// @brief Registers a codec.
  /// @param codec The pointer to the codec.
  /// @return true: success, false: failed.
  bool Register(config::CodecPtr codec) {
    TRPC_ASSERT(codec != nullptr);
    config_codecs_[codec->Name()] = std::move(codec);
    return true;
  }

  /// @brief Gets the codec with the specified name.
  /// @param name The name of the codec.
  /// @return The pointer to the codec with the specified name, or nullptr if not found.
  config::CodecPtr Get(const std::string& name) const {
    auto it = config_codecs_.find(name);
    if (it != config_codecs_.end()) {
      return it->second;
    }
    return nullptr;
  }

  /// @brief Get all config codec plugins
  const std::unordered_map<std::string, config::CodecPtr>& GetAllPlugins() { return config_codecs_; }

  /// @brief Clear all config codec plugins
  void Clear() { config_codecs_.clear(); }

 private:
  CodecFactory() = default;

 private:
  std::unordered_map<std::string, config::CodecPtr> config_codecs_;
};

}  // namespace trpc::config
