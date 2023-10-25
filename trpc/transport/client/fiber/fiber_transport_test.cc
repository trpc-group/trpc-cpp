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

#include "trpc/transport/client/fiber/fiber_transport.h"

#include <atomic>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/client/testing/client_context_testing.h"
#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/common/future/future_utility.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/future.h"
#include "trpc/future/future.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/transport/client/fiber/testing/fake_server.h"
#include "trpc/transport/client/fiber/testing/thread_model_op.h"

namespace trpc::tesing {

int SendRecvAfterStop(std::unique_ptr<FiberTransport>& transport);
int SendOnlyAfterStop(std::unique_ptr<FiberTransport>& transport);
stream::StreamReaderWriterProviderPtr CreateStreamAfterStop(std::unique_ptr<FiberTransport>& transport);

class FiberTransportFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    trpc::codec::Init();

    fake_server = std::make_unique<FakeServer>();
    fake_server->StartServer();

    fake_backup_server = std::make_unique<FakeServer>();
    fake_backup_server->StartServer();

    FiberTransport::Options tcp_complex_opt;
    tcp_complex_opt.trans_info = MakeTransInfo(true);
    tcp_complex_transport = std::make_unique<FiberTransport>();
    tcp_complex_transport->Init(std::move(tcp_complex_opt));

    FiberTransport::Options tcp_pool_opt;
    tcp_pool_opt.trans_info = MakeTransInfo(true, false);
    tcp_pool_transport = std::make_unique<FiberTransport>();
    tcp_pool_transport->Init(std::move(tcp_pool_opt));

    FiberTransport::Options tcp_short_opt;
    tcp_short_opt.trans_info = MakeTransInfo(true, false, false, true);
    tcp_short_transport = std::make_unique<FiberTransport>();
    tcp_short_transport->Init(std::move(tcp_short_opt));

    FiberTransport::Options tcp_pipeline_opt;
    tcp_pipeline_opt.trans_info = MakeTransInfo(true, false, true);
    tcp_pipeline_transport = std::make_unique<FiberTransport>();
    tcp_pipeline_transport->Init(std::move(tcp_pipeline_opt));

    FiberTransport::Options udp_complex_opt;
    udp_complex_opt.trans_info = MakeTransInfo(false);
    udp_complex_transport = std::make_unique<FiberTransport>();
    udp_complex_transport->Init(std::move(udp_complex_opt));

