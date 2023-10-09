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

#include <functional>

#include "trpc/common/future/future.h"
#include "trpc/stream/http/async/stream_reader_writer.h"
#include "trpc/util/http/handler.h"
#include "trpc/util/log/logging.h"

namespace trpc::stream {

/// @brief The FuncHandler for http asynchronous stream
/// @note Only the asynchronous interface can be used.
class HttpAsyncStreamFuncHandler : public trpc::http::HandlerBase {
 public:
  using BidiStreamFunction = std::function<Future<>(const ServerContextPtr&, HttpServerAsyncStreamReaderWriterPtr)>;

 public:
  explicit HttpAsyncStreamFuncHandler(const BidiStreamFunction& func) : HandlerBase(true), bidi_stream_func_(func) {}

  trpc::Status Handle(const std::string& path, ServerContextPtr context, trpc::http::HttpRequestPtr req,
                      trpc::http::HttpResponse* rsp) override {
    TRPC_ASSERT(false && "Should not be invoked");
  }

  Future<> Handle(ServerContextPtr context, HttpServerAsyncStreamPtr stream) {
    HttpServerAsyncStreamReaderWriterPtr rw = MakeRefCounted<HttpServerAsyncStreamReaderWriter>(stream);
    return bidi_stream_func_(context, rw).Then([stream]() {
      if (!stream->IsStateTerminate()) {
        trpc::Status err_status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, "stream not close"};
        TRPC_FMT_ERROR(err_status.ErrorMessage());
        return MakeExceptionFuture<>(StreamError(err_status));
      }
      return MakeReadyFuture<>();
    });
  }

 private:
  BidiStreamFunction bidi_stream_func_{nullptr};
};

}  // namespace trpc::stream
