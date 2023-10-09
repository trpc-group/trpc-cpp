// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#include <string>
#include <vector>

#include "google/protobuf/arena.h"
#include "gtest/gtest.h"

#include "test/end2end/unary/trpc/common_funcs.h"
#include "test/end2end/unary/trpc/common_test.h"
#include "test/end2end/unary/trpc/trpc_server_transport_test_inc.h"
#include "test/end2end/unary/trpc/trpc_test.trpc.fb.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/runtime/init_runtime.h"
#include "trpc/util/function.h"
#include "trpc/util/string_helper.h"

namespace trpc::testing {

using namespace ::trpc::testing::unarytrpc;

class TrpcUnaryTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    CommonTest::SetUpTestEnv();
    tcp_transport_proxy_ = ::trpc::GetTrpcClient()->GetProxy<TrpcTransportServiceProxy>("tcp_transport_client");
    udp_transport_proxy_ = ::trpc::GetTrpcClient()->GetProxy<TrpcTransportServiceProxy>("udp_transport_client");

    trpc_proxy_ = ::trpc::GetTrpcClient()->GetProxy<TrpcTestServiceProxy>("trpc_test_client");
    fbs_proxy_ = ::trpc::GetTrpcClient()->GetProxy<FbsTestServiceProxy>("fbs_test_client");
    json_proxy_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::RpcServiceProxy>("json_test_client");
    noop_proxy_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::RpcServiceProxy>("noop_test_client");
    route_proxy_ = ::trpc::GetTrpcClient()->GetProxy<RouteTestServiceProxy>("route_test_client");
    transparent_proxy_ =
        ::trpc::GetTrpcClient()->GetProxy<::trpc::testing::unarytrpc::TrpcTestServiceProxy>("route_transparent_client");
    no_fulllink_route_proxy_ =
        ::trpc::GetTrpcClient()->GetProxy<RouteTestServiceProxy>("no_fulllink_route_test_client");
  }

  static void TearDownTestSuite() { CommonTest::TearDownTestEnv(); }

 public:
  // tcp_transport_proxy_ and udp_transport_proxy_ are used to test server transport
  static std::shared_ptr<TrpcTransportServiceProxy> tcp_transport_proxy_;
  static std::shared_ptr<TrpcTransportServiceProxy> udp_transport_proxy_;

  static std::shared_ptr<TrpcTestServiceProxy> trpc_proxy_;
  static std::shared_ptr<FbsTestServiceProxy> fbs_proxy_;
  static std::shared_ptr<::trpc::RpcServiceProxy> json_proxy_;
  static std::shared_ptr<::trpc::RpcServiceProxy> noop_proxy_;
  static std::shared_ptr<RouteTestServiceProxy> route_proxy_;
  static std::shared_ptr<::trpc::testing::unarytrpc::TrpcTestServiceProxy> transparent_proxy_;
  static std::shared_ptr<RouteTestServiceProxy> no_fulllink_route_proxy_;
};

std::shared_ptr<TrpcTransportServiceProxy> TrpcUnaryTest::tcp_transport_proxy_ = nullptr;
std::shared_ptr<TrpcTransportServiceProxy> TrpcUnaryTest::udp_transport_proxy_ = nullptr;
std::shared_ptr<TrpcTestServiceProxy> TrpcUnaryTest::trpc_proxy_ = nullptr;
std::shared_ptr<FbsTestServiceProxy> TrpcUnaryTest::fbs_proxy_ = nullptr;
std::shared_ptr<::trpc::RpcServiceProxy> TrpcUnaryTest::json_proxy_ = nullptr;
std::shared_ptr<::trpc::RpcServiceProxy> TrpcUnaryTest::noop_proxy_ = nullptr;
std::shared_ptr<RouteTestServiceProxy> TrpcUnaryTest::route_proxy_ = nullptr;
std::shared_ptr<::trpc::testing::unarytrpc::TrpcTestServiceProxy> TrpcUnaryTest::transparent_proxy_ = nullptr;
std::shared_ptr<RouteTestServiceProxy> TrpcUnaryTest::no_fulllink_route_proxy_ = nullptr;

// ----- server transport testcases -----

// Test Case:
//   Server returns failure when over maximum connection limit.
// Expected:
//   Success when under limit and fail when over limit.
TEST_F(TrpcUnaryTest, OverMaxConnLimit) {
  RunByEnv([&]() {
    CommonRequest request;
    request.set_msg("test");
    CommonReply reply;

    // Connect
    for (int i = 0; i <= kMaxConnNum; i++) {
      ServiceProxyOption option = *(tcp_transport_proxy_->GetServiceProxyOption());
      option.name = "tmp_" + std::to_string(i);
      auto prx = ::trpc::GetTrpcClient()->GetProxy<TrpcTransportServiceProxy>(option.name, &option);

      ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(prx);
      ctx->SetTimeout(10000);
      ::trpc::Status status = prx->TestCommon(ctx, request, &reply);
      if (i < kMaxConnNum) {
        ASSERT_TRUE(status.OK());
        ASSERT_EQ("test", reply.msg());
      } else {
        ASSERT_TRUE(!status.OK());
        if (::trpc::runtime::IsInFiberRuntime()) {
          ASSERT_EQ(status.GetFrameworkRetCode(), ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR);
        } else {
          ASSERT_TRUE(status.GetFrameworkRetCode() == ::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR ||
                      status.GetFrameworkRetCode() == ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR);
        }
      }
    }

    // Disconnect
    for (int i = 0; i <= kMaxConnNum; i++) {
      std::string prx_name = "tmp_" + std::to_string(i);
      auto prx = ::trpc::GetTrpcClient()->GetProxy<TrpcTransportServiceProxy>(prx_name);
      prx->Stop();
      prx->Destroy();
    }
  });
}

// Test Case:
//   Can invoke by tcp or udp when server listens both tcp and udp.
// Expected:
//   Invoke successfully.
TEST_F(TrpcUnaryTest, InvokeByTcpUdp) {
  RunByEnv([&]() {
    std::string msg = "hello, world!";
    CommonRequest request;
    request.set_msg(msg);
    CommonReply reply;

    std::vector<std::shared_ptr<TrpcTransportServiceProxy>> proxys = {tcp_transport_proxy_, udp_transport_proxy_};
    for (auto& proxy : proxys) {
      {
        ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
        ::trpc::Status status = proxy->TestCommon(ctx, request, &reply);
        ASSERT_TRUE(status.OK());
        ASSERT_EQ(msg, reply.msg());
      }
    }
  });
}

// Test Case:
//   Server returns failure when over maximum packet size limit.
// Expected:
//   Success when under limit and fail when over limit.
TEST_F(TrpcUnaryTest, OverMaxPacketSizeLimit) {
  RunByEnv([&]() {
    std::string under_limit_msg = "hello, world!";
    std::string over_limit_msg = "hello, world! hello, world!";

    CommonRequest request;
    CommonReply reply;

    std::vector<std::shared_ptr<TrpcTransportServiceProxy>> proxys = {tcp_transport_proxy_, udp_transport_proxy_};
    for (auto& proxy : proxys) {
      // under limit
      {
        request.set_msg(under_limit_msg);
        ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
        ::trpc::Status status = proxy->TestCommon(ctx, request, &reply);
        ASSERT_TRUE(status.OK());
        ASSERT_EQ(under_limit_msg, reply.msg());
      }

      // over limit
      {
        request.set_msg(over_limit_msg);
        ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
        ::trpc::Status status = proxy->TestCommon(ctx, request, &reply);
        ASSERT_FALSE(status.OK());
      }
    }
  });
}

