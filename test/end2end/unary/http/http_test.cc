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
#include "test/end2end/unary/http/http_server.h"
#include "trpc/client/http/http_service_proxy.h"
#include "trpc/future/future_utility.h"

int test_argc;
char** client_argv;
char** server_argv;

namespace trpc::testing {

class HttpTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    // Run before the first test case.
    test_signal_ = std::make_unique<TestSignaller>();
    server_process_ = std::make_unique<SubProcess>([]() {
      HttpServer server(test_signal_.get());
      server.Main(test_argc, server_argv);
      server.Wait();
    });
    // Wait for the server to start.
    test_signal_->ClientWaitToContinue();

    InitializeRuntimeEnv(test_argc, client_argv);
    proxy_ = trpc::GetTrpcClient()->GetProxy<trpc::http::HttpServiceProxy>("http_client");
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
  static std::shared_ptr<::trpc::http::HttpServiceProxy> proxy_;
};

std::unique_ptr<TestSignaller> HttpTest::test_signal_;
std::unique_ptr<SubProcess> HttpTest::server_process_;
std::shared_ptr<::trpc::http::HttpServiceProxy> HttpTest::proxy_;

bool DoGetJson(std::string url, rapidjson::Document& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status = HttpTest::proxy_->Get(ctx, url, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncGetJson(std::string url, rapidjson::Document& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<> fut = HttpTest::proxy_->AsyncGet(ctx, url).Then([&](::trpc::Future<rapidjson::Document>&& response) {
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
// GetJson.
// The client expects normal access and gets the expected JSON data.
TEST_F(HttpTest, TestGetJson) {
  RunByEnv([&]() {
    rapidjson::Document rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoGetJson("/test-get-json", rsp, ret_code));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json"));
  });
}
TEST_F(HttpTest, TestAsyncGetJson) {
  RunByEnv([&]() {
    rapidjson::Document rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncGetJson("/test-get-json", rsp, ret_code));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json"));
  });
}

// Testing case:
// GetJson, accessing a non-existent URL.
// The client expects the access to fail and the return code to be 404.
TEST_F(HttpTest, TestGetJsonNonExistUrl) {
  RunByEnv([&]() {
    rapidjson::Document rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoGetJson("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncGetJsonNonExistUrl) {
  RunByEnv([&]() {
    rapidjson::Document rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncGetJson("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

// Testing case:
// GetJson, the response body is a string.
// The client expects the access to fail and the error code to be decoding failure.
TEST_F(HttpTest, TestGetJsonButRspString) {
  RunByEnv([&]() {
    rapidjson::Document rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoGetJson("/test-get-string", rsp, ret_code));
    ASSERT_EQ(ret_code, ::trpc::codec::GetDefaultClientRetCode(::trpc::codec::ClientRetCode::DECODE_ERROR));
  });
}
TEST_F(HttpTest, AsyncTestGetJsonButRspString) {
  RunByEnv([&]() {
    rapidjson::Document rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncGetJson("/test-get-string", rsp, ret_code));
    ASSERT_EQ(ret_code, ::trpc::codec::GetDefaultClientRetCode(::trpc::codec::ClientRetCode::DECODE_ERROR));
  });
}

// Testing case:
// GetJson , the response body is empty.
// The client expects normal access and an empty response body.
TEST_F(HttpTest, TestGetJsonEmptyRsp) {
  RunByEnv([&]() {
    rapidjson::Document rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoGetJson("/test-get-json-empty", rsp, ret_code));
    ASSERT_TRUE(rsp.IsNull());
  });
}
TEST_F(HttpTest, TestAsyncGetJsonEmptyRsp) {
  RunByEnv([&]() {
    rapidjson::Document rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncGetJson("/test-get-json-empty", rsp, ret_code));
    ASSERT_TRUE(rsp.IsNull());
  });
}

bool DoGetString(std::string url, std::string& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status = HttpTest::proxy_->GetString(ctx, url, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncGetString(std::string url, std::string& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<> fut = HttpTest::proxy_->AsyncGetString(ctx, url).Then([&](::trpc::Future<std::string>&& response) {
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
// GetString.
// The client expects normal access and gets the expected string data.
TEST_F(HttpTest, TestGetString) {
  RunByEnv([&]() {
    std::string rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoGetString("/test-get-string", rsp, ret_code));
    ASSERT_EQ(rsp, "test-rsp");
  });
}
TEST_F(HttpTest, TestAsyncGetString) {
  RunByEnv([&]() {
    std::string rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncGetString("/test-get-string", rsp, ret_code));
    ASSERT_EQ(rsp, "test-rsp");
  });
}

// Testing case:
// GetString, accessing a non-existent URL
// The client expects the access to fail and the error code to be 404.
TEST_F(HttpTest, TestGetStringNonExistUrl) {
  RunByEnv([&]() {
    std::string rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoGetString("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncGetStringNonExistUrl) {
  RunByEnv([&]() {
    std::string rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncGetString("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

// Testing case:
// GetString, the response body is empty.
// The client expects normal access and an empty body.
TEST_F(HttpTest, TestGetStringEmptyRsp) {
  RunByEnv([&]() {
    std::string rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoGetString("/test-get-string-empty", rsp, ret_code));
    ASSERT_TRUE(rsp.empty());
  });
}
TEST_F(HttpTest, TestAsyncGetStringEmptyRsp) {
  RunByEnv([&]() {
    std::string rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncGetString("/test-get-string-empty", rsp, ret_code));
    ASSERT_TRUE(rsp.empty());
  });
}

bool DoGet2(std::string url, ::trpc::http::HttpResponse& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status = HttpTest::proxy_->Get2(ctx, url, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncGet2(std::string url, ::trpc::http::HttpResponse& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<> fut =
      HttpTest::proxy_->AsyncGet2(ctx, url).Then([&](::trpc::Future<::trpc::http::HttpResponse>&& response) {
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
// Get2, filling the response header and body.
// The client expects normal access and the filled header and body to match expectations.
TEST_F(HttpTest, TestGet2) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoGet2("/test-get-rsp-struct", rsp, ret_code));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_TRUE(rsp.HasHeader("testkey"));
    ASSERT_EQ(rsp.GetHeader("testkey"), "testval");
    ASSERT_EQ(rsp.GetContent(), "test-rsp");
  });
}
TEST_F(HttpTest, TestAsyncGet2) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncGet2("/test-get-rsp-struct", rsp, ret_code));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_TRUE(rsp.HasHeader("testkey"));
    ASSERT_EQ(rsp.GetHeader("testkey"), "testval");
    ASSERT_EQ(rsp.GetContent(), "test-rsp");
  });
}

// Testing case:
// Get2, accessing a non-existent URL.
// The client expects the access to fail and the error code to be 404.
TEST_F(HttpTest, TestGet2NonExistUrl) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoGet2("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncGet2NonExistUrl) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncGet2("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

// Testing case:
// Get2, filling the header but the body is empty.
// The client expects normal access and an empty body.
TEST_F(HttpTest, TestGet2EmptyRsp) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoGet2("/test-get-rsp-struct-empty", rsp, ret_code));
    ASSERT_TRUE(rsp.HasHeader("testkey"));
    ASSERT_EQ(rsp.GetHeader("testkey"), "testval");
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}
TEST_F(HttpTest, TestAsyncGet2EmptyRsp) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncGet2("/test-get-rsp-struct-empty", rsp, ret_code));
    ASSERT_TRUE(rsp.HasHeader("testkey"));
    ASSERT_EQ(rsp.GetHeader("testkey"), "testval");
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}

bool DoPostJson(std::string url, rapidjson::Document& req, rapidjson::Document& rsp, int& ret_code, bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status;
  if (rh) {
    status = HttpTest::proxy_->Post(ctx, url, std::move(req), &rsp);
  } else {
    status = HttpTest::proxy_->Post(ctx, url, req, &rsp);
  }
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncPostJson(std::string url, rapidjson::Document& req, rapidjson::Document& rsp, int& ret_code,
                     bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<rapidjson::Document> fut_rsp;
  if (rh) {
    fut_rsp = HttpTest::proxy_->AsyncPost(ctx, url, std::move(req));
  } else {
    fut_rsp = HttpTest::proxy_->AsyncPost(ctx, url, req);
  }
  ::trpc::Future<> fut = fut_rsp.Then([&](::trpc::Future<rapidjson::Document>&& response) {
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
// PostJson, filling the request and response JSON.
// The server expects the request body to be the expected JSON.
// The client expects normal access and the response body to be the expected JSON.
TEST_F(HttpTest, TestPostJson) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    req.SetObject();
    req.AddMember("msg", "test-json-req", req.GetAllocator());
    int ret_code = 0;
    ASSERT_TRUE(DoPostJson("/test-post-json", req, rsp, ret_code));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
    // Test using the right value interface.
    ASSERT_TRUE(DoPostJson("/test-post-json", req, rsp, ret_code, true));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
  });
}
TEST_F(HttpTest, TestAsyncPostJson) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    req.SetObject();
    req.AddMember("msg", "test-json-req", req.GetAllocator());
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPostJson("/test-post-json", req, rsp, ret_code));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
    // Test using the right value interface.
    ASSERT_TRUE(DoAsyncPostJson("/test-post-json", req, rsp, ret_code, true));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
  });
}

// Testing case:
// PostJson, the response body is not JSON.
// The client expects the access to fail and the error code to be decoding failure.
TEST_F(HttpTest, TestPostJsonButRspString) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoPostJson("/test-post-json-ret-string", req, rsp, ret_code));
    ASSERT_EQ(ret_code, ::trpc::codec::GetDefaultClientRetCode(::trpc::codec::ClientRetCode::DECODE_ERROR));
  });
}
TEST_F(HttpTest, TestAsyncPostJsonButRspString) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncPostJson("/test-post-json-ret-string", req, rsp, ret_code));
    ASSERT_EQ(ret_code, ::trpc::codec::GetDefaultClientRetCode(::trpc::codec::ClientRetCode::DECODE_ERROR));
  });
}

