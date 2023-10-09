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

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/transport/server/default/bind_adapter.h"
#include "trpc/transport/server/default/server_connection_handler.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief Default connection handler factory
class DefaultServerConnectionHandlerFactory {
 public:
  static DefaultServerConnectionHandlerFactory* GetInstance() {
    static DefaultServerConnectionHandlerFactory instance;
    return &instance;
  }

  /// @brief Register the function of create default connection handler
  bool Register(const std::string& protocol, DefaultServerConnectionHandlerCreator&& func);

  /// @brief Create default connection handler by connection/bindadapter/bindinfo
  std::unique_ptr<DefaultServerConnectionHandler> Create(Connection* conn, BindAdapter* bind_adapter,
                                                         BindInfo* bind_info);

  /// @brief Clear all registered default connection handler
  void Clear();

 private:
  DefaultServerConnectionHandlerFactory() = default;
  DefaultServerConnectionHandlerFactory(const DefaultServerConnectionHandlerFactory&) = delete;
  DefaultServerConnectionHandlerFactory& operator=(const DefaultServerConnectionHandlerFactory&) = delete;

 private:
  std::unordered_map<std::string, DefaultServerConnectionHandlerCreator> connection_handlers_;
};

}  // namespace trpc
