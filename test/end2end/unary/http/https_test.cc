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

#include "gtest/gtest.h"

#include "test/end2end/common/subprocess.h"
#include "test/end2end/common/test_common.trpc.pb.h"
#include "test/end2end/common/util.h"
#include "test/end2end/unary/http/http_rpc.trpc.pb.h"
#include "test/end2end/unary/http/https_server.h"
#include "trpc/client/http/http_service_proxy.h"
#include "trpc/future/future_utility.h"

int test_argc;
char** client_argv;
char** server_argv;

namespace trpc::testing {

class HttpsTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    // Run before the first test case.
    test_signal_ = std::make_unique<TestSignaller>();
    server_process_ = std::make_unique<SubProcess>([]() {
      HttpsServer server(test_signal_.get());
      server.Main(test_argc, server_argv);
      server.Wait();
    });
    // Wait for the server to start.
    test_signal_->ClientWaitToContinue();

    InitializeRuntimeEnv(test_argc, client_argv);
    http_proxy_no_ca_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>("http_client_no_ca");
    http_proxy_ca1_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>("http_client_ca1");
    http_proxy_ca2_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>("http_client_ca2");
    rpc_proxy_no_ca_ =
        ::trpc::GetTrpcClient()->GetProxy<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy>("rpc_client_no_ca");
    rpc_proxy_ca1_ =
        ::trpc::GetTrpcClient()->GetProxy<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy>("rpc_client_ca1");
    rpc_proxy_ca2_ =
        ::trpc::GetTrpcClient()->GetProxy<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy>("rpc_client_ca2");
    test_common_proxy_ =
        ::trpc::GetTrpcClient()->GetProxy<::trpc::testing::common::TestCommonServiceProxy>("test_common_service");
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
  static std::shared_ptr<::trpc::http::HttpServiceProxy> http_proxy_no_ca_;
  static std::shared_ptr<::trpc::http::HttpServiceProxy> http_proxy_ca1_;
  static std::shared_ptr<::trpc::http::HttpServiceProxy> http_proxy_ca2_;
  static std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> rpc_proxy_no_ca_;
  static std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> rpc_proxy_ca1_;
  static std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> rpc_proxy_ca2_;
  static std::shared_ptr<::trpc::testing::common::TestCommonServiceProxy> test_common_proxy_;
};

std::unique_ptr<TestSignaller> HttpsTest::test_signal_;
std::unique_ptr<SubProcess> HttpsTest::server_process_;
std::shared_ptr<::trpc::http::HttpServiceProxy> HttpsTest::http_proxy_no_ca_;
std::shared_ptr<::trpc::http::HttpServiceProxy> HttpsTest::http_proxy_ca1_;
std::shared_ptr<::trpc::http::HttpServiceProxy> HttpsTest::http_proxy_ca2_;
std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> HttpsTest::rpc_proxy_no_ca_;
std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> HttpsTest::rpc_proxy_ca1_;
std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> HttpsTest::rpc_proxy_ca2_;
std::shared_ptr<::trpc::testing::common::TestCommonServiceProxy> HttpsTest::test_common_proxy_;

using TestRequest = ::trpc::testing::httpsvr::TestRequest;
using TestReply = ::trpc::testing::httpsvr::TestReply;
using ServiceInfo = ::trpc::testing::common::ServiceInfo;
using CommonRet = ::trpc::testing::common::CommonRet;

// Dynamic startup service refers to the ability to start a temporary server during testing by setting parameters in the
// YAML configuration files of both the client and server. This can help save ports during testing.
bool StartService(std::string service_name) {
  // Dynamic stopping of the service has issues in fiber and separate modes, and will be implemented after further
  // investigation.
  return true;
  // ServiceInfo info;
  // info.set_service_name(service_name);
  // CommonRet ret;
  // ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpsTest::test_common_proxy_);
  // ::trpc::Status status = HttpsTest::test_common_proxy_->StartService(ctx, info, &ret);
  // return status.OK() && ret.status() == "succ" ? true : false;
}
bool StopService(std::string service_name) {
  // Dynamic stopping of the service has issues in fiber and separate modes, and will be implemented after further
  // investigation.
  return true;
  // ServiceInfo info;
  // info.set_service_name(service_name);
  // CommonRet ret;
  // ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpsTest::test_common_proxy_);
  // ::trpc::Status status = HttpsTest::test_common_proxy_->StopService(ctx, info, &ret);
  // return status.OK() && ret.status() == "succ" ? true : false;
}

