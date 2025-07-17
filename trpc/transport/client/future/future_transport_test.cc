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

#include "trpc/transport/client/future/future_transport.h"

#include <deque>
#include <iostream>
#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/common/future/future_utility.h"
#include "trpc/stream/stream_handler_manager.h"
#include "trpc/transport/client/future/testing/fake_trpc_server.h"
#include "trpc/transport/common/connection_handler_manager.h"
#include "trpc/util/thread/latch.h"
#include "trpc/util/unique_id.h"

namespace trpc::testing {

class FutureTransportImplTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    InitConnectionHandler();
    stream::InitStreamHandler();

    FakeTrpcServer::InitThreadModel(use_merge_threadmodel);
    fake_trpc_server.SetUpServer("tcp,udp");
    fake_backup_trpc_server.SetUpServer("tcp,udp");

    tcp_complex_transport = GetClientTransport(ConnectionType::kTcpLong, true, false);
    tcp_pool_transport = GetClientTransport(ConnectionType::kTcpLong, false, false);
    tcp_pipeline_transport = GetClientTransport(ConnectionType::kTcpLong, false, true);
    udp_complex_transport = GetClientTransport(ConnectionType::kUdp, true, false);
    udp_pool_transport = GetClientTransport(ConnectionType::kUdp, false, false);
  }

  static void TearDownTestCase() {
    udp_pool_transport->Stop();
    udp_pool_transport->Destroy();
    udp_pool_transport.reset();

    udp_complex_transport->Stop();
    udp_complex_transport->Destroy();
    udp_complex_transport.reset();

    tcp_pipeline_transport->Stop();
    tcp_pipeline_transport->Destroy();
    tcp_pipeline_transport.reset();

    tcp_pool_transport->Stop();
    tcp_pool_transport->Destroy();
    tcp_pool_transport.reset();

    tcp_complex_transport->Stop();
    tcp_complex_transport->Destroy();
    tcp_complex_transport.reset();

    fake_backup_trpc_server.TearDownServer();
    fake_trpc_server.TearDownServer();

    stream::DestroyStreamHandler();
    DestroyConnectionHandler();
  }

  static std::unique_ptr<FutureTransport> GetClientTransport(ConnectionType conn_type, bool is_complex_conn = true,
                                                             bool support_pipeline = false) {
    FutureTransport::Options options = DefaultOptions();
    options.trans_info.conn_type = conn_type;
    options.trans_info.is_complex_conn = is_complex_conn;
    options.trans_info.support_pipeline = support_pipeline;

    std::unique_ptr<FutureTransport> client_transport = std::make_unique<FutureTransport>();
    int ret = client_transport->Init(std::move(options));
    EXPECT_EQ(ret, 0);
    return client_transport;
  }

  static FutureTransport::Options DefaultOptions() {
    FutureTransport::Options options;
    options.thread_model = fake_trpc_server.GetThreadModel();
    options.trans_info.connect_timeout = 500;
    options.trans_info.max_conn_num = 4;
    options.trans_info.protocol = "trpc";
    options.trans_info.checker_function = [](const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                             std::deque<std::any>& out) {
      return CheckTrpcProtocolMessage(conn, in, out);
    };
    options.trans_info.rsp_dispatch_function = [](object_pool::LwUniquePtr<MsgTask>&& task) {
      auto* req_msg = static_cast<CTransportReqMsg*>(task->param);
      TRPC_ASSERT(req_msg->extend_info);
      auto& client_extend_info = req_msg->extend_info;
      if (client_extend_info->is_blocking_invoke) {
        task->handler();
        return true;
      }

      task->group_id = fake_trpc_server.GetThreadModel()->GroupId();
      fake_trpc_server.GetThreadModel()->SubmitHandleTask(task.Leak());
      return true;
    };
    options.trans_info.rsp_decode_function = [](std::any&& in, ProtocolPtr& out) {
      out = std::make_shared<TrpcResponseProtocol>();
      auto buf = std::any_cast<NoncontiguousBuffer&&>(std::move(in));
      return out->ZeroCopyDecode(buf);
    };
    options.trans_info.conn_type = ConnectionType::kTcpLong;
    options.trans_info.is_complex_conn = true;
    options.trans_info.support_pipeline = false;

    return options;
  }

  static void UseMergeThreadModel(bool val) { use_merge_threadmodel = val; }

  // Test cases
  void SendRecv(std::unique_ptr<FutureTransport>& transport);
  void SendRecvFromHandleThread(std::unique_ptr<FutureTransport>& transport);
  void AsyncSendRecv(std::unique_ptr<FutureTransport>& transport);
  void SendOnly(std::unique_ptr<FutureTransport>& transport);
  void SendRecvWhenEndpointError(std::unique_ptr<FutureTransport>& transport, bool is_tcp = true);
  void AsyncSendRecvWhenEndpointError(std::unique_ptr<FutureTransport>& transport, bool is_tcp = true);
  void SendOnlyWhenEndpointError(std::unique_ptr<FutureTransport>& transport);
  void AsyncSendRecvWhenDecodeError(std::unique_ptr<FutureTransport>& transport, bool is_pool = true);
  void AsyncSendRecvWhenServerClose(std::unique_ptr<FutureTransport>& transport, bool is_pool = true);
  void AsyncSendRecvTimeout(std::unique_ptr<FutureTransport>& transport);
  void GetOrCreateConnectorWhenEndpointError(std::unique_ptr<FutureTransport>& transport);
  void GetOrCreateConnectorUtilMaxConn(std::unique_ptr<FutureTransport>& transport, uint32_t max_conn_num,
                                       bool is_conn_complex);
  void AsyncSendRecvByFixedConnId(std::unique_ptr<FutureTransport>& transport);
  void CreateStream(std::unique_ptr<FutureTransport>& transport);
  void ConnectTimeout(std::unique_ptr<FutureTransport>& transport, int error_code);
  void ConnectionIdle(bool is_conn_complex, bool support_pipeline);
  void DisConnect(bool is_complex_conn, bool support_pipeline);

  // Test cases of backup request
  // No backup request has been triggered.
  void BackupRequestFirstSuccessWithoutResend(std::unique_ptr<FutureTransport>& transport);
  // The response for first request returns successfully while the response for second request has not returned yet.
  void BackupRequestFirstSuccessWithResend(std::unique_ptr<FutureTransport>& transport);
  // The response for second request returns successfully while the response for first request has not returned yet.
  void BackupRequestSecondSuccessWithResend(std::unique_ptr<FutureTransport>& transport);
  // Responses for both requests have returned.
  void BackupRequestBothSuccessWithResend(std::unique_ptr<FutureTransport>& transport);
  // Implementation of the backup request calling process, which is used by the above interface. The first parameter
  // returned is the number of retries for the backup request, and the second parameter is the index of the successful
  // request (0 or 1). If the second parameter is 0, it means the first request succeeded, and if it is 1, it means the
  // backup request succeeded.
  std::pair<uint32_t, int> BackupRequestSuccess(std::unique_ptr<FutureTransport>& transport,
                                                const std::string& request_msg);

  // Called the wrong backend node.
  void BackupRequestBothFailedWhenEndpointError(std::unique_ptr<FutureTransport>& transport);
  // Failure caused by a server-side error.
  void BackupRequestBothFailedWhenServerError(std::unique_ptr<FutureTransport>& transport);
  // Implementation of the backup request calling process, which is used by the above interface.
  std::pair<uint32_t, int> BackupRequestBothFailed(std::unique_ptr<FutureTransport>& transport,
                                                   const std::string& request_msg, bool use_invalid_port);

 public:
  static bool use_merge_threadmodel;
  static FakeTrpcServer fake_trpc_server;
  static FakeTrpcServer fake_backup_trpc_server;
  static std::unique_ptr<FutureTransport> tcp_complex_transport;
  static std::unique_ptr<FutureTransport> tcp_pool_transport;
  static std::unique_ptr<FutureTransport> tcp_pipeline_transport;
  static std::unique_ptr<FutureTransport> udp_complex_transport;
  static std::unique_ptr<FutureTransport> udp_pool_transport;
  static UniqueId unique_id;
};