// Test Case:
//   The UDP packet returned by the server cannot exceed the maximum UDP packet size.
// Expected:
//   Success when under limit and fail when over limit.
TEST_F(TrpcUnaryTest, UdpResponseSize) {
  RunByEnv([&]() {
    CommonRequest request;
    CommonReply reply;

    // under limit
    {
      request.set_msg(kUdpPacketUnderLimit);
      ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(udp_transport_proxy_);
      ::trpc::Status status = udp_transport_proxy_->TestUdpResponse(ctx, request, &reply);
      ASSERT_TRUE(status.OK());
    }

    // over limit
    {
      request.set_msg(kUdpPacketOverLimit);
      ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(udp_transport_proxy_);
      ::trpc::Status status = udp_transport_proxy_->TestUdpResponse(ctx, request, &reply);
      ASSERT_FALSE(status.OK());
      ASSERT_EQ(status.GetFrameworkRetCode(), ::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
    }
  });
}

// Test Case:
//   Requests can be dispatched to the same thread for execution by specifying the dispatcher.
// Expected:
//   Requests are executed in the same thread.
// Note:
//   Only supported in separate thread mode.
TEST_F(TrpcUnaryTest, CustomDispatcher) {
  RunByEnv([&]() {
    int invoke_times = 10;
    std::string thread_id;

    std::vector<std::shared_ptr<TrpcTransportServiceProxy>> proxys = {tcp_transport_proxy_, udp_transport_proxy_};
    for (auto& proxy : proxys) {
      for (int i = 0; i < invoke_times; i++) {
        CommonRequest request;
        CommonReply reply;
        request.set_msg("test");
        ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
        ::trpc::Status status = proxy->TestCustomDispatcher(ctx, request, &reply);
        ASSERT_TRUE(status.OK());
        if (i == 0) {
          thread_id = reply.msg();
          ASSERT_TRUE(!thread_id.empty());
        } else {
          ASSERT_TRUE(thread_id == reply.msg()) << thread_id << " " << reply.msg();
        }
      }
    }
  });
}

// Test Case:
//   When the connection reaches the server-configured idle time, it will be disconnected.
// Expected:
//   1. Call successfully when server processing time less than the idle time.
//   2. Call failed when the server processing time exceeds the idle time.
TEST_F(TrpcUnaryTest, ServerIdleTimeout) {
  RunByEnv([&]() {
    CommonRequest request;
    CommonReply reply;
    // Invoke successfully if delay response time less than idle time
    {
      uint32_t delay_response_ms = 200;
      request.set_msg(std::to_string(delay_response_ms));
      ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(tcp_transport_proxy_);
      ctx->SetTimeout(delay_response_ms + 1000);
      ::trpc::Status status = tcp_transport_proxy_->TestIdleTimeout(ctx, request, &reply);
      ASSERT_TRUE(status.OK());
    }

    // Invoke failed if delay response time bigger than idle time
    {
      // note: due to the fact that idle connections are cleaned up every 1 second in separate thread mode, it may
      // actually take 3 seconds to trigger the cleaning process, so an additional 2 seconds are added here.
      uint32_t delay_response_ms = kIdleTimeoutMs + 2000;
      request.set_msg(std::to_string(delay_response_ms));
      ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(tcp_transport_proxy_);
      // use a big timeout
      ctx->SetTimeout(delay_response_ms + 1000);
      ::trpc::Status status = tcp_transport_proxy_->TestIdleTimeout(ctx, request, &reply);
      ASSERT_TRUE(!status.OK());
      if (::trpc::runtime::IsInFiberRuntime()) {
        ASSERT_EQ(status.GetFrameworkRetCode(), ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR);
      } else {
        ASSERT_TRUE(status.GetFrameworkRetCode() == ::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR ||
                    status.GetFrameworkRetCode() == ::trpc::TrpcRetCode::TRPC_CLIENT_NETWORK_ERR);
      }
    }
  });
}

// Test Case:
//   The server adopts different ways of responding with packages.
// Expected:
//   1. Invoke timeout when server does not respond with a package.
//   2. Invoke successfully when server respond with a package using sync or async way.
TEST_F(TrpcUnaryTest, DifferResponseWayByServer) {
  RunByEnv([&]() {
    CommonRequest request;
    CommonReply reply;

    std::vector<std::shared_ptr<TrpcTransportServiceProxy>> proxys = {tcp_transport_proxy_, udp_transport_proxy_};
    for (auto& proxy : proxys) {
      // invoke timeout without response
      {
        request.set_msg(kNoResponse);
        ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
        ctx->SetTimeout(1000);
        ::trpc::Status status = proxy->TestDifferResponseWay(ctx, request, &reply);
        ASSERT_TRUE(!status.OK() &&
                    status.GetFrameworkRetCode() == ::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
      }

      // invoke successfully with sync response
      {
        request.set_msg(kSyncResponse);
        ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
        ::trpc::Status status = proxy->TestDifferResponseWay(ctx, request, &reply);
        ASSERT_TRUE(status.OK());
      }

      // invoke successfully with async response
      {
        request.set_msg(kAsyncResponse);
        ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
        ::trpc::Status status = proxy->TestDifferResponseWay(ctx, request, &reply);
        ASSERT_TRUE(status.OK());
      }
    }
  });
}

// Test Case:
//   The call type obtained by the server can match the call type of the client.
// Expected:
//   1. Get RpcCallType::ONEWAY_CALL when received a oneway request.
//   2. Get RpcCallType::UNARY_CALL when received a oneway request.
TEST_F(TrpcUnaryTest, DifferCallType) {
  RunByEnv([&]() {
    CommonRequest request;
    CommonReply reply;

    std::vector<std::shared_ptr<TrpcTransportServiceProxy>> proxys = {tcp_transport_proxy_, udp_transport_proxy_};
    for (auto& proxy : proxys) {
      // oneway request
      {
        request.set_msg(kOnewayRequest);
        ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
        ::trpc::Status status = proxy->TestDifferCallType(ctx, request);
        ASSERT_TRUE(status.OK());
      }

      // unary request
      {
        request.set_msg(kUnaryRequest);
        ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
        ::trpc::Status status = proxy->TestDifferCallType(ctx, request, &reply);
        ASSERT_TRUE(status.OK());
        ASSERT_TRUE(reply.msg() == request.msg());
      }
    }
  });
}

#ifdef TRPC_INCLUDE_UDS_TEST

// construct a trpc binary packet
size_t FillTrpcRequestData(::trpc::TrpcRequestProtocol& req) {
  req.fixed_header.magic_value = ::trpc::TrpcMagic::TRPC_MAGIC_VALUE;
  req.req_header.set_timeout(1000);
  req.req_header.set_caller("test_client");
  req.req_header.set_callee("trpc.testing.unarytrpc.TrpcTransport");
  req.req_header.set_func("/trpc.testing.unarytrpc.TrpcTransport/TestCommon");

  uint32_t req_header_size = req.req_header.ByteSizeLong();
  req.fixed_header.pb_header_size = req_header_size;

  CommonRequest request;
  request.set_msg("test");
  std::string body_str = request.SerializeAsString();
  size_t encode_buff_size = ::trpc::TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + req_header_size + body_str.size();
  req.fixed_header.data_frame_size = encode_buff_size;
  req.req_body = CreateBufferSlow(body_str);

  return encode_buff_size;
}

// Test Case:
//   Server has support listen unix domain socket.
// Expected:
//   Invoke successfully using unix domain socket.
TEST_F(TrpcUnaryTest, ListenUds) {
  ::trpc::TrpcRequestProtocol req;

  size_t encode_size = FillTrpcRequestData(req);
  ::trpc::NoncontiguousBuffer buff;
  ASSERT_TRUE(req.ZeroCopyEncode(buff));
  ASSERT_EQ(encode_size, buff.ByteSize());

  auto socket = std::make_unique<::trpc::Socket>(::trpc::Socket::CreateUnixSocket());
  ::trpc::UnixAddress addr(kUdsPath);
  int ret = socket->Connect(addr);
  ASSERT_EQ(ret, 0);

  ASSERT_TRUE(buff.size() < 4);
  struct iovec iov[4];
  int index = 0;
  for (auto iter = buff.begin(); iter != buff.end(); ++iter, index++) {
    iov[index].iov_base = iter->data();
    iov[index].iov_len = iter->size();
  }

  int send_size = socket->Writev(iov, index);
  ASSERT_TRUE(send_size > 0);

  char recvbuf[1024] = {0};
  int recv_size = socket->Recv(recvbuf, sizeof(recvbuf));
  ASSERT_TRUE(recv_size > 0);

  socket->Close();
}
#endif

// ----- compressor testcases -----

void TestCompressor(::trpc::compressor::CompressType req_type, ::trpc::compressor::LevelType req_level,
                    ::trpc::Function<void(CompressorRequest& request)> req_setup_func,
                    ::trpc::Function<void(const ::trpc::Status& status, const ::trpc::ClientContextPtr& ctx,
                                          const CompressorResponse& reply)>
                        check_func) {
  CompressorRequest request;
  req_setup_func(request);
  CompressorResponse reply;

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
  ctx->SetReqCompressType(req_type);
  ctx->SetReqCompressLevel(req_level);
  ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestCompressor(ctx, request, &reply);

  check_func(status, ctx, reply);
}

void TestCompressorWithDefaultBehavior(::trpc::compressor::CompressType req_type,
                                       ::trpc::compressor::LevelType req_level) {
  TestCompressor(
      req_type, req_level,
      [](CompressorRequest& request) {
        request.set_test_case_type(CompressorRequest::DEFAULT);
        request.set_msg("test");
      },
      [req_type](const ::trpc::Status& status, const ::trpc::ClientContextPtr& ctx, const CompressorResponse& reply) {
        ASSERT_TRUE(status.OK());
        ASSERT_EQ(req_type, ctx->GetRspCompressType());
        ASSERT_EQ("test", reply.msg());
      });
}

void TestCompressorWithSpecifiedLevel(::trpc::compressor::CompressType req_type,
                                      ::trpc::compressor::LevelType req_level,
                                      ::trpc::compressor::LevelType expected_rsp_level) {
  TestCompressor(
      req_type, req_level,
      [expected_rsp_level](CompressorRequest& request) {
        request.set_test_case_type(CompressorRequest::SPECIFY_LEVEL);
        request.set_expected_compress_level(expected_rsp_level);
        request.set_msg("test");
      },
      [req_type](const ::trpc::Status& status, const ::trpc::ClientContextPtr& ctx, const CompressorResponse& reply) {
        ASSERT_TRUE(status.OK());
        ASSERT_EQ(req_type, ctx->GetRspCompressType());
        ASSERT_EQ("test", reply.msg());
      });
}

void TestCompressorWithSpecifiedAlgorithm(::trpc::compressor::CompressType req_type,
                                          ::trpc::compressor::LevelType req_level,
                                          ::trpc::compressor::CompressType expected_rsp_type,
                                          ::trpc::compressor::LevelType expected_rsp_level) {
  TestCompressor(
      req_type, req_level,
      [expected_rsp_type, expected_rsp_level](CompressorRequest& request) {
        request.set_test_case_type(CompressorRequest::SPECIFY_ALGORITHM);
        request.set_expected_compress_type(expected_rsp_type);
        request.set_expected_compress_level(expected_rsp_level);
        request.set_msg("test");
      },
      [expected_rsp_type](const ::trpc::Status& status, const ::trpc::ClientContextPtr& ctx,
                          const CompressorResponse& reply) {
        ASSERT_TRUE(status.OK());
        ASSERT_EQ(expected_rsp_type, ctx->GetRspCompressType());
        ASSERT_EQ("test", reply.msg());
      });
}

void TestCompressorWithServerNotCompress(::trpc::compressor::CompressType req_type,
                                         ::trpc::compressor::LevelType req_level) {
  TestCompressor(
      req_type, req_level,
      [](CompressorRequest& request) {
        request.set_test_case_type(CompressorRequest::NOT_COMPRESS);
        request.set_msg("test");
      },
      [](const ::trpc::Status& status, const ::trpc::ClientContextPtr& ctx, const CompressorResponse& reply) {
        ASSERT_TRUE(status.OK());
        ASSERT_EQ(::trpc::compressor::kNone, ctx->GetRspCompressType());
        ASSERT_EQ("test", reply.msg());
      });
}

// Test Case:
//   The client uses kZlib compression type and kFastest compression level to make the request, while the server will
//   respond with the default compression behavior
// Expected:
//   Call successfully. The response was compressed using kZlib compression type, kDefault compression level
TEST_F(TrpcUnaryTest, CompressorZlibFastest) {
  RunByEnv([&]() { TestCompressorWithDefaultBehavior(::trpc::compressor::kZlib, ::trpc::compressor::kFastest); });
}

// Test Case:
//   The client uses kZlib compression type and kDefault compression level to make the request, while the server will
//   respond with the default compression behavior
// Expected:
//   Call successfully. The response was compressed using kZlib compression type, kDefault compression level
TEST_F(TrpcUnaryTest, CompressorZlibDefault) {
  RunByEnv([&]() { TestCompressorWithDefaultBehavior(::trpc::compressor::kZlib, ::trpc::compressor::kDefault); });
}

// Test Case:
//   The client uses kZlib compression type and kBest compression level to make the request, while the server will
//   respond with the default compression behavior
// Expected:
//   Call successfully. The response was compressed using kZlib compression type, kDefault compression level
TEST_F(TrpcUnaryTest, CompressorZlibBest) {
  RunByEnv([&]() { TestCompressorWithDefaultBehavior(::trpc::compressor::kZlib, ::trpc::compressor::kBest); });
}

// Test Case:
//   The client uses kZlib compression type and kDefault compression level to make the request, while the server will
//   respond with compression level modified to kFastest
// Expected:
//   Call successfully. The response was compressed using the kZlib compression type, kFastest compression level
TEST_F(TrpcUnaryTest, CompressorZlibServerSpecifyLevel) {
  RunByEnv([&]() {
    TestCompressorWithSpecifiedLevel(::trpc::compressor::kZlib, ::trpc::compressor::kDefault,
                                     ::trpc::compressor::kFastest);
  });
}

// Test Case:
//   The client uses kZlib compression type and kDefault compression level to make the request, while the server will
//   respond without using a compressor
// Expected:
//   Call successfully. The response content is not compressed
TEST_F(TrpcUnaryTest, CompressorZlibServerNotCompress) {
  RunByEnv([&]() { TestCompressorWithServerNotCompress(::trpc::compressor::kZlib, ::trpc::compressor::kDefault); });
}

// Test Case:
//   The client do not use compressor to make the request, while the server will respond with compression type and
//   compression level modified to kZlib and kDefault
// Expected:
//   Call successfully. The response was compressed using kZlib compression type, kDefault compression level
TEST_F(TrpcUnaryTest, CompressorZlibServerSpecifyAlgorithm) {
  RunByEnv([&]() {
    TestCompressorWithSpecifiedAlgorithm(::trpc::compressor::kNone, ::trpc::compressor::kDefault,
                                         ::trpc::compressor::kZlib, ::trpc::compressor::kDefault);
  });
}

// Test Case:
//   The client uses kGzip compression type and kFastest compression level to make the request, while the server will
//   respond with the default compression behavior
// Expected:
//   Call successfully. The response was compressed using kGzip compression type, kDefault compression level
TEST_F(TrpcUnaryTest, CompressorGzipFastest) {
  RunByEnv([&]() { TestCompressorWithDefaultBehavior(::trpc::compressor::kGzip, ::trpc::compressor::kFastest); });
}

// Test Case:
//   The client uses kGzip compression type and kDefault compression level to make the request, while the server will
//   respond with the default compression behavior
// Expected:
//   Call successfully. The response was compressed using kGzip compression type, kDefault compression level
TEST_F(TrpcUnaryTest, CompressorGzipDefault) {
  RunByEnv([&]() { TestCompressorWithDefaultBehavior(::trpc::compressor::kGzip, ::trpc::compressor::kDefault); });
}

// Test Case:
//   The client uses kGzip compression type and kBest compression level to make the request, while the server will
//   respond with the default compression behavior
// Expected:
//   Call successfully. The response was compressed using kGzip compression type, kDefault compression level
TEST_F(TrpcUnaryTest, CompressorGzipBest) {
  RunByEnv([&]() { TestCompressorWithDefaultBehavior(::trpc::compressor::kGzip, ::trpc::compressor::kBest); });
}

// Test Case:
//   The client uses kGzip compression type and kDefault compression level to make the request, while the server will
//   respond with compression level modified to kFastest
// Expected:
//   Call successfully. The response was compressed using the kGzip compression type, kFastest compression level
TEST_F(TrpcUnaryTest, CompressorGzipServerSpecifyLevel) {
  RunByEnv([&]() {
    TestCompressorWithSpecifiedLevel(::trpc::compressor::kGzip, ::trpc::compressor::kDefault,
                                     ::trpc::compressor::kFastest);
  });
}

// Test Case:
//   The client uses kGzip compression type and kDefault compression level to make the request, while the server will
//   respond without using a compressor
// Expected:
//   Call successfully. The response content is not compressed
TEST_F(TrpcUnaryTest, CompressorGzipServerNotCompress) {
  RunByEnv([&]() { TestCompressorWithServerNotCompress(::trpc::compressor::kGzip, ::trpc::compressor::kDefault); });
}

// Test Case:
//   The client do not use compressor to make the request, while the server will respond with compression type and
//   compression level modified to kGzip and kDefault
// Expected:
//   Call successfully. The response was compressed using kGzip compression type, kDefault compression level
TEST_F(TrpcUnaryTest, CompressorGzipServerSpecifyAlgorithm) {
  RunByEnv([&]() {
    TestCompressorWithSpecifiedAlgorithm(::trpc::compressor::kNone, ::trpc::compressor::kDefault,
                                         ::trpc::compressor::kGzip, ::trpc::compressor::kDefault);
  });
}

// Test Case:
//   The client uses kSnappy compression type to make the request, while the server will respond with the default
//   compression behavior
// Expected:
//   Call successfully. The response was compressed using kSnappy compression type
TEST_F(TrpcUnaryTest, CompressorSnappy) {
  RunByEnv([&]() { TestCompressorWithDefaultBehavior(::trpc::compressor::kSnappy, ::trpc::compressor::kDefault); });
}

// Test Case:
//   The client uses kSnappyBlock compression type to make the request, while the server will respond with the
//   default compression behavior
// Expected:
//   Call successfully. The response was compressed using kSnappyBlock compression type
TEST_F(TrpcUnaryTest, CompressorSnappyBlock) {
  RunByEnv(
      [&]() { TestCompressorWithDefaultBehavior(::trpc::compressor::kSnappyBlock, ::trpc::compressor::kDefault); });
}

// Test Case:
//   The client uses kSnappy compression type to make the request, while the server will respond without using a
//   compressor
// Expected:
//   Call successfully. The response content is not compressed
TEST_F(TrpcUnaryTest, CompressorSnappyServerNotCompress) {
  RunByEnv([&]() { TestCompressorWithServerNotCompress(::trpc::compressor::kSnappy, ::trpc::compressor::kDefault); });
}

// Test Case:
//   The client do not use compressor to make the request, while the server will respond with compression type
//   modified to kSnappy
// Expected:
//   Call successfully. The response was compressed using kSnappy compression type
TEST_F(TrpcUnaryTest, CompressorSnappyServerSpecifyAlgorithm) {
  RunByEnv([&]() {
    TestCompressorWithSpecifiedAlgorithm(::trpc::compressor::kNone, ::trpc::compressor::kDefault,
                                         ::trpc::compressor::kSnappy, ::trpc::compressor::kDefault);
  });
}

// Test Case:
//   The client uses kLz4Frame compression type to make the request, while the server will respond with the default
//   compression behavior
// Expected:
//   Call successfully. The response was compressed using kLz4Frame compression type
TEST_F(TrpcUnaryTest, CompressorLz4) {
  RunByEnv([&]() { TestCompressorWithDefaultBehavior(::trpc::compressor::kLz4Frame, ::trpc::compressor::kDefault); });
}

// Test Case:
//   The client uses kLz4Frame compression type to make the request, while the server will respond without using a
//   compressor
// Expected:
//   Call successfully. The response content is not compressed
TEST_F(TrpcUnaryTest, CompressorLz4ServerNotCompress) {
  RunByEnv([&]() { TestCompressorWithServerNotCompress(::trpc::compressor::kLz4Frame, ::trpc::compressor::kDefault); });
}

// Test Case:
//   The client do not use compressor to make the request, while the server will respond with compression type
//   modified to kLz4Frame
// Expected:
//   Call successfully. The response was compressed using kLz4Frame compression type
TEST_F(TrpcUnaryTest, CompressorLz4ServerSpecifyAlgorithm) {
  RunByEnv([&]() {
    TestCompressorWithSpecifiedAlgorithm(::trpc::compressor::kNone, ::trpc::compressor::kDefault,
                                         ::trpc::compressor::kLz4Frame, ::trpc::compressor::kDefault);
  });
}

// Test Case:
//   The client uses kZlib compression type and kFastest compression level to make the request, while the server will
//   respond with compression type and compression level modified to kGzip and kBest
// Expected:
//   Call successfully. The response was compressed using kGzip compression type, kBest compression level
TEST_F(TrpcUnaryTest, CompressorZlibGzip) {
  RunByEnv([&]() {
    TestCompressorWithSpecifiedAlgorithm(::trpc::compressor::kZlib, ::trpc::compressor::kFastest,
                                         ::trpc::compressor::kGzip, ::trpc::compressor::kBest);
  });
}

// Test Case:
//   The client uses invalid compression type and valid compression level to make the request
// Expected:
//   The client's request encoding is incorrect, returning TRPC_CLIENT_ENCODE_ERR error code
TEST_F(TrpcUnaryTest, CompressorClientInvalidType) {
  RunByEnv([&]() {
    TestCompressor(
        std::numeric_limits<uint8_t>::max(), ::trpc::compressor::kDefault,
        [](CompressorRequest& request) { request.set_test_case_type(CompressorRequest::DEFAULT); },
        [](const ::trpc::Status& status, const ::trpc::ClientContextPtr& ctx, const CompressorResponse& reply) {
          ASSERT_FALSE(status.OK());
          ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, status.GetFrameworkRetCode());
        });
  });
}

// Test Case:
//   The client uses valid compression type and invalid compression level to make the request
// Expected:
//   Call successfully. (The framework corrected an invalid compression level to the kDefault level)
TEST_F(TrpcUnaryTest, CompressorClientInvalidLevel) {
  RunByEnv([&]() {
    TestCompressor(
        ::trpc::compressor::kZlib, std::numeric_limits<uint8_t>::max(),
        [](CompressorRequest& request) { request.set_test_case_type(CompressorRequest::DEFAULT); },
        [](const ::trpc::Status& status, const ::trpc::ClientContextPtr& ctx, const CompressorResponse& reply) {
          ASSERT_TRUE(status.OK());
          ASSERT_EQ(::trpc::compressor::kZlib, ctx->GetRspCompressType());
        });
  });
}

// Test Case:
//   The server responds with invalid compression type
// Expected:
//   The server's response encoding is incorrect, returning TRPC_SERVER_ENCODE_ERR error code
TEST_F(TrpcUnaryTest, CompressorServerInvalidType) {
  RunByEnv([&]() {
    TestCompressor(
        ::trpc::compressor::kZlib, ::trpc::compressor::kDefault,
        [](CompressorRequest& request) { request.set_test_case_type(CompressorRequest::INVALID_TYPE); },
        [](const ::trpc::Status& status, const ::trpc::ClientContextPtr& ctx, const CompressorResponse& reply) {
          ASSERT_FALSE(status.OK());
          ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_ENCODE_ERR, status.GetFrameworkRetCode());
        });
  });
}

