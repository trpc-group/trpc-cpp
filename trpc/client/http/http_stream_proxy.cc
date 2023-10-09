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

#include "trpc/client/http/http_stream_proxy.h"

#include "trpc/codec/http/http_protocol.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"

namespace trpc::stream {

Future<HttpClientAsyncStreamReaderWriterPtr> HttpStreamProxy::GetAsyncStreamReaderWriter(const ClientContextPtr& ctx) {
  if (!ctx->GetRequest()) {
    ctx->SetRequest(codec_->CreateRequestPtr());
  }
  ctx->SetServiceProxyOption(ServiceProxy::GetMutableServiceProxyOption());
  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  if (filter_status == FilterStatus::REJECT) {
    Status status{TRPC_STREAM_UNKNOWN_ERR, 0,
                  "service name:" + GetServiceName() + ", filter PRE_PRC_INVOKE execute failed."};
    TRPC_FMT_ERROR(status.ErrorMessage());
    ctx->SetStatus(status);
    return MakeExceptionFuture<HttpClientAsyncStreamReaderWriterPtr>(stream::StreamError(status));
  }
  filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_SEND_MSG, ctx);
  if (filter_status == FilterStatus::REJECT) {
    Status status{TRPC_STREAM_UNKNOWN_ERR, 0,
                  "service name:" + GetServiceName() + ", filter PRE_SEND_MSG execute failed."};
    TRPC_FMT_ERROR(status.ErrorMessage());
    ctx->SetStatus(status);
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
    return MakeExceptionFuture<HttpClientAsyncStreamReaderWriterPtr>(stream::StreamError(status));
  }

  auto req = dynamic_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  TRPC_ASSERT(req && "unmatched codec");
  bool invoke_by_worker_thread = WorkerThread::GetCurrentWorkerThread() != nullptr;
  // Streams need to be created in the `merge` thread model. Calls initiated by non-worker threads will be thrown into
  // worker threads for execution.
  if (!invoke_by_worker_thread) {
    if (GetThreadModel()->Type() != std::string(kMerge)) {
      Status status{TRPC_STREAM_CLIENT_NETWORK_ERR, 0, "Only merge threads can create stream"};
      TRPC_FMT_ERROR(status.ErrorMessage());
      ctx->SetStatus(status);
      filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RECV_MSG, ctx);
      filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
      return MakeExceptionFuture<HttpClientAsyncStreamReaderWriterPtr>(stream::StreamError(status));
    }
    auto prom = std::make_shared<::trpc::Promise<HttpClientAsyncStreamReaderWriterPtr>>();
    // Choose a random reactor to submit IO tasks.
    bool ret = static_cast<MergeThreadModel*>(GetThreadModel())->GetReactor(-1)->SubmitTask([&, prom, ctx]() {
      auto stream = SelectStreamProvider(ctx, nullptr);
      if (stream->GetStatus().OK()) {
        auto async_stream = static_pointer_cast<HttpClientAsyncStream>(stream);
        async_stream->SetFilterController(&filter_controller_);
        prom->SetValue(MakeRefCounted<HttpClientAsyncStreamReaderWriter>(async_stream));
      } else {
        ctx->SetStatus(stream->GetStatus());
        TRPC_FMT_ERROR(stream->GetStatus().ErrorMessage());
        filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RECV_MSG, ctx);
        filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
        prom->SetException(stream::StreamError(stream->GetStatus()));
      }
    });

    if (!ret) {
      Status status{TRPC_CLIENT_OVERLOAD_ERR, 0, "overload cannot submit task."};
      TRPC_FMT_ERROR(status.ErrorMessage());
      ctx->SetStatus(status);
      filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RECV_MSG, ctx);
      filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
      return MakeExceptionFuture<HttpClientAsyncStreamReaderWriterPtr>(stream::StreamError(status));
    }

    return prom->GetFuture();
  }
  auto stream = SelectStreamProvider(ctx, nullptr);
  if (!stream->GetStatus().OK()) {
    TRPC_FMT_ERROR(stream->GetStatus().ErrorMessage());
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RECV_MSG, ctx);
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
    return MakeExceptionFuture<HttpClientAsyncStreamReaderWriterPtr>(stream::StreamError(stream->GetStatus()));
  }
  auto async_stream = static_pointer_cast<HttpClientAsyncStream>(stream);
  async_stream->SetFilterController(&filter_controller_);
  return MakeReadyFuture<HttpClientAsyncStreamReaderWriterPtr>(
      MakeRefCounted<HttpClientAsyncStreamReaderWriter>(async_stream));
}

}  // namespace trpc::stream
