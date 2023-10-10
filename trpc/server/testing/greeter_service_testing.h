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

#include "trpc/proto/testing/helloworld.pb.h"

#include "trpc/server/rpc/rpc_service_impl.h"
#include "trpc/server/rpc/stream_rpc_method_handler.h"

namespace trpc {
namespace test {
namespace helloworld {

class GreeterServiceTest : public ::trpc::RpcServiceImpl {
 public:
  GreeterServiceTest();

  ::trpc::Status SayHello(::trpc::ServerContextPtr context, const ::trpc::test::helloworld::HelloRequest* req,
                          ::trpc::test::helloworld::HelloReply* rsp);  // NOLINT
};

class AsyncGreeterServiceTest : public ::trpc::RpcServiceImpl {
 public:
  AsyncGreeterServiceTest();

  ::trpc::Future<::trpc::test::helloworld::HelloReply> SayHello(
      const ::trpc::ServerContextPtr& context, const ::trpc::test::helloworld::HelloRequest* req);  // NOLINT
};

class TestStreamGreeter : public ::trpc::RpcServiceImpl {
 public:
  TestStreamGreeter();

  ::trpc::Status ClientStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
      ::trpc::test::helloworld::HelloReply* reply);
  ::trpc::Status ServerStreamSayHello(const ::trpc::ServerContextPtr& context,
                                      const ::trpc::test::helloworld::HelloRequest& request,
                                      ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer);
  ::trpc::Status BidiStreamSayHello(const ::trpc::ServerContextPtr& context,
                                    const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
                                    ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer);
};

}  // end namespace helloworld
}  // end namespace test
}  // end namespace trpc