// Test Case:
//   The server responds with invalid compression level
// Expected:
//   Call successfully. (The framework corrected an invalid compression level to the kDefault level)
TEST_F(TrpcUnaryTest, CompressorServerInvalidLevel) {
  RunByEnv([&]() {
    TestCompressor(
        ::trpc::compressor::kZlib, ::trpc::compressor::kDefault,
        [](CompressorRequest& request) { request.set_test_case_type(CompressorRequest::INVALID_LEVEL); },
        [](const ::trpc::Status& status, const ::trpc::ClientContextPtr& ctx, const CompressorResponse& reply) {
          ASSERT_TRUE(status.OK());
        });
  });
}

// ----- serialization testcases -----

SerializationRequest CreateSerializationRequest(SerializationRequest::TestCaseType type) {
  SerializationRequest request;
  request.set_test_case_type(type);
  request.set_msg("test");
  return request;
}

// Test Case:
//   The client's EncodeType is set to kPbType, and the request data is pb data. The server responds with pb data.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ClientPbMessage) {
  RunByEnv([&]() {
    SerializationRequest request = CreateSerializationRequest(SerializationRequest::PB_PB);
    SerializationResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestPbSerialization(ctx, request, &reply);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", reply.msg());
  });
}

// Test Case:
//   The client's EncodeType is set to kPbType, and the request data is NonContiguousBuffer which has been
//   serialized. The server responds with pb data.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ClientPbNonContiguousBuffer) {
  RunByEnv([&]() {
    SerializationRequest request = CreateSerializationRequest(SerializationRequest::PB_PB);
    SerializationResponse reply;

    ::trpc::serialization::Serialization* pb_serialization =
        ::trpc::serialization::SerializationFactory::GetInstance()->Get(::trpc::EncodeType::PB);
    ::trpc::NoncontiguousBuffer req_buffer;
    bool encode_ret =
        pb_serialization->Serialize(trpc::serialization::kPbMessage, reinterpret_cast<void*>(&request), &req_buffer);
    ASSERT_TRUE(encode_ret);

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.TrpcTest/TestPbSerialization");
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->PbSerializedReqUnaryInvoke(ctx, std::move(req_buffer), &reply);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", reply.msg());
  });
}

