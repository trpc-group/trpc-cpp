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
#include "trpc/rpcz/span.h"

#include "gtest/gtest.h"

#include "trpc/rpcz/collector.h"
#include "trpc/rpcz/util/rpcz_fixture.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc::testing {

/// @brief Test for normal set/get.
TEST(SpanTest, SetGetParamsTest) {
  trpc::rpcz::Span* span_ptr = trpc::object_pool::New<trpc::rpcz::Span>();
  if (span_ptr) {
    span_ptr->SetTraceId(1);
    ASSERT_EQ(span_ptr->TraceId(), 1);

    span_ptr->SetSpanId(1);
    ASSERT_EQ(span_ptr->SpanId(), 1);

    span_ptr->SetReceivedRealUs(1000);
    ASSERT_EQ(span_ptr->ReceivedRealUs(), 1000);

    span_ptr->SetCallType(0);

    span_ptr->SetRemoteSide("0.0.0.0:12345");
    ASSERT_EQ(span_ptr->RemoteSide(), "0.0.0.0:12345");

    span_ptr->SetProtocolName("trpc");
    ASSERT_EQ(span_ptr->ProtocolName(), "trpc");

    span_ptr->SetRequestSize(999);
    ASSERT_EQ(span_ptr->RequestSize(), 999);

    span_ptr->SetSpanType(trpc::rpcz::SpanType::kSpanTypeServer);

    span_ptr->SetFullMethodName("trpc.app.server.greater.SayHello");
    ASSERT_EQ(span_ptr->FullMethodName(), "trpc.app.server.greater.SayHello");

    span_ptr->SetRemoteName("trpc.app.server.greater");
    ASSERT_EQ(span_ptr->RemoteName(), "trpc.app.server.greater");

    span_ptr->AppendCustomLogs("rpcz log");
    ASSERT_EQ(span_ptr->CustomLogs(), "rpcz log");

    span_ptr->SetStartHandleRealUs(11111111111);
    ASSERT_EQ(span_ptr->StartHandleRealUs(), 11111111111);

    span_ptr->SetHandleRealUs(2222222222);
    ASSERT_EQ(span_ptr->HandleRealUs(), 2222222222);

    span_ptr->SetStartCallbackRealUs(33333333333);
    ASSERT_EQ(span_ptr->StartCallbackRealUs(), 33333333333);

    span_ptr->SetStartEncodeRealUs(44444444);
    ASSERT_EQ(span_ptr->StartEncodeRealUs(), 44444444);

    span_ptr->SetStartSendRealUs(55555555);
    ASSERT_EQ(span_ptr->StartSendRealUs(), 55555555);

    span_ptr->SetSendRealUs(66666666666);
    ASSERT_EQ(span_ptr->SendRealUs(), 66666666666);

    span_ptr->SetSendRealUs(7777777777);
    ASSERT_EQ(span_ptr->SendRealUs(), 7777777777);

    span_ptr->SetResponseSize(888);
    ASSERT_EQ(span_ptr->ResponseSize(), 888);

    span_ptr->SetErrorCode(0);
    ASSERT_EQ(span_ptr->ErrorCode(), 0);

    span_ptr->SetLastLogRealUs(9999999999);
    ASSERT_EQ(span_ptr->LastLogRealUs(), 9999999999);
    trpc::object_pool::Delete<trpc::rpcz::Span>(span_ptr);
  }
}

