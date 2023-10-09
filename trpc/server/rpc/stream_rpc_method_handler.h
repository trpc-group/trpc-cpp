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
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "trpc/codec/server_codec_factory.h"
#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/server/method.h"
#include "trpc/server/method_handler.h"
#include "trpc/server/server_context.h"
#include "trpc/stream/stream.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief Implementation of streaming RPC.
template <class RequestType, class ResponseType>
class StreamRpcMethodHandler : public RpcMethodHandlerInterface {
 public:
  using ServerStreamFunction =
      std::function<Status(const ServerContextPtr&, const RequestType&, stream::StreamWriter<ResponseType>*)>;

  using ClientStreamFunction =
      std::function<Status(const ServerContextPtr&, const stream::StreamReader<RequestType>&, ResponseType*)>;

  using BidiStreamFunction = std::function<Status(const ServerContextPtr, const stream::StreamReader<RequestType>&,
                                                  stream::StreamWriter<ResponseType>*)>;

  explicit StreamRpcMethodHandler(const ServerStreamFunction& func)
      : server_stream_func_(func), type_(MethodType::SERVER_STREAMING) {}

  explicit StreamRpcMethodHandler(const ClientStreamFunction& func)
      : client_stream_func_(func), type_(MethodType::CLIENT_STREAMING) {}

  explicit StreamRpcMethodHandler(const BidiStreamFunction& func)
      : bidi_stream_func_(func), type_(MethodType::BIDI_STREAMING) {}

  ~StreamRpcMethodHandler() override {}

  void Execute(const ServerContextPtr& context, NoncontiguousBuffer&& req_data,
               NoncontiguousBuffer& rsp_data) noexcept override {
    TRPC_ASSERT(false && "Unreachable");
  }

  void Execute(const ServerContextPtr& context) noexcept override {
    using R = RequestType;
    using W = ResponseType;

    // StreamReaderWriterProvider object will be moved from context.
    auto stream_provider = context->GetStreamReaderWriterProvider();
    Status status{kSuccStatus};

    // Starts the stream, and waits for the stream to be ready.
    status = stream_provider->Start();
    if (!status.OK()) {
      stream_provider->Reset(status);
      return;
    }

    // Initializes codec options.
    stream::MessageContentCodecOptions content_codec_ops{
        .serialization = serialization::SerializationFactory::GetInstance()->Get(context->GetReqEncodeType()),
        .content_type = context->GetReqEncodeType(),
    };

    auto rw = stream::Create<W, R>(stream_provider, content_codec_ops);

    // Dispatches streaming RPC method.
    switch (type_) {
      case MethodType::SERVER_STREAMING: {
        R request{};
        // Waits for the request, default timeout: 32s.
        // FIXME: Reads timeout from configure.
//        int read_timeout = context->GetService()->GetServiceAdapterOption().stream_read_timeout;
        int read_timeout = 32000;
        status = rw.Read(&request, read_timeout);
        if (status.OK()) {
          auto writer = rw.Writer();
          status = server_stream_func_(context, request, &writer);
        }
        break;
      }
      case MethodType::CLIENT_STREAMING: {
        W response{};
        auto reader = rw.Reader();
        status = client_stream_func_(context, reader, &response);
        Status write_status = rw.Write(response);
        if (!write_status.OK()) {
          status = write_status;
        }
        break;
      }
      case MethodType::BIDI_STREAMING: {
        auto reader = rw.Reader();
        auto writer = rw.Writer();
        status = bidi_stream_func_(context, reader, &writer);
        break;
      }
      default: {
        TRPC_LOG_ERROR("unsupported method");
        status = Status{-1, -1, "unsupported method"};
      }
    }

    // Sets final status of RPC execution.
    context->SetStatus(status);
    stream_provider->Close(status);
  }

 private:
  // Server-side streaming RPC.
  ServerStreamFunction server_stream_func_;
  // Client-side streaming RPC.
  ClientStreamFunction client_stream_func_;
  // Bidirectional streaming RPC.
  BidiStreamFunction bidi_stream_func_;
  trpc::MethodType type_;
};

}  // namespace trpc
