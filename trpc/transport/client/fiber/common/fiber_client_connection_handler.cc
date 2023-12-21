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

#include "trpc/transport/client/fiber/common/fiber_client_connection_handler.h"

#include "trpc/filter/filter_point.h"

namespace trpc {

FiberClientConnectionHandler::FiberClientConnectionHandler(Connection* conn, TransInfo* trans_info)
    : connection_(conn), trans_info_(trans_info) {}

int FiberClientConnectionHandler::CheckMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                         std::deque<std::any>& out) {
  TRPC_ASSERT(trans_info_->checker_function);
  return trans_info_->checker_function(conn, in, out);
}

bool FiberClientConnectionHandler::HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& data) {
  TRPC_ASSERT(message_handle_func_);
  return message_handle_func_(conn, data);
}

void FiberClientConnectionHandler::CleanResource() {
  if (clean_func_) {
    clean_func_(connection_);
  }
}

void FiberClientConnectionHandler::ConnectionEstablished() {
  if (trans_info_->conn_establish_function) {
    trans_info_->conn_establish_function(connection_);
  }
}

void FiberClientConnectionHandler::ConnectionClosed() {
  if (trans_info_->conn_close_function) {
    trans_info_->conn_close_function(connection_);
  }
}

void FiberClientConnectionHandler::PreSendMessage(const IoMessage& io_msg) {
  if (pipeline_send_notify_func_) {
    pipeline_send_notify_func_(io_msg);
  }
}

void FiberClientConnectionHandler::MessageWriteDone(const IoMessage& message) {
  TRPC_FMT_DEBUG("MessageWriteDone request_id: {}", message.seq_id);

  if (trans_info_->run_io_filters_function) {
    trans_info_->run_io_filters_function(FilterPoint::CLIENT_POST_IO_SEND_MSG, message.msg);
  }
}

void FiberClientConnectionHandler::NotifySendMessage() {
  if (notify_msg_send_func_) {
    notify_msg_send_func_(connection_);
  }
}

uint32_t FiberClientConnectionHandler::GetMergeRequestCount() {
  if (get_merge_request_count_func_) {
    return get_merge_request_count_func_();
  }

  return 1;
}

}  // namespace trpc
