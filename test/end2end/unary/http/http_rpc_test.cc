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

#include "gtest/gtest.h"

#include "test/end2end/common/subprocess.h"
#include "test/end2end/common/util.h"
#include "test/end2end/unary/http/http_rpc.trpc.pb.h"
#include "test/end2end/unary/http/http_rpc_server.h"
#include "trpc/client/http/http_service_proxy.h"
#include "trpc/future/future_utility.h"

int test_argc;
char** client_argv;
char** server_argv;

namespace trpc::testing {

class HttpRpcTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    // Run before the first test case.
    test_signal_ = std::make_unique<TestSignaller>();
    server_process_ = std::make_unique<SubProcess>([]() {
      HttpRpcServer server(test_signal_.get());
      server.Main(test_argc, server_argv);
      server.Wait();
    });
    // Wait for the server to start.
    test_signal_->ClientWaitToContinue();

    InitializeRuntimeEnv(test_argc, client_argv);
    trpc::log::Init();
    http_proxy_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>("http_client");
    rpc_proxy_ =
        ::trpc::GetTrpcClient()->GetProxy<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy>("http_rpc_client");
  }

  static void TearDownTestSuite() {
    // After the last test case.
    // Destroy the client environment.
    DestroyRuntimeEnv();
    // Kill the server.
    server_process_.reset();
  }

  void SetUp() override {}

  void TearDown() override {}

 protected:
  static std::unique_ptr<TestSignaller> test_signal_;
  static std::unique_ptr<SubProcess> server_process_;

 public:
  static std::shared_ptr<::trpc::http::HttpServiceProxy> http_proxy_;
  static std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> rpc_proxy_;
};

std::unique_ptr<TestSignaller> HttpRpcTest::test_signal_;
std::unique_ptr<SubProcess> HttpRpcTest::server_process_;
std::shared_ptr<::trpc::http::HttpServiceProxy> HttpRpcTest::http_proxy_;
std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> HttpRpcTest::rpc_proxy_;

using TestRequest = trpc::testing::httpsvr::TestRequest;
using TestReply = trpc::testing::httpsvr::TestReply;

using TestFunc = std::function<bool(std::string, TestRequest&, TestReply&, int&)>;

// Testing case:
// Filling the request and response.
// The server expects to receive the expected request message.
// The client expects the call to be successful and to receive the expected response message.
void TestInvoke(TestFunc func) {
  TestRequest req;
  TestReply rsp;
  req.set_msg("test-pb-req");
  int ret_code = 0;
  ASSERT_TRUE(func("/TestPb", req, rsp, ret_code));
  ASSERT_EQ(rsp.msg(), "test-pb-rsp");
}

// Testing case:
// Accessing a non-existent URL.
// The client expects the access to fail and the return code to be an RPC error code.
void TestNonExistUrl(TestFunc func) {
  TestRequest req;
  TestReply rsp;
  int ret_code = 0;
  ASSERT_FALSE(func("/test-non-exist", req, rsp, ret_code));
  ASSERT_EQ(ret_code, ::trpc::codec::GetDefaultServerRetCode(::trpc::codec::ServerRetCode::NOT_FUN_ERROR));
}

// Testing case:
// Filling the request body but not the response body.
// The server expects the request body to be non-empty.
// The client expects the response body to be empty.
void TestEmptyRsp(TestFunc func) {
  TestRequest req;
  TestReply rsp;
  req.set_msg("test-pb-req");
  int ret_code = 0;
  ASSERT_TRUE(func("/TestPbEmptyRsp", req, rsp, ret_code));
  ASSERT_TRUE(rsp.msg().empty());
}

// Testing case:
// Not filling the request body but filling the response body.
// The server expects the request body to be empty.
// The client expects the response body to be non-empty.
void TestEmptyReq(TestFunc func) {
  TestRequest req;
  TestReply rsp;
  int ret_code = 0;
  ASSERT_TRUE(func("/TestPbEmptyReq", req, rsp, ret_code));
  ASSERT_EQ(rsp.msg(), "test-pb-rsp");
}

