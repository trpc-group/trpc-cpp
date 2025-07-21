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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include <chrono>
#include <string>
#include <thread>

#include "test/end2end/unary/rpcz/rpcz_fixture.h"
#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"
#include "trpc/util/function.h"

namespace {

#define TEST_RPC_FUNC(func) \
  trpc::testing::rpcz::RpczRequest macro_req; \
  trpc::testing::rpcz::RpczReply macro_rsp; \
  trpc::ClientContextPtr macro_ctx = trpc::MakeClientContext(TestRpcz::rpcz_proxy_); \
  trpc::Status macro_status = func(macro_ctx, macro_req, &macro_rsp); \
  ASSERT_TRUE(macro_status.OK())

/// @brief Extract command line argv into global client/proxy/real_server argv.
bool ExtractArgs(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "./rpcz_test --client_config=c.yaml --config=p.yaml --config=r.yaml" << std::endl;
    return false;
  }

  trpc::Function<void(char*&, char*)> copy_fn = [](char*& dst, char* src) {
    int src_len = strlen(src);
    dst = new char[src_len + 1];
    strcpy(dst, src);
    dst[src_len] = 0;
  };

  test_argc = argc - 2;

  client_argv = new char*[test_argc];
  proxy_argv = new char*[test_argc];
  real_server_argv = new char*[test_argc];

  int idx_client = 0;
  int idx_proxy = 0;
  int idx_real_server = 0;
  for (int i = 0; i < argc; ++i) {
    if (i == 1) {
      // client configuration
      copy_fn((client_argv)[idx_client++], argv[i]);
      continue;
    } else if (i == 2) {
      // proxy configuration
      copy_fn((proxy_argv)[idx_proxy++], argv[i]);
      continue;
    } else if (i == 3) {
      // real server configuration
      copy_fn((real_server_argv)[idx_real_server++], argv[i]);
      continue;
    }
    // Copy all other parameters.
    copy_fn((client_argv)[idx_client++], argv[i]);
    copy_fn((proxy_argv)[idx_proxy++], argv[i]);
    copy_fn((real_server_argv)[idx_real_server++], argv[i]);
  }

  return true;
}

/// @brief Find out how many times one string contains another.
size_t ContainCount(std::string src, std::string key) {
  size_t count = 0;
  size_t pos = 0;
  while ((pos = src.find(key, pos)) != std::string::npos) {
    ++count;
    pos += key.length();
  }
  return count;
}

/// @brief Extract one span id from resource, with keyword in the same line.
/// @note Line format:
///       2023-06-01 15:30:42:1647   cost=229(us) span_type=S span_id=9 a.b.c.d/E request=105 response=31 [ok]
bool ExtractSpanId(std::string src, std::string key, std::string& span_id) {
  std::stringstream ss(src);
  std::string line;
  while (std::getline(ss, line, '\n')) {
    // Not matched line.
    if (line.find(key) == std::string::npos) {
      continue;
    }

    std::string delimiter = "span_id=";
    size_t pos = 0;
    // No span_id found.
    if ((pos = line.find(delimiter)) == std::string::npos) {
      continue;
    }

    line.erase(0, pos + delimiter.length());
    if ((pos = line.find(" ")) == std::string::npos) {
      continue;
    }

    // Found span id.
    span_id = line.substr(0, pos);
    return true;
  }

  return false;
}

struct OpEQ {
  void operator()(std::string rsp, std::string expected_rsp) const {
    ASSERT_EQ(rsp, expected_rsp);
  }
};

/// @brief One string ConTains another.
struct OpCT {
  void operator()(std::string rsp, std::string expected_rsp) const {
    ASSERT_TRUE(rsp.find(expected_rsp) != std::string::npos);
  }
};

constexpr const char kAdminUrlBase[] = "http://www.testrpcz.com/cmds/rpcz";

template <typename Op>
std::string TestHttpGetString(std::string tail_url, std::string expected_rsp) {
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(trpc::testing::TestRpcz::http_proxy_);
  std::string url = std::string(kAdminUrlBase) + tail_url;
  std::string rsp;
  trpc::Status status = trpc::testing::TestRpcz::http_proxy_->GetString(ctx, url, &rsp);
  EXPECT_TRUE(status.OK());
  Op()(rsp, expected_rsp);
  return rsp;
}

}  // namespace

namespace trpc::testing {

/// @brief 1. Get general info of rpcz.
///        2. No element found.
/// @note Must in front of any other testcases.
TEST_F(TestRpcz, GetRpczNotFound) {
  TestHttpGetString<OpEQ>("", "have no rpcz span");
}

/// @brief 1. Get a huge span id.
///        2. Span not found.
/// @note Must in front of any other testcases.
TEST_F(TestRpcz, GetSpanIdNotFound) {
  // Make span id huge.
  TestHttpGetString<OpEQ>("?span_id=10000000", "have no rpcz span");
}

/// @brief 1. Send rpc call N times.
///        2. Get general info of rpcz.
///        3. Found N times of proxy key indicating proxy sent N requests to rs, which
///           recorded in client type span.
TEST_F(TestRpcz, ClientGetRpczOk) {
  // Must no more than 5 to avoid server intercept rpcz.
  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ClientGetRpczOk);
  }

  // Let proxy take time to collect local spans.
  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "ProxyClientGetRpczOk");
  ASSERT_EQ(ContainCount(rsp, "ProxyClientGetRpczOk"), number);
}

