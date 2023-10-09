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

#include "examples/features/trpc_async_stream/server/stream_service.h"

#include "trpc/future/async_timer.h"
#include "trpc/future/future_utility.h"
#include "trpc/util/string_util.h"

using namespace std::chrono_literals;

namespace test::helloworld {

// Client streaming RPC.
::trpc::Future<::trpc::test::helloworld::HelloReply> AsyncStreamGreeterServiceImpl::ClientStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::AsyncReaderPtr<::trpc::test::helloworld::HelloRequest>& reader) {
  struct State {
    std::size_t request_counter{0};
    std::size_t request_bytes{0};
  };
  auto state = std::make_shared<State>();

  return ::trpc::DoUntil([state, reader]() {
           return reader->Read(3000ms).Then(
               [state, reader](std::optional<::trpc::test::helloworld::HelloRequest>&& req) {
                 if (req) {
                   TRPC_FMT_INFO("Client says: {}", req.value().msg());
                   ++state->request_counter;
                   state->request_bytes += req.value().msg().size();
                   return ::trpc::MakeReadyFuture<bool>(true);
                 } else {
                   TRPC_FMT_INFO("Client EOF");
                   return ::trpc::MakeReadyFuture<bool>(false);
                 }
               });
         })
      .Then([state]() {
        auto msg =
            ::trpc::util::FormatString("get {} requests, {} bytes", state->request_counter, state->request_bytes);
        TRPC_FMT_INFO("Server says: {}", msg);
        ::trpc::test::helloworld::HelloReply reply;
        reply.set_msg(std::move(msg));
        return ::trpc::MakeReadyFuture<::trpc::test::helloworld::HelloReply>(std::move(reply));
      });
}

// Server streaming RPC.
::trpc::Future<> AsyncStreamGreeterServiceImpl::ServerStreamSayHello(
    const ::trpc::ServerContextPtr& context, ::trpc::test::helloworld::HelloRequest&& request,
    const ::trpc::stream::AsyncWriterPtr<::trpc::test::helloworld::HelloReply>& writer) {
  TRPC_FMT_INFO("Client says: {}", request.msg());
  return ::trpc::DoFor(10,
                       [writer, request = std::move(request)](std::size_t i) {
                         auto msg = ::trpc::util::FormatString("{}#{}", request.msg(), i + 1);
                         TRPC_FMT_INFO("Server says: {}", msg);
                         ::trpc::test::helloworld::HelloReply reply;
                         reply.set_msg(std::move(msg));
                         return writer->Write(std::move(reply));
                       })
      .Then([]() {
        // Trigger a read timeout(>3000ms) for client to show its usage
        return ::trpc::AsyncTimer(false).After(3050 /*ms*/);
      });
}

// Bi-direction streaming RPC.
::trpc::Future<> AsyncStreamGreeterServiceImpl::BidiStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::AsyncReaderWriterPtr<::trpc::test::helloworld::HelloRequest,
                                               ::trpc::test::helloworld::HelloReply>& rw) {
  struct State {
    std::size_t request_counter{0};
    std::size_t request_bytes{0};
  };
  auto state = std::make_shared<State>();

  return ::trpc::DoUntil([state, rw]() {
    return rw->Read(3000ms).Then([state, rw](std::optional<::trpc::test::helloworld::HelloRequest>&& req) {
      if (req) {
        TRPC_FMT_INFO("Client says: {}", req.value().msg());
        ++state->request_counter;
        state->request_bytes += req.value().msg().size();

        auto msg =
            ::trpc::util::FormatString("get {} requests, {} bytes", state->request_counter, state->request_bytes);
        TRPC_FMT_INFO("Server says: {}", msg);
        ::trpc::test::helloworld::HelloReply reply;
        reply.set_msg(std::move(msg));
        return rw->Write(std::move(reply)).Then([]() { return ::trpc::MakeReadyFuture<bool>(true); });
      } else {
        TRPC_FMT_INFO("Client EOF");
        return ::trpc::MakeReadyFuture<bool>(false);
      }
    });
  });
}

}  // namespace test::helloworld