TEST(SpanTest, UserSpan) {
  trpc::rpcz::Span* span_ptr = trpc::object_pool::New<trpc::rpcz::Span>("test_user_span");
  ASSERT_NE(span_ptr, nullptr);

  ASSERT_EQ(span_ptr->ViewerName(), "test_user_span");

  trpc::rpcz::ViewerEvent begin_viewer_event;
  begin_viewer_event.type = trpc::rpcz::PhaseType::kPhaseTypeBegin;
  span_ptr->SetBeginViewerEvent(begin_viewer_event);
  ASSERT_EQ(span_ptr->BeginViewerEvent().type, trpc::rpcz::PhaseType::kPhaseTypeBegin);

  trpc::rpcz::ViewerEvent end_viewer_event;
  end_viewer_event.type = trpc::rpcz::PhaseType::kPhaseTypeEnd;
  span_ptr->SetEndViewerEvent(end_viewer_event);
  ASSERT_EQ(span_ptr->EndViewerEvent().type, trpc::rpcz::PhaseType::kPhaseTypeEnd);

  span_ptr->AddAttribute("name", "bob");
  ASSERT_EQ(span_ptr->Attributes().size(), 1);

  trpc::rpcz::Span* sub_span_ptr = span_ptr->CreateSubSpan("test_sub_span");
  ASSERT_NE(sub_span_ptr, nullptr);

  sub_span_ptr->End();
  ASSERT_EQ(sub_span_ptr->EndViewerEvent().type, trpc::rpcz::PhaseType::kPhaseTypeEnd);

  span_ptr->End();
  ASSERT_EQ(span_ptr->EndViewerEvent().type, trpc::rpcz::PhaseType::kPhaseTypeEnd);

  // Test UserSpanToString.
  std::string span_info = span_ptr->UserSpanToString();
  std::string::size_type pos_name = span_info.find("name");
  ASSERT_NE(pos_name, span_info.npos);
  std::string::size_type pos_ph = span_info.find("ph");
  ASSERT_NE(pos_ph, span_info.npos);
  std::string::size_type pos_ts = span_info.find("ts");
  ASSERT_NE(pos_ts, span_info.npos);
  std::string::size_type pos_pid = span_info.find("pid");
  ASSERT_NE(pos_pid, span_info.npos);
  std::string::size_type pos_tid = span_info.find("tid");
  ASSERT_NE(pos_tid, span_info.npos);
  std::string::size_type pos_args = span_info.find("args");
  ASSERT_NE(pos_args, span_info.npos);

  trpc::object_pool::Delete<trpc::rpcz::Span>(sub_span_ptr);
  trpc::object_pool::Delete<trpc::rpcz::Span>(span_ptr);
}

/// @brief Test for pure client span.
TEST(SpanTest, ClientSpanToString) {
  trpc::rpcz::Span* client_span = PackClientSpan(2);
  std::string span_info = client_span->ClientSpanToString();
  std::string::size_type p1 =
      span_info.find("start send request(100) to trpc.app.server.service(0.0.0.0:12345) protocol=trpc span_id=2");
  ASSERT_NE(p1, span_info.npos);
  std::string::size_type p2 = span_info.find("10(us) start transport invoke");
  ASSERT_NE(p2, span_info.npos);
  std::string::size_type p3 = span_info.find("10(us) enter send queue");
  ASSERT_NE(p3, span_info.npos);
  std::string::size_type p4 = span_info.find("10(us) leave send queue");
  ASSERT_NE(p4, span_info.npos);
  std::string::size_type p5 = span_info.find("10(us) io send done");
  ASSERT_NE(p5, span_info.npos);
  std::string::size_type p6 = span_info.find("10(us) enter recv queue");
  ASSERT_NE(p6, span_info.npos);
  std::string::size_type p7 = span_info.find("10(us) leave recv func");
  ASSERT_NE(p7, span_info.npos);
  std::string::size_type p8 = span_info.find("10(us) finish transport invoke");
  ASSERT_NE(p8, span_info.npos);
  std::string::size_type p9 = span_info.find("10(us) finish rpc invoke");
  ASSERT_NE(p9, span_info.npos);
  std::string::size_type p10 = span_info.find("0(us)Received response(70) from trpc.app.server.service(0.0.0.0:12345)");
  ASSERT_NE(p10, span_info.npos);
  trpc::object_pool::Delete<trpc::rpcz::Span>(client_span);
}

/// @brief Test for pure server span.
TEST(SpanTest, ServerSpanToString) {
  trpc::rpcz::Span* server_span = PackServerSpan(1);
  std::string span_info = server_span->ServerSpanToString();
  std::string::size_type p1 =
      span_info.find(" Received request(100) from trpc.app.server.greater(0.0.0.0:12345) protocol=trpc span_id=1");
  ASSERT_NE(p1, span_info.npos);
  std::string::size_type p2 = span_info.find("10(us) enter recv queue");
  ASSERT_NE(p2, span_info.npos);
  std::string::size_type p3 = span_info.find("10(us) leave recv queue");
  ASSERT_NE(p3, span_info.npos);
  std::string::size_type p4 = span_info.find("10(us) enter customer func");
  ASSERT_NE(p4, span_info.npos);
  std::string::size_type p5 = span_info.find("10(us) leave customer func");
  ASSERT_NE(p5, span_info.npos);
  std::string::size_type p6 = span_info.find("10(us) start encode");
  ASSERT_NE(p6, span_info.npos);
  std::string::size_type p7 = span_info.find("10(us) enter send queue");
  ASSERT_NE(p7, span_info.npos);
  std::string::size_type p8 = span_info.find("10(us) leave send queue");
  ASSERT_NE(p8, span_info.npos);
  std::string::size_type p9 = span_info.find("io send done");
  ASSERT_NE(p9, span_info.npos);
  std::string::size_type p10 = span_info.find("Send response(0) to trpc.app.server.greater(0.0.0.0:12345)");
  ASSERT_NE(p10, span_info.npos);
  trpc::object_pool::Delete<trpc::rpcz::Span>(server_span);
}