bool FutureTransportImplTest::use_merge_threadmodel = false;
FakeTrpcServer FutureTransportImplTest::fake_trpc_server;
FakeTrpcServer FutureTransportImplTest::fake_backup_trpc_server;
std::unique_ptr<FutureTransport> FutureTransportImplTest::tcp_complex_transport = nullptr;
std::unique_ptr<FutureTransport> FutureTransportImplTest::tcp_pool_transport = nullptr;
std::unique_ptr<FutureTransport> FutureTransportImplTest::tcp_pipeline_transport = nullptr;
std::unique_ptr<FutureTransport> FutureTransportImplTest::udp_complex_transport = nullptr;
std::unique_ptr<FutureTransport> FutureTransportImplTest::udp_pool_transport = nullptr;
UniqueId FutureTransportImplTest::unique_id;

void FutureTransportImplTest::SendRecv(std::unique_ptr<FutureTransport>& transport) {
  NodeAddr addr = fake_trpc_server.GetBindAddr();
  object_pool::LwUniquePtr<CTransportReqMsg> msg_ptr = EncodeTrpcRequestProtocol(addr, "hello", unique_id.GenerateId());
  CTransportReqMsg req_msg = std::move(*msg_ptr);
  CTransportRspMsg rsp_msg;
  auto status = transport->SendRecv(&req_msg, &rsp_msg);
  ASSERT_TRUE(status == TrpcRetCode::TRPC_INVOKE_SUCCESS);
}

void FutureTransportImplTest::SendRecvFromHandleThread(std::unique_ptr<FutureTransport>& transport) {
  // In the merge thread model, synchronous calls cannot be initiated from worker threads and will return directly.
  if (FakeTrpcServer::GetThreadModel()->Type() == kMerge) {
    EXPECT_TRUE(true);
    return;
  }

  Latch l(1);
  auto task = object_pool::New<MsgTask>();
  task->group_id = fake_trpc_server.GetThreadModel()->GroupId();
  task->handler = [this, &l, &transport]() {
    this->SendRecv(transport);
    l.count_down();
  };
  FakeTrpcServer::GetThreadModel()->SubmitHandleTask(task);
  l.wait();
}

void FutureTransportImplTest::AsyncSendRecv(std::unique_ptr<FutureTransport>& transport) {
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg =
      EncodeTrpcRequestProtocol(fake_trpc_server.GetBindAddr(), "hello", unique_id.GenerateId());
  auto fut = transport->AsyncSendRecv(req_msg.Get()).Then([](Future<CTransportRspMsg>&& rsp_fut) {
    if (rsp_fut.IsReady()) {
      auto rsp_msg = rsp_fut.GetValue0();
      const auto& rsp = std::any_cast<ProtocolPtr>(rsp_msg.msg);
      // decode data
      TrpcResponseProtocol* trpc_rsp = dynamic_cast<TrpcResponseProtocol*>(rsp.get());
      trpc::test::helloworld::HelloReply reply;
      NoncontiguousBufferInputStream nbis(&trpc_rsp->rsp_body);
      EXPECT_EQ(reply.ParseFromZeroCopyStream(&nbis), true);

      EXPECT_EQ(reply.msg(), "hello");
      return MakeReadyFuture<>();
    }

    auto ex = rsp_fut.GetException();
    EXPECT_FALSE(true);
    return MakeReadyFuture<>();
  });

  future::BlockingGet(std::move(fut));
}

void FutureTransportImplTest::SendOnly(std::unique_ptr<FutureTransport>& transport) {
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg =
      EncodeTrpcRequestProtocol(fake_trpc_server.GetBindAddr(), "hello", unique_id.GenerateId());
  int ret = transport->SendOnly(req_msg.Leak());
  ASSERT_TRUE(ret == 0);
}

