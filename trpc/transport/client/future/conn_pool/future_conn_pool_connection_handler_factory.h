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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "trpc/transport/client/future/conn_pool/future_conn_pool_connection_handler.h"
#include "trpc/util/function.h"

namespace trpc {

using FutureConnPoolConnectionHandlerCreator = Function<std::unique_ptr<FutureConnectionHandler>(
  const FutureConnectorOptions&, FutureConnPoolMessageTimeoutHandler&)>;

/// @brief Connection handler factory for future connection pool.
/// @note  Both tcp and udp go through factory.
class FutureConnPoolConnectionHandlerFactory {
 public:
  /// @brief Get global unique instance.
  static FutureConnPoolConnectionHandlerFactory* GetIntance() {
    static FutureConnPoolConnectionHandlerFactory instance;
    return &instance;
  }

  /// @brief Register the function for creating future connection pool handler.
  bool Register(const std::string& protocol, FutureConnPoolConnectionHandlerCreator&& func);

  /// @brief Clear all the future connection pool handler creators.
  void Clear();

  /// @brief Create connection handler for connection pool.
  std::unique_ptr<FutureConnectionHandler> Create(const std::string& protocol,
    const FutureConnectorOptions& options, FutureConnPoolMessageTimeoutHandler& handler);

 private:
  FutureConnPoolConnectionHandlerFactory() = default;
  FutureConnPoolConnectionHandlerFactory(const FutureConnPoolConnectionHandlerFactory&) = delete;
  FutureConnPoolConnectionHandlerFactory& operator=(
    const FutureConnPoolConnectionHandlerFactory&) = delete;

 private:
  std::unordered_map<std::string, FutureConnPoolConnectionHandlerCreator> creators_;
};

}  // namespace trpc
