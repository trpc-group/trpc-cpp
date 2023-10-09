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

#include <string>

#include "trpc/filter/rpc_filter.h"

#include "examples/helloworld/helloworld.pb.h"

namespace examples::filter {

// User-defined server filter base on RpcServerFilter.
class UserGreeterPbRpcServerFilter : public ::trpc::RpcServerFilter {
 public:
  int Init() override { return 0; }
  std::string Name() override { return "user_pb_rpc_server_filter"; }

 protected:
  ::trpc::Status BeforeRpcInvoke(::trpc::ServerContextPtr context, google::protobuf::Message* req_raw,
                                 google::protobuf::Message* rsp_raw) override {
    std::string func_name_prefix = "/trpc.test.helloworld.Greeter/";
    if (context->GetFuncName() == func_name_prefix + "SayHello") {
      auto req = static_cast<::trpc::test::helloworld::HelloRequest*>(req_raw);
      auto rsp = static_cast<::trpc::test::helloworld::HelloReply*>(rsp_raw);
      return DoBeforeSayHelloInvoke(context, req, rsp);
    }

    return ::trpc::kSuccStatus;
  }

  ::trpc::Status AfterRpcInvoke(::trpc::ServerContextPtr context, google::protobuf::Message* req_raw,
                                google::protobuf::Message* rsp_raw) override {
    std::string func_name_prefix = "/trpc.test.helloworld.Greeter/";
    if (context->GetFuncName() == func_name_prefix + "SayHello") {
      auto req = static_cast<::trpc::test::helloworld::HelloRequest*>(req_raw);
      auto rsp = static_cast<::trpc::test::helloworld::HelloReply*>(rsp_raw);
      return DoAfterSayHelloInvoke(context, req, rsp);
    }

    return ::trpc::kSuccStatus;
  }

 private:
  ::trpc::Status DoBeforeSayHelloInvoke(::trpc::ServerContextPtr context, ::trpc::test::helloworld::HelloRequest* req,
                                        ::trpc::test::helloworld::HelloReply* rsp) {
    req->set_msg(req->msg() + ", server_before_rpc_invoke");
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status DoAfterSayHelloInvoke(::trpc::ServerContextPtr context, ::trpc::test::helloworld::HelloRequest* req,
                                       ::trpc::test::helloworld::HelloReply* rsp) {
    rsp->set_msg(rsp->msg() + ", server_after_rpc_invoke");
    return ::trpc::kSuccStatus;
  }
};

// User-defined client filter base on RpcClientFilter.
class UserGreeterPbRpcClientFilter : public ::trpc::RpcClientFilter {
 public:
  int Init() override { return 0; }
  std::string Name() override { return "user_pb_rpc_client_filter"; }

 protected:
  ::trpc::Status BeforeRpcInvoke(::trpc::ClientContextPtr context, google::protobuf::Message* req_raw,
                                 google::protobuf::Message* rsp_raw) override {
    std::string func_name_prefix = "/trpc.test.helloworld.Greeter/";
    if (context->GetFuncName() == func_name_prefix + "SayHello") {
      auto req = static_cast<::trpc::test::helloworld::HelloRequest*>(req_raw);
      auto rsp = static_cast<::trpc::test::helloworld::HelloReply*>(rsp_raw);
      return DoBeforeSayHelloInvoke(context, req, rsp);
    }

    return ::trpc::kSuccStatus;
  }

  ::trpc::Status AfterRpcInvoke(::trpc::ClientContextPtr context, google::protobuf::Message* req_raw,
                                google::protobuf::Message* rsp_raw) override {
    std::string func_name_prefix = "/trpc.test.helloworld.Greeter/";
    if (context->GetFuncName() == func_name_prefix + "SayHello") {
      auto req = static_cast<::trpc::test::helloworld::HelloRequest*>(req_raw);
      auto rsp = static_cast<::trpc::test::helloworld::HelloReply*>(rsp_raw);
      return DoAfterSayHelloInvoke(context, req, rsp);
    }

    return ::trpc::kSuccStatus;
  }

 private:
  ::trpc::Status DoBeforeSayHelloInvoke(::trpc::ClientContextPtr context, ::trpc::test::helloworld::HelloRequest* req,
                                        ::trpc::test::helloworld::HelloReply* rsp) {
    req->set_msg(req->msg() + ", client_before_rpc_invoke");
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status DoAfterSayHelloInvoke(::trpc::ClientContextPtr context, ::trpc::test::helloworld::HelloRequest* req,
                                       ::trpc::test::helloworld::HelloReply* rsp) {
    if (context->GetStatus().OK()) {
      rsp->set_msg(rsp->msg() + ", client_after_rpc_invoke");
      return ::trpc::kSuccStatus;
    }

    return context->GetStatus();
  }
};

}  // namespace examples::filter
