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

#include "trpc/stream/common_stream.h"
#include "trpc/stream/trpc/trpc_stream_flow_controller.h"

namespace trpc::stream {

/// @brief Implements the specific interaction logic between the business and the Trpc stream.
/// Implements the specific interaction logic between the transport layer and the Trpc stream.
class TrpcStream : public CommonStream {
 public:
  explicit TrpcStream(StreamOptions&& options) : CommonStream(std::move(options)) {}

  ~TrpcStream() = default;

  Status Read(NoncontiguousBuffer* msg, int timeout = -1) override;

  Status Write(NoncontiguousBuffer&& msg) override;

  void Close(Status status) override;

  void Reset(Status status) override;

 protected:
  RetCode HandleInit(StreamRecvMessage&& msg) override { return RetCode::kError; }

  RetCode HandleData(StreamRecvMessage&& msg) override;

  RetCode HandleFeedback(StreamRecvMessage&& msg) override;

  RetCode HandleClose(StreamRecvMessage&& msg) override { return RetCode::kError; }

  RetCode HandleReset(StreamRecvMessage&& msg) override {
    // The reset frame of Trpc stream is carried by the Close frame.
    return RetCode::kError;
  }

  RetCode SendInit(StreamSendMessage&& msg) override { return RetCode::kError; }

  RetCode SendData(StreamSendMessage&& msg) override;

  RetCode SendFeedback(StreamSendMessage&& msg) override;

  RetCode SendClose(StreamSendMessage&& msg) override;

  RetCode SendReset(StreamSendMessage&& msg) override {
    // The reset frame of Trpc stream is carried by the Close frame.
    return RetCode::kError;
  }

 protected:
  bool EncodeMessage(Protocol* protocol, NoncontiguousBuffer* buffer);

  bool DecodeMessage(NoncontiguousBuffer* buffer, Protocol* protocol);

  virtual void SendReset(const Status& status);

  void OnError(Status status) override;

 protected:
  RefPtr<TrpcStreamRecvController> recv_flow_controller_{nullptr};
  RefPtr<TrpcStreamSendController> send_flow_controller_{nullptr};
  FiberMutex flow_control_mutex_;
  FiberConditionVariable flow_control_cond_;
};

using TrpcStreamPtr = RefPtr<TrpcStream>;

}  // namespace trpc::stream