// Testing case:
// PostJson, accessing a non-existent URL.
// The client expects the access to fail and the error code to be 404.
TEST_F(HttpTest, TestPostJsonNonExistUrl) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoPostJson("/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncPostJsonNonExistUrl) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncPostJson("/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

// Testing case:
// PostJson, the request body is not empty but the response body is empty.
// The server expects the request body to be non-empty.
// The client expects successful access and an empty response body.
TEST_F(HttpTest, TestPostJsonButRspEmpty) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    req.SetObject();
    req.AddMember("msg", "test-json-req", req.GetAllocator());
    int ret_code = 0;
    ASSERT_TRUE(DoPostJson("/test-post-json-empty-rsp", req, rsp, ret_code));
    ASSERT_TRUE(rsp.IsNull());
  });
}
TEST_F(HttpTest, TestAsyncPostJsonButRspEmpty) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    req.SetObject();
    req.AddMember("msg", "test-json-req", req.GetAllocator());
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPostJson("/test-post-json-empty-rsp", req, rsp, ret_code));
    ASSERT_TRUE(rsp.IsNull());
  });
}

// Testing case:
// PostJson, the request body is empty but the response body is not empty.
// The server expects the request body to be empty.
// The client expects successful access and a non-empty response body.
TEST_F(HttpTest, TestPostJsonButReqEmpty) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoPostJson("/test-post-json-empty-req", req, rsp, ret_code));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
  });
}
TEST_F(HttpTest, TestAsyncPostJsonButReqEmpty) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPostJson("/test-post-json-empty-req", req, rsp, ret_code));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
  });
}