/// @brief 1. Send rpc call N times.
///        2. Get general info of rpcz.
///        3. Extract one client span id from general info.
///        4. Get info of this span id.
///        5. Found keyword [start send request] in it.
TEST_F(TestRpcz, ClientGetSpanIdOk) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ClientGetSpanIdOk);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "ProxyClientGetSpanIdOk");
  std::string span_id;
  ASSERT_TRUE(ExtractSpanId(rsp, "ProxyClientGetSpanIdOk", span_id));
  TestHttpGetString<OpCT>("?span_id=" + span_id, "start send request");
}

/// @brief 1. Send rpc call N times.
///        2. Get general info of rpcz.
///        3. Get how many times client span appears.
///        4. Client spans less than N.
/// @note Stop sample is inside this case too!
TEST_F(TestRpcz, ClientProbableSample) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  /// @note Send 25 requests in burst, which must finished in one second, generating up to 50 spans
  ///       without proxy intercepting, in the worst situation, even the end of 1s window cut the
  ///       50 requests into two parts, proxy can not record all requests in rpcz as high level is
  ///       20, less than 25, we have to make it accurate as the 1s window is pretty pretty rough in proxy.
  int number = 25;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ClientProbableSample);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "ProxyClientProbableSample");
  ASSERT_LT(ContainCount(rsp, "ProxyClientProbableSample"), number);
}

/// @brief 1. Send rpc call 1 times.
///        2. Get general info of rpcz.
///        3. Get how many times client span appears.
///        4. Expected 1 times.
///        5. Sleep 13 seconds, to let proxy expired span.
///        6. Get general info of rpcz.
///        7. No element found.
TEST_F(TestRpcz, ClientSpanExpired) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ClientSpanExpired);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "ProxyClientSpanExpired");
  ASSERT_EQ(ContainCount(rsp, "ProxyClientSpanExpired"), number);

  std::this_thread::sleep_for(std::chrono::seconds(13));
  TestHttpGetString<OpEQ>("", "have no rpcz span");
}

/// @brief 1. Send rpc call to change client sample function.
///        2. Send rpc call 1 times.
///        3. Get general info of rpcz.
///        4. Get how many times client span appears.
///        5. Expected 0 time.
///        6. Send rpc call to restore client sample function.
TEST_F(TestRpcz, FireClientSampleFunction) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ChangeClientSampleFunction);
  }

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->FireClientSampleFunction);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  // No matter what, s is always inside general rpcz info.
  std::string rsp = TestHttpGetString<OpCT>("", "s");
  ASSERT_EQ(ContainCount(rsp, "ProxyFireClientSampleFunction"), 0);

  {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->RestoreClientSampleFunction);
  }
}

/// @brief 1. Send rpc call 11 times.
///        2. Get general info of rpcz.
///        3. Found 1 times of key indicating 1 requests to proxy, which
///           recorded in server type span.
TEST_F(TestRpcz, ServerGetRpczOk) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Must no more than 10 to avoid server intercept rpcz.
  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ServerGetRpczOk);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  // Add / prifix to distinguish from proxy.
  std::string rsp = TestHttpGetString<OpCT>("", "/ServerGetRpczOk");
  ASSERT_EQ(ContainCount(rsp, "/ServerGetRpczOk"), number);
}

/// @brief 1. Send rpc call 1 times.
///        2. Get general info of rpcz.
///        3. Extract one server span id from general info.
///        4. Get info of this span id.
///        5. Found keyword [Recieved request] in it.
TEST_F(TestRpcz, ServerGetSpanIdOk) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ServerGetSpanIdOk);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "/ServerGetSpanIdOk");
  std::string span_id;
  ASSERT_TRUE(ExtractSpanId(rsp, "/ServerGetSpanIdOk", span_id));
  TestHttpGetString<OpCT>("?span_id=" + span_id, "Received request");
}

/// @brief 1. Send rpc call N times.
///        2. Get general info of rpcz.
///        3. Get how many times server span appears.
///        4. Server spans less than N.
TEST_F(TestRpcz, ServerProbableSample) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Must more than 2 * [high level] = 40
  int number = 50;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ServerProbableSample);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "/ServerProbableSample");
  ASSERT_LT(ContainCount(rsp, "/ServerProbableSample"), number);
}

