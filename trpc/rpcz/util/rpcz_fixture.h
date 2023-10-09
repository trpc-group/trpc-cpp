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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <utility>

#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/rpcz/span.h"
#include "trpc/server/rpc/rpc_service_impl.h"
#include "trpc/server/testing/server_context_testing.h"
#include "trpc/transport/server/server_transport_message.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/time.h"

namespace trpc::testing {

/// @brief Create and init a transport message with context and buffer set.
/// @note Do not want to create isolated cc file anymore, use maybe_unused here to make compiler happy.
[[maybe_unused]] static trpc::STransportReqMsg PackTransportReqMsg(trpc::ServicePtr& service) {
  DummyTrpcProtocol req_data;
  trpc::test::helloworld::HelloRequest hello_req;
  trpc::NoncontiguousBuffer req_bin_data;
  // Create request binary data here.
  EXPECT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));
  // Create service here.
  service = std::make_shared<trpc::RpcServiceImpl>();
  // Create server context, binary data already decode inside.
  trpc::ServerContextPtr context = MakeTestServerContext("trpc", service.get(), std::move(req_bin_data));
  // Continue to init server context.
  context->SetConnectionId(0);
  context->SetFd(1001);
  context->SetRecvTimestampUs(trpc::time::GetMicroSeconds());
  context->SetBeginTimestampUs(trpc::time::GetMicroSeconds());
  context->SetIp("127.0.0.1");
  context->SetPort(10000);
  // Create and init transport message.
  trpc::STransportReqMsg req_msg;
  req_msg.context = context;
  req_msg.buffer = std::move(req_bin_data);

  return req_msg;
}

/// @brief Generate and init a new client span.
/// @note Do not want to create isolated cc file anymore, use maybe_unused here to make compiler happy.
[[maybe_unused]] static trpc::rpcz::Span* PackClientSpan(uint32_t span_id) {
  uint64_t now_time = trpc::GetSystemMicroSeconds();
  trpc::rpcz::Span* span_ptr = trpc::object_pool::New<trpc::rpcz::Span>();
  span_ptr->SetTraceId(0);
  span_ptr->SetSpanId(span_id);
  span_ptr->SetStartRpcInvokeRealUs(now_time + 50);
  span_ptr->SetFirstLogRealUs(now_time + 50);
  span_ptr->SetCallType(0);
  span_ptr->SetRemoteSide("0.0.0.0:12345");
  span_ptr->SetRemoteName("trpc.app.server.service");
  span_ptr->SetTimeout(6000);
  span_ptr->SetProtocolName("trpc");
  span_ptr->SetRequestSize(100);
  span_ptr->SetSpanType(trpc::rpcz::SpanType::kSpanTypeClient);
  span_ptr->SetStartTransInvokeRealUs(now_time + 60);
  span_ptr->SetStartSendRealUs(now_time + 70);
  span_ptr->SetSendRealUs(now_time + 80);
  span_ptr->SetSendDoneRealUs(now_time + 90);
  span_ptr->SetStartHandleRealUs(now_time + 100);
  span_ptr->SetHandleRealUs(now_time + 110);
  span_ptr->SetTransInvokeDoneRealUs(now_time + 120);
  span_ptr->SetRpcInvokeDoneRealUs(now_time + 130);
  span_ptr->SetLastLogRealUs(now_time + 130);
  span_ptr->SetErrorCode(0);
  span_ptr->SetResponseSize(70);
  return span_ptr;
}

/// @brief Generate and init a new server span.
/// @note Do not want to create isolated cc file anymore, use maybe_unused here to make compiler happy.
[[maybe_unused]] static trpc::rpcz::Span* PackServerSpan(uint32_t span_id) {
  uint64_t now_time = trpc::GetSystemMicroSeconds();
  trpc::rpcz::Span* span_ptr = trpc::object_pool::New<trpc::rpcz::Span>();
  span_ptr->SetTraceId(0);
  span_ptr->SetSpanId(span_id);
  span_ptr->SetReceivedRealUs(now_time);
  span_ptr->SetFirstLogRealUs(now_time);
  span_ptr->SetCallType(0);
  span_ptr->SetRemoteSide("0.0.0.0:12345");
  span_ptr->SetTimeout(6000);
  span_ptr->SetProtocolName("trpc");
  span_ptr->SetRequestSize(100);
  span_ptr->SetSpanType(trpc::rpcz::SpanType::kSpanTypeServer);
  span_ptr->SetStartHandleRealUs(now_time + 10);
  span_ptr->SetHandleRealUs(now_time + 20);
  span_ptr->SetStartCallbackRealUs(now_time + 30);
  span_ptr->SetCallbackRealUs(now_time + 140);
  span_ptr->SetStartEncodeRealUs(now_time + 150);
  span_ptr->SetStartSendRealUs(now_time + 160);
  span_ptr->SetSendRealUs(now_time + 170);
  span_ptr->SetSendDoneRealUs(now_time + 180);
  span_ptr->SetFullMethodName("trpc.app.server.greater.SayHello");
  span_ptr->SetRemoteName("trpc.app.server.greater");
  span_ptr->SetErrorCode(0);
  span_ptr->SetLastLogRealUs(now_time + 180);
  return span_ptr;
}

[[maybe_unused]] static trpc::rpcz::Span* PackUserSpan(uint32_t span_id) {
  trpc::rpcz::Span* span_ptr = trpc::object_pool::New<trpc::rpcz::Span>("pack_user_span");
  span_ptr->SetTraceId(0);
  span_ptr->SetSpanId(span_id);
  span_ptr->AddAttribute("key1", "value1");
  span_ptr->AddAttribute("key2", "value2");
  span_ptr->End();
  return span_ptr;
}

}  // namespace trpc::testing
#endif