// Test Case:
//   The client's EncodeType is set to kPbType, and the request data is pb data. The server responds with
//   NonContiguousBuffer which has been serialized.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ServerPbNonContiguousBuffer) {
  RunByEnv([&]() {
    SerializationRequest request = CreateSerializationRequest(SerializationRequest::PB_NONCONTIGUOUSBUFFER);
    SerializationResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestPbSerialization(ctx, request, &reply);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", reply.msg());
  });
}

// Test Case:
//   The client sends the request using invalid binary data.
// Expected:
//   Call failed. Client received TRPC_SERVER_DECODE_ERR error.
TEST_F(TrpcUnaryTest, ClientInvalidPbData) {
  RunByEnv([&]() {
    SerializationResponse reply;
    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);

    ::trpc::NoncontiguousBuffer req_buffer = ::trpc::CreateBufferSlow("invalid data");
    ctx->SetFuncName("/trpc.testing.unarytrpc.TrpcTest/TestPbSerialization");
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->PbSerializedReqUnaryInvoke(ctx, std::move(req_buffer), &reply);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_DECODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The server responded with the invalid binary data.
// Expected:
//   Call failed. Client received TRPC_CLIENT_DECODE_ERR error.
TEST_F(TrpcUnaryTest, ServerInvalidPbData) {
  RunByEnv([&]() {
    SerializationRequest request = CreateSerializationRequest(SerializationRequest::INVALID_DATA);
    SerializationResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestPbSerialization(ctx, request, &reply);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_DECODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The server responded with the invalid encode data type.
// Expected:
//   Call failed. Client received TRPC_SERVER_ENCODE_ERR error.
TEST_F(TrpcUnaryTest, ServerInvalidType) {
  RunByEnv([&]() {
    SerializationRequest request = CreateSerializationRequest(SerializationRequest::INVALID_TYPE);
    SerializationResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestPbSerialization(ctx, request, &reply);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_ENCODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The server responded with the kNoopType data while the client expect kPbType data.
// Expected:
//   Call failed. Client received TRPC_CLIENT_DECODE_ERR error.
TEST_F(TrpcUnaryTest, InvalidRspType) {
  RunByEnv([&]() {
    SerializationRequest request = CreateSerializationRequest(SerializationRequest::NOOP_STRING);
    SerializationResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestPbSerialization(ctx, request, &reply);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_DECODE_ERR, status.GetFrameworkRetCode());
  });
}

rapidjson::Document CreateTestJsonRequest() {
  rapidjson::Document request;
  std::string req_str = "{\"msg\":\"test\"}";
  request.Parse(req_str.c_str());
  return request;
}

void CheckJsonSuccessResult(rapidjson::Document& response) {
  ASSERT_TRUE(response.IsObject());
  ASSERT_TRUE(response.HasMember("msg"));
  ASSERT_STREQ("test", response["msg"].GetString());
}

// Test Case:
//   The client's EncodeType is set to kJsonType, and the request data is RapidJson data. The server responds with
//   RapidJson data.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ClientRapidJson) {
  RunByEnv([&]() {
    rapidjson::Document request = CreateTestJsonRequest();
    rapidjson::Document response;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::json_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.JsonTest/TestRapidJson");
    ctx->SetEncodeType(trpc::serialization::kJsonType);
    ::trpc::Status status = TrpcUnaryTest::json_proxy_->UnaryInvoke(ctx, request, &response);

    ASSERT_TRUE(status.OK());
    CheckJsonSuccessResult(response);
  });
}

// Test Case:
//   The client's EncodeType is set to kJsonType, and the request data is pb data. The server responds with
//   RapidJson data.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ClientPbJson) {
  RunByEnv([&]() {
    CommonRequest request;
    request.set_msg("test");
    rapidjson::Document response;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::json_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.JsonTest/TestRapidJson");
    ctx->SetEncodeType(trpc::serialization::kJsonType);
    ::trpc::Status status = TrpcUnaryTest::json_proxy_->UnaryInvoke(ctx, request, &response);

    ASSERT_TRUE(status.OK());
    CheckJsonSuccessResult(response);
  });
}

// Test Case:
//   The client's EncodeType is set to kJsonType, and the request data is RapidJson data. The server responds with
//   pb data.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ServerPbJson) {
  RunByEnv([&]() {
    rapidjson::Document request = CreateTestJsonRequest();
    rapidjson::Document response;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::json_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.JsonTest/TestPbJson");
    ::trpc::Status status = TrpcUnaryTest::json_proxy_->UnaryInvoke(ctx, request, &response);

    ASSERT_TRUE(status.OK());
    CheckJsonSuccessResult(response);
  });
}

// Test Case:
//   The client's EncodeType is set to kJsonType, and the request data is string data.
// Expected:
//   Call failed. kJsonType only support RapidJson and pb data.
TEST_F(TrpcUnaryTest, JsonInvalidEncodeDataType) {
  RunByEnv([&]() {
    std::string request = "test";
    rapidjson::Document response;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::json_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.JsonTest/TestRapidJson");
    ctx->SetEncodeType(trpc::serialization::kJsonType);
    ::trpc::Status status = TrpcUnaryTest::json_proxy_->UnaryInvoke(ctx, request, &response);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The client's EncodeType is set to kNoopType, and the request data is string data. The server responds with
//   string data.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ClientStringNoop) {
  RunByEnv([&]() {
    std::string request = "test";
    std::string response;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::noop_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.NoopTest/TestStringNoop");
    ctx->SetEncodeType(trpc::serialization::kNoopType);
    ::trpc::Status status = TrpcUnaryTest::noop_proxy_->UnaryInvoke(ctx, request, &response);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", response);
  });
}

// Test Case:
//   The client's EncodeType is set to kNoopType, and the request data is NoncontiguousBuffer data. The server responds
//   with string data.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ClientNoncontiguousBufferNoop) {
  RunByEnv([&]() {
    std::string req_str = "test";
    ::trpc::NoncontiguousBuffer request = ::trpc::CreateBufferSlow(req_str.c_str(), req_str.size());
    std::string response;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::noop_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.NoopTest/TestStringNoop");
    ctx->SetEncodeType(trpc::serialization::kNoopType);
    ::trpc::Status status = TrpcUnaryTest::noop_proxy_->UnaryInvoke(ctx, request, &response);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", response);
  });
}

// Test Case:
//   The client's EncodeType is set to kNoopType, and the request data is string data. The server responds
//   with NoncontiguousBuffer data.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ServerNoncontiguousBufferNoop) {
  RunByEnv([&]() {
    std::string request = "test";
    std::string response;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::noop_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.NoopTest/TestNonContiguousBufferNoop");
    ctx->SetEncodeType(trpc::serialization::kNoopType);
    ::trpc::Status status = TrpcUnaryTest::noop_proxy_->UnaryInvoke(ctx, request, &response);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", response);
  });
}

// Test Case:
//   The client's EncodeType is set to kNoopType, and the request data is pb data.
// Expected:
//   Call failed. kNoopType only support string and NonContiguousBuffer data.
TEST_F(TrpcUnaryTest, NoopInvalidEncodeDataType) {
  RunByEnv([&]() {
    CommonRequest request;
    std::string response;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::noop_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.NoopTest/TestStringNoop");
    ctx->SetEncodeType(trpc::serialization::kNoopType);
    ::trpc::Status status = TrpcUnaryTest::noop_proxy_->UnaryInvoke(ctx, request, &response);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The client sends the request using invalid encode type.
// Expected:
//   Call failed. Client received TRPC_CLIENT_ENCODE_ERR error.
TEST_F(TrpcUnaryTest, ClientInvalidEncodeType) {
  RunByEnv([&]() {
    SerializationRequest request = CreateSerializationRequest(SerializationRequest::PB_PB);
    SerializationResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ctx->SetEncodeType(std::numeric_limits<uint8_t>::max());
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestPbSerialization(ctx, request, &reply);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The server responded with not exist data type.
// Expected:
//   Call failed. Client received TRPC_SERVER_ENCODE_ERR error.
TEST_F(TrpcUnaryTest, ServerInvalidEncodeType) {
  RunByEnv([&]() {
    SerializationRequest request = CreateSerializationRequest(SerializationRequest::NOT_EXIST_TYPE);
    SerializationResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestPbSerialization(ctx, request, &reply);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_ENCODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The client sends the request using data with invalid type.
// Expected:
//   Call failed. Client received TRPC_CLIENT_ENCODE_ERR error.
TEST_F(TrpcUnaryTest, ClientInvalidRequestEncodeDataType) {
  RunByEnv([&]() {
    int request = 0;
    SerializationResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.TrpcTest/TestPbSerialization");
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->UnaryInvoke(ctx, request, &reply);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The client expects the response data  with invalid type.
// Expected:
//   Call failed. Client received TRPC_CLIENT_ENCODE_ERR error.
TEST_F(TrpcUnaryTest, ClientInvalidResponseEncodeDataType) {
  RunByEnv([&]() {
    SerializationRequest request;
    request.set_test_case_type(SerializationRequest::PB_PB);
    int reply = 0;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ctx->SetFuncName("/trpc.testing.unarytrpc.TrpcTest/TestPbSerialization");
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->UnaryInvoke(ctx, request, &reply);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, status.GetFrameworkRetCode());
  });
}

// ----- arena testcases -----

#ifdef TRPC_PROTO_USE_ARENA
// Test Case:
//   The client calls the server in a normal way.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ClientNotUseArena) {
  RunByEnv([&]() {
    CommonRequest request;
    request.set_msg("test");
    CommonReply reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestCommon(ctx, request, &reply);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", reply.msg());
  });
}

// Test Case:
//   The client creates the request and response using arena and then calls the server.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, ClientUseArena) {
  RunByEnv([&]() {
    google::protobuf::Arena arena_req;
    auto* request = google::protobuf::Arena::CreateMessage<CommonRequest>(&arena_req);
    request->set_msg("test");
    google::protobuf::Arena arena_rsp;
    auto* reply = google::protobuf::Arena::CreateMessage<CommonReply>(&arena_rsp);

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestCommon(ctx, *request, reply);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", reply->msg());
  });
}

// Test Case:
//   The client creates the request and response using arena and then calls the server after setting ArenaOptions.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, SetArenaOptions) {
  RunByEnv([&]() {
    google::protobuf::ArenaOptions options;
    options.start_block_size = 10240;

    google::protobuf::Arena arena_req(options);
    auto* request = google::protobuf::Arena::CreateMessage<CommonRequest>(&arena_req);
    request->set_msg("test");
    google::protobuf::Arena arena_rsp(options);
    auto* reply = google::protobuf::Arena::CreateMessage<CommonReply>(&arena_rsp);

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestCommon(ctx, *request, reply);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", reply->msg());
  });
}
#endif

// ----- flatbuffers testcases -----

::flatbuffers::trpc::Message<FbsRequest> CreateFbsTestRequest(FbsTestCaseType type) {
  ::flatbuffers::trpc::MessageBuilder mb;
  auto msg_offset = mb.CreateString("test");
  auto request_offset = CreateFbsRequest(mb, type, msg_offset);
  mb.Finish(request_offset);
  return mb.ReleaseMessage<FbsRequest>();
}

// Test Case:
//   The client sends the request in the correct format of MessageFbs data, and the server responds in the correct
//   format of MessageFbs data.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, FbsSuccess) {
  RunByEnv([&]() {
    ::flatbuffers::trpc::Message<FbsRequest> request_msg = CreateFbsTestRequest(FbsTestCaseType_FORMAL);
    ::flatbuffers::trpc::Message<FbsResponse> response_msg;

    auto context = ::trpc::MakeClientContext(fbs_proxy_);
    auto status = fbs_proxy_->TestFbs(context, request_msg, &response_msg);

    ASSERT_TRUE(status.OK());
    const FbsResponse* reply = response_msg.GetRoot();
    ASSERT_EQ("test", reply->message()->str());
  });
}

// Test Case:
//   The client sends the request in the incorrect format of MessageFbs data.
// Expected:
//   The call failed and got the TRPC_CLIENT_ENCODE_ERR error.
TEST_F(TrpcUnaryTest, FbsClientInvalidData) {
  RunByEnv([&]() {
    // do not fill the request
    flatbuffers::trpc::Message<trpc::testing::unarytrpc::FbsRequest> request_msg;
    ::flatbuffers::trpc::Message<FbsResponse> reply_msg;

    auto context = ::trpc::MakeClientContext(fbs_proxy_);
    auto status = fbs_proxy_->TestFbs(context, request_msg, &reply_msg);
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The client sends the request in the correct format of MessageFbs data, and the server responds in the incorrect
//   format of MessageFbs data.
// Expected:
//   The call failed and got the TRPC_SERVER_ENCODE_ERR error.
TEST_F(TrpcUnaryTest, FbsServerInvalidData) {
  RunByEnv([&]() {
    ::flatbuffers::trpc::Message<FbsRequest> request_msg = CreateFbsTestRequest(FbsTestCaseType_INVALID_DATA);
    ::flatbuffers::trpc::Message<FbsResponse> reply_msg;

    auto context = ::trpc::MakeClientContext(fbs_proxy_);
    auto status = fbs_proxy_->TestFbs(context, request_msg, &reply_msg);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_ENCODE_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   The client sends the request in the correct format of MessageFbs data, and the server invalid data type
// Expected:
//   The call failed and got the TRPC_CLIENT_DECODE_ERR error.
TEST_F(TrpcUnaryTest, FbsServerInvalidType) {
  RunByEnv([&]() {
    ::flatbuffers::trpc::Message<FbsRequest> request_msg = CreateFbsTestRequest(FbsTestCaseType_INVALID_TYPE);
    ::flatbuffers::trpc::Message<FbsResponse> reply_msg;

    auto context = ::trpc::MakeClientContext(fbs_proxy_);
    auto status = fbs_proxy_->TestFbs(context, request_msg, &reply_msg);

    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_DECODE_ERR, status.GetFrameworkRetCode());
  });
}

// ----- transinfo testcases -----

void TestServerTransInfo(TransRequest::TestCaseType type,
                         ::trpc::Function<void(::trpc::ClientContextPtr&)> set_transinfo_func,
                         ::trpc::Function<void(TransResponse&)> check_server_recv_func) {
  TransRequest request;
  request.set_test_case_type(type);
  request.set_key_prefix("req-key-");
  TransResponse reply;

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);

  set_transinfo_func(ctx);

  ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestTransInfo(ctx, request, &reply);

  // confirming that the trans info received by the server is correct
  ASSERT_TRUE(status.OK());
  check_server_recv_func(reply);

  // confirming that the trans info received by the client is correct
  std::vector<std::tuple<std::string, std::string>> recv_vec;
  for (const auto& [key, value] : ctx->GetPbRspTransInfo()) {
    if (::trpc::StartsWith(key, reply.key_prefix())) {
      recv_vec.push_back(std::make_tuple(key, value));
    }
  }
  ASSERT_EQ(reply.rsp_kv().size(), recv_vec.size());
  for (auto kv : recv_vec) {
    ASSERT_TRUE(reply.rsp_kv().find(std::get<0>(kv)) != reply.rsp_kv().end());
    ASSERT_EQ((reply.rsp_kv()).at(std::get<0>(kv)), std::get<1>(kv));
  }
}

void TestRouteTransInfo(
    RouteTransRequest::TestCaseType type,
    ::trpc::Function<void(::trpc::ClientContextPtr&, RouteTransResponse&, std::map<std::string, std::string>&)>
        check_res_func) {
  RouteTransRequest request;
  request.set_test_case_type(type);
  request.set_key_prefix("req-key-");
  RouteTransResponse reply;

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);

  std::map<std::string, std::string> req_map;
  req_map["req-key-1"] = "req-value-1";
  req_map["req-key-2"] = "req-value-2";
  std::map<std::string, std::string> check_map = req_map;
  ctx->SetReqTransInfo(req_map.begin(), req_map.end());

  auto status = TrpcUnaryTest::route_proxy_->TestTransInfo(ctx, request, &reply);
  ASSERT_TRUE(status.OK());

  check_res_func(ctx, reply, check_map);
}

// Test Case:
//   The client uses the ClientContext::SetReqTransInfo interface to set trans info with different key values.
//   The server use ServerContext::SetRspTransInfo interface to respond trans info with different key values.
// Expected:
//   The server obtains the two transinfo transmitted by the client, and the client obtains two transinfo returned by
//   the server.
TEST_F(TrpcUnaryTest, TransSetDifferentKey) {
  RunByEnv([&]() {
    std::map<std::string, std::string> req_map;
    req_map["req-key-1"] = "req-value-1";
    req_map["req-key-2"] = "req-value-2";

    TestServerTransInfo(
        TransRequest::DIFFERENT_KEY_USE_SET_INTERFACE,
        [req_map](::trpc::ClientContextPtr& ctx) { ctx->SetReqTransInfo(req_map.begin(), req_map.end()); },
        [&](TransResponse& reply) {
          ASSERT_EQ(req_map.size(), reply.recv_kv().size());
          for (auto kv : reply.recv_kv()) {
            ASSERT_TRUE(req_map.find(kv.key()) != req_map.end());
            ASSERT_EQ(req_map[kv.key()], kv.value());
          }
        });
  });
}

// Test Case:
//   The client uses the ClientContext::AddReqTransInfo interface to set trans info with different key values.
//   The server use ServerContext::AddRspTransInfo interface to respond trans info with different key values.
// Expected:
//   The server obtains the two transinfo transmitted by the client, and the client obtains two transinfo returned by
//   the server.
TEST_F(TrpcUnaryTest, TransAddDifferentKey) {
  RunByEnv([&]() {
    std::map<std::string, std::string> req_map;
    req_map["req-key-1"] = "req-value-1";
    req_map["req-key-2"] = "req-value-2";
    TestServerTransInfo(
        TransRequest::DIFFERENT_KEY_USE_ADD_INTERFACE,
        [&req_map](::trpc::ClientContextPtr& ctx) {
          for (auto& [key, value] : req_map) {
            ctx->AddReqTransInfo(key, value);
          }
        },
        [&req_map](TransResponse& reply) {
          ASSERT_EQ(req_map.size(), reply.recv_kv().size());
          for (auto kv : reply.recv_kv()) {
            ASSERT_TRUE(req_map.find(kv.key()) != req_map.end());
            ASSERT_EQ(req_map[kv.key()], kv.value());
          }
        });
  });
}

// Test Case:
//   The client uses the ClientContext::AddReqTransInfo interface to set trans info with same key value.
//   The server use ServerContext::AddRspTransInfo interface to respond trans info with same key value.
// Expected:
//   The server obtains one transinfo transmitted by the client, and the client obtains one transinfo returned by
//   the server.
TEST_F(TrpcUnaryTest, TransSameKey) {
  RunByEnv([&]() {
    TestServerTransInfo(
        TransRequest::SAME_KEY_USE_ADD_INTERFACE,
        [](::trpc::ClientContextPtr& ctx) {
          ctx->AddReqTransInfo("req-key-1", "req-value-1");
          ctx->AddReqTransInfo("req-key-1", "req-value-2");
        },
        [](TransResponse& reply) {
          ASSERT_EQ(1, reply.recv_kv().size());
          ASSERT_EQ("req-key-1", reply.recv_kv()[0].key());
          ASSERT_EQ("req-value-2", reply.recv_kv()[0].value());
        });
  });
}

// Test Case:
//   Client A calls route service B with trans info. B constructs a ClientContext using
//   MakeClientContext(ServerContextPtr) and further calls service C.
// Expected:
//   Service C can obtain the trans info transmitted from A.
TEST_F(TrpcUnaryTest, TransRouteUseContext) {
  RunByEnv([&]() {
    TestRouteTransInfo(
        RouteTransRequest::USE_SERVER_CONTEXT,
        [](::trpc::ClientContextPtr& ctx, RouteTransResponse& reply, std::map<std::string, std::string>& req_map) {
          ASSERT_EQ(req_map.size(), reply.recv_kv().size());
          for (auto kv : reply.recv_kv()) {
            ASSERT_TRUE(req_map.find(kv.key()) != req_map.end());
            ASSERT_EQ(req_map[kv.key()], kv.value());
          }
        });
  });
}

// Test Case:
//   Client A calls route service B with trans info. B does not use MakeClientContext(ServerContextPtr) to construct
//   ClientContext and further calls service C.
// Expected:
//   Service C can not obtain the trans info transmitted from A.
TEST_F(TrpcUnaryTest, TransRouteNotUseContext) {
  RunByEnv([&]() {
    TestRouteTransInfo(RouteTransRequest::NOT_USE_SERVER_CONTEXT,
                       [](::trpc::ClientContextPtr& ctx, RouteTransResponse& reply,
                          std::map<std::string, std::string>& req_map) { ASSERT_EQ(0, reply.recv_kv().size()); });
  });
}

// ----- transparent testcases -----

// Test Case:
//   Client A calls the transparent route service B, and then B uses MakeTransparentClientContext to construct a
//   ClientContext to further call service C.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, TransparentEffictent) {
  RunByEnv([&]() {
    CommonRequest request;
    request.set_msg("test");
    CommonReply reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::transparent_proxy_);
    ctx->AddReqTransInfo("MakeTransparentClientContext", "");
    ::trpc::Status status = TrpcUnaryTest::transparent_proxy_->TestCommon(ctx, request, &reply);

    ASSERT_TRUE(status.OK());
    ASSERT_EQ("test", reply.msg());
  });
}

// Test Case:
//   Client A calls the transparent route service B, and then B uses MakeClientContext to construct a
//   ClientContext to further call service C.
// Expected:
//   Call failed.
TEST_F(TrpcUnaryTest, TransparentFail) {
  RunByEnv([&]() {
    CommonRequest request;
    request.set_msg("test");
    CommonReply reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::transparent_proxy_);
    ctx->AddReqTransInfo("MakeClientContext", "");
    ::trpc::Status status = TrpcUnaryTest::transparent_proxy_->TestCommon(ctx, request, &reply);

    ASSERT_FALSE(status.OK());
  });
}

// ----- attachment testcases -----

// Call the TestAttachment function
void CallTestAttachment(AttachmentRequest::TestCaseType type, std::string req_msg, std::string req_atta) {
  AttachmentRequest request;
  request.set_test_case_type(type);
  request.set_msg(req_msg);
  AttachmentResponse response;

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
  ctx->SetRequestAttachment(::trpc::CreateBufferSlow(req_atta.c_str(), req_atta.size()));
  ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestAttachment(ctx, request, &response);

  EXPECT_TRUE(status.OK());
  EXPECT_EQ(req_msg, response.recv_msg());
  EXPECT_EQ(req_atta, response.recv_atta());
}

// Test Case:
//   The client sends a non-empty request attachment.
// Expected:
//   The server successfully received the pb data and attachment.
TEST_F(TrpcUnaryTest, ClientNotEmpty) {
  RunByEnv([&]() {
    std::string req_msg = "req_msg";
    std::string req_atta = "req_atta";
    CallTestAttachment(AttachmentRequest::NO_ATTACHMENT, req_msg, req_atta);
  });
}

// Test Case:
//   The client sends an empty request attachment.
// Expected:
//   The server successfully received the pb data and empty attachment.
TEST_F(TrpcUnaryTest, ClientEmpty) {
  RunByEnv([&]() {
    std::string req_msg = "req_msg";
    std::string req_atta = "";
    CallTestAttachment(AttachmentRequest::NO_ATTACHMENT, req_msg, req_atta);
  });
}

// Test Case:
//   The server sends a non-empty response attachment.
// Expected:
//   The client successfully received the pb data and attachment.
TEST_F(TrpcUnaryTest, ServerNotEmpty) {
  RunByEnv([&]() {
    AttachmentRequest request;
    request.set_test_case_type(AttachmentRequest::NOT_EMPTY_ATTACHMENT);
    AttachmentResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestAttachment(ctx, request, &reply);

    ASSERT_TRUE(status.OK());
    ASSERT_NE(0, reply.rsp_msg().size());
    ASSERT_NE(0, ctx->GetResponseAttachment().ByteSize());
    ASSERT_EQ(reply.rsp_msg(), ::trpc::FlattenSlow(ctx->GetResponseAttachment()));
  });
}

// Test Case:
//   The server sends an empty response attachment.
// Expected:
//   The client successfully received the pb data and empty attachment.
TEST_F(TrpcUnaryTest, ServerEmpty) {
  RunByEnv([&]() {
    AttachmentRequest request;
    request.set_test_case_type(AttachmentRequest::EMPTY_ATTACHMENT);
    AttachmentResponse reply;

    ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
    ::trpc::Status status = TrpcUnaryTest::trpc_proxy_->TestAttachment(ctx, request, &reply);

    ASSERT_TRUE(status.OK());
    ASSERT_NE(0, reply.rsp_msg().size());
    ASSERT_EQ(0, ctx->GetResponseAttachment().ByteSize());
  });
}

// ----- timeout testcases -----

// Call the TestTimeout function of TrpcTestService
::trpc::Status CallServerTestTimeout(uint32_t client_timeout, uint32_t server_deal_time) {
  TimeoutRequest request;
  request.set_server_deal_time(server_deal_time);
  TimeoutResponse reply;

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
  ctx->SetTimeout(client_timeout);
  return TrpcUnaryTest::trpc_proxy_->TestTimeout(ctx, request, &reply);
}

// Call the QueryLastTimeoutInfo function of TrpcTestService
QueryTimeoutResponse CallServerQueryLastTimeoutInfo() {
  QueryTimeoutRequest query_request;
  QueryTimeoutResponse query_response;
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(TrpcUnaryTest::trpc_proxy_);
  auto status = TrpcUnaryTest::trpc_proxy_->QueryLastTimeoutInfo(ctx, query_request, &query_response);
  EXPECT_TRUE(status.OK());
  return query_response;
}

// Call the TestTimeout function of RouteTestService
::trpc::Status CallRouteTestTimeout(std::shared_ptr<RouteTestServiceProxy>& proxy, uint32_t client_timeout,
                                    uint32_t route_deal_time, uint32_t server_deal_time) {
  TimeoutRequest request;
  request.set_route_deal_time(route_deal_time);
  request.set_server_deal_time(server_deal_time);
  TimeoutResponse response;

  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(client_timeout);
  return proxy->TestTimeout(ctx, request, &response);
}

// Call the QueryLastTimeoutInfo function of RouteTestService
QueryTimeoutResponse CallRouteQueryLastTimeoutInfo(std::shared_ptr<RouteTestServiceProxy>& proxy) {
  QueryTimeoutRequest query_request;
  QueryTimeoutResponse query_response;
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(proxy);
  auto status = proxy->QueryLastTimeoutInfo(ctx, query_request, &query_response);
  EXPECT_TRUE(status.OK());
  return query_response;
}

// Test Case:
//   NonRoute mode, the server has sufficient time to process the request.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, NonRouteSuccess) {
  RunByEnv([&]() {
    ::trpc::Status status = CallServerTestTimeout(50, 10);
    ASSERT_TRUE(status.OK());
  });
}

// Test Case:
//   NonRoute mode, server fulllink timeout.
// Expected:
//   Call failed. Client timed out and returned TRPC_CLIENT_INVOKE_TIMEOUT_ERR, while server reported
//   TRPC_SERVER_FULL_LINK_TIMEOUT_ERR due to full-link timeout on server side.
TEST_F(TrpcUnaryTest, NonRouteServerFulllinkTimeout) {
  RunByEnv([&]() {
    ::trpc::Status status = CallServerTestTimeout(50, 80);
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, status.GetFrameworkRetCode());

    // wait for the server to finish processing
    ::trpc::testing::SleepFor(50);

    QueryTimeoutResponse query_response = CallServerQueryLastTimeoutInfo();
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_FULL_LINK_TIMEOUT_ERR, query_response.server_last_ret_code());
  });
}

// Test Case:
//   NonRoute mode, server timeout.
// Expected:
//   Call failed. Client received TRPC_SERVER_TIMEOUT_ERR returned by the server.
TEST_F(TrpcUnaryTest, NonRouteServerTimeout) {
  RunByEnv([&]() {
    ::trpc::Status status = CallServerTestTimeout(500, 300);
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_TIMEOUT_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   Route mode, the route server has sufficient time to process the request.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, RouteSuccess) {
  RunByEnv([&]() {
    ::trpc::Status status = CallRouteTestTimeout(TrpcUnaryTest::route_proxy_, 100, 10, 10);
    ASSERT_TRUE(status.OK());
  });
}

// Test Case:
//   Route mode, the route service returned due to server timeout before calling the server.
// Expected:
//   Call failed. Client received TRPC_SERVER_TIMEOUT_ERR returned by the server.
TEST_F(TrpcUnaryTest, RouteServerTimeout) {
  RunByEnv([&]() {
    ::trpc::Status status = CallRouteTestTimeout(TrpcUnaryTest::route_proxy_, 700, 600, 10);
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_TIMEOUT_ERR, status.GetFrameworkRetCode());
  });
}

// Test Case:
//   Route mode, the route service returned due to server fulllink timeout before calling the server.
// Expected:
//   Call failed. Client timed out and returned TRPC_CLIENT_INVOKE_TIMEOUT_ERR, while server reported
//   TRPC_SERVER_FULL_LINK_TIMEOUT_ERR due to full-link timeout on server side.
TEST_F(TrpcUnaryTest, RouteServerFulllinkTimeout) {
  RunByEnv([&]() {
    ::trpc::Status status = CallRouteTestTimeout(TrpcUnaryTest::route_proxy_, 100, 150, 10);
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, status.GetFrameworkRetCode());

    // wait for the route server to finish processing
    ::trpc::testing::SleepFor(100);

    QueryTimeoutResponse query_response = CallRouteQueryLastTimeoutInfo(TrpcUnaryTest::route_proxy_);
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_FULL_LINK_TIMEOUT_ERR, query_response.server_last_ret_code());
  });
}

// Test Case:
//   Route mode, the route service encountered a client timeout when calling the server.
// Expected:
//   Call failed. Client received TRPC_SERVER_TIMEOUT_ERR returned by the server, while server reported
//   TRPC_CLIENT_INVOKE_TIMEOUT_ERR due to timeout on client side.
TEST_F(TrpcUnaryTest, RouteClientTimeout) {
  RunByEnv([&]() {
    ::trpc::Status status = CallRouteTestTimeout(TrpcUnaryTest::route_proxy_, 700, 400, 150);
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_SERVER_TIMEOUT_ERR, status.GetFrameworkRetCode());

    QueryTimeoutResponse query_response = CallRouteQueryLastTimeoutInfo(TrpcUnaryTest::route_proxy_);
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, query_response.client_last_ret_code());
  });
}

// Test Case:
//   Route mode, the route service encountered a client fulllink timeout when calling the server.
// Expected:
//   Call failed. Client timed out, while route server reported TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR due to full-link
//   timeout on client side.
TEST_F(TrpcUnaryTest, RouteClientFulllinkTimeout) {
  RunByEnv([&]() {
    ::trpc::Status status = CallRouteTestTimeout(TrpcUnaryTest::route_proxy_, 300, 200, 150);
    ASSERT_FALSE(status.OK());
    // Because there is almost no communication time in local communication, and the timeout judgment has a certain
    // accuracy problem, the client may directly wait for the timeout, or may receive a timeout error from the server.
    ASSERT_TRUE(status.GetFrameworkRetCode() == ::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR ||
                status.GetFrameworkRetCode() == ::trpc::TrpcRetCode::TRPC_SERVER_FULL_LINK_TIMEOUT_ERR);

    // wait for the route server to finish processing
    ::trpc::testing::SleepFor(100);

    QueryTimeoutResponse query_response = CallRouteQueryLastTimeoutInfo(TrpcUnaryTest::route_proxy_);
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR, query_response.client_last_ret_code());
  });
}

// Test Case:
//   Route mode with route server disable the fulllink timeout, the route server has sufficient time to process the
//   request.
// Expected:
//   Call successfully.
TEST_F(TrpcUnaryTest, NoFulllinkSuccess) {
  RunByEnv([&]() {
    ::trpc::Status status = CallRouteTestTimeout(TrpcUnaryTest::no_fulllink_route_proxy_, 100, 10, 10);
    ASSERT_TRUE(status.OK());

    QueryTimeoutResponse query_response = CallRouteQueryLastTimeoutInfo(TrpcUnaryTest::route_proxy_);
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS, query_response.server_last_ret_code());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS, query_response.client_last_ret_code());
  });
}

// Test Case:
//   Route mode with route server disable the fulllink timeout, the route server has sufficient time to process the
//   request even if the client was no longer waiting. As a comparison with the RouteServerFulllinkTimeout test case.
// Expected:
//   Call failed. Client timed out and returned TRPC_CLIENT_INVOKE_TIMEOUT_ERR, while server call successful.
TEST_F(TrpcUnaryTest, NoFulllinkClientTimeout1) {
  RunByEnv([&]() {
    ::trpc::Status status = CallRouteTestTimeout(TrpcUnaryTest::no_fulllink_route_proxy_, 100, 150, 10);
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, status.GetFrameworkRetCode());

    // wait for the route server to finish processing
    ::trpc::testing::SleepFor(100);

    QueryTimeoutResponse query_response = CallRouteQueryLastTimeoutInfo(TrpcUnaryTest::no_fulllink_route_proxy_);
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS, query_response.server_last_ret_code());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS, query_response.client_last_ret_code());
  });
}

