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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/common/request_priority.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/overload_control/overload_control_defs.h"

namespace trpc::overload_control {
namespace testing {

TEST(ContextPriorityTest, ServerPriority) {
  auto context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(std::make_shared<TrpcRequestProtocol>());

  ASSERT_EQ(GetServerPriority(context), 0);
  DefaultSetServerPriority(context, 255);

  const auto& transinfo = context->GetPbReqTransInfo();
  auto it = transinfo.find(kTransinfoKeyTrpcPriority);
  ASSERT_NE(it, transinfo.end());
  ASSERT_EQ(it->second, "\xFF");

  ASSERT_EQ(GetServerPriority(context), 255);

  SetServerGetPriorityFunc([](const ServerContextPtr&) { return 1024; });
  ASSERT_EQ(GetServerPriority(context), 1024);
}

TEST(ContextPriorityTest, ClientPriority) {
  auto context = MakeRefCounted<ClientContext>();
  context->SetRequest(std::make_shared<TrpcRequestProtocol>());

  ASSERT_EQ(GetClientPriority(context), 0);
  DefaultSetClientPriority(context, 255);

  const auto& transinfo = context->GetPbReqTransInfo();
  auto it = transinfo.find(kTransinfoKeyTrpcPriority);
  ASSERT_NE(it, transinfo.end());
  ASSERT_EQ(it->second, "\xFF");

  ASSERT_EQ(GetClientPriority(context), 255);

  SetClientGetPriorityFunc([](const ClientContextPtr&) { return 1024; });
  ASSERT_EQ(GetClientPriority(context), 1024);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
