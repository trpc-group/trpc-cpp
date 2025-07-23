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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "trpc/transport/client/future/conn_complex/future_conn_complex_connection_handler.h"
#include "trpc/util/function.h"

namespace trpc {

/// @note  Why connection handler factory is different for connection complex and connection pool.
///        1. In conn complex scenario, one connector group has only one connector, no conn_pool is needed.
///           On the other hand, conn_pool is required for connection pool scenario.
///        2. Timeout handler is different for complex and pool scenarios, no based class is designed, in
///           conn complex, reference to global timeout handler is stored in connector, but in conn pool,
///           every connector has its own timeout handler.
///        So it is hard to unify the connection handler constructor signature for both complex and pool.
///        Udp is different from tcp, as udp can not be used as transport layer protocol in stream scenario.
///        But the difference is reflected inside creator, not in factory.
using FutureConnComplexConnectionHandlerCreator = Function<std::unique_ptr<FutureConnectionHandler>(
  const FutureConnectorOptions&, FutureConnComplexMessageTimeoutHandler&)>;

/// @brief Connection handler factory for future connection complex.
/// @note  Both tcp and udp go through factory.
class FutureConnComplexConnectionHandlerFactory {
 public:
  /// @brief Get global unique instance.
  static FutureConnComplexConnectionHandlerFactory* GetIntance() {
    static FutureConnComplexConnectionHandlerFactory instance;
    return &instance;
  }

  /// @brief Register the function for creating future connection complex handler.
  bool Register(const std::string& protocol, FutureConnComplexConnectionHandlerCreator&& func);

  /// @brief Clear all the future connection complex handler creators.
  void Clear();

  /// @brief Create connection handler for connection complex.
  std::unique_ptr<FutureConnectionHandler> Create(const std::string& protocol,
    const FutureConnectorOptions& options, FutureConnComplexMessageTimeoutHandler& handler);

 private:
  FutureConnComplexConnectionHandlerFactory() = default;
  FutureConnComplexConnectionHandlerFactory(const FutureConnComplexConnectionHandlerFactory&) = delete;
  FutureConnComplexConnectionHandlerFactory& operator=(
    const FutureConnComplexConnectionHandlerFactory&) = delete;

 private:
  std::unordered_map<std::string, FutureConnComplexConnectionHandlerCreator> creators_;
};

}  // namespace trpc