::trpc::ClientContextPtr MakeClientContextWithIpPort(std::shared_ptr<::trpc::ServiceProxy> proxy, std::string target) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
  int dot_pos = target.find(":");
  std::string ip = target.substr(0, dot_pos);
  int port = std::stoi(target.substr(dot_pos + 1));
  ctx->SetAddr(ip, port);
  return ctx;
}

bool StrHasBoth(std::string str, std::string key1, std::string key2) {
  return str.find(key1) != std::string::npos && str.find(key2) != std::string::npos;
}

std::shared_ptr<::trpc::ServiceProxy> GetProxyByCA(std::string ca) {
  if (StrHasBoth(ca, "http", "no_ca")) {
    return HttpsTest::http_proxy_no_ca_;
  } else if (StrHasBoth(ca, "http", "ca1")) {
    return HttpsTest::http_proxy_ca1_;
  } else if (StrHasBoth(ca, "http", "ca2")) {
    return HttpsTest::http_proxy_ca2_;
  } else if (StrHasBoth(ca, "rpc", "no_ca")) {
    return HttpsTest::rpc_proxy_no_ca_;
  } else if (StrHasBoth(ca, "rpc", "ca1")) {
    return HttpsTest::rpc_proxy_ca1_;
  } else if (StrHasBoth(ca, "rpc", "ca2")) {
    return HttpsTest::rpc_proxy_ca2_;
  }
  return nullptr;
}

std::string GetServerAddr(std::string server_ca) {
  std::shared_ptr<::trpc::ServiceProxy> proxy = GetProxyByCA(server_ca);
  if (proxy == nullptr) {
    return "";
  }
  return proxy->GetServiceProxyOption()->target;
}

using HttpProxyTestFunc = std::function<bool(std::shared_ptr<::trpc::http::HttpServiceProxy> proxy, std::string target,
                                             bool is_async, int& ret_code)>;
using RpcProxyTestFunc = std::function<bool(std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> proxy,
                                            std::string target, bool is_async, int& ret_code)>;
// Either test PB or test HTTP.
void TestWithCAMatch(std::string client_ca, std::string server_ca, bool is_async, HttpProxyTestFunc http_proxy_func,
                     RpcProxyTestFunc rpc_proxy_func) {
  // During testing, either http_proxy_func or rpc_proxy_func must be non-empty. This check is not performed here, but
  // it must be ensured by the external caller. Select the corresponding client proxy and the corresponding server
  // address through client_ca.
  std::shared_ptr<::trpc::ServiceProxy> proxy = GetProxyByCA(client_ca);
  ASSERT_TRUE(proxy != nullptr);
  std::string svr_addr = GetServerAddr(server_ca);
  ASSERT_FALSE(svr_addr.empty());
  std::cout << "server_ca: " << server_ca << ", svr_addr: " << svr_addr << std::endl;
  ASSERT_TRUE(StartService(server_ca));
  int ret_code = 0;
  if (http_proxy_func != nullptr) {
    ASSERT_TRUE(http_proxy_func(std::dynamic_pointer_cast<::trpc::http::HttpServiceProxy>(proxy), svr_addr, is_async,
                                ret_code));
  } else if (rpc_proxy_func != nullptr) {
    ASSERT_TRUE(rpc_proxy_func(std::dynamic_pointer_cast<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy>(proxy),
                               svr_addr, is_async, ret_code));
  }
  ASSERT_TRUE(StopService(server_ca));
}

void TestWithCANotMatch(std::string client_ca, std::string server_ca, bool is_async, HttpProxyTestFunc http_proxy_func,
                        RpcProxyTestFunc rpc_proxy_func) {
  // During testing, either http_proxy_func or rpc_proxy_func must be non-empty. This check is not performed here, but
  // it must be ensured by the external caller. Select the corresponding client proxy and the corresponding server
  // address through client_ca.
  std::shared_ptr<::trpc::ServiceProxy> proxy = GetProxyByCA(client_ca);
  std::string svr_addr = GetServerAddr(server_ca);
  ASSERT_FALSE(svr_addr.empty());
  std::cout << "server_ca: " << server_ca << ", svr_addr: " << svr_addr << std::endl;
  ASSERT_TRUE(StartService(server_ca));
  int ret_code = 0;
  if (http_proxy_func != nullptr) {
    ASSERT_FALSE(http_proxy_func(std::dynamic_pointer_cast<::trpc::http::HttpServiceProxy>(proxy), svr_addr, is_async,
                                 ret_code));
  } else if (rpc_proxy_func != nullptr) {
    ASSERT_FALSE(rpc_proxy_func(std::dynamic_pointer_cast<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy>(proxy),
                                svr_addr, is_async, ret_code));
  }
  ASSERT_EQ(ret_code, 141);
  ASSERT_TRUE(StopService(server_ca));
}

