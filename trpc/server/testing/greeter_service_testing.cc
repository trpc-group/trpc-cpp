//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/server/testing/greeter_service_testing.h"

#include "trpc/server/rpc_async_method_handler.h"
#include "trpc/server/rpc_method_handler.h"

namespace trpc {
namespace test {
namespace helloworld {

static const std::vector<std::vector<std::string_view>> Greeter_method_names = {
    {"/trpc.test.helloworld.Greeter/SayHello"},
};

GreeterServiceTest::GreeterServiceTest() {
  for (const std::string_view& method : Greeter_method_names[0]) {
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
        method.data(), ::trpc::MethodType::UNARY,
        new ::trpc::RpcMethodHandler<::trpc::test::helloworld::HelloRequest, ::trpc::test::helloworld::HelloReply>(
            std::bind(&GreeterServiceTest::SayHello, this, std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3))));  // NOLINT
  }
}

::trpc::Status GreeterServiceTest::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::test::helloworld::HelloRequest* req,
                                            ::trpc::test::helloworld::HelloReply* rsp) {  // NOLINT
  std::cout << "recv req msg: " << req->msg() << std::endl;

  rsp->set_msg(req->msg());

  std::cout << "send rsp msg: " << rsp->msg() << std::endl;

  return ::trpc::kSuccStatus;
}

AsyncGreeterServiceTest::AsyncGreeterServiceTest() {
  for (const std::string_view& method : Greeter_method_names[0]) {
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
        method.data(), ::trpc::MethodType::UNARY,
        new ::trpc::AsyncRpcMethodHandler<::trpc::test::helloworld::HelloRequest, ::trpc::test::helloworld::HelloReply>(
            std::bind(&AsyncGreeterServiceTest::SayHello, this, std::placeholders::_1,
                      std::placeholders::_2))));  // NOLINT
  }
}

::trpc::Future<::trpc::test::helloworld::HelloReply> AsyncGreeterServiceTest::SayHello(
    const ::trpc::ServerContextPtr& context, const ::trpc::test::helloworld::HelloRequest* req) {  // NOLINT
  ::trpc::test::helloworld::HelloReply rsp;
  rsp.set_msg(req->msg());
  return MakeReadyFuture<HelloReply>(std::move(rsp));
}

static const std::vector<std::vector<std::string_view>> StreamGreeter_method_names = {
    {"/trpc.test.helloworld.Greeter/ClientStreamSayHello"},
    {"/trpc.test.helloworld.Greeter/ServerStreamSayHello"},
    {"/trpc.test.helloworld.Greeter/BidiStreamSayHello"},
};

TestStreamGreeter::TestStreamGreeter() {
  for (const std::string_view& method : StreamGreeter_method_names[0]) {
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
        method.data(), ::trpc::MethodType::CLIENT_STREAMING,
        new ::trpc::StreamRpcMethodHandler<::trpc::test::helloworld::HelloRequest,
                                           ::trpc::test::helloworld::HelloReply>(
            std::bind(&TestStreamGreeter::ClientStreamSayHello, this, std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3))));
  }
  for (const std::string_view& method : StreamGreeter_method_names[1]) {
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
        method.data(), ::trpc::MethodType::SERVER_STREAMING,
        new ::trpc::StreamRpcMethodHandler<::trpc::test::helloworld::HelloRequest,
                                           ::trpc::test::helloworld::HelloReply>(
            std::bind(&TestStreamGreeter::ServerStreamSayHello, this, std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3))));
  }
  for (const std::string_view& method : StreamGreeter_method_names[2]) {
    AddRpcServiceMethod(
        new ::trpc::RpcServiceMethod(method.data(), ::trpc::MethodType::BIDI_STREAMING,
                                     new ::trpc::StreamRpcMethodHandler<::trpc::test::helloworld::HelloRequest,
                                                                        ::trpc::test::helloworld::HelloReply>(
                                         std::bind(&TestStreamGreeter::BidiStreamSayHello, this, std::placeholders::_1,
                                                   std::placeholders::_2, std::placeholders::_3))));
  }
}

::trpc::Status TestStreamGreeter::ClientStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
    ::trpc::test::helloworld::HelloReply* reply) {  // NOLINT

  ::trpc::Status status{};
  int count{0};
  for (;;) {
    ::trpc::test::helloworld::HelloRequest request{};
    status = reader.Read(&request);
    if (status.OK()) {
      count++;
      continue;
    }
    break;
  }
  reply->set_msg(std::to_string(count));
  return ::trpc::Status(0, "OK, ClientStreamSayHello");
}

::trpc::Status TestStreamGreeter::ServerStreamSayHello(
    const ::trpc::ServerContextPtr& context, const ::trpc::test::helloworld::HelloRequest& request,
    ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {  // NOLINT

  for (int i = 10; i > 0; i--) {
    ::trpc::test::helloworld::HelloReply reply;
    reply.set_msg(request.msg());
    writer->Write(reply);
  }
  return ::trpc::Status(0, "OK, ServerStreamSayHello");
}

::trpc::Status TestStreamGreeter::BidiStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
    ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {  // NOLINT

  ::trpc::Status status{};
  for (;;) {
    ::trpc::test::helloworld::HelloRequest request{};
    status = reader.Read(&request);
    if (status.OK()) {
      ::trpc::test::helloworld::HelloReply reply;
      reply.set_msg(request.msg());
      writer->Write(reply);
      continue;
    }
    break;
  }
  return ::trpc::Status(0, "OK, BidiStreamSayHello");
}

}  // end namespace helloworld
}  // end namespace test
}  // end namespace trpc
