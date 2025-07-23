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

#include "trpc/codec/server_codec.h"

namespace trpc {

/// @brief The plugin factory class for server-side application layer protocol
///        provides codec implementations for registering and obtaining
///        various application layer protocols
class ServerCodecFactory {
 public:
  static ServerCodecFactory* GetInstance() {
    static ServerCodecFactory instance;
    return &instance;
  }

  /// @brief Register codec
  /// @note  This method is not thread-safe, it needs to be invoked in advance
  ///        when the program is initialized
  bool Register(const ServerCodecPtr& codec);

  /// @brief Get codec by name
  ServerCodecPtr Get(const std::string& codec_name);

  /// @brief Clear the registered codec, framework use
  /// @private For internal use purpose only.
  void Clear();

 private:
  ServerCodecFactory() = default;
  ServerCodecFactory(const ServerCodecFactory&) = delete;
  ServerCodecFactory& operator=(const ServerCodecFactory&) = delete;

 private:
  std::unordered_map<std::string, ServerCodecPtr> codecs_;
};

}  // namespace trpc
