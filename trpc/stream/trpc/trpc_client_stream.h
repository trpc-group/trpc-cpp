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

#include "trpc/filter/client_filter_controller.h"
#include "trpc/stream/trpc/trpc_stream.h"

namespace trpc::stream {

/// @brief Implementation of HTTP client stream.
/// @note Used internally by framework.
class TrpcClientStream : public TrpcStream {
 public:
  explicit TrpcClientStream(StreamOptions&& options) : TrpcStream(std::move(options)) {}

  ~TrpcClientStream() override = default;

  Status WriteDone() override;

  Status Start() override;

  Future<> AsyncStart() override;

  Status Finish() override;

  Future<> AsyncFinish() override;

  int GetEncodeErrorCode() override;

  int GetDecodeErrorCode() override;

  int GetReadTimeoutErrorCode() override;

  void SetFilterController(ClientFilterController* filter_controller) { filter_controller_ = filter_controller; }

 protected:
  bool CheckState(State state, Action action);

  RetCode SendInit(StreamSendMessage&& msg) override;

  RetCode SendData(StreamSendMessage&& msg) override;

  RetCode SendClose(StreamSendMessage&& msg) override;

  RetCode HandleInit(StreamRecvMessage&& msg) override;

  RetCode HandleData(StreamRecvMessage&& msg) override;

  RetCode HandleClose(StreamRecvMessage&& msg) override;

  FilterStatus RunMessageFilter(const FilterPoint& point, const ClientContextPtr& context);

  void SendReset(const Status& status) override;

 private:
  using TrpcStream::SendReset;

 private:
  // Used to deserialize results when sending response messages.
  MessageContentCodecOptions msg_codec_options_;

  // Buried point executor; the `stream_handler` is set when creating a stream, and one connection corresponds to one
  // `stream_handler`. The `stream_handler` is always destroyed only after all the streams have terminated, so it is
  // always safe to use `filter_controller_` in the stream.
  ClientFilterController* filter_controller_{nullptr};
};

}  // namespace trpc::stream