void FutureTransportImplTest::SendRecvWhenEndpointError(std::unique_ptr<FutureTransport>& transport, bool is_tcp) {
  // Set invalid port
  NodeAddr addr = fake_trpc_server.GetBindAddr();
  addr.port = trpc::util::GenRandomAvailablePort();
  object_pool::LwUniquePtr<CTransportReqMsg> msg_ptr = EncodeTrpcRequestProtocol(addr, "hello", unique_id.GenerateId());
  CTransportReqMsg req_msg = std::move(*msg_ptr);
  CTransportRspMsg rsp_msg;
  auto status = transport->SendRecv(&req_msg, &rsp_msg);
  if (is_tcp) {
    EXPECT_TRUE(status == TrpcRetCode::TRPC_CLIENT_NETWORK_ERR || status == TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
  } else {
    EXPECT_TRUE(status == TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
  }
}

void FutureTransportImplTest::AsyncSendRecvWhenEndpointError(std::unique_ptr<FutureTransport>& transport, bool is_tcp) {
  // Set invalid port
  NodeAddr addr = fake_trpc_server.GetBindAddr();
  addr.port = trpc::util::GenRandomAvailablePort();
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg = EncodeTrpcRequestProtocol(addr, "hello", unique_id.GenerateId());
  auto fut = transport->AsyncSendRecv(req_msg.Get()).Then([is_tcp](Future<CTransportRspMsg>&& rsp_fut) {
    EXPECT_TRUE(rsp_fut.IsFailed());
    auto ex = rsp_fut.GetException();
    auto err_code = ex.GetExceptionCode();
    COUT << "Async send fail due to " << ex.what() << ", err_code = " << err_code << std::endl;
    if (is_tcp) {
      EXPECT_TRUE(err_code == TrpcRetCode::TRPC_CLIENT_NETWORK_ERR || err_code == TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
    } else {
      EXPECT_TRUE(err_code == TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
    }
    return MakeReadyFuture<>();
  });

  future::BlockingGet(std::move(fut));
}

void FutureTransportImplTest::SendOnlyWhenEndpointError(std::unique_ptr<FutureTransport>& transport) {
  // Set invalid port
  NodeAddr addr = fake_trpc_server.GetBindAddr();
  addr.port = trpc::util::GenRandomAvailablePort();
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg = EncodeTrpcRequestProtocol(addr, "hello", unique_id.GenerateId());
  CTransportRspMsg rsp_msg;
  int ret = transport->SendOnly(req_msg.Leak());
  ASSERT_TRUE(ret == 0);
}

void FutureTransportImplTest::AsyncSendRecvWhenDecodeError(std::unique_ptr<FutureTransport>& transport, bool is_pool) {
  NodeAddr addr = fake_trpc_server.GetBindAddr();
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg =
      EncodeTrpcRequestProtocol(addr, "decode error", unique_id.GenerateId());
  auto fut = transport->AsyncSendRecv(req_msg.Get()).Then([is_pool](Future<CTransportRspMsg>&& rsp_fut) {
    EXPECT_TRUE(rsp_fut.IsFailed());
    auto ex = rsp_fut.GetException();
    auto err_code = ex.GetExceptionCode();
    COUT << "Async send fail due to " << ex.what() << ", err_code = " << err_code << std::endl;
    if (is_pool) {
      EXPECT_TRUE(err_code == TrpcRetCode::TRPC_CLIENT_DECODE_ERR);
    } else {
      EXPECT_TRUE(err_code == TrpcRetCode::TRPC_CLIENT_NETWORK_ERR ||
                  err_code == TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
    }
    return MakeReadyFuture<>();
  });

  future::BlockingGet(std::move(fut));
}

void FutureTransportImplTest::AsyncSendRecvWhenServerClose(std::unique_ptr<FutureTransport>& transport, bool is_pool) {
  NodeAddr addr = fake_trpc_server.GetBindAddr();
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg = EncodeTrpcRequestProtocol(addr, "close", unique_id.GenerateId());
  auto fut = transport->AsyncSendRecv(req_msg.Get()).Then([is_pool](Future<CTransportRspMsg>&& rsp_fut) {
    EXPECT_TRUE(rsp_fut.IsFailed());
    auto ex = rsp_fut.GetException();
    auto err_code = ex.GetExceptionCode();
    COUT << "Async send fail due to " << ex.what() << ", err_code = " << err_code << std::endl;
    if (is_pool) {
      EXPECT_TRUE(err_code == TrpcRetCode::TRPC_CLIENT_NETWORK_ERR);
    } else {
      EXPECT_TRUE(err_code == TrpcRetCode::TRPC_CLIENT_NETWORK_ERR ||
                  err_code == TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
    }
    return MakeReadyFuture<>();
  });

  future::BlockingGet(std::move(fut));
}

void FutureTransportImplTest::AsyncSendRecvTimeout(std::unique_ptr<FutureTransport>& transport) {
  uint32_t timeout = 200;
  NodeAddr addr = fake_trpc_server.GetBindAddr();
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg =
      EncodeTrpcRequestProtocol(addr, "timeout", unique_id.GenerateId(), timeout);
  auto fut = transport->AsyncSendRecv(req_msg.Get()).Then([](Future<CTransportRspMsg>&& rsp_fut) {
    EXPECT_TRUE(rsp_fut.IsFailed());
    auto ex = rsp_fut.GetException();
    auto err_code = ex.GetExceptionCode();
    COUT << "Async send fail due to " << ex.what() << ", err_code = " << err_code << std::endl;
    EXPECT_TRUE(err_code == TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
    return MakeReadyFuture<>();
  });

  future::BlockingGet(std::move(fut));
}

void FutureTransportImplTest::GetOrCreateConnectorWhenEndpointError(std::unique_ptr<FutureTransport>& transport) {
  trpc::PreallocationOption prealloc_option;
  prealloc_option.node_addr = fake_trpc_server.GetBindAddr();
  prealloc_option.node_addr.port = trpc::util::GenRandomAvailablePort();
  auto fixed_connector_id = transport->GetOrCreateConnector(prealloc_option);
  if (fixed_connector_id != std::nullopt) {
    // connect does not fail immediately
    object_pool::LwUniquePtr<CTransportReqMsg> req_msg =
        EncodeTrpcRequestProtocol(fake_trpc_server.GetBindAddr(), "hello", unique_id.GenerateId());
    req_msg->context->SetFixedConnectorId(*fixed_connector_id);
    auto fut = transport->AsyncSendRecv(req_msg.Get()).Then([](Future<CTransportRspMsg>&& rsp_fut) {
      EXPECT_TRUE(rsp_fut.IsFailed());
      auto ex = rsp_fut.GetException();
      auto err_code = ex.GetExceptionCode();
      COUT << "Async send fail due to " << ex.what() << ", err_code = " << err_code << std::endl;
      EXPECT_TRUE(err_code == TrpcRetCode::TRPC_CLIENT_NETWORK_ERR || err_code == TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);
      return MakeReadyFuture<>();
    });
    future::BlockingGet(std::move(fut));
    ASSERT_TRUE(transport->ReleaseFixedConnector(*fixed_connector_id));
  } else {
    // connect fail immediately
    ASSERT_EQ(fixed_connector_id, std::nullopt);
  }
}

void FutureTransportImplTest::GetOrCreateConnectorUtilMaxConn(std::unique_ptr<FutureTransport>& transport,
                                                              uint32_t max_conn_num, bool is_conn_complex) {
  std::vector<uint64_t> fixed_connector_ids;
  for (uint32_t i = 0; i < max_conn_num + 1; i++) {
    trpc::PreallocationOption prealloc_option;
    prealloc_option.node_addr = fake_trpc_server.GetBindAddr();
    auto fixed_connector_id = transport->GetOrCreateConnector(prealloc_option);
    if (is_conn_complex || i < max_conn_num) {
      ASSERT_NE(fixed_connector_id, std::nullopt);
      fixed_connector_ids.push_back(*fixed_connector_id);
    } else {
      ASSERT_EQ(fixed_connector_id, std::nullopt);
    }
  }

  for (auto& id : fixed_connector_ids) {
    ASSERT_TRUE(transport->ReleaseFixedConnector(id));
  }
}

void FutureTransportImplTest::AsyncSendRecvByFixedConnId(std::unique_ptr<FutureTransport>& transport) {
  trpc::PreallocationOption prealloc_option;
  prealloc_option.node_addr = fake_trpc_server.GetBindAddr();
  auto fixed_connector_id = transport->GetOrCreateConnector(prealloc_option);
  ASSERT_NE(fixed_connector_id, std::nullopt);
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg =
      EncodeTrpcRequestProtocol(fake_trpc_server.GetBindAddr(), "hello", unique_id.GenerateId());
  req_msg->context->SetFixedConnectorId(*fixed_connector_id);
  auto fut = transport->AsyncSendRecv(req_msg.Get()).Then([](Future<CTransportRspMsg>&& rsp_fut) {
    if (rsp_fut.IsReady()) {
      auto rsp_msg = rsp_fut.GetValue0();
      const auto& rsp = std::any_cast<ProtocolPtr>(rsp_msg.msg);
      // decode data
      TrpcResponseProtocol* trpc_rsp = dynamic_cast<TrpcResponseProtocol*>(rsp.get());
      trpc::test::helloworld::HelloReply reply;
      NoncontiguousBufferInputStream nbis(&trpc_rsp->rsp_body);
      EXPECT_EQ(reply.ParseFromZeroCopyStream(&nbis), true);

      EXPECT_EQ(reply.msg(), "hello");
      return MakeReadyFuture<>();
    }

    auto ex = rsp_fut.GetException();
    EXPECT_FALSE(true);
    return MakeReadyFuture<>();
  });

  future::BlockingGet(std::move(fut));
  ASSERT_TRUE(transport->ReleaseFixedConnector(*fixed_connector_id));
}

void FutureTransportImplTest::CreateStream(std::unique_ptr<FutureTransport>& transport) {
  auto task = object_pool::New<MsgTask>();
  Latch l(1);

  if (fake_trpc_server.GetThreadModel()->Type() == kSeparate) {
    task->handler = [&transport, &l]() {
      NodeAddr addr;
      addr.port = trpc::util::GenRandomAvailablePort();
      addr.ip = "127.0.0.1";

      auto context = MakeRefCounted<ClientContext>();
      stream::StreamOptions opt;
      opt.context.context = context;
      EXPECT_EQ(transport->CreateStream(addr, std::move(opt)), nullptr);
      l.count_down();
    };
    task->group_id = fake_trpc_server.GetThreadModel()->GroupId();
    EXPECT_TRUE(trpc::separate::SubmitIoTask(fake_trpc_server.GetThreadModel(), task));
  } else {
    task->handler = [this, &transport, &l]() {
      NodeAddr addr = fake_trpc_server.GetBindAddr();

      auto context = MakeRefCounted<ClientContext>();
      stream::StreamOptions opt;
      opt.context.context = context;
      EXPECT_NE(transport->CreateStream(addr, std::move(opt)), nullptr);
      l.count_down();
    };
    task->group_id = fake_trpc_server.GetThreadModel()->GroupId();
    EXPECT_TRUE(trpc::merge::SubmitIoTask(fake_trpc_server.GetThreadModel(), task));
  }

  l.wait();
}

void FutureTransportImplTest::ConnectTimeout(std::unique_ptr<FutureTransport>& transport, int error_code) {
  NodeAddr addr;
  addr.ip = "255.255.255.1";
  addr.port = 10000;
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg = EncodeTrpcRequestProtocol(addr, "hello", unique_id.GenerateId());
  auto fut = transport->AsyncSendRecv(req_msg.Get()).Then([error_code](Future<CTransportRspMsg>&& rsp_fut) {
    EXPECT_TRUE(rsp_fut.IsFailed());
    auto ex = rsp_fut.GetException();
    EXPECT_TRUE(ex.GetExceptionCode() == error_code) << ex.GetExceptionCode();
    return MakeReadyFuture<>();
  });

  future::BlockingGet(std::move(fut));
}

void FutureTransportImplTest::ConnectionIdle(bool is_complex_conn, bool support_pipeline) {
  bool has_establish = false;
  bool has_close = false;
  uint32_t idle_timeout = 2000;
  FutureTransport::Options options = DefaultOptions();
  options.trans_info.is_complex_conn = is_complex_conn;
  options.trans_info.support_pipeline = support_pipeline;
  options.trans_info.conn_establish_function = [&has_establish](const Connection*) { has_establish = true; };
  options.trans_info.conn_close_function = [&has_close](const Connection*) { has_close = true; };
  options.trans_info.connection_idle_timeout = idle_timeout;
  FutureTransport transport;
  EXPECT_EQ(transport.Init(std::move(options)), 0);
  // Establishing a connection.
  auto addr = fake_trpc_server.GetBindAddr();
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg = EncodeTrpcRequestProtocol(addr, "hello", unique_id.GenerateId());
  auto fut = transport.AsyncSendRecv(req_msg.Get()).Then([](Future<CTransportRspMsg>&& rsp_fut) {
    EXPECT_TRUE(rsp_fut.IsReady());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(fut));

  // Currently, the idle check is performed every 10 seconds, so the maximum waiting time is 13(>10+2) seconds.
  uint32_t loop = 0;
  while (!has_close && loop++ < 13) {
    usleep(1000000);
  }

  // Checking if the connection has been disconnected.
  EXPECT_EQ(has_establish, true);
  EXPECT_EQ(has_close, true);

  transport.Stop();
  transport.Destroy();
}

void FutureTransportImplTest::DisConnect(bool is_complex_conn, bool support_pipeline) {
  bool has_establish = false;
  bool has_close = false;
  FutureTransport::Options options = DefaultOptions();
  options.trans_info.is_complex_conn = is_complex_conn;
  options.trans_info.support_pipeline = support_pipeline;
  options.trans_info.connection_idle_timeout = 60;
  options.trans_info.conn_establish_function = [&has_establish](const Connection*) { has_establish = true; };
  options.trans_info.conn_close_function = [&has_close](const Connection*) { has_close = true; };
  FutureTransport transport;
  EXPECT_EQ(transport.Init(std::move(options)), 0);
  // Establishing a connection.
  auto addr = fake_trpc_server.GetBindAddr();
  object_pool::LwUniquePtr<CTransportReqMsg> req_msg = EncodeTrpcRequestProtocol(addr, "hello", unique_id.GenerateId());
  auto fut = transport.AsyncSendRecv(req_msg.Get()).Then([](Future<CTransportRspMsg>&& rsp_fut) {
    EXPECT_TRUE(rsp_fut.IsReady());
    return MakeReadyFuture<>();
  });
  future::BlockingGet(std::move(fut));

  EXPECT_EQ(has_establish, true);
  // Disconnect connection
  transport.Disconnect(addr.ip);

  // Checking if the connection has been disconnected after a maximum waiting time of 2 seconds.
  uint32_t loop = 0;
  while (!has_close && loop++ < 20) {
    usleep(100000);
  }
  EXPECT_EQ(has_close, true);

  transport.Stop();
  transport.Destroy();
}

std::pair<uint32_t, int> FutureTransportImplTest::BackupRequestSuccess(std::unique_ptr<FutureTransport>& transport,
                                                                       const std::string& request_msg) {
  ExtendNodeAddr first_extend_addr;
  NodeAddr first_node = fake_trpc_server.GetBindAddr();
  first_extend_addr.addr = first_node;
  first_extend_addr.metadata["instance_id_1"] = "instance_1";

  ExtendNodeAddr second_extend_addr;
  NodeAddr second_node = fake_backup_trpc_server.GetBindAddr();
  second_extend_addr.addr = second_node;
  second_extend_addr.metadata["instance_id_2"] = "instance_2";

  std::vector<ExtendNodeAddr> extend_addrs;
  extend_addrs.emplace_back(std::move(first_extend_addr));
  extend_addrs.emplace_back(std::move(second_extend_addr));

  uint32_t request_id = unique_id.GenerateId();
  // construct_req_msg object is released at the transport layer.
  object_pool::LwUniquePtr<CTransportReqMsg> construct_req_msg =
      EncodeTrpcRequestProtocol(first_node, request_msg, request_id);
  construct_req_msg->context->SetBackupRequestDelay(20);
  construct_req_msg->context->SetBackupRequestAddrsByNaming(std::move(extend_addrs));
  auto context = construct_req_msg->context;
  auto future = transport->AsyncSendRecv(construct_req_msg.Leak())
                    .Then([&request_msg, request_id](Future<CTransportRspMsg>&& fut) {
                      if (fut.IsReady()) {
                        CTransportRspMsg rsp_msg = fut.GetValue0();

                        const auto& rsp = std::any_cast<ProtocolPtr>(rsp_msg.msg);
                        TrpcResponseProtocol* trpc_rsp = dynamic_cast<TrpcResponseProtocol*>(rsp.get());
                        trpc::test::helloworld::HelloReply reply;

                        EXPECT_EQ(trpc_rsp->rsp_header.request_id(), request_id);

                        NoncontiguousBufferInputStream nbis(&trpc_rsp->rsp_body);

                        // decode app data
                        EXPECT_EQ(reply.ParseFromZeroCopyStream(&nbis), true);

                        EXPECT_EQ(reply.msg(), request_msg);
                      } else {
                        // In this use case, it should not reach this point.
                        EXPECT_FALSE(true);
                        std::cout << "exception = " << fut.GetException().what() << std::endl;
                      }

                      return trpc::MakeReadyFuture<>();
                    });
  future::BlockingGet(std::move(future));

  auto backup_request_info = context->GetBackupRequestRetryInfo();
  return std::make_pair(backup_request_info->resend_count, backup_request_info->succ_rsp_node_index);
}

std::pair<uint32_t, int> FutureTransportImplTest::BackupRequestBothFailed(std::unique_ptr<FutureTransport>& transport,
                                                                          const std::string& request_msg,
                                                                          bool use_invalid_port) {
  std::vector<ExtendNodeAddr> extend_addrs;
  if (!use_invalid_port) {
    ExtendNodeAddr first_extend_addr;
    NodeAddr first_node = fake_trpc_server.GetBindAddr();
    first_extend_addr.addr = first_node;
    first_extend_addr.metadata["instance_id_1"] = "instance_1";

    ExtendNodeAddr second_extend_addr;
    NodeAddr second_node = fake_backup_trpc_server.GetBindAddr();
    second_extend_addr.addr = second_node;
    second_extend_addr.metadata["instance_id_2"] = "instance_2";
    extend_addrs.emplace_back(std::move(first_extend_addr));
    extend_addrs.emplace_back(std::move(second_extend_addr));
  } else {
    // Using the wrong server port.
    ExtendNodeAddr first_extend_addr;
    NodeAddr first_node = fake_trpc_server.GetBindAddr();
    first_node.port = trpc::util::GenRandomAvailablePort();
    first_extend_addr.addr = first_node;
    first_extend_addr.metadata["instance_id_1"] = "instance_1";

    ExtendNodeAddr second_extend_addr;
    NodeAddr second_node = fake_backup_trpc_server.GetBindAddr();
    second_node.port = trpc::util::GenRandomAvailablePort();
    second_extend_addr.addr = second_node;
    second_extend_addr.metadata["instance_id_2"] = "instance_2";
    extend_addrs.emplace_back(std::move(first_extend_addr));
    extend_addrs.emplace_back(std::move(second_extend_addr));
  }

  uint32_t request_id = unique_id.GenerateId();
  // construct_req_msg object is released at the transport layer.
  object_pool::LwUniquePtr<CTransportReqMsg> construct_req_msg =
      EncodeTrpcRequestProtocol(fake_trpc_server.GetBindAddr(), request_msg, request_id);
  construct_req_msg->context->SetBackupRequestDelay(20);
  construct_req_msg->context->SetBackupRequestAddrsByNaming(std::move(extend_addrs));
  auto context = construct_req_msg->context;

  auto future = transport->AsyncSendRecv(construct_req_msg.Leak()).Then([](Future<CTransportRspMsg>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    return trpc::MakeReadyFuture<>();
  });

  future::BlockingGet(std::move(future));

  auto backup_request_info = context->GetBackupRequestRetryInfo();
  return std::make_pair(backup_request_info->resend_count, backup_request_info->succ_rsp_node_index);
}
// Backup request was not triggered.
void FutureTransportImplTest::BackupRequestFirstSuccessWithoutResend(std::unique_ptr<FutureTransport>& transport) {
  auto [resend_count, success_index] = BackupRequestSuccess(transport, "hello");
  EXPECT_EQ(resend_count, 0);
  EXPECT_EQ(success_index, 0);
}

// The response for the first request has been received successfully, but the response for the second request has not
// been received yet.
void FutureTransportImplTest::BackupRequestFirstSuccessWithResend(std::unique_ptr<FutureTransport>& transport) {
  auto [resend_count, success_index] = BackupRequestSuccess(transport, kFirstRequestSuccessWithResend);
  EXPECT_EQ(resend_count, 1);
  EXPECT_EQ(success_index, 0);
}

// The response for the second request has been received successfully, but the response for the first request has not
// been received yet.
void FutureTransportImplTest::BackupRequestSecondSuccessWithResend(std::unique_ptr<FutureTransport>& transport) {
  auto [resend_count, success_index] = BackupRequestSuccess(transport, kSecondRequestSuccessWithResend);
  EXPECT_EQ(resend_count, 1);
  EXPECT_EQ(success_index, 1);
}

// Both responses have been received, and the first request's response has been successful.
void FutureTransportImplTest::BackupRequestBothSuccessWithResend(std::unique_ptr<FutureTransport>& transport) {
  auto [resend_count, success_index] = BackupRequestSuccess(transport, kBothSuccessWithResend);
  EXPECT_EQ(resend_count, 1);
  EXPECT_EQ(success_index, 0);
}

// Call the wrong backend nodes.
void FutureTransportImplTest::BackupRequestBothFailedWhenEndpointError(std::unique_ptr<FutureTransport>& transport) {
  auto [resend_count, success_index] = BackupRequestBothFailed(transport, kBothRequestFailWithResend, true);
  EXPECT_EQ(resend_count, 1);
  EXPECT_EQ(success_index, 0);
}

// Server response error
void FutureTransportImplTest::BackupRequestBothFailedWhenServerError(std::unique_ptr<FutureTransport>& transport) {
  auto [resend_count, success_index] = BackupRequestBothFailed(transport, kBothRequestFailWithResend, false);
  EXPECT_EQ(resend_count, 1);
  EXPECT_EQ(success_index, 0);
}

// Synchronous invoke successfully
TEST_F(FutureTransportImplTest, testSendRecv_normal) {
  COUT << "tcp_complex_transport" << std::endl;
  SendRecv(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  SendRecv(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  SendRecv(tcp_pipeline_transport);

  COUT << "udp_complex_transport" << std::endl;
  SendRecv(udp_complex_transport);

  COUT << "udp_pool_transport" << std::endl;
  SendRecv(udp_pool_transport);
}

// Synchronous invoke from handle thread successfully
TEST_F(FutureTransportImplTest, testSendRecvFromHandle_normal) {
  COUT << "tcp_complex_transport" << std::endl;
  SendRecvFromHandleThread(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  SendRecvFromHandleThread(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  SendRecvFromHandleThread(tcp_pipeline_transport);

  COUT << "udp_complex_transport" << std::endl;
  SendRecvFromHandleThread(udp_complex_transport);

  COUT << "udp_pool_transport" << std::endl;
  SendRecvFromHandleThread(udp_pool_transport);
}

// Asynchronous invoke successfully
TEST_F(FutureTransportImplTest, testAsyncSendRecv_normal) {
  COUT << "tcp_complex_transport" << std::endl;
  AsyncSendRecv(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  AsyncSendRecv(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  AsyncSendRecv(tcp_pipeline_transport);

  COUT << "udp_complex_transport" << std::endl;
  AsyncSendRecv(udp_complex_transport);

  COUT << "udp_pool_transport" << std::endl;
  AsyncSendRecv(udp_pool_transport);
}

// Oneway invoke successfully
TEST_F(FutureTransportImplTest, testSendOnly_normal) {
  COUT << "tcp_complex_transport" << std::endl;
  SendOnly(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  SendOnly(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  SendOnly(tcp_pipeline_transport);

  COUT << "udp_complex_transport" << std::endl;
  SendOnly(udp_complex_transport);

  COUT << "udp_pool_transport" << std::endl;
  SendOnly(udp_pool_transport);
}

// An asynchronous invoke was made to a non-existent backend node.
TEST_F(FutureTransportImplTest, testAsyncSendRecv_endpoint_error) {
  COUT << "tcp_complex_transport" << std::endl;
  AsyncSendRecvWhenEndpointError(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  AsyncSendRecvWhenEndpointError(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  AsyncSendRecvWhenEndpointError(tcp_pipeline_transport);

  COUT << "udp_complex_transport" << std::endl;
  AsyncSendRecvWhenEndpointError(udp_complex_transport, false);

  COUT << "udp_pool_transport" << std::endl;
  AsyncSendRecvWhenEndpointError(udp_pool_transport, false);
}

// An synchronous invoke was made to a non-existent backend node.
TEST_F(FutureTransportImplTest, testSendRecv_endpoint_error) {
  COUT << "tcp_complex_transport" << std::endl;
  SendRecvWhenEndpointError(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  SendRecvWhenEndpointError(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  SendRecvWhenEndpointError(tcp_pipeline_transport);

  COUT << "udp_complex_transport" << std::endl;
  SendRecvWhenEndpointError(udp_complex_transport, false);

  COUT << "udp_pool_transport" << std::endl;
  SendRecvWhenEndpointError(udp_pool_transport, false);
}

// An one way invoke was made to a non-existent backend node.
TEST_F(FutureTransportImplTest, testSendOnly_endpoint_error) {
  COUT << "tcp_complex_transport" << std::endl;
  SendOnlyWhenEndpointError(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  SendOnlyWhenEndpointError(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  SendOnlyWhenEndpointError(tcp_pipeline_transport);

  COUT << "udp_complex_transport" << std::endl;
  SendOnlyWhenEndpointError(udp_complex_transport);

  COUT << "udp_pool_transport" << std::endl;
  SendOnlyWhenEndpointError(udp_pool_transport);
}

// The server responded with an incorrect package after receiving the request.
TEST_F(FutureTransportImplTest, testAsyncSendRecv_decode_error) {
  COUT << "tcp_complex_transport" << std::endl;
  AsyncSendRecvWhenDecodeError(tcp_complex_transport, false);

  COUT << "tcp_pool_transport" << std::endl;
  AsyncSendRecvWhenDecodeError(tcp_pool_transport, true);

  COUT << "tcp_pipeline_transport" << std::endl;
  AsyncSendRecvWhenDecodeError(tcp_pipeline_transport, true);

  COUT << "udp_complex_transport" << std::endl;
  AsyncSendRecvWhenDecodeError(udp_complex_transport, false);

  COUT << "udp_pool_transport" << std::endl;
  AsyncSendRecvWhenDecodeError(udp_pool_transport, true);
}

// The server close connection after receiving the request.
TEST_F(FutureTransportImplTest, testAsyncSendRecv_server_close) {
  COUT << "tcp_complex_transport" << std::endl;
  AsyncSendRecvWhenServerClose(tcp_complex_transport, false);

  COUT << "tcp_pool_transport" << std::endl;
  AsyncSendRecvWhenServerClose(tcp_pool_transport, true);

  COUT << "tcp_pipeline_transport" << std::endl;
  AsyncSendRecvWhenServerClose(tcp_pipeline_transport, true);
}

// Invoke timeout
TEST_F(FutureTransportImplTest, testAsyncSendRecv_timeout) {
  COUT << "tcp_complex_transport" << std::endl;
  AsyncSendRecvTimeout(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  AsyncSendRecvTimeout(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  AsyncSendRecvTimeout(tcp_pipeline_transport);

  COUT << "udp_complex_transport" << std::endl;
  AsyncSendRecvTimeout(udp_complex_transport);

  COUT << "udp_pool_transport" << std::endl;
  AsyncSendRecvTimeout(udp_pool_transport);
}

// Create a fixed connection fail when the backend node is in wrong.
TEST_F(FutureTransportImplTest, testGetOrCreateConnector_when_endpoint_error) {
  COUT << "tcp_complex_transport" << std::endl;
  GetOrCreateConnectorWhenEndpointError(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  GetOrCreateConnectorWhenEndpointError(tcp_pool_transport);
}

// Testing the scenario where the allocated fixed connection count is insufficient.
TEST_F(FutureTransportImplTest, testGetOrCreateConnector_util_max_conn) {
  uint32_t max_conn_num = DefaultOptions().trans_info.max_conn_num;

  COUT << "tcp_complex_transport" << std::endl;
  GetOrCreateConnectorUtilMaxConn(tcp_complex_transport, max_conn_num, true);

  COUT << "tcp_pool_transport" << std::endl;
  GetOrCreateConnectorUtilMaxConn(tcp_pool_transport, max_conn_num, false);
}

// Using fixed connections for data transmission.
TEST_F(FutureTransportImplTest, testAsyncSendRecv_by_fixed_conn) {
  COUT << "tcp_complex_transport" << std::endl;
  AsyncSendRecvByFixedConnId(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  AsyncSendRecvByFixedConnId(tcp_pool_transport);
}

// Creating a stream: Currently only TCP connection multiplexing is supported.
TEST_F(FutureTransportImplTest, testCreateStream) {
  COUT << "tcp_complex_transport" << std::endl;
  CreateStream(tcp_complex_transport);
}

// Connection timeout.
TEST_F(FutureTransportImplTest, testConnectTimeout) {
  COUT << "tcp_complex_transport" << std::endl;
  ConnectTimeout(tcp_complex_transport, TrpcRetCode::TRPC_CLIENT_CONNECT_ERR);

  COUT << "tcp_pool_transport" << std::endl;
  ConnectTimeout(tcp_pool_transport, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR);

  COUT << "tcp_pipeline_transport" << std::endl;
  ConnectTimeout(tcp_pipeline_transport, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR);
}

// Idle connection recycling.
TEST_F(FutureTransportImplTest, testConnectionIdel) {
  std::thread t1 = std::thread([this]() {
    COUT << "tcp_complex_transport" << std::endl;
    ConnectionIdle(true, false);
  });

  std::thread t2 = std::thread([this]() {
    COUT << "tcp_pool_transport" << std::endl;
    ConnectionIdle(false, false);
  });

  std::thread t3 = std::thread([this]() {
    COUT << "tcp_pipeline_transport" << std::endl;
    ConnectionIdle(false, true);
  });

  t1.join();
  t2.join();
  t3.join();
}

// Disconnect connection.
TEST_F(FutureTransportImplTest, DisConnect) {
  COUT << "tcp_complex_transport" << std::endl;
  DisConnect(true, false);

  COUT << "tcp_pool_transport" << std::endl;
  DisConnect(false, false);

  COUT << "tcp_pipeline_transport" << std::endl;
  DisConnect(false, true);
}

// ************ Test cases of backup requst *************************
// The first request succeeded within the resend time and did not trigger a backup request.
TEST_F(FutureTransportImplTest, testBackupRequest_first_success_without_resend) {
  COUT << "tcp_complex_transport" << std::endl;
  BackupRequestFirstSuccessWithoutResend(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  BackupRequestFirstSuccessWithoutResend(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  BackupRequestFirstSuccessWithoutResend(tcp_pipeline_transport);
}

// The first request did not succeed within the resend time, triggering a backup request. Subsequently, when the first
// request received a successful response, the backup request was considered successful.
TEST_F(FutureTransportImplTest, testBackupRequest_first_success_with_resend) {
  COUT << "tcp_complex_transport" << std::endl;
  BackupRequestFirstSuccessWithResend(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  BackupRequestFirstSuccessWithResend(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  BackupRequestFirstSuccessWithResend(tcp_pipeline_transport);
}

// The first request did not succeed within the resend time, triggering a backup request. Subsequently, when the second
// request received a successful response, it was considered successful.
TEST_F(FutureTransportImplTest, testBackupRequest_second_success_with_resend) {
  COUT << "tcp_complex_transport" << std::endl;
  BackupRequestSecondSuccessWithResend(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  BackupRequestSecondSuccessWithResend(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  BackupRequestSecondSuccessWithResend(tcp_pipeline_transport);
}

// The first request did not succeed within the resend time, triggering a backup request. Subsequently, when both the
// backup requests received successful responses, they were considered successful.
TEST_F(FutureTransportImplTest, testBackupRequest_both_success_with_resend) {
  COUT << "tcp_complex_transport" << std::endl;
  BackupRequestFirstSuccessWithResend(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  BackupRequestFirstSuccessWithResend(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  BackupRequestFirstSuccessWithResend(tcp_pipeline_transport);
}

// The request failed as it called two incorrect nodes.
TEST_F(FutureTransportImplTest, testBackupRequest_both_failed_when_endpoint_error) {
  COUT << "tcp_complex_transport" << std::endl;
  BackupRequestBothFailedWhenEndpointError(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  BackupRequestBothFailedWhenEndpointError(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  BackupRequestBothFailedWhenEndpointError(tcp_pipeline_transport);
}

// The request failed as the server did not respond in a timely manner.
TEST_F(FutureTransportImplTest, testBackupRequest_both_failed_when_server_error) {
  COUT << "tcp_complex_transport" << std::endl;
  BackupRequestBothFailedWhenServerError(tcp_complex_transport);

  COUT << "tcp_pool_transport" << std::endl;
  BackupRequestBothFailedWhenServerError(tcp_pool_transport);

  COUT << "tcp_pipeline_transport" << std::endl;
  BackupRequestBothFailedWhenServerError(tcp_pipeline_transport);
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  ::testing::FLAGS_gtest_filter = "*.*";
  trpc::testing::FakeTrpcServer::InitRuntime();

  // use separate thread mode
  trpc::testing::FutureTransportImplTest::UseMergeThreadModel(false);
  TRPC_ASSERT(RUN_ALL_TESTS() == 0);

  // use merge thread model
  trpc::testing::FutureTransportImplTest::UseMergeThreadModel(true);
  TRPC_ASSERT(RUN_ALL_TESTS() == 0);

  trpc::testing::FakeTrpcServer::DestroyRuntime();
  return 0;
}
