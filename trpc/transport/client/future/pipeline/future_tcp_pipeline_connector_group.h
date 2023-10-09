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

#include "trpc/transport/client/future/future_connector_group.h"
#include "trpc/transport/client/future/pipeline/future_tcp_pipeline_connector.h"

namespace trpc {

/// @brief Connector group for future tcp pipeline.
class FutureTcpPipelineConnectorGroup : public FutureConnectorGroup {
 public:
  /// @brief Constructor.
  explicit FutureTcpPipelineConnectorGroup(FutureConnectorGroupOptions&& options)
    : FutureConnectorGroup(std::move(options)), conn_pool_(options_.trans_info->max_conn_num) {}

  /// @brief Destructor.
  ~FutureTcpPipelineConnectorGroup() override = default;

  /// @brief Init connector group.
  bool Init() override;

  /// @brief Stop Init connector group.
  void Stop() override;

  /// @brief Destroy Init connector group.
  void Destroy() override;

  /// @brief Fixed type connector is not supported in pipeline.
  bool ReleaseConnector(uint64_t fixed_connector_id) override { return false; }

  /// @note Get connector, create if necessary.
  bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) override;

  /// @brief Send request.
  int SendReqMsg(CTransportReqMsg* req_msg) override;

  /// @brief Send request, not expect response.
  int SendOnly(CTransportReqMsg* req_msg) override;

 private:
  /// @brief Get connector by connector id, create if necessary.
  FutureTcpPipelineConnector* GetOrCreateConnector(uint64_t conn_id);

  /// @brief Handle request timeout.
  void HandleReqTimeout();

  /// @brief Handle idle connection.
  void HandleIdleConnection();

 private:
  // Connection pool.
  TcpPipelineConnPool conn_pool_;

  // Timer ids to detect timeout.
  std::vector<uint64_t> timer_ids_;
};

}  // namespace trpc