/// @brief Test for route scenario sub span.
TEST(SpanTest, FillServerSpanInfoWithSubspan) {
  trpc::rpcz::Span* route_span = PackServerSpan(3);
  trpc::rpcz::Span* sub_span = PackClientSpan(4);
  route_span->AppendSubSpan(sub_span);
  std::string span_info = route_span->ServerSpanToString();
  std::string::size_type p1 =
      span_info.find(" Received request(100) from trpc.app.server.greater(0.0.0.0:12345) protocol=trpc span_id=3");
  ASSERT_NE(p1, span_info.npos);
  std::string::size_type p2 = span_info.find("10(us) enter recv queue");
  ASSERT_NE(p2, span_info.npos);
  std::string::size_type p3 = span_info.find("10(us) leave recv queue");
  ASSERT_NE(p3, span_info.npos);
  std::string::size_type p4 = span_info.find("10(us) enter customer func");
  ASSERT_NE(p4, span_info.npos);
  std::string::size_type p5 = span_info.find("------------------------start----------------------------->");
  ASSERT_NE(p5, span_info.npos);

  std::string send_info =
      "start send request(100) to trpc.app.server.service(0.0.0.0:12345) protocol=trpc span_id=4";
  std::string::size_type p6 = span_info.find(send_info);
  ASSERT_NE(p6, span_info.npos);
  std::string::size_type p7 = span_info.find("10(us) start transport invoke");
  ASSERT_NE(p7, span_info.npos);
  std::string::size_type p8 = span_info.find("10(us) enter send queue");
  ASSERT_NE(p8, span_info.npos);
  std::string::size_type p9 = span_info.find("10(us) leave send queue");
  ASSERT_NE(p9, span_info.npos);
  std::string::size_type p10 = span_info.find("10(us) io send done");
  ASSERT_NE(p10, span_info.npos);
  std::string::size_type p11 = span_info.find("10(us) enter recv queue");
  ASSERT_NE(p11, span_info.npos);
  std::string::size_type p12 = span_info.find("10(us) leave recv func");
  ASSERT_NE(p12, span_info.npos);
  std::string::size_type p13 = span_info.find("10(us) finish transport invoke");
  ASSERT_NE(p13, span_info.npos);
  std::string::size_type p14 = span_info.find("10(us) finish rpc invoke");
  ASSERT_NE(p14, span_info.npos);
  std::string::size_type p15 = span_info.find("0(us)Received response(70) from trpc.app.server.service(0.0.0.0:12345)");
  ASSERT_NE(p15, span_info.npos);
  std::string::size_type p16 = span_info.find(" <------------------------end-------------------------------");
  ASSERT_NE(p16, span_info.npos);
  std::string::size_type p17 = span_info.find("leave customer func");
  ASSERT_NE(p17, span_info.npos);
  std::string::size_type p18 = span_info.find("10(us) start encode");
  ASSERT_NE(p18, span_info.npos);
  std::string::size_type p19 = span_info.find("10(us) enter send queue");
  ASSERT_NE(p19, span_info.npos);
  std::string::size_type p20 = span_info.find("10(us) leave send queue");
  ASSERT_NE(p20, span_info.npos);
  std::string::size_type p21 = span_info.find("10(us) io send done");
  ASSERT_NE(p21, span_info.npos);
  std::string::size_type p22 = span_info.find("Send response(0) to trpc.app.server.greater(0.0.0.0:12345)");
  ASSERT_NE(p22, span_info.npos);
  trpc::object_pool::Delete<trpc::rpcz::Span>(route_span);
  trpc::object_pool::Delete<trpc::rpcz::Span>(sub_span);
}

}  // namespace trpc::testing
#endif
