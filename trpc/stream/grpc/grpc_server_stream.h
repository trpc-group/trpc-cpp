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

#include "trpc/filter/server_filter_controller.h"
#include "trpc/stream/grpc/grpc_stream.h"

namespace trpc::stream {

/// @brief Implementation of the gRPC server stream.
class GrpcServerStream : public GrpcStream {
 public:
  explicit GrpcServerStream(StreamOptions&& options, http2::Session* session, FiberMutex* mutex,
                            FiberConditionVariable* cv)
      : GrpcStream(std::move(options), session, mutex, cv) {}

  ~GrpcServerStream() override = default;

  Status Write(NoncontiguousBuffer&& msg) override;

  Status WriteDone() override;

  Status Start() override;

  Status Finish() override;

 protected:
  // @brief Check if the stream's state is normal.
  bool CheckState(State state, Action action);

  RetCode HandleInit(StreamRecvMessage&& msg) override;

  RetCode HandleData(StreamRecvMessage&& msg) override;

  RetCode HandleClose(StreamRecvMessage&& msg) override;

  RetCode SendInit(StreamSendMessage&& msg) override;

  RetCode SendData(StreamSendMessage&& msg) override;

  RetCode SendClose(StreamSendMessage&& msg) override;

  FilterStatus RunMessageFilter(const FilterPoint& point, const ServerContextPtr& context);

  // @brief When the stream is abnormal, send a Close frame with Reset directly.
  void SendReset(uint32_t error_code) override;

 private:
  using GrpcStream::SendReset;

 private:
  // Used to check if the HEADER of the response has already been sent.
  bool send_header_{false};
  // Used for flow control timeout control to prevent flow control triggers in exception cases, but the stream has
  // already ended. It will support configuration later (default is 1 second).
  int write_flow_control_timeout_{1000};
};

}  // namespace trpc::stream
