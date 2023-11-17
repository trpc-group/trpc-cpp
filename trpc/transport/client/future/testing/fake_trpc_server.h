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

#include <deque>
#include <iostream>
#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/codec/trpc/trpc_proto_checker.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/serialization/serialization_type.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/make_server_context.h"
#include "trpc/transport/server/default/server_transport_impl.h"
#include "trpc/util/buffer/zero_copy_stream.h"
#include "trpc/util/net_util.h"

namespace trpc::testing {

#define COUT std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "|"

const char kSecondRequestSuccessWithResend[] = "backup request success with resend";
const char kFirstRequestSuccessWithResend[] = "first request success with resend";
const char kBothSuccessWithResend[] = "both request success with resend";
const char kBothRequestFailWithResend[] = "both request fail with resend";

// Encode the TRPC response, and if error_packet is true, then reply an error packet.
bool EncodeTrpcResponseProtocol(ProtocolPtr& in, uint32_t request_id, NoncontiguousBuffer& out, bool error_packet) {
  bool ret = true;

  try {
    TrpcResponseProtocol* rsp = dynamic_cast<TrpcResponseProtocol*>(in.get());

    rsp->rsp_header.set_version(0);
    rsp->rsp_header.set_request_id(request_id);
    rsp->rsp_header.set_ret(0);
    rsp->rsp_header.set_func_ret(0);
    rsp->rsp_header.set_error_msg("");
    if (error_packet) {
      rsp->rsp_header.set_error_msg("error packet");
    }

    ret = rsp->ZeroCopyEncode(out);
    if (error_packet) {
      uint32_t rsp_header_size = rsp->rsp_header.ByteSizeLong();
      auto fixed_header = out.Cut(TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE);
      auto rsp_header = out.Cut(rsp_header_size);
      // Tamper with the PB header to construct an erroneous packet.
      std::vector<char> str;
      str.assign(rsp_header_size, '1');
      rsp_header = std::move(CreateBufferSlow(static_cast<void*>(&str[0]), rsp_header_size));

      fixed_header.Append(rsp_header);
      fixed_header.Append(out);
      out = std::move(fixed_header);
    }
  } catch (std::exception& ex) {
    ret = false;
  }

  return ret;
}

object_pool::LwUniquePtr<CTransportReqMsg> EncodeTrpcRequestProtocol(const NodeAddr& addr,
                                                                     std::string hello_request_msg,
                                                                     uint32_t hello_request_request_id,
                                                                     uint32_t timeout = 1000) {
  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg(hello_request_msg);

  TrpcRequestProtocol trpc_req_protocol;

  // Fill header
  trpc_req_protocol.req_header.set_version(0);
  trpc_req_protocol.req_header.set_request_id(hello_request_request_id);
  trpc_req_protocol.req_header.set_content_type(serialization::kPbType);

  // Fill body
  serialization::SerializationType serialization_type = serialization::kPbType;
  serialization::SerializationFactory* serializationfactory = serialization::SerializationFactory::GetInstance();
  TRPC_ASSERT(serializationfactory->Get(serialization_type));

  serialization::DataType type = serialization::kPbMessage;

  NoncontiguousBuffer data;
  bool encode_ret = serializationfactory->Get(serialization_type)->Serialize(type, &hello_req, &data);
  EXPECT_TRUE(encode_ret);

  trpc_req_protocol.SetNonContiguousProtocolBody(std::move(data));

  NoncontiguousBuffer buffer;
  trpc_req_protocol.ZeroCopyEncode(buffer);

  auto construct_req_msg = trpc::object_pool::MakeLwUnique<CTransportReqMsg>();

  auto codec = std::make_shared<TrpcClientCodec>();
  construct_req_msg->context = MakeRefCounted<ClientContext>(codec);
  construct_req_msg->context->SetAddr(addr.ip, addr.port);
  construct_req_msg->context->SetRequestId(hello_request_request_id);
  construct_req_msg->context->SetTimeout(timeout);
  construct_req_msg->extend_info = trpc::object_pool::MakeLwShared<trpc::ClientExtendInfo>();
  construct_req_msg->send_data = buffer;

  return construct_req_msg;
}

class FakeTrpcServer {
 public:
  static void InitRuntime() {
    int ret = TrpcConfig::GetInstance()->Init("./trpc/transport/client/future/testing/merge_separate_threadmodel.yaml");
    ASSERT_EQ(ret, 0);
    trpc::serialization::Init();
    trpc::codec::Init();

    // Start the thread model runtime environment.
    trpc::merge::StartRuntime();
    trpc::separate::StartRuntime();
  }

