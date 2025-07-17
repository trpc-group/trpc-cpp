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
#include <unordered_map>

#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/transport/client/trans_info.h"

namespace trpc {

/// @brief The function of create client io handler
using ClientIoHandlerCreator = std::function<std::unique_ptr<IoHandler>(Connection* conn, TransInfo* trans_info)>;

/// @brief Client io handler factory
class ClientIoHandlerFactory {
 public:
  static ClientIoHandlerFactory* GetInstance() {
    static ClientIoHandlerFactory instance;
    return &instance;
  }

  /// @brief Register the function of create client io handler
  bool Register(const std::string& protocol, ClientIoHandlerCreator&& creator);

  /// @brief Create io handler by protocol/connection/trans_info
  std::unique_ptr<IoHandler> Create(const std::string& protocol, Connection* conn, TransInfo* trans_info);

  /// @brief Clear all registered client io handler
  void Clear();

 private:
  ClientIoHandlerFactory() = default;
  ClientIoHandlerFactory(const ClientIoHandlerFactory&) = delete;
  ClientIoHandlerFactory& operator=(const ClientIoHandlerFactory&) = delete;

 private:
  std::unordered_map<std::string, ClientIoHandlerCreator> io_handlers_;
};

}  // namespace trpc