bool DoPostString(std::string url, std::string& req, std::string& rsp, int& ret_code, bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status;
  if (rh) {
    status = HttpTest::proxy_->Post(ctx, url, std::move(req), &rsp);
  } else {
    status = HttpTest::proxy_->Post(ctx, url, req, &rsp);
  }
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncPostString(std::string url, std::string& req, std::string& rsp, int& ret_code, bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<std::string> fut_rsp;
  if (rh) {
    fut_rsp = HttpTest::proxy_->AsyncPost(ctx, url, std::move(req));
  } else {
    fut_rsp = HttpTest::proxy_->AsyncPost(ctx, url, req);
  }
  ::trpc::Future<> fut = fut_rsp.Then([&](::trpc::Future<std::string>&& response) {
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
// PostString, filling the request and response text.
// The server expects the request text to match expectations.
// The client expects successful access and the response text to match expectations.
TEST_F(HttpTest, TestPostString) {
  RunByEnv([&]() {
    std::string req, rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoPostString("/test-post-string", req, rsp, ret_code));
    ASSERT_EQ(rsp, "test-string-rsp");
    // Test using the right value interface.
    ASSERT_TRUE(DoPostString("/test-post-string", req, rsp, ret_code, true));
    ASSERT_EQ(rsp, "test-string-rsp");
  });
}
TEST_F(HttpTest, TestAsyncPostString) {
  RunByEnv([&]() {
    std::string req, rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPostString("/test-post-string", req, rsp, ret_code));
    ASSERT_EQ(rsp, "test-string-rsp");
    // Test using the right value interface.
    ASSERT_TRUE(DoAsyncPostString("/test-post-string", req, rsp, ret_code, true));
    ASSERT_EQ(rsp, "test-string-rsp");
  });
}

// Testing case:
// PostString, accessing a non-existent URL.
// The client expects the access to fail and the error code to be 404.
TEST_F(HttpTest, TestPostStringNonExistUrl) {
  RunByEnv([&]() {
    std::string req, rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoPostString("/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncPostStringNonExistUrl) {
  RunByEnv([&]() {
    std::string req, rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncPostString("/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

// Testing case:
// PostString, the request body is not empty but the response body is empty.
// The server expects the request body to be non-empty.
// The client expects normal access and an empty response body.
TEST_F(HttpTest, TestPostStringButRspEmpty) {
  RunByEnv([&]() {
    std::string req, rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoPostString("/test-post-string-empty-rsp", req, rsp, ret_code));
    ASSERT_TRUE(rsp.empty());
  });
}
TEST_F(HttpTest, TestAsyncPostStringButRspEmpty) {
  RunByEnv([&]() {
    std::string req, rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPostString("/test-post-string-empty-rsp", req, rsp, ret_code));
    ASSERT_TRUE(rsp.empty());
  });
}

// Testing case:
// PostString, the request body is empty but the response body is not empty.
// The server expects the request body to be empty.
// The client expects normal access and a non-empty response body.
TEST_F(HttpTest, TestPostStringButReqEmpty) {
  RunByEnv([&]() {
    std::string req, rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoPostString("/test-post-string-empty-req", req, rsp, ret_code));
    ASSERT_EQ(rsp, "test-string-rsp");
  });
}
TEST_F(HttpTest, TestAsyncPostStringButReqEmpty) {
  RunByEnv([&]() {
    std::string req, rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPostString("/test-post-string-empty-req", req, rsp, ret_code));
    ASSERT_EQ(rsp, "test-string-rsp");
  });
}

bool DoPostBuffer(std::string url, ::trpc::NoncontiguousBuffer& req, ::trpc::NoncontiguousBuffer& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status = HttpTest::proxy_->Post(ctx, url, std::move(req), &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncPostBuffer(std::string url, ::trpc::NoncontiguousBuffer& req, ::trpc::NoncontiguousBuffer& rsp,
                       int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<> fut = HttpTest::proxy_->AsyncPost(ctx, url, std::move(req))
                             .Then([&](::trpc::Future<::trpc::NoncontiguousBuffer>&& response) {
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
// PostBuffer, filling the request and response body.
// The server expects the request body to match expectations.
// The client expects normal access and the response body to match expectations.
TEST_F(HttpTest, TestPostNoncontiguousBuffer) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    ::trpc::NoncontiguousBufferBuilder builder;
    builder.Append("test-string-req");
    req = builder.DestructiveGet();
    int ret_code = 0;
    ASSERT_TRUE(DoPostBuffer("/test-post-string", req, rsp, ret_code));
    std::string rsp_str = ::trpc::FlattenSlow(rsp);
    ASSERT_EQ(rsp_str, "test-string-rsp");
  });
}
TEST_F(HttpTest, TestAsyncPostNoncontiguousBuffer) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    ::trpc::NoncontiguousBufferBuilder builder;
    builder.Append("test-string-req");
    req = builder.DestructiveGet();
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPostBuffer("/test-post-string", req, rsp, ret_code));
    std::string rsp_str = ::trpc::FlattenSlow(rsp);
    ASSERT_EQ(rsp_str, "test-string-rsp");
  });
}

// Testing case:
// PostBuffer, accessing a non-existent URL.
// The client expects the access to fail and the return code to be 404.
TEST_F(HttpTest, TestPostNoncontiguousBufferNonExistUrl) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoPostBuffer("/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncPostNoncontiguousBufferNonExistUrl) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncPostBuffer("/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

// Testing case:
// PostBuffer, filling the request body but not the response body.
// The server expects the request body to match expectations.
// The client expects successful access and an empty response body.
TEST_F(HttpTest, TestPostNoncontiguousBufferButRspEmpty) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    ::trpc::NoncontiguousBufferBuilder builder;
    builder.Append("test-string-req");
    req = builder.DestructiveGet();
    int ret_code = 0;
    ASSERT_TRUE(DoPostBuffer("/test-post-string-empty-rsp", req, rsp, ret_code));
    std::string rsp_str = ::trpc::FlattenSlow(rsp);
    ASSERT_TRUE(rsp_str.empty());
  });
}
TEST_F(HttpTest, TestAsyncPostNoncontiguousBufferButRspEmpty) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    ::trpc::NoncontiguousBufferBuilder builder;
    builder.Append("test-string-req");
    req = builder.DestructiveGet();
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPostBuffer("/test-post-string-empty-rsp", req, rsp, ret_code));
    std::string rsp_str = ::trpc::FlattenSlow(rsp);
    ASSERT_TRUE(rsp_str.empty());
  });
}