// Testing case:
// The server returns an error.
// The client expects the call to fail, with error code 12345, and an empty response body.
void TestRetErr(TestFunc func) {
  TestRequest req;
  TestReply rsp;
  int ret_code = 0;
  ASSERT_FALSE(func("/TestPbRetErr", req, rsp, ret_code));
  ASSERT_EQ(ret_code, 12345);
}

bool DoHttpPbInvoke(std::string url, TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = trpc::MakeClientContext(HttpRpcTest::http_proxy_);
  url = "http://test.com/trpc.testing.httpsvr.HttpRpcService" + url;
  ::trpc::Status status = HttpRpcTest::http_proxy_->PBInvoke<TestRequest, TestReply>(ctx, url, req, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  std::cerr << "DoHttpPbInvoke, status: " << status.ToString() << std::endl;
  return status.OK();
}

bool DoHttpAsyncPbInvoke(std::string url, TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpRpcTest::http_proxy_);
  url = "http://test.com/trpc.testing.httpsvr.HttpRpcService" + url;
  bool succ = false;
  ::trpc::Future<> fut = HttpRpcTest::http_proxy_->AsyncPBInvoke<TestRequest, TestReply>(ctx, url, req)
                             .Then([&](::trpc::Future<TestReply>&& response) {
                               succ = !response.IsFailed();
                               if (succ) {
                                 rsp = response.GetValue0();
                               } else {
                                 ret_code = response.GetException().GetExceptionCode();
                               }
                               return ::trpc::MakeReadyFuture<>();
                             });
  future::BlockingGet(std::move(fut));
  return succ;
}