// Test Case:
//   Route mode with route server disable the fulllink timeout, the route server has sufficient time to process the
//   request even if the client was no longer waiting. As a comparison with the RouteClientFulllinkTimeout test case.
// Expected:
//   Call failed. Client timed out and returned TRPC_CLIENT_INVOKE_TIMEOUT_ERR, while server call successful.
TEST_F(TrpcUnaryTest, NoFulllinkClientTimeout2) {
  RunByEnv([&]() {
    ::trpc::Status status = CallRouteTestTimeout(TrpcUnaryTest::no_fulllink_route_proxy_, 300, 200, 150);
    ASSERT_FALSE(status.OK());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, status.GetFrameworkRetCode());

    // wait for the route server to finish processing
    ::trpc::testing::SleepFor(100);

    QueryTimeoutResponse query_response = CallRouteQueryLastTimeoutInfo(TrpcUnaryTest::no_fulllink_route_proxy_);
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS, query_response.server_last_ret_code());
    ASSERT_EQ(::trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS, query_response.client_last_ret_code());
  });
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  int test_argc;
  char **client_argv, **server1_argv, **server2_argv;
  if (!::trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server1_argv)) {
    exit(EXIT_FAILURE);
  }
  if (!::trpc::testing::ExtractServerAndClientArgs(test_argc, client_argv, &test_argc, &client_argv, &server2_argv)) {
    exit(EXIT_FAILURE);
  }

  trpc::testing::CommonTest::SetArgv(test_argc, client_argv, server1_argv, server2_argv);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