// Testing case:
// PostBuffer, not filling the request body but filling the response body.
// The server expects the request body to be empty.
// The client expects successful access and the response body to match expectations.
TEST_F(HttpTest, TestPostNoncontiguousBufferButReqEmpty) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoPostBuffer("/test-post-string-empty-req", req, rsp, ret_code));
    std::string rsp_str = ::trpc::FlattenSlow(rsp);
    ASSERT_EQ(rsp_str, "test-string-rsp");
  });
}
TEST_F(HttpTest, TestAsyncPostNoncontiguousBufferButReqEmpty) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPostBuffer("/test-post-string-empty-req", req, rsp, ret_code));
    std::string rsp_str = ::trpc::FlattenSlow(rsp);
    ASSERT_EQ(rsp_str, "test-string-rsp");
  });
}

bool DoPost2(std::string url, std::string& req, ::trpc::http::HttpResponse& rsp, int& ret_code, bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status;
  if (rh) {
    status = HttpTest::proxy_->Post2(ctx, url, std::move(req), &rsp);
  } else {
    status = HttpTest::proxy_->Post2(ctx, url, req, &rsp);
  }
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncPost2(std::string url, std::string& req, ::trpc::http::HttpResponse& rsp, int& ret_code, bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<::trpc::http::HttpResponse> fut_rsp;
  if (rh) {
    fut_rsp = HttpTest::proxy_->AsyncPost2(ctx, url, std::move(req));
  } else {
    fut_rsp = HttpTest::proxy_->AsyncPost2(ctx, url, req);
  }
  ::trpc::Future<> fut = fut_rsp.Then([&](::trpc::Future<::trpc::http::HttpResponse>&& response) {
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
// Post2, filling the request and response body.
// The server expects the request body to match expectations.
// The client expects the response body to match expectations.
TEST_F(HttpTest, TestPost2) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoPost2("/test-post-string", req, rsp, ret_code));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
    ASSERT_TRUE(DoPost2("/test-post-string", req, rsp, ret_code, true));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
  });
}
TEST_F(HttpTest, TestAsyncPost2) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPost2("/test-post-string", req, rsp, ret_code));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
    ASSERT_TRUE(DoAsyncPost2("/test-post-string", req, rsp, ret_code, true));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
  });
}

