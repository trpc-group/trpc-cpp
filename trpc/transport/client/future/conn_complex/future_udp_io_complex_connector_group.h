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
#include <memory>

#include "trpc/transport/client/future/conn_complex/future_udp_io_complex_connector.h"
#include "trpc/transport/client/future/future_connector_group.h"

namespace trpc {

/// @brief Connector group for future udp io complex.
class FutureUdpIoComplexConnectorGroup : public FutureConnectorGroup {
 public:
  /// @brief Constructor.
  explicit FutureUdpIoComplexConnectorGroup(FutureConnectorGroupOptions&& options);

  /// @brief Init connector group.
  bool Init() override;

  /// @brief Stop connector group.
  void Stop() override;

  /// @brief Destroy connector group.
  void Destroy() override;

  /// @brief Used for connection pool to release a fixed connector, not supported in udp.
  bool ReleaseConnector(uint64_t fixed_connector_id) override {
    return false;
  }

  /// @brief Not support in udp.
  bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) override {
    return false;
  }

  /// @brief Send request.
  int SendReqMsg(CTransportReqMsg* req_msg) override;

  /// @brief Send request, not expect response.
  int SendOnly(CTransportReqMsg* req_msg) override;

 private:
  // Udp connector.
  std::unique_ptr<FutureUdpIoComplexConnector> connector_{nullptr};
};

}  // namespace trpc
