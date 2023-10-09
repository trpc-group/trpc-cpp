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

#include "trpc/filter/server_filter_controller.h"
#include "trpc/stream/trpc/trpc_stream.h"

namespace trpc::stream {

/// @brief Implementation of the tRPC server stream.
class TrpcServerStream : public TrpcStream {
 public:
  explicit TrpcServerStream(StreamOptions&& options) : TrpcStream(std::move(options)) {}

  ~TrpcServerStream() override = default;

  Status WriteDone() override;

  Status Start() override;

  Status Finish() override;

  int GetEncodeErrorCode() override;

  int GetDecodeErrorCode() override;

  int GetReadTimeoutErrorCode() override;

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
  void SendReset(const Status& status) override;

 private:
  using TrpcStream::SendReset;
};

}  // namespace trpc::stream