// Testing case:
// Post2, accessing a non-existent URL.
// The client expects the access to fail and the error code to be 404.
TEST_F(HttpTest, TestPost2NonExistUrl) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoPost2("/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncPost2NonExistUrl) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncPost2("/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

// Testing case:
// Post2, filling the request body but not the response body.
// The server expects the request body to be non-empty.
// The client expects an empty response body.
TEST_F(HttpTest, TestPost2EmptyRsp) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoPost2("/test-post-string-empty-rsp", req, rsp, ret_code));
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}
TEST_F(HttpTest, TestAsyncPost2EmptyRsp) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPost2("/test-post-string-empty-rsp", req, rsp, ret_code));
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}

// Testing case:
// Post2, not filling the request body but filling the response body.
// The server expects the request body to be empty.
// The client expects the response body to be non-empty.
TEST_F(HttpTest, TestPost2EmptyReq) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoPost2("/test-post-string-empty-req", req, rsp, ret_code));
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
  });
}
TEST_F(HttpTest, TestAsyncPost2EmptyReq) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPost2("/test-post-string-empty-req", req, rsp, ret_code));
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
  });
}

bool DoHead(std::string url, ::trpc::http::HttpResponse& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status = HttpTest::proxy_->Head(ctx, url, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncHead(std::string url, ::trpc::http::HttpResponse& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<> fut =
      HttpTest::proxy_->AsyncHead(ctx, url).Then([&](::trpc::Future<::trpc::http::HttpResponse>&& response) {
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
// Head request.
// The client expects the response header to have the Content-Length field and the body to be empty.
TEST_F(HttpTest, TestHead) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoHead("/test-head", rsp, ret_code));
    ASSERT_EQ(rsp.GetHeader("Content-Length"), "100");
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}
TEST_F(HttpTest, TestAsyncHead) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncHead("/test-head", rsp, ret_code));
    ASSERT_EQ(rsp.GetHeader("Content-Length"), "100");
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}

// Testing case:
// Head request, accessing a non-existent URL.
// The client expects the call to fail and the error code to be 404.
TEST_F(HttpTest, TestHeadNonExistUrl) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoHead("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncHeadNonExistUrl) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncHead("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

bool DoOptions(std::string url, ::trpc::http::HttpResponse& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status = HttpTest::proxy_->Options(ctx, url, &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncOptions(std::string url, ::trpc::http::HttpResponse& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<> fut =
      HttpTest::proxy_->AsyncOptions(ctx, url).Then([&](::trpc::Future<::trpc::http::HttpResponse>&& response) {
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
// Options request.
// The client expects the Content-Length header to be 0, the body to be 0, and the Allow field to be correctly filled.
TEST_F(HttpTest, TestOptions) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoOptions("/test-options", rsp, ret_code));
    ASSERT_EQ(rsp.GetHeader("Content-Length"), "0");
    ASSERT_EQ(rsp.GetHeader("Allow"), "OPTIONS, GET, HEAD, POST");
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}
TEST_F(HttpTest, TestAsyncOptions) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncOptions("/test-options", rsp, ret_code));
    ASSERT_EQ(rsp.GetHeader("Content-Length"), "0");
    ASSERT_EQ(rsp.GetHeader("Allow"), "OPTIONS, GET, HEAD, POST");
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}

// Testing case:
// Options request, accessing a non-existent URL.
// The client expects the call to fail and the error code to be 404.
TEST_F(HttpTest, TestOptionsNonExistUrl) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoOptions("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncOptionsNonExistUrl) {
  RunByEnv([&]() {
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncOptions("/test-non-exist", rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

bool DoHttpUnaryInvoke(std::string method, std::string url, ::trpc::http::HttpRequest& req,
                       ::trpc::http::HttpResponse& rsp, int& ret_code) {
  req.SetMethod(method);
  req.SetUrl(url);
  req.SetVersion("1.1");
  req.SetHeader("Content-Type", "application/json");
  req.SetHeader("Accept", "*/*");
  req.SetHeader("Content-Length", std::to_string(req.GetContent().length()));
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  ::trpc::Status status =
      HttpTest::proxy_->HttpUnaryInvoke<::trpc::http::HttpRequest, ::trpc::http::HttpResponse>(ctx, req, &rsp);
  ret_code = status.GetFuncRetCode();
  return status.OK();
}

bool DoAsyncHttpUnaryInvoke(std::string method, std::string url, ::trpc::http::HttpRequest& req,
                            ::trpc::http::HttpResponse& rsp, int& ret_code) {
  req.SetMethod(method);
  req.SetUrl(url);
  req.SetVersion("1.1");
  req.SetHeader("Content-Type", "application/json");
  req.SetHeader("Accept", "*/*");
  req.SetHeader("Content-Length", std::to_string(req.GetContent().length()));
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  bool succ = false;
  ::trpc::Future<> fut =
      HttpTest::proxy_->AsyncHttpUnaryInvoke<::trpc::http::HttpRequest, ::trpc::http::HttpResponse>(ctx, req).Then(
          [&](::trpc::Future<::trpc::http::HttpResponse>&& response) {
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
// HttpUnaryInvoke, using GetJson.
// The client expects the call to be successful and the response JSON to match expectations.
TEST_F(HttpTest, TestHttpUnaryInvokeGetJson) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoHttpUnaryInvoke("GET", "/test-get-json", req, rsp, ret_code));
    rapidjson::Document rsp_json;
    rsp_json.Parse(rsp.GetContent());
    ASSERT_TRUE(CheckJsonKV(rsp_json, "msg", "test-json"));
  });
}
TEST_F(HttpTest, TestAsyncHttpUnaryInvokeGetJson) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncHttpUnaryInvoke("GET", "/test-get-json", req, rsp, ret_code));
    rapidjson::Document rsp_json;
    rsp_json.Parse(rsp.GetContent());
    ASSERT_TRUE(CheckJsonKV(rsp_json, "msg", "test-json"));
  });
}

// Testing case:
// HttpUnaryInvoke, using GetJson and filling the response header.
// The client expects the call to be successful, the response JSON to match expectations, and the response header to
// match expectations.
TEST_F(HttpTest, TestHttpUnaryInvokeGetJsonHeader) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    req.SetHeader("reqkey1", "reqval1");
    req.SetHeader("reqkey2", "reqval2");
    int ret_code = 0;
    ASSERT_TRUE(DoHttpUnaryInvoke("GET", "/test-get-json-header", req, rsp, ret_code));
    rapidjson::Document rsp_json;
    rsp_json.Parse(rsp.GetContent());
    ASSERT_TRUE(CheckJsonKV(rsp_json, "msg", "test-json"));
    ASSERT_EQ(rsp.GetHeader("rspkey1"), "rspval1");
    ASSERT_EQ(rsp.GetHeader("rspkey2"), "rspval2");
  });
}
TEST_F(HttpTest, TestAsyncHttpUnaryInvokeGetJsonHeader) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    req.SetHeader("reqkey1", "reqval1");
    req.SetHeader("reqkey2", "reqval2");
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncHttpUnaryInvoke("GET", "/test-get-json-header", req, rsp, ret_code));
    rapidjson::Document rsp_json;
    rsp_json.Parse(rsp.GetContent());
    ASSERT_TRUE(CheckJsonKV(rsp_json, "msg", "test-json"));
    ASSERT_EQ(rsp.GetHeader("rspkey1"), "rspval1");
    ASSERT_EQ(rsp.GetHeader("rspkey2"), "rspval2");
  });
}

