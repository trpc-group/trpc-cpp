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

#include "trpc/codec/server_codec_factory.h"
#include "trpc/common/future/future.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/server/method.h"
#include "trpc/server/method_handler.h"
#include "trpc/server/server_context.h"
#include "trpc/stream/stream_async.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief Implementation of asynchronous streaming RPC.
template <class RequestType, class ResponseType>
class AsyncStreamRpcMethodHandler final : public RpcMethodHandlerInterface {
  using R = RequestType;
  using W = ResponseType;

 public:
  using ServerStreamFunc = std::function<Future<>(const ServerContextPtr&, R&&, const stream::AsyncWriterPtr<W>&)>;

  using ClientStreamFunc = std::function<Future<W>(const ServerContextPtr&, const stream::AsyncReaderPtr<R>&)>;

  using BidiStreamFunc = std::function<Future<>(const ServerContextPtr&, const stream::AsyncReaderWriterPtr<R, W>&)>;

  explicit AsyncStreamRpcMethodHandler(const ServerStreamFunc& func)
      : server_stream_func_(func), type_(MethodType::SERVER_STREAMING) {}

  explicit AsyncStreamRpcMethodHandler(const ClientStreamFunc& func)
      : client_stream_func_(func), type_(MethodType::CLIENT_STREAMING) {}

  explicit AsyncStreamRpcMethodHandler(const BidiStreamFunc& func)
      : bidi_stream_func_(func), type_(MethodType::BIDI_STREAMING) {}

  void Execute(const ServerContextPtr& context) noexcept override {
    auto provider = context->GetStreamReaderWriterProvider();

    provider->AsyncStart()
        .Then([this, context, provider]() {
          stream::MessageContentCodecOptions content_codec_ops{
              .serialization = serialization::SerializationFactory::GetInstance()->Get(context->GetReqEncodeType()),
              .content_type = context->GetReqEncodeType(),
          };

          auto rw = MakeRefCounted<stream::AsyncReaderWriter<R, W>>(provider, content_codec_ops);

          switch (type_) {
            case MethodType::SERVER_STREAMING:
              return rw->Read().Then([this, context, rw](std::optional<R>&& req) {
                if (!req) {
                  return MakeExceptionFuture<>(stream::StreamError(Status(-1, -1, "unexcepted stream eof")));
                }
                return server_stream_func_(context, std::move(req.value()), rw->Writer());
              });
            case MethodType::CLIENT_STREAMING:
              return client_stream_func_(context, rw->Reader()).Then([rw](W&& rsp) {
                return rw->Write(std::move(rsp));
              });
            case MethodType::BIDI_STREAMING:
              return bidi_stream_func_(context, rw);
            default:
              return MakeExceptionFuture<>(stream::StreamError(Status(-1, -1, "unsupported method type")));
          }
        })
        .Then([this, provider](Future<>&& ft) {
          if (ft.IsFailed()) {
            auto ex = ft.GetException();
            // If there is a network error, there is no need to send a reset, because the connection is already unusable
            // at this point.
            if (ex.GetExceptionCode() != TRPC_STREAM_SERVER_NETWORK_ERR) {
              TRPC_FMT_ERROR("Reset server stream with error: {}", ex.what());
              provider->Reset(ExceptionToStatus(std::move(ex)));
            }
          } else {
            TRPC_FMT_DEBUG("Close server stream.");
            provider->Close(kSuccStatus);
          }
          return MakeReadyFuture<>();
        });  // future discarded
  }

 private:
  Status ExceptionToStatus(Exception&& ex) {
    if (ex.is<stream::StreamError>()) {
      return ex.Get<stream::StreamError>()->GetStatus();
    } else {
      return Status(ex.GetExceptionCode(), 0, ex.what());
    }
  }

  void Execute(const ServerContextPtr& context, NoncontiguousBuffer&& req_data,
               NoncontiguousBuffer& rsp_data) noexcept override {
    TRPC_ASSERT(false && "Unreachable");
  }

 private:
  ServerStreamFunc server_stream_func_;

  ClientStreamFunc client_stream_func_;

  BidiStreamFunc bidi_stream_func_;

  MethodType type_;
};

}  // namespace trpc
