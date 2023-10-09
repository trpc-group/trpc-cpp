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
#include "trpc/stream/trpc/trpc_stream_handler.h"
#include "trpc/util/unique_id.h"

namespace trpc::stream {

/// @brief The implementation of tRPC client stream handler.
class TrpcClientStreamHandler : public TrpcStreamHandler {
 public:
  explicit TrpcClientStreamHandler(StreamOptions&& options)
      : TrpcStreamHandler(std::move(options)), stream_id_generator_{100} {}

  ~TrpcClientStreamHandler() override = default;

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override;

  int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) override;

 protected:
  inline int GetNetworkErrorCode() override;

 private:
  UniqueId stream_id_generator_;
  ClientFilterController filter_controller_;
};

}  // namespace trpc::stream