  static void DestroyRuntime() {
    // terminate the thread model runtime environment.
    trpc::merge::TerminateRuntime();
    trpc::separate::TerminateRuntime();
    trpc::codec::Destroy();
    trpc::serialization::Destroy();
  }

  static void InitThreadModel(bool merge = false) {
    // Get threadmodel
    if (merge) {
      thread_model_ = ThreadModelManager::GetInstance()->Get("default_merge");
    } else {
      thread_model_ = ThreadModelManager::GetInstance()->Get("default_separate");
    }
    ASSERT_TRUE(thread_model_ != nullptr);
  }

  static ThreadModel* GetThreadModel() { return thread_model_; }

  void SetUpServer(const std::string& network) {
    std::vector<Reactor*> reactors;
    if (thread_model_->Type() == kMerge) {
      reactors = ::trpc::merge::GetReactors(thread_model_);
    } else {
      reactors = ::trpc::separate::GetReactors(thread_model_);;
    }

    server_transport_ = std::make_unique<ServerTransportImpl>(std::move(reactors));

    bind_info_.is_ipv6 = false;
    bind_info_.socket_type = "net";
    bind_info_.ip = "127.0.0.1";
    bind_info_.port = trpc::util::GenRandomAvailablePort();
    bind_info_.network = network;
    bind_info_.protocol = "trpc";
    bind_info_.accept_function = nullptr;
    bind_info_.checker_function = [](const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
      return CheckTrpcProtocolMessage(conn, in, out);
    };

    bind_info_.msg_handle_function = [this](const ConnectionPtr& conn, std::deque<std::any>& msg) -> bool {
      EXPECT_TRUE(msg.size() > 0);

      for (auto it = msg.begin(); it != msg.end(); ++it) {
        auto& buff = std::any_cast<trpc::NoncontiguousBuffer&>(*it);

        ProtocolPtr req = std::make_shared<TrpcRequestProtocol>();
        auto ret = req->ZeroCopyDecode(buff);
        EXPECT_TRUE(ret == true);
        trpc::test::helloworld::HelloRequest hello_req;
        auto req_body_pb = req->GetNonContiguousProtocolBody();
        NoncontiguousBufferInputStream nbis(&req_body_pb);
        ret = hello_req.ParseFromZeroCopyStream(&nbis);
        if (!ret) {
          TRPC_ASSERT(false && "ParseFromZeroCopyStream failed");
        }
        nbis.Flush();

        uint32_t req_id;
        req->GetRequestId(req_id);

        // Process the request based on its content according to a specified strategy.
        COUT << "request_id:" << req_id << ",hello_req.msg():" << hello_req.msg() << std::endl;
        const std::string& req_str = hello_req.msg();
        if (!req_str.compare(kSecondRequestSuccessWithResend)) {
          // Simulate the first request to fail and the second request to succeed.
          static std::atomic<int> count = 0;
          auto count_value = count.fetch_add(1);
          if (count_value % 2 == 0) {
            continue;
          }
        } else if (!req_str.compare(kFirstRequestSuccessWithResend)) {
          // Simulate the first request to succeed and the second request to fail.
          static std::atomic<int> count = 0;
          auto count_value = count.fetch_add(1);
          if (count_value % 2 != 0) {
            continue;
          } else {
            // Delay the response by 50ms.
            DelayExecute([this, conn, msg = hello_req.msg(), req_id]() { DoResponse(conn, msg, req_id); }, 100);
            continue;
          }
        } else if (!req_str.compare(kBothSuccessWithResend)) {
          // Delay the response by 50ms to simulate the first request to respond first.
          DelayExecute([this, conn, msg = hello_req.msg(), req_id]() { DoResponse(conn, msg, req_id); }, 50);
          continue;
        } else if (!req_str.compare(kBothRequestFailWithResend)) {
          // Simulate the scenario where both requests fail.
          continue;
        } else if (req_str.find("timeout") != std::string::npos) {
          continue;
        } else if (req_str.find("oneway") != std::string::npos) {
          continue;
        } else if (req_str.find("close") != std::string::npos) {
          return false;
        } else if (req_str.find("decode error") != std::string::npos) {
          DoResponse(conn, hello_req.msg(), req_id, true);
          continue;
        }

        DoResponse(conn, hello_req.msg(), req_id);
      }

      return true;
    };
    bind_info_.run_server_filters_function = [](FilterPoint, STransportRspMsg*) { return FilterStatus::CONTINUE; };

    server_transport_->Bind(bind_info_);
    server_transport_->Listen();
  }