// Testing case:
// HttpUnaryInvoke, accessing a non-existent URL.
// The client expects the call to fail and the error code to be 404.
TEST_F(HttpTest, TestHttpUnaryInvokeGetJsonNonExistUrl) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoHttpUnaryInvoke("GET", "/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}
TEST_F(HttpTest, TestAsyncHttpUnaryInvokeGetJsonNonExistUrl) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncHttpUnaryInvoke("GET", "/test-non-exist", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 404);
  });
}

// Testing case:
// HttpUnaryInvoke, using PostJson and filling the request and response JSON.
// The server expects the request JSON to match expectations.
// The client expects the call to be successful and the response JSON to match expectations.
TEST_F(HttpTest, TestHttpUnaryInvokePostJson) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    req.SetContent(ConstructJsonStr({{"msg", "test-json-req"}}));
    int ret_code = 0;
    ASSERT_TRUE(DoHttpUnaryInvoke("POST", "/test-post-json", req, rsp, ret_code));
    rapidjson::Document rsp_json;
    rsp_json.Parse(rsp.GetContent());
    ASSERT_TRUE(CheckJsonKV(rsp_json, "msg", "test-json-rsp"));
  });
}
TEST_F(HttpTest, TestAsyncHttpUnaryInvokePostJson) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    req.SetContent(ConstructJsonStr({{"msg", "test-json-req"}}));
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncHttpUnaryInvoke("POST", "/test-post-json", req, rsp, ret_code));
    rapidjson::Document rsp_json;
    rsp_json.Parse(rsp.GetContent());
    ASSERT_TRUE(CheckJsonKV(rsp_json, "msg", "test-json-rsp"));
  });
}

// Testing case:
// HttpUnaryInvoke, using Post and filling the request body but not the response body.
// The server expects the request body to be non-empty.
// The client expects the call to be successful and the response body to be empty.
TEST_F(HttpTest, TestHttpUnaryInvokePostJsonButRspEmpty) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    req.SetContent(ConstructJsonStr({{"msg", "test-json-req"}}));
    int ret_code = 0;
    ASSERT_TRUE(DoHttpUnaryInvoke("POST", "/test-post-json-empty-rsp", req, rsp, ret_code));
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}
TEST_F(HttpTest, TestAsyncHttpUnaryInvokePostJsonButRspEmpty) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    req.SetContent(ConstructJsonStr({{"msg", "test-json-req"}}));
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncHttpUnaryInvoke("POST", "/test-post-json-empty-rsp", req, rsp, ret_code));
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}