/// @brief 1. Send rpc call 1 times.
///        2. Get general info of rpcz.
///        3. Get how many times server span appears.
///        4. Expected 1 times.
///        5. Sleep 13 seconds, to let proxy expired span.
///        6. Get general info of rpcz.
///        7. No element found.
TEST_F(TestRpcz, ServerSpanExpired) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ServerSpanExpired);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "/ServerSpanExpired");
  ASSERT_EQ(ContainCount(rsp, "/ServerSpanExpired"), number);

  std::this_thread::sleep_for(std::chrono::seconds(13));
  TestHttpGetString<OpEQ>("", "have no rpcz span");
}

/// @brief 1. Send rpc call to change server sample function.
///        2. Send rpc call 1 times.
///        3. Get general info of rpcz.
///        4. Get how many times server span appears.
///        5. Expected 0 time.
///        6. Send rpc call to restore server sample function.
TEST_F(TestRpcz, FireServerSampleFunction) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ChangeServerSampleFunction);
  }

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->FireServerSampleFunction);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  // No matter what, s is always inside general rpcz info.
  std::string rsp = TestHttpGetString<OpCT>("", "s");
  ASSERT_EQ(ContainCount(rsp, "/FireServerSampleFunction"), 0);

  {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->RestoreServerSampleFunction);
  }
}

/// @brief 1. Send rpc call 1 times.
///        2. Get general info of rpcz.
///        3. Extract one server span id from general info.
///        4. Get info of this span id.
///        5. Found keyword [USER RPCZ PRINT] in it.
TEST_F(TestRpcz, ServerRpczPrint) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->ServerRpczPrint);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "/ServerRpczPrint");
  std::string span_id;
  ASSERT_TRUE(ExtractSpanId(rsp, "/ServerRpczPrint", span_id));
  TestHttpGetString<OpCT>("?span_id=" + span_id, "USER RPCZ PRINT");
}

/// @brief 1. Send rpc call 1 times.
///        2. Get general info of rpcz.
///        3. Found 1 times of server key indicating 1 requests to proxy, which
///           recorded in server type span.
TEST_F(TestRpcz, RouteGetRpczOk) {
  // Must no more than 10 to avoid server intercept rpcz.
  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->RouteGetRpczOk);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "/RouteGetRpczOk");
  ASSERT_EQ(ContainCount(rsp, "/RouteGetRpczOk"), number);
  // No client span expected.
  ASSERT_EQ(ContainCount(rsp, "ProxyRouteGetRpczOk"), 0);
}

/// @brief 1. Send rpc call 1 times.
///        2. Get general info of rpcz.
///        3. Extract one span id from general info.
///        4. Get info of this span id.
///        5. Found keyword [start send request] and [Recieved request] in it.
TEST_F(TestRpcz, RouteGetSpanIdOk) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->RouteGetSpanIdOk);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "/RouteGetSpanIdOk");
  std::string span_id;
  ASSERT_TRUE(ExtractSpanId(rsp, "/RouteGetSpanIdOk", span_id));
  rsp = TestHttpGetString<OpCT>("?span_id=" + span_id, "start send request");
  ASSERT_GE(ContainCount(rsp, "Received request"), 1);
}

/// @brief 1. Send rpc call N times.
///        2. Get general info of rpcz.
///        3. Get how many times span appears.
///        4. Spans less than N.
TEST_F(TestRpcz, RouteProbableSample) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Must more than 2 * [high level] = 40
  int number = 50;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->RouteProbableSample);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "/RouteProbableSample");
  ASSERT_LT(ContainCount(rsp, "/RouteProbableSample"), number);
}

/// @brief 1. Send rpc call 1 times.
///        2. Get general info of rpcz.
///        3. Get how many times span appears.
///        4. Expected 1 times.
///        5. Sleep 13 seconds, to let proxy expired span.
///        6. Get general info of rpcz.
///        7. No element found.
TEST_F(TestRpcz, RouteSpanExpired) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->RouteSpanExpired);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "/RouteSpanExpired");
  ASSERT_EQ(ContainCount(rsp, "/RouteSpanExpired"), number);

  std::this_thread::sleep_for(std::chrono::seconds(13));
  TestHttpGetString<OpEQ>("", "have no rpcz span");
}

/// @brief 1. Send rpc call 1 times.
///        2. Get general info of rpcz.
///        3. Extract one user span id from general info.
///        4. Get info of this span id.
///        5. Found keyword [root_span] in it.
TEST_F(TestRpcz, UserGetSpanIdOk) {
  // To avoid server intercept rpcz of aftercoming requests.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  int number = 1;
  for (int i = 0; i < number; i++) {
    TEST_RPC_FUNC(TestRpcz::rpcz_proxy_->UserGetSpanIdOk);
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::string rsp = TestHttpGetString<OpCT>("", "span_type=U");
  std::string span_id;
  ASSERT_TRUE(ExtractSpanId(rsp, "span_type=U", span_id));
  TestHttpGetString<OpCT>("?span_id=" + span_id, "root_span");
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  if (!ExtractArgs(argc, argv)) {
    exit(EXIT_FAILURE);
  }

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