    FiberTransport::Options udp_pool_opt;
    udp_pool_opt.trans_info = MakeTransInfo(false, false);
    udp_pool_transport = std::make_unique<FiberTransport>();
    udp_pool_transport->Init(std::move(udp_pool_opt));
  }

  static void TearDownTestCase() {
    std::cout << "TearDownTestCase begin" << std::endl;
    udp_pool_transport->Stop();
    udp_pool_transport->Destroy();
    udp_pool_transport.reset();

    std::cout << "TearDownTestCase udp_pool_transport destroy" << std::endl;

    udp_complex_transport->Stop();
    udp_complex_transport->Destroy();
    udp_complex_transport.reset();

    std::cout << "TearDownTestCase udp_complex_transport destroy" << std::endl;

    tcp_pipeline_transport->Stop();
    tcp_pipeline_transport->Destroy();
    tcp_pipeline_transport.reset();

    std::cout << "TearDownTestCase tcp_pipeline_transport destroy" << std::endl;

    tcp_pool_transport->Stop();
    ASSERT_EQ(SendRecvAfterStop(tcp_pool_transport), TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
    ASSERT_EQ(SendOnlyAfterStop(tcp_pool_transport), TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
    ASSERT_EQ(CreateStreamAfterStop(tcp_pool_transport), nullptr);
    tcp_pool_transport->Destroy();
    tcp_pool_transport.reset();

    std::cout << "TearDownTestCase tcp_pool_transport destroy" << std::endl;

    tcp_short_transport->Stop();
    tcp_short_transport->Destroy();
    tcp_short_transport.reset();

    std::cout << "TearDownTestCase tcp_short_transport destroy" << std::endl;

    tcp_complex_transport->Stop();

    ASSERT_EQ(SendRecvAfterStop(tcp_complex_transport), TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
    ASSERT_EQ(SendOnlyAfterStop(tcp_complex_transport), TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
    ASSERT_EQ(CreateStreamAfterStop(tcp_complex_transport), nullptr);
    tcp_complex_transport->Destroy();
    tcp_complex_transport.reset();

    std::cout << "TearDownTestCase tcp_complex_transport destroy" << std::endl;

    fake_backup_server->StopServer();
    fake_backup_server.reset();

    fake_server->StopServer();
    fake_server.reset();

    trpc::codec::Destroy();
  }

 public:
  static std::unique_ptr<FakeServer> fake_server;
  static std::unique_ptr<FakeServer> fake_backup_server;
  static std::unique_ptr<FiberTransport> tcp_complex_transport;
  static std::unique_ptr<FiberTransport> tcp_pool_transport;
  static std::unique_ptr<FiberTransport> tcp_short_transport;
  static std::unique_ptr<FiberTransport> tcp_pipeline_transport;
  static std::unique_ptr<FiberTransport> udp_complex_transport;
  static std::unique_ptr<FiberTransport> udp_pool_transport;
  static std::atomic<int> id_gen;
};

std::unique_ptr<FakeServer> FiberTransportFixture::fake_server = nullptr;
std::unique_ptr<FakeServer> FiberTransportFixture::fake_backup_server = nullptr;
std::unique_ptr<FiberTransport> FiberTransportFixture::tcp_complex_transport = nullptr;
std::unique_ptr<FiberTransport> FiberTransportFixture::tcp_pool_transport = nullptr;
std::unique_ptr<FiberTransport> FiberTransportFixture::tcp_short_transport = nullptr;
std::unique_ptr<FiberTransport> FiberTransportFixture::tcp_pipeline_transport = nullptr;
std::unique_ptr<FiberTransport> FiberTransportFixture::udp_complex_transport = nullptr;
std::unique_ptr<FiberTransport> FiberTransportFixture::udp_pool_transport = nullptr;
std::atomic<int> FiberTransportFixture::id_gen = 1;

int SendRecvAfterStop(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "hello";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  trpc::CTransportRspMsg rsp_msg;

  return transport->SendRecv(&req_msg, &rsp_msg);
}

int SendOnlyAfterStop(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());
  auto* msg = trpc::object_pool::New<CTransportReqMsg>();
  trpc::CTransportReqMsg& req_msg = *msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "oneway";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  return transport->SendOnly(msg);
}

stream::StreamReaderWriterProviderPtr CreateStreamAfterStop(std::unique_ptr<FiberTransport>& transport) {
  NodeAddr addr;
  addr.ip = FiberTransportFixture::fake_server->GetServerAddr().Ip();
  addr.port = FiberTransportFixture::fake_server->GetServerAddr().Port();
  stream::StreamOptions stream_options;
  return transport->CreateStream(addr, std::move(stream_options));
}

void SendRecv(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "hello";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  trpc::CTransportRspMsg rsp_msg;

  int ret = transport->SendRecv(&req_msg, &rsp_msg);

  ASSERT_EQ(ret, 0);

  auto rsp_protocol = std::any_cast<trpc::ProtocolPtr&&>(std::move(rsp_msg.msg));
  auto* rsp = static_cast<trpc::testing::TestProtocol*>(rsp_protocol.get());

  ASSERT_EQ("hello", rsp->body_);
}

// Test the test case of sending and receiving packets normally on the fiber worker thread
// under different connection modes of transport
TEST_F(FiberTransportFixture, testSendRecv_normal) {
  std::cout << "tcp_complex_transport" << std::endl;

  SendRecv(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  SendRecv(tcp_pool_transport);

  std::cout << "tcp_short_transport" << std::endl;

  SendRecv(tcp_short_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  SendRecv(tcp_pipeline_transport);

  std::cout << "udp_complex_transport" << std::endl;

  SendRecv(udp_complex_transport);

  std::cout << "udp_pool_transport" << std::endl;

  SendRecv(udp_pool_transport);
}

// Test the test cases for sending and receiving packets normally on non-fiber worker threads
// under different connection modes of transport
TEST_F(FiberTransportFixture, testSendRecv_normal_outside) {
  auto func = [&] {
    std::cout << "tcp_complex_transport" << std::endl;

    SendRecv(tcp_complex_transport);

    std::cout << "tcp_pool_transport" << std::endl;

    SendRecv(tcp_pool_transport);

    std::cout << "tcp_short_transport" << std::endl;

    SendRecv(tcp_short_transport);

    std::cout << "tcp_pipeline_transport" << std::endl;

    SendRecv(tcp_pipeline_transport);

    std::cout << "udp_complex_transport" << std::endl;

    SendRecv(udp_complex_transport);

    std::cout << "udp_pool_transport" << std::endl;

    SendRecv(udp_pool_transport);
  };

  std::thread th(func);
  th.join();
}

void AsyncSendRecv(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;
  req_msg.extend_info = trpc::object_pool::MakeLwShared<trpc::ClientExtendInfo>();

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "hello";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  auto fut = transport->AsyncSendRecv(&req_msg).Then([](trpc::Future<trpc::CTransportRspMsg>&& rsp_msg) {
    EXPECT_TRUE(rsp_msg.IsReady());

    auto rsp_protocol = std::any_cast<trpc::ProtocolPtr&&>(std::move(rsp_msg.GetValue0().msg));
    auto* rsp = static_cast<trpc::testing::TestProtocol*>(rsp_protocol.get());

    EXPECT_EQ("hello", rsp->body_);

    return trpc::MakeReadyFuture<>();
  });

  if (IsRunningInFiberWorker()) {
    fut = fiber::BlockingGet(std::move(fut));
  } else {
    fut = future::BlockingGet(std::move(fut));
  }

  ASSERT_TRUE(fut.IsReady());
}

// Test the test case of sending and receiving packets asynchronously on the fiber worker thread
// under different connection modes of transport
TEST_F(FiberTransportFixture, testSendRecv_normal_async) {
  std::cout << "tcp_complex_transport" << std::endl;

  AsyncSendRecv(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  AsyncSendRecv(tcp_pool_transport);

  std::cout << "tcp_short_transport" << std::endl;

  AsyncSendRecv(tcp_short_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  AsyncSendRecv(tcp_pipeline_transport);

  std::cout << "udp_complex_transport" << std::endl;

  AsyncSendRecv(udp_complex_transport);

  std::cout << "udp_pool_transport" << std::endl;

  AsyncSendRecv(udp_pool_transport);
}

// Test the test case of sending and receiving packets asynchronously on the non-fiber worker thread
// under different connection modes of transport
TEST_F(FiberTransportFixture, testSendRecv_normal_async_outside) {
  auto func = [&] {
    std::cout << "tcp_complex_transport" << std::endl;

    AsyncSendRecv(tcp_complex_transport);

    std::cout << "tcp_pool_transport" << std::endl;

    AsyncSendRecv(tcp_pool_transport);

    std::cout << "tcp_short_transport" << std::endl;

    AsyncSendRecv(tcp_short_transport);

    std::cout << "tcp_pipeline_transport" << std::endl;

    AsyncSendRecv(tcp_pipeline_transport);

    std::cout << "udp_complex_transport" << std::endl;

    AsyncSendRecv(udp_complex_transport);

    std::cout << "udp_pool_transport" << std::endl;

    AsyncSendRecv(udp_pool_transport);
  };

  std::thread th(func);
  th.join();
}

void SendOnly(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  auto* msg = trpc::object_pool::New<CTransportReqMsg>();

  trpc::CTransportReqMsg& req_msg = *msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "oneway";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  int ret = transport->SendOnly(msg);

  ASSERT_EQ(ret, 0);
}

// Test the case of one-way packet sending on the fiber worker thread under different connection modes of transport
TEST_F(FiberTransportFixture, testSendOnly) {
  std::cout << "tcp_complex_transport" << std::endl;

  SendOnly(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  SendOnly(tcp_pool_transport);

  std::cout << "tcp_short_transport" << std::endl;

  SendOnly(tcp_short_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  SendOnly(tcp_pipeline_transport);

  std::cout << "udp_complex_transport" << std::endl;

  SendOnly(udp_complex_transport);

  std::cout << "udp_pool_transport" << std::endl;

  SendOnly(udp_pool_transport);
}

// Test the case of one-way packet sending on non-fiber worker threads under different connection modes of transport
TEST_F(FiberTransportFixture, testSendOnly_outside) {
  auto func = [&] {
    std::cout << "tcp_complex_transport" << std::endl;

    SendOnly(tcp_complex_transport);

    std::cout << "tcp_pool_transport" << std::endl;

    SendOnly(tcp_pool_transport);

    std::cout << "tcp_short_transport" << std::endl;

    SendOnly(tcp_short_transport);

    std::cout << "tcp_pipeline_transport" << std::endl;

    SendOnly(tcp_pipeline_transport);

    std::cout << "udp_complex_transport" << std::endl;

    SendOnly(udp_complex_transport);

    std::cout << "udp_pool_transport" << std::endl;

    SendOnly(udp_pool_transport);

    std::cout << "end" << std::endl;
  };
  std::cout << "testSendOnly_outside begin" << std::endl;
  std::thread th(func);
  std::cout << "testSendOnly_outside execute" << std::endl;
  th.join();
  std::cout << "testSendOnly_outside end" << std::endl;
}

void SendRecvWhenConnectFailed(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "close";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  trpc::CTransportRspMsg rsp_msg;

  int ret = transport->SendRecv(&req_msg, &rsp_msg);

  ASSERT_EQ(ret, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR);
}

// Test the case of receiving and sending packets under different connection modes of transport
// when the connection is abnormal on the fiber worker thread
// The result expected to be returned is `TrpcRetCode::TRPC_CLIENT_NETWORK_ERR`
TEST_F(FiberTransportFixture, testSendRecv_connect_fail) {
  std::cout << "tcp_complex_transport" << std::endl;

  SendRecvWhenConnectFailed(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  SendRecvWhenConnectFailed(tcp_pool_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  SendRecvWhenConnectFailed(tcp_pipeline_transport);
}

void SendRecvWhenTimeout(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "timeout";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  trpc::CTransportRspMsg rsp_msg;

  int ret = transport->SendRecv(&req_msg, &rsp_msg);

  ASSERT_EQ(ret, TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
}

// Test the case of sending and receiving packets on the fiber worker thread in different connection modes
// when request timeout
// The result expected to be returned is
// `TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR` or `TrpcRetCode::TRPC_CLIENT_NETWORK_ERR`
TEST_F(FiberTransportFixture, testSendRecv_timeout) {
  std::cout << "tcp_complex_transport" << std::endl;

  SendRecvWhenTimeout(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  SendRecvWhenTimeout(tcp_pool_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  SendRecvWhenTimeout(tcp_pipeline_transport);

  std::cout << "udp_complex_transport" << std::endl;

  SendRecvWhenTimeout(udp_complex_transport);

  std::cout << "udp_pool_transport" << std::endl;

  SendRecvWhenTimeout(udp_pool_transport);
}

void SendRecvWhenEndpointError(std::unique_ptr<FiberTransport>& transport) {
  trpc::NetworkAddress addr("127.0.0.1", 16004);

  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000, addr);

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "hello";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  trpc::CTransportRspMsg rsp_msg;

  int ret = transport->SendRecv(&req_msg, &rsp_msg);

  ASSERT_TRUE(ret == TrpcRetCode::TRPC_CLIENT_NETWORK_ERR ||
    ret == TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
}

// Test the case of sending and receiving packets on the fiber worker thread
// with incorrect ip/port under different connection modes of transport
// The result expected to be returned is
// `TrpcRetCode::TRPC_CLIENT_NETWORK_ERR` or `TRPC_CLIENT_INVOKE_TIMEOUT_ERR`
TEST_F(FiberTransportFixture, testSendRecv_endpoint_error) {
  std::cout << "tcp_complex_transport" << std::endl;

  SendRecvWhenEndpointError(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  SendRecvWhenEndpointError(tcp_pool_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  SendRecvWhenEndpointError(tcp_pipeline_transport);

  std::cout << "udp_complex_transport" << std::endl;

  SendRecvWhenEndpointError(udp_complex_transport);

  std::cout << "udp_pool_transport" << std::endl;

  SendRecvWhenEndpointError(udp_pool_transport);
}

void SendOnlyWhenEndpointError(std::unique_ptr<FiberTransport>& transport) {
  trpc::NetworkAddress addr("127.0.0.1", 16004);

  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000, addr);

  auto* msg = trpc::object_pool::New<CTransportReqMsg>();

  trpc::CTransportReqMsg& req_msg = *msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "hello";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  int ret = transport->SendOnly(msg);

  ASSERT_TRUE(ret == 0);
}

// Test the case of sending and receiving packets on the fiber worker thread
// with incorrect ip/port under different connection modes of transport
TEST_F(FiberTransportFixture, testSendOnly_endpoint_error) {
  std::cout << "tcp_complex_transport" << std::endl;

  SendOnlyWhenEndpointError(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  SendOnlyWhenEndpointError(tcp_pool_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  SendOnlyWhenEndpointError(tcp_pipeline_transport);

  std::cout << "udp_complex_transport" << std::endl;

  SendOnlyWhenEndpointError(udp_complex_transport);

  std::cout << "udp_pool_transport" << std::endl;

  SendOnlyWhenEndpointError(udp_pool_transport);
}

void BackupRequest(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  std::vector<NodeAddr> backup_addrs;
  backup_addrs.push_back(FiberTransportFixture::fake_server->GetServerNodeAddr());
  backup_addrs.push_back(FiberTransportFixture::fake_backup_server->GetServerNodeAddr());

  context->SetBackupRequestDelay(10);
  context->SetBackupRequestAddrs(backup_addrs);

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "hello";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  trpc::CTransportRspMsg rsp_msg;

  int ret = transport->SendRecv(&req_msg, &rsp_msg);

  ASSERT_EQ(ret, 0);

  auto rsp_protocol = std::any_cast<trpc::ProtocolPtr&&>(std::move(rsp_msg.msg));
  auto* rsp = static_cast<trpc::testing::TestProtocol*>(rsp_protocol.get());

  ASSERT_EQ("hello", rsp->body_);
}

// Test the case of sending and receiving packets on the fiber worker thread
// and enabling backuprequest in different connection modes of transport
TEST_F(FiberTransportFixture, testBackupRequest) {
  std::cout << "tcp_complex_transport" << std::endl;

  BackupRequest(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  BackupRequest(tcp_pool_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  BackupRequest(tcp_pipeline_transport);
}

void BackupRequestWhenBothReturn(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  std::vector<NodeAddr> backup_addrs;
  backup_addrs.push_back(FiberTransportFixture::fake_server->GetServerNodeAddr());
  backup_addrs.push_back(FiberTransportFixture::fake_backup_server->GetServerNodeAddr());

  context->SetBackupRequestDelay(10);
  context->SetBackupRequestAddrs(backup_addrs);

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "backup request sleep";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  trpc::CTransportRspMsg rsp_msg;

  int ret = transport->SendRecv(&req_msg, &rsp_msg);

  ASSERT_EQ(ret, 0);

  auto rsp_protocol = std::any_cast<trpc::ProtocolPtr&&>(std::move(rsp_msg.msg));
  auto* rsp = static_cast<trpc::testing::TestProtocol*>(rsp_protocol.get());

  ASSERT_EQ("hello", rsp->body_);

  // Wait for the backup request response to come back
  sleep(1);
}

// Test the test case of sending and receiving packets on the fiber worker thread
// and enabling backuprequest in different connection modes of transport
// The backup request is triggered, and both responses return
TEST_F(FiberTransportFixture, testBackupRequest_both_return) {
  std::cout << "tcp_complex_transport" << std::endl;

  BackupRequestWhenBothReturn(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  BackupRequestWhenBothReturn(tcp_pool_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  BackupRequestWhenBothReturn(tcp_pipeline_transport);
}

void BackupRequestWhenFirstFailed(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  std::vector<NodeAddr> backup_addrs;

  trpc::NodeAddr addr;
  addr.addr_type = trpc::NodeAddr::AddrType::kIpV4;
  addr.ip = "127.0.0.1";
  addr.port = 16007;
  backup_addrs.push_back(addr);
  backup_addrs.push_back(FiberTransportFixture::fake_server->GetServerNodeAddr());

  context->SetBackupRequestDelay(10);
  context->SetBackupRequestAddrs(backup_addrs);

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "hello";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  trpc::CTransportRspMsg rsp_msg;

  int ret = transport->SendRecv(&req_msg, &rsp_msg);

  ASSERT_EQ(ret, 0);

  auto rsp_protocol = std::any_cast<trpc::ProtocolPtr&&>(std::move(rsp_msg.msg));
  auto* rsp = static_cast<trpc::testing::TestProtocol*>(rsp_protocol.get());

  ASSERT_EQ("hello", rsp->body_);
  ASSERT_EQ(1, context->GetBackupRequestRetryInfo()->succ_rsp_node_index);
}

// Test the test case of sending and receiving packets on the fiber worker thread
// and enabling backuprequest in different connection modes of transport
// After the first test request fails, the backup request is sent successfully
TEST_F(FiberTransportFixture, testBackupRequest_first_fail) {
  std::cout << "tcp_complex_transport" << std::endl;

  BackupRequestWhenFirstFailed(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  BackupRequestWhenFirstFailed(tcp_pool_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  BackupRequestWhenFirstFailed(tcp_pipeline_transport);
}

void BackupRequestWhenBothFailed(std::unique_ptr<FiberTransport>& transport) {
  uint32_t seq_id = FiberTransportFixture::id_gen.fetch_add(1);
  ClientContextPtr context = trpc::testing::MakeTestClientContext(seq_id, 1000,
      FiberTransportFixture::fake_server->GetServerAddr());

  std::vector<NodeAddr> backup_addrs;

  trpc::NodeAddr addr1;
  addr1.addr_type = trpc::NodeAddr::AddrType::kIpV4;
  addr1.ip = "127.0.0.1";
  addr1.port = 16005;

  trpc::NodeAddr addr2;
  addr2.addr_type = trpc::NodeAddr::AddrType::kIpV4;
  addr2.ip = "127.0.0.1";
  addr2.port = 16006;

  backup_addrs.push_back(addr1);
  backup_addrs.push_back(addr2);

  context->SetBackupRequestDelay(10);
  context->SetBackupRequestAddrs(backup_addrs);

  trpc::CTransportReqMsg req_msg;
  req_msg.context = context;

  trpc::testing::TestProtocol out;
  out.req_id_ = seq_id;
  out.body_ = "hello";

  trpc::NoncontiguousBuffer buff;
  out.ZeroCopyEncode(buff);

  req_msg.send_data = std::move(buff);

  trpc::CTransportRspMsg rsp_msg;

  int ret = transport->SendRecv(&req_msg, &rsp_msg);

  ASSERT_NE(ret, 0);
  ASSERT_TRUE(context->GetBackupRequestRetryInfo()->succ_rsp_node_index == -1);
}

// Test the test case of sending and receiving packets on the fiber worker thread
// and enabling backuprequest in different connection modes of transport
// Test the case where both requests fail
TEST_F(FiberTransportFixture, testBackupRequest_both_fail) {
  std::cout << "tcp_complex_transport" << std::endl;

  BackupRequestWhenBothFailed(tcp_complex_transport);

  std::cout << "tcp_pool_transport" << std::endl;

  BackupRequestWhenBothFailed(tcp_pool_transport);

  std::cout << "tcp_pipeline_transport" << std::endl;

  BackupRequestWhenBothFailed(tcp_pipeline_transport);
}

TEST_F(FiberTransportFixture, testGetConnectorGroupReturnNull) {
  trpc::FiberTransport::Options tcp_complex_opt;
  tcp_complex_opt.thread_model = trpc::fiber::GetFiberThreadModel();
  tcp_complex_opt.trans_info = MakeTransInfo(true);
  auto transport = std::make_unique<trpc::FiberTransport>();
  tcp_complex_transport->Init(std::move(tcp_complex_opt));
}

}  // namespace trpc::tesing

TEST_WITH_FIBER_MAIN