// Testing case:
// HttpUnaryInvoke, using Post and not filling the request body but filling the response body.
// The server expects the request body to be empty.
// The client expects the call to be successful and the response body to be non-empty.
TEST_F(HttpTest, TestHttpUnaryInvokePostJsonButReqEmpty) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoHttpUnaryInvoke("POST", "/test-post-json-empty-req", req, rsp, ret_code));
    rapidjson::Document rsp_json;
    rsp_json.Parse(rsp.GetContent());
    ASSERT_TRUE(CheckJsonKV(rsp_json, "msg", "test-json-rsp"));
  });
}
TEST_F(HttpTest, TestAsyncHttpUnaryInvokePostJsonButReqEmpty) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncHttpUnaryInvoke("POST", "/test-post-json-empty-req", req, rsp, ret_code));
    rapidjson::Document rsp_json;
    rsp_json.Parse(rsp.GetContent());
    ASSERT_TRUE(CheckJsonKV(rsp_json, "msg", "test-json-rsp"));
  });
}

// Testing case:
// HttpUnaryInvoke, using Head and matching expectations.
TEST_F(HttpTest, TestHttpUnaryInvokeHead) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoHttpUnaryInvoke("HEAD", "/test-head", req, rsp, ret_code));
    ASSERT_EQ(rsp.GetHeader("Content-Length"), "100");
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}
TEST_F(HttpTest, TestAsyncHttpUnaryInvokeHead) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncHttpUnaryInvoke("HEAD", "/test-head", req, rsp, ret_code));
    ASSERT_EQ(rsp.GetHeader("Content-Length"), "100");
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}

// Testing case:
// HttpUnaryInvoke, using Get and filling the response body when the response is not 200.
// The client expects the call to fail, the response error code to be 400, and the response body to be empty.
TEST_F(HttpTest, TestHttpUnaryInvokeGetNot200Ret) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoHttpUnaryInvoke("GET", "/test-get-not-200-ret", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 400);
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}
TEST_F(HttpTest, TestAsyncHttpUnaryInvokeGetNot200Ret) {
  RunByEnv([&]() {
    ::trpc::http::HttpRequest req;
    ::trpc::http::HttpResponse rsp;
    int ret_code = 0;
    ASSERT_FALSE(DoAsyncHttpUnaryInvoke("GET", "/test-get-not-200-ret", req, rsp, ret_code));
    ASSERT_EQ(ret_code, 400);
    ASSERT_TRUE(rsp.GetContent().empty());
  });
}

// Both Put and Post follow the same exception handling logic, so we will only test the normal logic here.
template <class RequestMessage, class ResponseMessage>
bool DoPut(std::string url, RequestMessage& req, ResponseMessage& rsp, int& ret_code, bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status;
  if (rh) {
    status = HttpTest::proxy_->Put(ctx, url, std::move(req), &rsp);
  } else {
    status = HttpTest::proxy_->Put(ctx, url, req, &rsp);
  }
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

template <class RequestMessage, class ResponseMessage>
bool DoAsyncPut(std::string url, RequestMessage& req, ResponseMessage& rsp, int& ret_code, bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<ResponseMessage> fut_rsp;
  if (rh) {
    fut_rsp = HttpTest::proxy_->AsyncPut(ctx, url, std::move(req));
  } else {
    fut_rsp = HttpTest::proxy_->AsyncPut(ctx, url, req);
  }
  ::trpc::Future<> fut = fut_rsp.Then([&](::trpc::Future<ResponseMessage>&& response) {
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
// PutJson, filling the request and response JSON.
// The server expects the request body to be the expected JSON.
// The client expects normal access and the response body to be the expected JSON.
TEST_F(HttpTest, TestPutJson) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    req.SetObject();
    req.AddMember("msg", "test-json-req", req.GetAllocator());
    int ret_code = 0;
    ASSERT_TRUE(DoPut("/test-put-json", req, rsp, ret_code));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
    // Test using the right value interface.
    ASSERT_TRUE(DoPut("/test-put-json", req, rsp, ret_code, true));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
  });
}
TEST_F(HttpTest, TestAsyncPutJson) {
  RunByEnv([&]() {
    rapidjson::Document req, rsp;
    req.SetObject();
    req.AddMember("msg", "test-json-req", req.GetAllocator());
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPut("/test-put-json", req, rsp, ret_code));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
    // Test using the right value interface.
    ASSERT_TRUE(DoAsyncPut("/test-put-json", req, rsp, ret_code, true));
    ASSERT_TRUE(CheckJsonKV(rsp, "msg", "test-json-rsp"));
  });
}

