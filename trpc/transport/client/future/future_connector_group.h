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

#include <cstdint>
#include <vector>

#include "trpc/future/future.h"
#include "trpc/transport/client/future/common/future_connector.h"

namespace trpc {

/// @brief To manage a group of future connectors.
class FutureConnectorGroup {
 public:
  /// @brief Constructor.
  FutureConnectorGroup(FutureConnectorGroupOptions options) : options_(options) {}

  /// @brief Destructor.
  virtual ~FutureConnectorGroup() = default;

  /// @brief Init connector group.
  virtual bool Init() = 0;

  /// @brief Stop connector group.
  virtual void Stop() = 0;

  /// @brief Destroy connector group.
  virtual void Destroy() = 0;

  /// @brief Create stream.
  virtual stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&&) {
    TRPC_FMT_ERROR("stream is not supported");
    return nullptr;
  }

  /// @brief Release a fiexed type connector.
  /// @param fixed_connector_id Connector id.
  /// @return true: success, false: failed.
  /// @note Maybe failed if data still left in connection.
  virtual bool ReleaseConnector(uint64_t fixed_connector_id) = 0;

  /// @brief Get connector, create if necessary.
  /// @param[in] node_addr Backend address.
  /// @param[out] Promise<uint64_t>& promise Result.
  /// @return true: success, false: failed.
  /// @note Maybe failed if no connector available.
  virtual bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) = 0;

  /// @brief Send request.
  /// @param[in] req_msg Request to send.
  /// @return 0: success, -1: failed.
  /// @note Will retry if send failed.
  virtual int SendReqMsg(CTransportReqMsg* req_msg) = 0;

  /// @brief Send request, not expect response.
  /// @param[in] req_msg Request to send.
  /// @return 0: success, -1: failed.
  /// @note Will retry if send failed.
  virtual int SendOnly(CTransportReqMsg* req_msg) = 0;

 protected:
  // Group options.
  FutureConnectorGroupOptions options_;
};

}  // namespace trpc
