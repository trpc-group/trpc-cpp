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

#include <memory>
#include <string>
#include <unordered_map>

#include "trpc/transport/client/fiber/common/fiber_client_connection_handler.h"
#include "trpc/util/function.h"

namespace trpc {

using FiberClientConnectionHandlerCreator =
    Function<std::unique_ptr<FiberClientConnectionHandler>(Connection* conn, TransInfo*)>;

/// @brief Fiber client connection handler factory
class FiberClientConnectionHandlerFactory {
 public:
  static FiberClientConnectionHandlerFactory* GetInstance() {
    static FiberClientConnectionHandlerFactory instance;
    return &instance;
  }

  /// @brief Register the function of create fiber client connection handler
  bool Register(const std::string& protocol, FiberClientConnectionHandlerCreator&& func);

  /// @brief Create fiber client connection handler by protocol/connection/trans_info
  std::unique_ptr<FiberClientConnectionHandler> Create(const std::string& protocol,
                                                       Connection* conn,
                                                       TransInfo* trans_info);

  /// @brief Clear all registered fiber connection handler
  void Clear();

 private:
  FiberClientConnectionHandlerFactory() = default;
  FiberClientConnectionHandlerFactory(const FiberClientConnectionHandlerFactory&) = delete;
  FiberClientConnectionHandlerFactory& operator=(const FiberClientConnectionHandlerFactory&) = delete;

 private:
  std::unordered_map<std::string, FiberClientConnectionHandlerCreator> connection_handlers_;
};

}  // namespace trpc
