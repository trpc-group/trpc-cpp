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

#include <optional>
#include <queue>

#include "trpc/transport/client/future/common/future_connection_handler.h"
#include "trpc/transport/client/future/common/future_connector.h"
#include "trpc/transport/client/future/common/utils.h"
#include "trpc/transport/client/future/pipeline/future_pipeline_message_timeout_handler.h"
#include "trpc/transport/client/future/pipeline/pipeline_conn_pool.h"

namespace trpc {

class FutureTcpPipelineConnector;
using TcpPipelineConnPool = PipelineConnPool<FutureTcpPipelineConnector>;

class FuturePipelineConnectionHandler final : public FutureConnectionHandler {
 public:
  FuturePipelineConnectionHandler(const FutureConnectorOptions& options,
                                  FuturePipelineMessageTimeoutHandler& msg_timeout_handler);

  /// @brief Callback to handle response message.
  /// @param conn Related connection.
  /// @param rsp_list Response.
  /// @return true: success, false: failed.
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) override;

  /// @brief Callback to clean resource when connection closed.
  void CleanResource() override;

  /// @brief Get merge request count.
  uint32_t GetMergeRequestCount() override { return 1; }

  /// @brief Set connection pool.
  /// @note  Called after instance created.
  void SetConnPool(TcpPipelineConnPool* conn_pool) { conn_pool_ = conn_pool; }

 private:
  /// @brief Get request id when request message finish sending over network, only for pipeline.
  void NotifyPipelineSendMsg(uint32_t req_id);

  // Get request id from queue.
  std::optional<uint32_t> GetRequestId();

 private:
  // Timeout handler.
  FuturePipelineMessageTimeoutHandler& msg_timeout_handler_;

  // Connection pool.
  TcpPipelineConnPool* conn_pool_;

  // Ids of sent requests.
  std::queue<uint32_t> send_request_id_;
};

/// @brief Connector for tcp pipeline.
class FutureTcpPipelineConnector : public FutureConnector {
 public:
  /// @brief Constructor.
  explicit FutureTcpPipelineConnector(FutureConnectorOptions&& options, TcpPipelineConnPool* conn_pool)
    : FutureConnector(std::move(options)),
      conn_pool_(conn_pool),
      msg_timeout_handler_(options_.group_options->trans_info->rsp_dispatch_function) {}

  /// @brief Destructor.
  ~FutureTcpPipelineConnector() override = default;

  /// @brief Init connector.
  bool Init() override;

  /// @brief Stop connector.
  void Stop() override;

  /// @brief Destroy connector.
  void Destroy() override;

  /// @brief Send request over connector and expect response.
  int SendReqMsg(CTransportReqMsg* req_msg) override;

  /// @brief Send request over connector, not expect response.
  int SendOnly(CTransportReqMsg* req_msg) override;

  /// @brief Get connection state.
  ConnectionState GetConnectionState() override {
    return future::GetConnectionState(connection_.Get());
  }

  /// @brief Handle request timeout.
  void HandleRequestTimeout();

  /// @brief Handle connection idle.
  void HandleIdleConnection(uint64_t now_ms);

 private:
  // Which connection pool belongs to.
  TcpPipelineConnPool* conn_pool_;

  // Timeout handler.
  FuturePipelineMessageTimeoutHandler msg_timeout_handler_;

  // Tcp connection.
  RefPtr<TcpConnection> connection_ = nullptr;
};

}  // namespace trpc
