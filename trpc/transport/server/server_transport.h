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

#include <any>
#include <memory>
#include <string>

#include "trpc/transport/server/server_transport_def.h"
#include "trpc/transport/server/server_transport_message.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief Base class for server transport
class ServerTransport {
 public:
  virtual ~ServerTransport() = default;

  /// @brief The name of transport
  virtual std::string Name() const = 0;

  /// @brief The version of transport
  virtual std::string Version() const = 0;

  /// @brief Init transport
  virtual int Init(const std::any& params) { return 0; }

  /// @brief Bind ip/port
  virtual void Bind(const BindInfo& bind_info) = 0;

  /// @brief Listen and Serve
  virtual bool Listen() = 0;

  /// @brief Stop listen connection and close connection read/write event
  virtual void Stop() = 0;

  /// @brief Stop listen connection
  virtual void StopListen(bool clean_conn) {}

  /// @brief Destroy the inner resources of transport
  virtual void Destroy() = 0;

  /// @brief Send the response msg
  virtual int SendMsg(STransportRspMsg* msg) = 0;

  /// @brief Close connection
  virtual void DoClose(const CloseConnectionInfo& close_connection_info) {}

  /// @brief Check the connection whether ok(not real-time)
  virtual bool IsConnected(uint64_t connection_id) { return true; }

  /// @brief Flow control the connection
  /// @param conn_id The connection id
  /// @param set True: enable flow control, suspend reading data
  ///            False: disable flow control, resume reading data
  virtual void ThrottleConnection(uint64_t conn_id, bool set) {}
};

}  // namespace trpc
