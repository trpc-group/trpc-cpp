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
#include <unordered_map>

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/transport/server/fiber/fiber_bind_adapter.h"
#include "trpc/transport/server/fiber/fiber_server_connection_handler.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief Fiber server connection handler factory
class FiberServerConnectionHandlerFactory {
 public:
  static FiberServerConnectionHandlerFactory* GetInstance() {
    static FiberServerConnectionHandlerFactory instance;
    return &instance;
  }

  /// @brief Register the function of create fiber connection handler
  bool Register(const std::string& protocol, FiberServerConnectionHandlerCreator&& func);

  /// @brief Create fiber connection handler by connection/bindadapter/bindinfo
  std::unique_ptr<FiberServerConnectionHandler> Create(Connection* conn, FiberBindAdapter* bind_adapter,
                                                       BindInfo* bind_info);

  /// @brief Clear all registered fiber connection handler
  void Clear();

 private:
  FiberServerConnectionHandlerFactory() = default;
  FiberServerConnectionHandlerFactory(const FiberServerConnectionHandlerFactory&) = delete;
  FiberServerConnectionHandlerFactory& operator=(const FiberServerConnectionHandlerFactory&) = delete;

 private:
  std::unordered_map<std::string, FiberServerConnectionHandlerCreator> connection_handlers_;
};

}  // namespace trpc