// Testing case:
// PutString, filling the request and response text.
// The server expects the request text to match expectations.
// The client expects successful access and the response text to match expectations.
TEST_F(HttpTest, TestPutString) {
  RunByEnv([&]() {
    std::string req, rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoPut("/test-put-string", req, rsp, ret_code));
    ASSERT_EQ(rsp, "test-string-rsp");
    // Test using the right value interface.
    ASSERT_TRUE(DoPut("/test-put-string", req, rsp, ret_code, true));
    ASSERT_EQ(rsp, "test-string-rsp");
  });
}
TEST_F(HttpTest, TestAsyncPutString) {
  RunByEnv([&]() {
    std::string req, rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPut("/test-put-string", req, rsp, ret_code));
    ASSERT_EQ(rsp, "test-string-rsp");
    // Test using the right value interface.
    ASSERT_TRUE(DoAsyncPut("/test-put-string", req, rsp, ret_code, true));
    ASSERT_EQ(rsp, "test-string-rsp");
  });
}

bool DoPutBuffer(std::string url, ::trpc::NoncontiguousBuffer& req, ::trpc::NoncontiguousBuffer& rsp, int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status = HttpTest::proxy_->Put(ctx, url, std::move(req), &rsp);
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncPutBuffer(std::string url, ::trpc::NoncontiguousBuffer& req, ::trpc::NoncontiguousBuffer& rsp,
                      int& ret_code) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<> fut = HttpTest::proxy_->AsyncPut(ctx, url, std::move(req))
                             .Then([&](::trpc::Future<::trpc::NoncontiguousBuffer>&& response) {
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
// PutBuffer, filling the request and response body.
// The server expects the request body to match expectations.
// The client expects normal access and the response body to match expectations.
TEST_F(HttpTest, TestPutNoncontiguousBuffer) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    ::trpc::NoncontiguousBufferBuilder builder;
    builder.Append("test-string-req");
    req = builder.DestructiveGet();
    int ret_code = 0;
    ASSERT_TRUE(DoPutBuffer("/test-put-string", req, rsp, ret_code));
    std::string rsp_str = ::trpc::FlattenSlow(rsp);
    ASSERT_EQ(rsp_str, "test-string-rsp");
  });
}
TEST_F(HttpTest, TestAsyncPutNoncontiguousBuffer) {
  RunByEnv([&]() {
    ::trpc::NoncontiguousBuffer req, rsp;
    ::trpc::NoncontiguousBufferBuilder builder;
    builder.Append("test-string-req");
    req = builder.DestructiveGet();
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPutBuffer("/test-put-string", req, rsp, ret_code));
    std::string rsp_str = ::trpc::FlattenSlow(rsp);
    ASSERT_EQ(rsp_str, "test-string-rsp");
  });
}

bool DoPut2(std::string url, std::string& req, ::trpc::http::HttpResponse& rsp, int& ret_code, bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  ::trpc::Status status;
  if (rh) {
    status = HttpTest::proxy_->Put2(ctx, url, std::move(req), &rsp);
  } else {
    status = HttpTest::proxy_->Put2(ctx, url, req, &rsp);
  }
  if (status.GetFrameworkRetCode() != 0) {
    ret_code = status.GetFrameworkRetCode();
  } else if (status.GetFuncRetCode() != 0) {
    ret_code = status.GetFuncRetCode();
  }
  return status.OK();
}

bool DoAsyncPut2(std::string url, std::string& req, ::trpc::http::HttpResponse& rsp, int& ret_code, bool rh = false) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(HttpTest::proxy_);
  url = "http://test.com" + url;
  bool succ = false;
  ::trpc::Future<::trpc::http::HttpResponse> fut_rsp;
  if (rh) {
    fut_rsp = HttpTest::proxy_->AsyncPut2(ctx, url, std::move(req));
  } else {
    fut_rsp = HttpTest::proxy_->AsyncPut2(ctx, url, req);
  }
  ::trpc::Future<> fut = fut_rsp.Then([&](::trpc::Future<::trpc::http::HttpResponse>&& response) {
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
// Put2, filling the request and response body.
// The server expects the request body to match expectations.
// The client expects the response body to match expectations.
TEST_F(HttpTest, TestPut2) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoPut2("/test-put-string", req, rsp, ret_code));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
    // Test using the right value interface.
    ASSERT_TRUE(DoPut2("/test-put-string", req, rsp, ret_code, true));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
  });
}
TEST_F(HttpTest, TestAsyncPut2) {
  RunByEnv([&]() {
    std::string req;
    ::trpc::http::HttpResponse rsp;
    req = "test-string-req";
    int ret_code = 0;
    ASSERT_TRUE(DoAsyncPut2("/test-put-string", req, rsp, ret_code));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
    // Test using the right value interface.
    ASSERT_TRUE(DoAsyncPut2("/test-put-string", req, rsp, ret_code, true));
    ASSERT_TRUE(rsp.IsOK());
    ASSERT_EQ(rsp.GetContent(), "test-string-rsp");
  });
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  if (!::trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server_argv)) {
    exit(EXIT_FAILURE);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
