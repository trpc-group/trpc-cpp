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

#include "examples/features/trpc_stream_forward/proxy/stream_forward_service.h"

#include <vector>

#include "gflags/gflags.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"

#include "examples/features/trpc_stream/server/stream.trpc.pb.h"

namespace test::helloworld {

using StreamGreeterServiceProxy = ::trpc::test::helloworld::StreamGreeterServiceProxy;
using StreamGreeterServiceProxyPtr = std::shared_ptr<StreamGreeterServiceProxy>;

const StreamGreeterServiceProxyPtr& GetStreamGreeterServiceProxyPtr() {
  static StreamGreeterServiceProxyPtr proxy =
      ::trpc::GetTrpcClient()->GetProxy<StreamGreeterServiceProxy>("trpc.test.helloworld.StreamGreeter");
  return proxy;
}

// Client streaming RPC.
::trpc::Status StreamForwardImpl::ClientStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
    ::trpc::test::helloworld::HelloReply* reply) {
  ::trpc::Status status{};
  const StreamGreeterServiceProxyPtr& stream_greeter_proxy = GetStreamGreeterServiceProxyPtr();
  auto client_context = ::trpc::MakeClientContext(context, stream_greeter_proxy);
  do {
    auto stream = stream_greeter_proxy->ClientStreamSayHello(client_context, reply);
    status = stream.GetStatus();
    if (!status.OK()) {
      TRPC_FMT_ERROR("got stream error: {}", status.ToString());
      break;
    }

    for (;;) {
      ::trpc::test::helloworld::HelloRequest request;
      status = reader.Read(&request);
      // Sends the request read from the client to upstream.
      if (status.OK()) {
        status = stream.Write(request);
        if (status.OK()) {
          TRPC_FMT_INFO("read request from client, then write request to upstream: {}", request.msg());
          continue;
        }
      }
      if (status.StreamEof()) {
        status = stream.WriteDone();
        if (status.OK()) {
          TRPC_FMT_INFO("read EOF from client, wait upstream to finish");
          status = stream.Finish();
        }
      }
      break;
    }

    if (status.OK()) {
      TRPC_FMT_INFO("stream rpc succeed, reply: {}", reply->msg());
    } else {
      TRPC_FMT_INFO("stream rpc failed: {}", status.ToString());
    }
  } while (0);
  TRPC_FMT_INFO("final status: {}", status.ToString());
  return status;
}

// Server streaming RPC.
::trpc::Status StreamForwardImpl::ServerStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::test::helloworld::HelloRequest& request,  // NO LINT
    ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
  ::trpc::Status status{};
  const StreamGreeterServiceProxyPtr& stream_greeter_proxy = GetStreamGreeterServiceProxyPtr();
  auto client_context = ::trpc::MakeClientContext(context, stream_greeter_proxy);
  TRPC_FMT_INFO("got request from client: {}", request.msg());
  do {
    auto stream = stream_greeter_proxy->ServerStreamSayHello(client_context, request);
    status = stream.GetStatus();
    if (!status.OK()) {
      TRPC_FMT_ERROR("got stream error: {}", status.ToString());
      break;
    }

    for (;;) {
      ::trpc::test::helloworld::HelloReply reply;
      status = stream.Read(&reply);
      // Sends reply written by upstream to the client.
      if (status.OK()) {
        status = writer->Write(reply);
        if (status.OK()) {
          TRPC_LOG_INFO("read response from upstream, then write reply to client: " << reply.msg());
          continue;
        }
      }
      if (status.StreamEof()) {
        TRPC_LOG_INFO("read EOF from upstream, wait upstream to finish");
        status = stream.Finish();
      }
      break;
    }

    if (status.OK()) {
      TRPC_FMT_INFO("stream rpc succeed");
    } else {
      TRPC_FMT_INFO("stream rpc failed: {}", status.ToString());
    }
  } while (0);
  TRPC_FMT_INFO("final status: {}", status.ToString());
  return status;
}

// Bi-direction streaming RPC.
::trpc::Status StreamForwardImpl::BidiStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
    ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
  ::trpc::Status status{};
  const StreamGreeterServiceProxyPtr& stream_greeter_proxy = GetStreamGreeterServiceProxyPtr();
  auto client_context = ::trpc::MakeClientContext(context, stream_greeter_proxy);
  do {
    auto stream = stream_greeter_proxy->BidiStreamSayHello(client_context);
    status = stream.GetStatus();
    if (!status.OK()) {
      TRPC_FMT_ERROR("got stream error: {}", status.ToString());
      break;
    }

    for (;;) {
      ::trpc::test::helloworld::HelloRequest request;
      status = reader.Read(&request);
      if (status.OK()) {
        status = stream.Write(request);
        if (status.OK()) {
          TRPC_FMT_INFO("read request from client, then write request to upstream: {}", request.msg());
          continue;
        }
      }
      if (status.StreamEof()) {
        status = stream.WriteDone();
        if (!status.OK()) {
          break;
        }
        for (;;) {
          ::trpc::test::helloworld::HelloReply reply;
          status = stream.Read(&reply);
          if (status.OK()) {
            status = writer->Write(reply);
            if (status.OK()) {
              TRPC_FMT_INFO("read response from upstream, then write response to client: {}", reply.msg());
              continue;
            }
          }
          if (status.StreamEof()) {
            TRPC_FMT_INFO("read EOF from upstream, wait upstream to finish");
            status = stream.Finish();
          }
          break;
        }
      }
      break;
    }

    if (status.OK()) {
      TRPC_FMT_INFO("stream rpc succeed");
    } else {
      TRPC_FMT_INFO("stream rpc failed: {}", status.ToString());
    }
  } while (0);
  TRPC_FMT_INFO("final status: {}", status.ToString());
  return status;
}

}  // namespace test::helloworld
