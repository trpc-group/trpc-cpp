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

#include "test/end2end/unary/http/http_rpc_server.h"

#include "test/end2end/common/util.h"
#include "test/end2end/unary/http/http_rpc.trpc.pb.h"

namespace trpc::testing {

class HttpRpcServiceImpl : public trpc::testing::httpsvr::HttpRpcService {
  ::trpc::Status TestPb(::trpc::ServerContextPtr context, const ::trpc::testing::httpsvr::TestRequest* request,
                        ::trpc::testing::httpsvr::TestReply* response) {
    if (request->msg() != "test-pb-req") {
      response->set_msg("failed");
    } else {
      response->set_msg("test-pb-rsp");
    }
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status TestPbEmptyReq(::trpc::ServerContextPtr context, const ::trpc::testing::httpsvr::TestRequest* request,
                                ::trpc::testing::httpsvr::TestReply* response) {
    if (!request->msg().empty()) {
      response->set_msg("failed");
    } else {
      response->set_msg("test-pb-rsp");
    }
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status TestPbEmptyRsp(::trpc::ServerContextPtr context, const ::trpc::testing::httpsvr::TestRequest* request,
                                ::trpc::testing::httpsvr::TestReply* response) {
    if (request->msg() != "test-pb-req") {
      response->set_msg("failed");
    }
    return ::trpc::kSuccStatus;
  }
  ::trpc::Status TestPbRetErr(::trpc::ServerContextPtr context, const ::trpc::testing::httpsvr::TestRequest* request,
                              ::trpc::testing::httpsvr::TestReply* response) {
    response->set_msg("test-pb-rsp");
    return ::trpc::Status(12345, "custom err");
  }
};

int HttpRpcServer::Initialize() {
  RegisterService("http_rpc_service", std::make_shared<HttpRpcServiceImpl>());

  test_signal_->SignalClientToContinue();

  return 0;
}

void HttpRpcServer::Destroy() { GcovFlush(); }

}  // namespace trpc::testing
