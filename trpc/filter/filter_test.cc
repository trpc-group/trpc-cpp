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

#include "trpc/filter/filter.h"

#include <chrono>
#include <iostream>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/codec/trpc/trpc_client_codec.h"
#include "trpc/filter/filter_controller.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/server/server_context.h"
#include "trpc/transport/common/transport_message.h"
#include "trpc/util/log/logging.h"

namespace trpc::testing {

TEST(FilterPoint, GetMatchPoint) {
  EXPECT_EQ(GetMatchPoint(FilterPoint::SERVER_POST_RECV_MSG), FilterPoint::SERVER_PRE_SEND_MSG);
  EXPECT_EQ(GetMatchPoint(FilterPoint::SERVER_PRE_SEND_MSG), FilterPoint::SERVER_POST_RECV_MSG);

  EXPECT_EQ(GetMatchPoint(FilterPoint::CLIENT_PRE_SEND_MSG), FilterPoint::CLIENT_POST_RECV_MSG);
  EXPECT_EQ(GetMatchPoint(FilterPoint::CLIENT_POST_RECV_MSG), FilterPoint::CLIENT_PRE_SEND_MSG);

  EXPECT_EQ(GetMatchPoint(FilterPoint::SERVER_PRE_RPC_INVOKE), FilterPoint::SERVER_POST_RPC_INVOKE);
  EXPECT_EQ(GetMatchPoint(FilterPoint::SERVER_POST_RPC_INVOKE), FilterPoint::SERVER_PRE_RPC_INVOKE);

  EXPECT_EQ(GetMatchPoint(FilterPoint::CLIENT_PRE_RPC_INVOKE), FilterPoint::CLIENT_POST_RPC_INVOKE);
  EXPECT_EQ(GetMatchPoint(FilterPoint::CLIENT_POST_RPC_INVOKE), FilterPoint::CLIENT_PRE_RPC_INVOKE);
}

class ServerLoggingFilter : public trpc::MessageServerFilter {
 public:
  std::string Name() override { return "server_logging_filter"; }

  std::vector<FilterPoint> GetFilterPoint() override {
    std::vector<FilterPoint> points = {FilterPoint::SERVER_POST_RECV_MSG,
                                       FilterPoint::SERVER_PRE_SEND_MSG};
    return points;
  }

  void operator()(FilterStatus& status, FilterPoint point,
                  const ServerContextPtr& context) override {
    std::cout << "ServerLoggingFilter" << std::endl;
    status = FilterStatus::CONTINUE;
  }
};

class ClientLoggingFilter : public trpc::MessageClientFilter {
 public:
  std::string Name() override { return "client_logging_filter"; }

  std::vector<FilterPoint> GetFilterPoint() override {
    std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_SEND_MSG,
                                       FilterPoint::CLIENT_POST_RECV_MSG};
    return points;
  }

  void operator()(FilterStatus& status, FilterPoint point,
                  const ClientContextPtr& context) override {
    std::cout << "ClientLoggingFilter" << std::endl;
    status = FilterStatus::CONTINUE;
  }
};

TEST(ServerMessageFilter, ServerMessageFilterTest) {
  MessageServerFilterPtr p(new ServerLoggingFilter());

  FilterManager::GetInstance()->AddMessageServerFilter(p);
  EXPECT_TRUE(FilterManager::GetInstance()->GetMessageServerFilter(p->Name()) != nullptr);

  ServerContextPtr context = trpc::MakeRefCounted<ServerContext>();
  FilterController filter_controller;
  FilterStatus status =
      filter_controller.RunMessageServerFilters(FilterPoint::SERVER_POST_RECV_MSG, context);

  EXPECT_EQ(status, FilterStatus::CONTINUE);
}

TEST(ClientMessageFilter, ClientMessageFilterTest) {
  MessageClientFilterPtr p(new ClientLoggingFilter());

  EXPECT_EQ(FilterType::kUnspecified, p->Type());
  EXPECT_EQ(0, p->Init());

  FilterManager::GetInstance()->AddMessageClientFilter(p);
  EXPECT_TRUE(FilterManager::GetInstance()->GetMessageClientFilter(p->Name()) != nullptr);

  auto trpc_codec = std::make_shared<TrpcClientCodec>();
  ClientContextPtr context = trpc::MakeRefCounted<ClientContext>(trpc_codec);
  FilterController filter_controller;
  FilterStatus status =
      filter_controller.RunMessageClientFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);

  EXPECT_EQ(status, FilterStatus::CONTINUE);
}

}  // namespace trpc::testing
