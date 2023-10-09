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

#include "trpc/coroutine/fiber_blocking_noncontiguous_buffer.h"
#include "trpc/stream/http/common.h"
#include "trpc/stream/stream_provider.h"

namespace trpc::stream {

/// @brief The interface of HTTP stream reader-writer.
class HttpStreamReaderWriterProvider : public StreamReaderWriterProvider {
 public:
  HttpStreamReaderWriterProvider() : read_buffer_(10000000) {}

  Status Read(NoncontiguousBuffer* msg, int timeout) override { return status_; }

  Status Write(NoncontiguousBuffer&& msg) override = 0;

  Status WriteDone() override = 0;

  Status Start() override { return status_; }

  Status Finish() override { return status_; }

  void Close(Status status) override { status_ = std::move(status); }

  void Reset(Status status) override { status_ = std::move(status); }

  Status GetStatus() const override { return status_; }

  StreamOptions* GetMutableStreamOptions() override = 0;

  int GetReadTimeoutErrorCode() override { return 1; }

 protected:
  FiberBlockingNoncontiguousBuffer read_buffer_;
  Status status_ = kDefaultStatus;
};
using HttpStreamReaderWriterProviderPtr = RefPtr<HttpStreamReaderWriterProvider>;

}  // namespace trpc::stream