// Testing case:
// PbInvoke, filling the request and response.
// The server expects to receive the expected request message.
// The client expects the call to be successful and to receive the expected response message.
TEST_F(HttpRpcTest, TestHttpPbInvoke) {
  RunByEnv([&]() { TestInvoke(DoHttpPbInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncPbInvoke) {
  RunByEnv([&]() { TestInvoke(DoHttpAsyncPbInvoke); });
}

// Testing case:
// PbInvoke, accessing a non-existent URL.
// The client expects the access to fail and the return code to be an RPC error code.
TEST_F(HttpRpcTest, TestHttpPbInvokeNonExistUrl) {
  RunByEnv([&]() { TestNonExistUrl(DoHttpPbInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncPbInvokeNonExistUrl) {
  RunByEnv([&]() { TestNonExistUrl(DoHttpAsyncPbInvoke); });
}

// Testing case:
// PbInvoke, filling the request body but not the response body.
// The server expects the request body to be non-empty.
// The client expects the response body to be empty.
TEST_F(HttpRpcTest, TestHttpPbInvokeEmptyRsp) {
  RunByEnv([&]() { TestEmptyRsp(DoHttpPbInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncPbInvokeEmptyRsp) {
  RunByEnv([&]() { TestEmptyRsp(DoHttpAsyncPbInvoke); });
}

// Testing case:
// PbInvoke, not filling the request body but filling the response body.
// The server expects the request body to be empty.
// The client expects the response body to be non-empty.
TEST_F(HttpRpcTest, TestHttpPbInvokeEmptyReq) {
  RunByEnv([&]() { TestEmptyReq(DoHttpPbInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncPbInvokeEmptyReq) {
  RunByEnv([&]() { TestEmptyReq(DoHttpAsyncPbInvoke); });
}

// Testing case:
// PbInvoke, the server returns an error.
// The client expects the call to fail, with error code 12345, and an empty response body.
TEST_F(HttpRpcTest, TestHttpPbInvokeRetErr) {
  RunByEnv([&]() { TestRetErr(DoHttpPbInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncPbInvokeRetErr) {
  RunByEnv([&]() { TestRetErr(DoHttpAsyncPbInvoke); });
}

bool DoHttpUnaryInovke(std::string func_name, TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = trpc::MakeClientContext(HttpRpcTest::http_proxy_);
  func_name = "/trpc.testing.httpsvr.HttpRpcService" + func_name;
  ctx->SetFuncName(func_name);
  ::trpc::Status status = HttpRpcTest::http_proxy_->UnaryInvoke<TestRequest, TestReply>(ctx, req, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoHttpAsyncUnaryInovke(std::string func_name, TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpRpcTest::http_proxy_);
  func_name = "/trpc.testing.httpsvr.HttpRpcService" + func_name;
  ctx->SetFuncName(func_name);
  bool succ = false;
  ::trpc::Future<> fut = HttpRpcTest::http_proxy_->AsyncUnaryInvoke<TestRequest, TestReply>(ctx, req).Then(
      [&](::trpc::Future<TestReply>&& response) {
        succ = !response.IsFailed();
        if (succ) {
          rsp = response.GetValue0();
        } else {
          ret_code = response.GetException().GetExceptionCode();
        }
        return ::trpc::MakeReadyFuture<>();
      });
  future::BlockingGet(std::move(fut));
  return succ;
}

// Testing case:
// UnaryInvoke, filling the request and response.
// The server expects to receive the expected request message.
// The client expects the call to be successful and to receive the expected response message.
TEST_F(HttpRpcTest, TestHttpUnaryInvoke) {
  RunByEnv([&]() { TestInvoke(DoHttpUnaryInovke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncUnaryInvoke) {
  RunByEnv([&]() { TestInvoke(DoHttpAsyncUnaryInovke); });
}

// Testing case:
// UnaryInvoke, accessing a non-existent URL.
// The client expects the access to fail and the return code to be an RPC error code.
TEST_F(HttpRpcTest, TestHttpUnaryInvokeNonExistUrl) {
  RunByEnv([&]() { TestNonExistUrl(DoHttpUnaryInovke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncUnaryInvokeNonExistUrl) {
  RunByEnv([&]() { TestNonExistUrl(DoHttpAsyncPbInvoke); });
}

// Testing case:
// During the UnaryInvoke request, populate the request body while leaving the response body empty.
// The server expects a non-empty request body.
// The client expects an empty response body.
TEST_F(HttpRpcTest, TestHttpUnaryInvokeEmptyRsp) {
  RunByEnv([&]() { TestEmptyRsp(DoHttpUnaryInovke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncUnaryInvokeEmptyRsp) {
  RunByEnv([&]() { TestEmptyRsp(DoHttpAsyncUnaryInovke); });
}

// Testing case:
// In the context of the UnaryInvoke request, refrain from populating the request body and instead populate the response
// body.
// The server expects an empty request body.
// The client expects a non-empty response body.
TEST_F(HttpRpcTest, TestHttpUnaryInvokeEmptyReq) {
  RunByEnv([&]() { TestEmptyReq(DoHttpUnaryInovke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncUnaryInvokeEmptyReq) {
  RunByEnv([&]() { TestEmptyReq(DoHttpAsyncUnaryInovke); });
}

// Testing case:
// In the scenario of the UnaryInvoke request, if the server returns an error, the client expects the invocation to fail
// with an error code of 12345 and an empty response body.
TEST_F(HttpRpcTest, TestHttpUnaryInovkeRetErr) {
  RunByEnv([&]() { TestRetErr(DoHttpUnaryInovke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncUnaryInovkeRetErr) {
  RunByEnv([&]() { TestRetErr(DoHttpAsyncUnaryInovke); });
}

bool DoHttpRpcInvoke(std::string func_name, TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = trpc::MakeClientContext(HttpRpcTest::rpc_proxy_);
  ::trpc::Status status;
  if (func_name == "/TestPb") {
    status = HttpRpcTest::rpc_proxy_->TestPb(ctx, req, &rsp);
  } else if (func_name == "/TestPbEmptyReq") {
    status = HttpRpcTest::rpc_proxy_->TestPbEmptyReq(ctx, req, &rsp);
  } else if (func_name == "/TestPbEmptyRsp") {
    status = HttpRpcTest::rpc_proxy_->TestPbEmptyRsp(ctx, req, &rsp);
  } else if (func_name == "/TestPbRetErr") {
    status = HttpRpcTest::rpc_proxy_->TestPbRetErr(ctx, req, &rsp);
  } else if (func_name == "/test-pb") {  // alias testing
    ctx->SetFuncName(func_name);
    status = HttpRpcTest::rpc_proxy_->TestPb(ctx, req, &rsp);
  } else {
    status = ::trpc::kUnknownErrorStatus;
  }

  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoHttpAsyncRpcInvoke(std::string func_name, TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpRpcTest::rpc_proxy_);
  bool succ = false;
  ::trpc::Future<TestReply> fut_reply;
  if (func_name == "/TestPb") {
    fut_reply = HttpRpcTest::rpc_proxy_->AsyncTestPb(ctx, req);
  } else if (func_name == "/TestPbEmptyReq") {
    fut_reply = HttpRpcTest::rpc_proxy_->AsyncTestPbEmptyReq(ctx, req);
  } else if (func_name == "/TestPbEmptyRsp") {
    fut_reply = HttpRpcTest::rpc_proxy_->AsyncTestPbEmptyRsp(ctx, req);
  } else if (func_name == "/TestPbRetErr") {
    fut_reply = HttpRpcTest::rpc_proxy_->AsyncTestPbRetErr(ctx, req);
  } else if (func_name == "/test-pb") {  // alias testing
    ctx->SetFuncName(func_name);
    fut_reply = HttpRpcTest::rpc_proxy_->AsyncTestPb(ctx, req);
  } else {
    fut_reply = MakeExceptionFuture<TestReply>(CommonException("unmatched funcname"));
  }

  Future<> fut = fut_reply.Then([&](::trpc::Future<TestReply>&& response) {
    succ = !response.IsFailed();
    if (succ) {
      rsp = response.GetValue0();
    } else {
      ret_code = response.GetException().GetExceptionCode();
    }
    return ::trpc::MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(fut));
  return succ;
}

// Testing case:
// RpcInvoke, populate both the request and response.
// The server expects to receive the expected request message.
// The client expects a successful invocation and to receive the expected response message.
TEST_F(HttpRpcTest, TestHttpRpcInvoke) {
  RunByEnv([&]() { TestInvoke(DoHttpRpcInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncRpcInvoke) {
  RunByEnv([&]() { TestInvoke(DoHttpAsyncRpcInvoke); });
}

// Testing case:
// RpcInvoke, accessing a non-existent URL. (This has already been tested in the previous tests.)
// The client expects the access to fail and the return code to be an RPC error code.

// Testing case:
// RpcInvoke, filling the request body but not the response body.
// The server expects the request body to be non-empty.
// The client expects the response body to be empty.
TEST_F(HttpRpcTest, TestHttpRpcInvokeEmptyRsp) {
  RunByEnv([&]() { TestEmptyRsp(DoHttpRpcInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncRpcInvokeEmptyRsp) {
  RunByEnv([&]() { TestEmptyRsp(DoHttpAsyncRpcInvoke); });
}

// Testing case:
// RpcInvoke request, not filling the request body but filling the response body.
// The server expects the request body to be empty.
// The client expects the response body to be non-empty.
TEST_F(HttpRpcTest, TestHttpRpcInvokeEmptyReq) {
  RunByEnv([&]() { TestEmptyReq(DoHttpRpcInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncRpcInvokeEmptyReq) {
  RunByEnv([&]() { TestEmptyReq(DoHttpAsyncRpcInvoke); });
}

// Testing case:
// RpcInvoke, the server returns an error.
// The client expects the call to fail, with error code 12345, and an empty response body.
TEST_F(HttpRpcTest, TestHttpRpcInvokeRetErr) {
  RunByEnv([&]() { TestRetErr(DoHttpRpcInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncRpcInvokeRetErr) {
  RunByEnv([&]() { TestRetErr(DoHttpAsyncRpcInvoke); });
}

// Testing case:
// RpcInvoke, accessing through an alias.
// The client expects the call to be successful and the result to be consistent with calling through TestPb.
void TestAlias(TestFunc func) {
  TestRequest req;
  TestReply rsp;
  req.set_msg("test-pb-req");
  int ret_code = 0;
  ASSERT_TRUE(func("/test-pb", req, rsp, ret_code));
  ASSERT_EQ(rsp.msg(), "test-pb-rsp");
}
TEST_F(HttpRpcTest, TestHttpRpcInvokeAlias) {
  RunByEnv([&]() { TestAlias(DoHttpRpcInvoke); });
}
TEST_F(HttpRpcTest, TestHttpAsyncRpcInvokeAlias) {
  RunByEnv([&]() { TestAlias(DoHttpAsyncRpcInvoke); });
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  if (!::trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server_argv)) {
    exit(EXIT_FAILURE);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
