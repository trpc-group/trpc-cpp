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

#include <memory>
#include <string>

#include "fmt/format.h"

#include "trpc/common/trpc_app.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/runtime/threadmodel/common/worker_thread.h"
#include "trpc/util/algorithm/random.h"
#include "trpc/util/buffer/zero_copy_stream.h"

#include "examples/helloworld/helloworld.trpc.pb.h"

namespace examples::attachment {

namespace {

uint32_t DispatchRequest(const ::trpc::STransportReqMsg* req) {
  // request can be dispatched to specific thread by various strategies

  // eg: by ip
  /*::trpc::ServerContextPtr context = req->context;
  std::string ip = context->GetIp();
  uint32_t uin = std::hash<std::string>{}(ip);

  return uin;*/

  // eg: get information from protocol header
  ::trpc::ServerContextPtr context = req->context;
  const ::trpc::TransInfoMap& trans_info = context->GetPbReqTransInfo();

  uint32_t uin = ::trpc::Random<uint32_t>();
  auto found = trans_info.find("uin");
  if (found != trans_info.end()) {
    uin = std::stoll(found->second);
  }

  return uin;

  // eg: if request is trpc protocol and get information from protocol body(other protocol can also be like this)
  /*::trpc::ServerContextPtr context = req->context;
  ::trpc::ProtocolPtr& protocol = context->GetRequestMsg();
  ::trpc::TrpcRequestProtocol* trpc_protocol = static_cast<::trpc::TrpcRequestProtocol*>(protocol.get());
  // get pb body buffer
  ::trpc::NoncontiguousBuffer protocol_body = trpc_protocol->GetNonContiguousProtocolBody();
  // `GetNonContiguousProtocolBody` will move body buffer,
  // we need to copy body buffer to trpc_protocol body, otherwise framework decode operation will fails
  trpc_protocol->SetNonContiguousProtocolBody(trpc::NoncontiguousBuffer(protocol_body));

  uint32_t uin = ::trpc::Random<uint32_t>();

  // deserialize request body
  ::trpc::test::helloworld::HelloRequest hello_request;
  ::trpc::NoncontiguousBufferInputStream nbis(&protocol_body);
  if (!hello_request.ParseFromZeroCopyStream(&nbis)) {
    return uin;
  }
  nbis.Flush();

  uin = std::hash<std::string>{}(hello_request.msg());

  return uin;*/
}

}  // namespace

class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
 public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) override {
    // Your can access more information from rpc context, eg: remote ip and port
    TRPC_FMT_INFO("remote address: {}:{}", context->GetIp(), context->GetPort());
    TRPC_FMT_INFO("request message: {}", request->msg());

    TRPC_FMT_INFO("thread logic id: {}, unique id: {}",
       ::trpc::WorkerThread::GetCurrentWorkerThread()->Id(),
       ::trpc::WorkerThread::GetCurrentWorkerThread()->GetUniqueId());

    reply->set_msg(request->msg());

    return ::trpc::kSuccStatus;
  }
};

class DemoServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
    // Set the service name, which must be the same as the value of the `/server/service/name` configuration item
    // in the configuration file, otherwise the framework cannot receive requests normally.
    std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "Greeter");
    TRPC_FMT_INFO("service name:{}", service_name);

    trpc::ServicePtr service(std::make_shared<GreeterServiceImpl>());

    service->SetHandleRequestDispatcherFunction(DispatchRequest);

    RegisterService(service_name, service);

    return 0;
  }

  void Destroy() override {}
};

}  // namespace examples::attachment

int main(int argc, char** argv) {
  examples::attachment::DemoServer demo_server;

  demo_server.Main(argc, argv);
  demo_server.Wait();

  return 0;
}