  void TearDownServer() {
    server_transport_->Stop();
    server_transport_.reset(nullptr);
  }

  void DoResponse(const ConnectionPtr& conn, const std::string& msg, uint32_t req_id, bool error_packet = false) {
    ProtocolPtr rsp_protocol = std::make_shared<TrpcResponseProtocol>();
    trpc::test::helloworld::HelloReply hello_rsp;
    hello_rsp.set_msg(msg);

    NoncontiguousBufferBuilder builder;
    {
      NoncontiguousBufferOutputStream nbos(&builder);
      if (!hello_rsp.SerializePartialToZeroCopyStream(&nbos)) {
        TRPC_ASSERT(false && "SerializePartialToZeroCopyStream failed");
        TRPC_ASSERT(false);
      }
      nbos.Flush();
      auto rsp_buffer = builder.DestructiveGet();
      rsp_protocol->SetNonContiguousProtocolBody(std::move(rsp_buffer));
    }

    // Encode TrpcResponseProtocol into binary data.
    NoncontiguousBuffer send_data;
    bool ret = EncodeTrpcResponseProtocol(rsp_protocol, req_id, send_data, error_packet);
    if (!ret) {
      TRPC_ASSERT(false && "request encode failed");
    }

    auto* rsp_msg = trpc::object_pool::New<STransportRspMsg>();
    rsp_msg->context = trpc::MakeServerContext();
    rsp_msg->context->SetConnectionId(conn->GetConnId());
    if (conn->GetConnType() == ConnectionType::kTcpLong || conn->GetConnType() == ConnectionType::kTcpShort) {
      rsp_msg->context->SetNetType(ServerContext::NetType::kTcp);
    } else if (conn->GetConnType() == ConnectionType::kUdp) {
      rsp_msg->context->SetNetType(ServerContext::NetType::kUdp);
    } else {
      assert(false);
    }

    rsp_msg->context->SetReserved(conn.Get());
    rsp_msg->context->SetIp(conn->GetPeerIp());
    rsp_msg->context->SetPort(conn->GetPeerPort());
    rsp_msg->buffer = std::move(send_data);

    server_transport_->SendMsg(rsp_msg);
  }

  static void DelayExecute(TimerExecutor&& executor, uint64_t delay_ms) {
    TRPC_ASSERT(thread_model_ != nullptr);
    Reactor* reactor = nullptr;
    if (thread_model_->Type() == kMerge) {
      reactor = trpc::merge::GetReactor(thread_model_, -1);
    } else {
      reactor = trpc::separate::GetReactor(thread_model_, -1);
    }
    TRPC_ASSERT(reactor != nullptr);
    auto timer_id = reactor->AddTimerAfter(delay_ms, 0, std::move(executor));
    TRPC_ASSERT(timer_id != kInvalidTimerId);
    reactor->DetachTimer(timer_id);
  }

  NodeAddr GetBindAddr() {
    NodeAddr server_addr;
    server_addr.ip = bind_info_.ip;
    server_addr.port = bind_info_.port;
    server_addr.addr_type = bind_info_.is_ipv6 ? NodeAddr::AddrType::kIpV6 : NodeAddr::AddrType::kIpV4;
    return server_addr;
  }

 private:
  static ThreadModel* thread_model_;
  std::unique_ptr<ServerTransportImpl> server_transport_;
  BindInfo bind_info_;
};

ThreadModel* FakeTrpcServer::thread_model_ = nullptr;

}  // namespace trpc::testing
