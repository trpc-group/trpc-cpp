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

#include "trpc/codec/server_codec_factory.h"
#include "trpc/filter/server_filter_controller.h"
#include "trpc/stream/trpc/trpc_stream_handler.h"

namespace trpc::stream {

/// @brief Implementation of stream handler for tRPC server stream.
class TrpcServerStreamHandler : public TrpcStreamHandler {
 public:
  explicit TrpcServerStreamHandler(StreamOptions&& options) : TrpcStreamHandler(std::move(options)) {
    server_codec_ = ServerCodecFactory::GetInstance()->Get("trpc");
    if (!server_codec_) {
      TRPC_LOG_ERROR("failed to get server_codec protocol: grpc");
      TRPC_ASSERT(server_codec_);
    }
  }

  ~TrpcServerStreamHandler() override = default;

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override;

  int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) override;

 protected:
  inline int GetNetworkErrorCode() override;

 private:
  ServerCodecPtr server_codec_{nullptr};
};

}  // namespace trpc::stream
