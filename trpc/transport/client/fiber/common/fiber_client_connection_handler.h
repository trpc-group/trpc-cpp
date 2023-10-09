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

namespace trpc {

/// @brief The implementation for fiber client connection handler
class FiberClientConnectionHandler : public ConnectionHandler {
 public:
  explicit FiberClientConnectionHandler(Connection* conn, TransInfo* trans_info);

  Connection* GetConnection() const override { return connection_; }

  int CheckMessage(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) override;

  bool HandleMessage(const ConnectionPtr&, std::deque<std::any>&) override;

  void CleanResource() override;

  void ConnectionEstablished() override;

  void ConnectionClosed() override;

  void PreSendMessage(const IoMessage&) override;

  void MessageWriteDone(const IoMessage& message) override;

  void NotifySendMessage() override;

  uint32_t GetMergeRequestCount() override;

  /// @brief Gets or creates a stream handler.
  virtual stream::StreamHandlerPtr GetOrCreateStreamHandler() { return nullptr; }

  void SetCleanFunc(ConnectionCleanFunction&& func) { clean_func_ = std::move(func); }
  void SetMsgHandleFunc(MessageHandleFunction&& func) { message_handle_func_ = std::move(func); }
  void SetNotifyMsgSendFunc(NotifySendMsgFunction&& func) { notify_msg_send_func_ = std::move(func); }
  void SetGetMergeRequestCountFunc(GetMergeRequestCountFunction&& func) {
    get_merge_request_count_func_ = std::move(func);
  }

  void SetPipelineSendNotifyFunc(PipelineSendNotifyFunction&& func) { pipeline_send_notify_func_ = std::move(func); }

  void SetCurrentContextExt(uint32_t context_ext) override { context_ext_ = context_ext; }
  uint32_t GetCurrentContextExt() override { return context_ext_; }

  virtual bool NeedSendStreamMessage() { return true; }

  TransInfo* GetTransInfo() const { return trans_info_; }

 private:
  Connection* connection_{nullptr};
  TransInfo* trans_info_{nullptr};
  uint32_t context_ext_{0};
  ConnectionCleanFunction clean_func_{nullptr};
  MessageHandleFunction message_handle_func_{nullptr};
  NotifySendMsgFunction notify_msg_send_func_{nullptr};
  GetMergeRequestCountFunction get_merge_request_count_func_{nullptr};
  PipelineSendNotifyFunction pipeline_send_notify_func_{nullptr};
};

namespace stream {

/// @brief The implementation of fiber client stream connection handler.
using FiberClientStreamConnectionHandler = FiberClientConnectionHandler;

}  // namespace stream

}  // namespace trpc
