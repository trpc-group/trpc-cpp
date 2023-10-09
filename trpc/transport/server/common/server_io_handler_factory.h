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
#include <unordered_map>

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/transport/server/server_transport_def.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief The function of create server io handler
using ServerIoHandlerCreator = Function<std::unique_ptr<IoHandler>(Connection*, const BindInfo&)>;

/// @brief Server io handler factory
class ServerIoHandlerFactory {
 public:
  static ServerIoHandlerFactory* GetInstance() {
    static ServerIoHandlerFactory instance;
    return &instance;
  }

  /// @brief Register the function of create server io handler
  bool Register(const std::string& protocol, ServerIoHandlerCreator&& creator);

  /// @brief Create server io handler by protocol/connection/bindinfo
  std::unique_ptr<IoHandler> Create(const std::string& protocol, Connection* conn,
                                    const BindInfo& bind_info);

  /// @brief Clear all registered server io handler
  void Clear();

 private:
  ServerIoHandlerFactory() = default;
  ServerIoHandlerFactory(const ServerIoHandlerFactory&) = delete;
  ServerIoHandlerFactory& operator=(const ServerIoHandlerFactory&) = delete;

 private:
  std::unordered_map<std::string, ServerIoHandlerCreator> io_handlers_;
};

}  // namespace trpc