bool DoPostString(std::shared_ptr<::trpc::http::HttpServiceProxy> proxy, std::string target, std::string url,
                  std::string& req, std::string& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = MakeClientContextWithIpPort(proxy, target);
  url = "http://test.com" + url;
  ::trpc::Status status = proxy->Post(ctx, url, req, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncPostString(std::shared_ptr<::trpc::http::HttpServiceProxy> proxy, std::string target, std::string url,
                       std::string& req, std::string& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = MakeClientContextWithIpPort(proxy, target);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<> fut = proxy->AsyncPost(ctx, url, req).Then([&](::trpc::Future<std::string>&& response) {
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

bool TestPostStringWithCA(std::shared_ptr<::trpc::http::HttpServiceProxy> proxy, std::string svr_addr, bool is_async,
                          int& ret_code) {
  std::string req, rsp;
  req = "test-string-req";
  bool succ;
  if (!is_async) {
    succ = DoPostString(proxy, svr_addr, "/test-post-string", req, rsp, ret_code);
  } else {
    succ = DoAsyncPostString(proxy, svr_addr, "/test-post-string", req, rsp, ret_code);
  }
  if (!succ) {
    std::cerr << "PostString failed" << std::endl;
    return false;
  }
  if (rsp != "test-string-rsp") {
    std::cerr << "rsp check failed, expect: test-string-rsp, actual: " << rsp << std::endl;
    return false;
  }
  return succ;
}

// Testing case:
// PostString, filling the request and response text.
// Both the client and server are configured with certificate c1.
// The server expects the request text to match expectations.
// The client expects successful access and the response text to match expectations.
TEST_F(HttpsTest, TestPostStringClientCA1ServerCA1) {
  RunByEnv([&]() { TestWithCAMatch("http_client_ca1", "http_service_ca1", false, TestPostStringWithCA, nullptr); });
}
TEST_F(HttpsTest, TestAsyncPostStringClientCA1ServerCA1) {
  RunByEnv([&]() { TestWithCAMatch("http_client_ca1", "http_service_ca1", true, TestPostStringWithCA, nullptr); });
}

// Testing case:
// PostString, filling the request and response text.
// The client is configured with certificate c1, but the server is not configured with a certificate.
// The client expects the call to fail, with error code 141.
TEST_F(HttpsTest, TestPostStringClientCA1ServerNOCA) {
  RunByEnv(
      [&]() { TestWithCANotMatch("http_client_ca1", "http_service_no_ca", false, TestPostStringWithCA, nullptr); });
}
TEST_F(HttpsTest, TestAsyncPostStringCA1ServerNOCA) {
  RunByEnv([&]() { TestWithCANotMatch("http_client_ca1", "http_service_no_ca", true, TestPostStringWithCA, nullptr); });
}

// Testing case:
// PostString, filling the request and response text.
// The client is not configured with a certificate, but the server is configured with certificate ca1.
// The client expects the call to fail, with error code 141.
TEST_F(HttpsTest, TestPostStringClientNOCAServerCA1) {
  RunByEnv(
      [&]() { TestWithCANotMatch("http_client_no_ca", "http_service_ca1", false, TestPostStringWithCA, nullptr); });
}
TEST_F(HttpsTest, TestAsyncPostStringClientNOCAServerCA1) {
  RunByEnv([&]() { TestWithCANotMatch("http_client_no_ca", "http_service_ca1", true, TestPostStringWithCA, nullptr); });
}

// Testing case:
// PostString, filling the request and response text.
// The client is configured with certificate c1, but the server is configured with certificate ca2.
// The client expects the call to fail, with error code 141.
TEST_F(HttpsTest, TestPostStringClientCA1ServerCA2) {
  RunByEnv(
      [&]() { TestWithCANotMatch("http_client_no_ca1", "http_service_ca2", false, TestPostStringWithCA, nullptr); });
}
TEST_F(HttpsTest, TestAsyncPostStringClientCA1ServerCA2) {
  RunByEnv(
      [&]() { TestWithCANotMatch("http_client_no_ca1", "http_service_ca2", true, TestPostStringWithCA, nullptr); });
}

bool DoHttpUnaryInvoke(std::shared_ptr<::trpc::http::HttpServiceProxy> proxy, std::string target, std::string func_name,
                       TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = MakeClientContextWithIpPort(proxy, target);
  func_name = "/trpc.testing.httpsvr.HttpRpcService" + func_name;
  ctx->SetFuncName(func_name);
  std::cout << "FuncName: " << ctx->GetFuncName() << ", target: " << target << std::endl;
  ::trpc::Status status = proxy->UnaryInvoke<TestRequest, TestReply>(ctx, req, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoHttpAsyncUnaryInvoke(std::shared_ptr<::trpc::http::HttpServiceProxy> proxy, std::string target,
                            std::string func_name, TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = MakeClientContextWithIpPort(proxy, target);
  func_name = "/trpc.testing.httpsvr.HttpRpcService" + func_name;
  ctx->SetFuncName(func_name);
  bool succ = false;
  ::trpc::Future<> fut =
      proxy->AsyncUnaryInvoke<TestRequest, TestReply>(ctx, req).Then([&](::trpc::Future<TestReply>&& response) {
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

bool TestHttpUnaryInvokeWithCA(std::shared_ptr<::trpc::http::HttpServiceProxy> proxy, std::string svr_addr,
                               bool is_async, int& ret_code) {
  TestRequest req;
  req.set_msg("test-pb-req");
  TestReply rsp;
  bool succ;
  if (!is_async) {
    succ = DoHttpUnaryInvoke(proxy, svr_addr, "/TestPb", req, rsp, ret_code);
  } else {
    succ = DoHttpAsyncUnaryInvoke(proxy, svr_addr, "/TestPb", req, rsp, ret_code);
  }
  if (!succ) {
    std::cerr << "HttpUnaryInvoke failed" << std::endl;
    return false;
  }
  if (rsp.msg() != "test-pb-rsp") {
    std::cerr << "rsp check failed, expect: test-string-rsp, actual: " << rsp.msg() << std::endl;
    return false;
  }
  return succ;
}

// Testing case:
// HttpUnaryInvoke, filling the request and response text.
// Both the client and server are configured with certificate c1.
// The server expects the request text to match expectations.
// The client expects successful access and the response text to match expectations.
TEST_F(HttpsTest, TestHttpUnaryInvokeClientCA1ServerCA1) {
  RunByEnv([&]() { TestWithCAMatch("http_client_ca1", "rpc_service_ca1", false, TestHttpUnaryInvokeWithCA, nullptr); });
}
TEST_F(HttpsTest, TestAsyncHttpUnaryInvokeClientCA1ServerCA1) {
  RunByEnv([&]() { TestWithCAMatch("http_client_ca1", "rpc_service_ca1", true, TestHttpUnaryInvokeWithCA, nullptr); });
}

// Testing case:
// HttpUnaryInvoke, filling the request and response text.
// The client is configured with certificate c1, but the server is not configured with a certificate.
// The client expects the call to fail, with error code 141.
TEST_F(HttpsTest, TestHttpUnaryInvokeClientCA1ServerNOCA) {
  RunByEnv(
      [&]() { TestWithCANotMatch("http_client_ca1", "rpc_service_no_ca", false, TestHttpUnaryInvokeWithCA, nullptr); });
}
TEST_F(HttpsTest, TestAsyncHttpUnaryInvokeCA1ServerNOCA) {
  RunByEnv(
      [&]() { TestWithCANotMatch("http_client_ca1", "rpc_service_no_ca", true, TestHttpUnaryInvokeWithCA, nullptr); });
}

// Testing case:
// HttpUnaryInvoke, filling the request and response text.
// The client is not configured with a certificate, but the server is configured with certificate ca1.
// The client expects the call to fail, with error code 141.
TEST_F(HttpsTest, TestHttpUnaryInvokeClientNOCAServerCA1) {
  RunByEnv(
      [&]() { TestWithCANotMatch("http_client_no_ca", "rpc_service_ca1", false, TestHttpUnaryInvokeWithCA, nullptr); });
}
TEST_F(HttpsTest, TestAsyncHttpUnaryInvokeClientNOCAServerCA1) {
  RunByEnv(
      [&]() { TestWithCANotMatch("http_client_no_ca", "rpc_service_ca1", true, TestHttpUnaryInvokeWithCA, nullptr); });
}

// Testing case:
// HttpUnaryInvoke, filling the request and response text.
// The client is configured with certificate c1, but the server is configured with certificate ca2.
// The client expects the call to fail, with error code 141.
TEST_F(HttpsTest, TestHttpUnaryInvokeClientCA1ServerCA2) {
  RunByEnv([&]() {
    TestWithCANotMatch("http_client_no_ca1", "rpc_service_ca2", false, TestHttpUnaryInvokeWithCA, nullptr);
  });
}
TEST_F(HttpsTest, TestAsyncHttpUnaryInvokeClientCA1ServerCA2) {
  RunByEnv(
      [&]() { TestWithCANotMatch("http_client_no_ca1", "rpc_service_ca2", true, TestHttpUnaryInvokeWithCA, nullptr); });
}

bool DoHttpRpcInvoke(std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> proxy, std::string target,
                     TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = MakeClientContextWithIpPort(proxy, target);
  ::trpc::Status status = proxy->TestPb(ctx, req, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoHttpAsyncRpcInvoke(std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> proxy,
                          std::string target, TestRequest& req, TestReply& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = MakeClientContextWithIpPort(proxy, target);
  bool succ = false;
  Future<> fut = proxy->AsyncTestPb(ctx, req).Then([&](::trpc::Future<TestReply>&& response) {
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

bool TestHttpRpcInvokeWithCA(std::shared_ptr<::trpc::testing::httpsvr::HttpRpcServiceServiceProxy> proxy,
                             std::string svr_addr, bool is_async, int& ret_code) {
  TestRequest req;
  req.set_msg("test-pb-req");
  TestReply rsp;
  bool succ;
  if (!is_async) {
    succ = DoHttpRpcInvoke(proxy, svr_addr, req, rsp, ret_code);
  } else {
    succ = DoHttpAsyncRpcInvoke(proxy, svr_addr, req, rsp, ret_code);
  }
  if (!succ) {
    std::cerr << "HttpRpcInvoke failed" << std::endl;
    return false;
  }
  if (rsp.msg() != "test-pb-rsp") {
    std::cerr << "rsp check failed, expect: test-string-rsp, actual: " << rsp.msg() << std::endl;
    return false;
  }
  return succ;
}

// Testing case:
// HttpRpcInvoke, filling the request and response text.
// Both the client and server are configured with certificate c1.
// The server expects the request text to match expectations.
// The client expects successful access and the response text to match expectations.
TEST_F(HttpsTest, TestHttpRpcInvokeClientCA1ServerCA1) {
  RunByEnv([&]() { TestWithCAMatch("rpc_client_ca1", "rpc_service_ca1", false, nullptr, TestHttpRpcInvokeWithCA); });
}
TEST_F(HttpsTest, TestAsyncHttpRpcInvokeClientCA1ServerCA1) {
  RunByEnv([&]() { TestWithCAMatch("rpc_client_ca1", "rpc_service_ca1", true, nullptr, TestHttpRpcInvokeWithCA); });
}

// Testing case:
// HttpRpcInvoke, filling the request and response text.
// The client is configured with certificate c1, but the server is not configured with a certificate.
// The client expects the call to fail, with error code 141.
TEST_F(HttpsTest, TestHttpRpcInvokeClientCA1ServerNOCA) {
  RunByEnv(
      [&]() { TestWithCANotMatch("rpc_client_ca1", "rpc_service_no_ca", false, nullptr, TestHttpRpcInvokeWithCA); });
}
TEST_F(HttpsTest, TestAsyncHttpRpcInvokeCA1ServerNOCA) {
  RunByEnv(
      [&]() { TestWithCANotMatch("rpc_client_ca1", "rpc_service_no_ca", true, nullptr, TestHttpRpcInvokeWithCA); });
}

// Testing case:
// HttpRpcInvoke, filling the request and response text.
// The client is not configured with a certificate, but the server is configured with certificate ca1.
// The client expects the call to fail, with error code 141.
TEST_F(HttpsTest, TestHttpRpcInvokeClientNOCAServerCA1) {
  RunByEnv(
      [&]() { TestWithCANotMatch("rpc_client_no_ca", "rpc_service_ca1", false, nullptr, TestHttpRpcInvokeWithCA); });
}
TEST_F(HttpsTest, TestAsyncHttpRpcInvokeClientNOCAServerCA1) {
  RunByEnv(
      [&]() { TestWithCANotMatch("rpc_client_no_ca", "rpc_service_ca1", true, nullptr, TestHttpRpcInvokeWithCA); });
}

// Testing case:
// HttpRpcInvoke, filling the request and response text.
// The client is configured with certificate c1, but the server is configured with certificate ca2.
// The client expects the call to fail, with error code 141.
TEST_F(HttpsTest, TestHttpRpcInvokeClientCA1ServerCA2) {
  RunByEnv(
      [&]() { TestWithCANotMatch("rpc_client_no_ca1", "rpc_service_ca2", false, nullptr, TestHttpRpcInvokeWithCA); });
}
TEST_F(HttpsTest, TestAsyncHttpRpcInvokeClientCA1ServerCA2) {
  RunByEnv(
      [&]() { TestWithCANotMatch("rpc_client_no_ca1", "rpc_service_ca2", true, nullptr, TestHttpRpcInvokeWithCA); });
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  if (!::trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server_argv)) {
    exit(EXIT_FAILURE);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
