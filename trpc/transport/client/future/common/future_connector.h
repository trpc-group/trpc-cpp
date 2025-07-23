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

#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/stream/stream_provider.h"
#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/future/common/future_connector_options.h"

namespace trpc {

/// @brief Wrap send and recv ability over network.
class FutureConnector {
 public:
  /// @brief Interval between two connects, in millisecond.
  static constexpr uint64_t kConnectInterval = 2000;

  /// @brief Constructor.
  FutureConnector(FutureConnectorOptions&& options) : options_(std::move(options)) {}

  /// @brief Destructor.
  virtual ~FutureConnector() = default;

  /// @brief Init connector.
  /// @return true: success，false: failed.
  virtual bool Init() { return true; }

  /// @brief Stop connector.
  virtual void Stop() {}

  /// @brief Destroy connector.
  virtual void Destroy() {}

  /// @brief Send request.
  /// @param req_msg Request to send.
  /// @return 0: success，-1: faled.
  /// @note There is Promise inside request message, use it to awake Future when request done.
  virtual int SendReqMsg(CTransportReqMsg* req_msg) = 0;

  /// @brief Send request only, not receive response.
  /// @param req_msg Request to send.
  /// @return 0: success，-1: faled.
  virtual int SendOnly(CTransportReqMsg* req_msg) = 0;

  /// @brief Get connection state.
  /// @note To find out connection available or not, typically by tcp.
  virtual ConnectionState GetConnectionState() = 0;

  /// @brief Create stream.
  /// @param stream_options Options of stream.
  /// @return Stream pointer.
  virtual stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&& stream_options) {
    return nullptr;
  }

 protected:
  FutureConnectorOptions options_;
};

}  // namespace trpc
