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

#include <deque>
#include <list>
#include <utility>

#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/transport/client/trans_info.h"
#include "trpc/transport/client/future/common/future_connector_options.h"

namespace trpc {

/// @brief ConnectionHandler implementation for future transport.
class FutureConnectionHandler : public ConnectionHandler {
 public:
  /// @brief Constructor.
  explicit FutureConnectionHandler(const FutureConnectorOptions& options, TransInfo* trans_info)
           : options_(options), pipeline_send_notify_func_(nullptr), trans_info_(trans_info) {}

  /// @brief Destructor.
  ~FutureConnectionHandler() override = default;

  /// @brief Check recv message is complete or not.
  int CheckMessage(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) override;

  /// @brief Callback when connection established.
  void ConnectionEstablished() override;

  /// @brief Callback when connection closed.
  void ConnectionClosed() override;

  /// @brief Callback when finish message write.
  void MessageWriteDone(const IoMessage& message) override;

  /// @brief Get pipeline count.
  uint32_t GetMergeRequestCount() override { return 1; }

  /// @brief Set pipeline send success callback.
  void SetPipelineSendNotifyFunc(PipelineSendNotifyFunction&& func) { pipeline_send_notify_func_ = std::move(func); }

  /// @brief Set connection.
  void SetConnection(Connection* connection) { connection_ = connection; }

  /// @brief Get connection.
  Connection* GetConnection() const override { return connection_; }

  /// @brief Get stream handler, create it if necessary.
  virtual stream::StreamHandlerPtr GetOrCreateStreamHandler() { return nullptr; }

 protected:
  // Options shared by all subclass.
  const FutureConnectorOptions& options_;

 private:
  // Pipeline send success callback.
  PipelineSendNotifyFunction pipeline_send_notify_func_;

  // Pointer to transport info.
  TransInfo* trans_info_;

  // Related connection.
  Connection* connection_;
};

}  // namespace trpc
