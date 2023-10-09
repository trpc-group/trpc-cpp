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
#include <utility>

#include "trpc/stream/stream_handler.h"

namespace trpc::stream {

/// @brief Stream handler factory on client side.
class ClientStreamHandlerFactory {
 public:
  static ClientStreamHandlerFactory* GetInstance() {
    static ClientStreamHandlerFactory instance;
    return &instance;
  }

  /// @brief Registers a stream handler creator.
  void Register(const std::string& codec_name, StreamHandlerCreator creator) {
    stream_handler_creators_[codec_name] = std::move(creator);
  }

  /// @brief Creates a stream handler.
  StreamHandlerPtr Create(const std::string& codec_name, StreamOptions&& options) {
    auto found = stream_handler_creators_.find(codec_name);
    if (found != stream_handler_creators_.end()) {
      if (found->second) {
        return (found->second)(std::move(options));
      }
    }
    return nullptr;
  }

  /// @brief Does cleanup job.
  void Clear() { stream_handler_creators_.clear(); }

 private:
  ClientStreamHandlerFactory() = default;

  ClientStreamHandlerFactory(const ClientStreamHandlerFactory&) = delete;
  ClientStreamHandlerFactory& operator=(const ClientStreamHandlerFactory&) = delete;

 private:
  std::unordered_map<std::string, StreamHandlerCreator> stream_handler_creators_;
};

}  // namespace trpc::stream
