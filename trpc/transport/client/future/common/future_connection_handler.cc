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

#include "trpc/transport/client/future/common/future_connection_handler.h"

#include "trpc/filter/filter_point.h"
#include "trpc/util/time.h"

namespace trpc {

int FutureConnectionHandler::CheckMessage(const ConnectionPtr& conn,
                                          NoncontiguousBuffer& in,
                                          std::deque<std::any>& out) {
  assert(trans_info_->checker_function);
  return trans_info_->checker_function(conn, in, out);
}

void FutureConnectionHandler::ConnectionEstablished() {
  if (trans_info_->conn_establish_function) {
    trans_info_->conn_establish_function(connection_);
  }
}

void FutureConnectionHandler::ConnectionClosed() {
  if (trans_info_->conn_close_function) {
    trans_info_->conn_close_function(connection_);
  }
}

void FutureConnectionHandler::MessageWriteDone(const IoMessage& message) {
  if (TRPC_UNLIKELY(pipeline_send_notify_func_)) {
    pipeline_send_notify_func_(message);
  }

  if (trans_info_->run_io_filters_function) {
    trans_info_->run_io_filters_function(FilterPoint::CLIENT_POST_IO_SEND_MSG, message.msg);
  }
}

}  // namespace trpc
