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
#include <string>
#include <vector>

#include "trpc/runtime/threadmodel/common/worker_thread.h"
#include "trpc/runtime/threadmodel/thread_model.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/transport/client/client_transport.h"
#include "trpc/transport/client/future/common/future_transport_adapter.h"

namespace trpc {

/// @brief Client transport implementation for future.
class FutureTransport final : public ClientTransport {
 public:
  /// @brief Destructor.
  ~FutureTransport() override = default;

  /// @brief Name of current transport plugin.
  std::string Name() const override { return "future_transport"; }

  /// @brief Version of current transport plugin.
  std::string Version() const override { return "0.1.0"; };

  /// @brief Init future transport plugin.
  int Init(Options&& options) override;

  /// @brief Stop plugin.
  void Stop() override;

  /// @brief Destroy plugin.
  void Destroy() override;

  /// @brief Send request and receive response in synchronous manner.
  /// @param req_msg Request message.
  /// @param rsp_msg Response message.
  /// @return 0: success，-1: faled.
  int SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) override;

  /// @brief Send request and receive response in asynchronous manner.
  /// @param req_msg Request message.
  /// @return Ready future: success, Failed future: failed.
  Future<CTransportRspMsg> AsyncSendRecv(CTransportReqMsg* req_msg) override;

  /// @brief Only send request and not expect response.
  /// @param req_msg Request message.
  /// @return 0: success，-1: faled.
  int SendOnly(CTransportReqMsg* req_msg);

  /// @brief Allocate connector.
  /// @param preallocate_option Preallocate option.
  /// @return std::optional<uint64_t> connector id, failed if task queue full or reach max connection.
  std::optional<uint64_t> GetOrCreateConnector(const PreallocationOption& preallocate_option) override;

  /// @brief Release a fixed connector.
  /// @param connector_id Connector id.
  /// @return true: success, false: failed.
  bool ReleaseFixedConnector(uint64_t connector_id) override;

  /// @brief Disconnect all the connections to a target backend.
  /// @param target_ip Target ip.
  void Disconnect(const std::string& target_ip) override;

  /// @brief Create stream.
  /// @param node_addr Backend address.
  /// @param stream_options Options of stream.
  /// @return Pointer to stream.
  stream::StreamReaderWriterProviderPtr CreateStream(const NodeAddr& node_addr,
                                                     stream::StreamOptions&& stream_options) override;

 private:
  // Select a io thread from thread model instance.
  uint16_t SelectIOThread();
  uint16_t SelectIOThread(CTransportReqMsg* msg);

  // Get low 16 bits of thread logic id in thread group.
  uint16_t GetLogicId(const WorkerThread* work_thread);

  // Select a adapter.
  uint16_t SelectTransportAdapter();

  // Select a transport adapter to send the request. Return the index of adapter.
  uint16_t SelectTransportAdapter(CTransportReqMsg* msg);

  // Select a transport adapter with index input.
  uint16_t SelectTransportAdapter(CTransportReqMsg* msg, uint16_t index);

  // Send request.
  bool SendRequest(CTransportReqMsg* msg, uint16_t id, TrpcCallType call_type);

  // Send the backup request if the first request does not succeed within the resend time.
  std::vector<Future<CTransportRspMsg>> SendBackupRequest(ClientContextPtr& msg_context,
                                                          Future<CTransportRspMsg>&& res_fut_first,
                                                          uint16_t id, bool is_blocking_invoke,
                                                          NoncontiguousBuffer&& send_data);

  // Send request and receive response in asynchronous manner.
  Future<CTransportRspMsg> AsyncSendRecvImpl(CTransportReqMsg* msg, uint16_t id);

  // Send backup request and receive response in asynchronous manner.
  Future<CTransportRspMsg> AsyncSendRecvForBackupRequest(CTransportReqMsg* msg);

  // Check target thread is same with current thread.
  bool IsSameIOThread(uint16_t index);

  // Create task for transport message.
  MsgTask* CreateTransportRequestTask(CTransportReqMsg* msg, uint16_t id, TrpcCallType call_type);

  // Create task for fixed connection request.
  MsgTask* CreateFixedConnectorRequestTask(CTransportReqMsg* msg, uint16_t id, TrpcCallType call_type);

  // Create task for allocate connector.
  MsgTask* CreateAllocateTask(const NodeAddr& node_addr, uint16_t index, Promise<uint64_t>&& promise);

  // Create task for release connector.
  MsgTask* CreateReleaseTask(uint64_t fixed_connector_id, uint16_t index, Promise<bool>&& promise);

  //  Get adapter by connector id.
  uint16_t GetAdapterIdByConnectorId(uint64_t fixed_connetor_id);

  // Submit io task.
  bool SubmitIoTask(MsgTask* task);

 private:
  // Count io index.
  uint16_t io_index_ = 0;

  // Connection options.
  Options options_;

  // Is merge thread or not.
  bool merge_thread_model_ = false;

  // All the adapter pointers.
  std::vector<std::unique_ptr<FutureTransportAdapter>> adapters_;

  // Time to wait for create or release connector, in millisecond.
  static const uint64_t kCreateRealseConnectorTimeout = 1000;
};

}  // namespace trpc
