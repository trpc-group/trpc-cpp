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

#include <memory>
#include <string>
#include <utility>

#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/server_codec_factory.h"

namespace trpc::codec {

/// @brief Initializes the default codec plugins registered by the framework.
/// @private For internal use purpose only.
bool Init();

/// @brief Initializes all the codec plugins.
/// @private For internal use purpose only.
void Destroy();

template <typename PluginType, typename... Args>
bool InitCodecPlugins(Args&&... args) {
  const bool is_client_codec = std::is_base_of<ClientCodec, PluginType>::value;
  if constexpr (is_client_codec) {
    std::shared_ptr<ClientCodec> client_codec(new PluginType(std::forward<Args>(args)...));
    ClientCodecFactory::GetInstance()->Register(client_codec);
    return true;
  }

  const bool is_server_codec = std::is_base_of<ServerCodec, PluginType>::value;
  if constexpr (is_server_codec) {
    std::shared_ptr<ServerCodec> server_codec(new PluginType(std::forward<Args>(args)...));
    ServerCodecFactory::GetInstance()->Register(server_codec);
    return true;
  }

  return false;
}

}  // namespace trpc::codec
