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
#include <optional>
#include <string>

#include "trpc/future/future.h"
#include "trpc/runtime/threadmodel/thread_model.h"
#include "trpc/stream/stream.h"
#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/preallocation_option.h"

namespace trpc {

/// @brief Base class for client transport
class ClientTransport {
 public:
  struct Options {
    ThreadModel* thread_model{nullptr};
    TransInfo trans_info;
  };

  virtual ~ClientTransport() = default;

  /// @brief The name of transport
  virtual std::string Name() const = 0;

  /// @brief The version of transport
  virtual std::string Version() const = 0;

  /// @brief Init transport
  /// @return 0: success, -1: failed
  virtual int Init(Options&& params) { return 0; }

  /// @brief  Stop and close connection read/write event
  virtual void Stop() {}

  /// @brief Destroy the inner resources of transport
  virtual void Destroy() {}

  /// @brief Sync send request message and recv response message
  /// @return 0: success, -1: failed
  virtual int SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) = 0;

  /// @brief Async send request message and recv response message
  virtual Future<CTransportRspMsg> AsyncSendRecv(CTransportReqMsg* msg) = 0;

  /// @brief Send request message and not recv response message(one-way invoke)
  virtual int SendOnly(CTransportReqMsg* msg) { return -1; }

  /// @brief Create stream
  virtual stream::StreamReaderWriterProviderPtr CreateStream(const NodeAddr& addr,
                                                             stream::StreamOptions&& stream_options) {
    return nullptr;
  }

  virtual std::optional<uint64_t> GetOrCreateConnector(const PreallocationOption& preallocate_option) {
    return std::nullopt;
  }

  virtual bool ReleaseFixedConnector(uint64_t connector_id) { return false; }

  virtual void Disconnect(const std::string& target_ip) {}
};

}  // namespace trpc
