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

#pragma once

#include <string>
#include <unordered_map>

#include "trpc/codec/client_codec.h"

namespace trpc {

/// @brief The plugin factory class for client-side application layer protocol
///        provides codec implementations for registering and obtaining
///        various application layer protocols
class ClientCodecFactory {
 public:
  static ClientCodecFactory* GetInstance() {
    static ClientCodecFactory instance;
    return &instance;
  }

  /// @brief Register codec
  /// @note  This method is not thread-safe, it needs to be invoked in advance
  ///        when the program is initialized
  bool Register(const ClientCodecPtr& codec);

  /// @brief Get codec by name
  ClientCodecPtr Get(const std::string& codec_name) const;

  /// @brief Clear the registered codec, framework use
  /// @private For internal use purpose only.
  void Clear();

 private:
  ClientCodecFactory() = default;
  ClientCodecFactory(const ClientCodecFactory&) = delete;
  ClientCodecFactory& operator=(const ClientCodecFactory&) = delete;

 private:
  std::unordered_map<std::string, ClientCodecPtr> codecs_;
};

}  // namespace trpc
