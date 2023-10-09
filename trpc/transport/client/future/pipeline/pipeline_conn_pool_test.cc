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

#include "trpc/transport/client/future/pipeline/pipeline_conn_pool.h"

#include "gtest/gtest.h"

namespace trpc::testing {

class TestFutureConnector : public FutureConnector {
 public:
  TestFutureConnector(FutureConnectorOptions&& options) : FutureConnector(std::move(options)) {}

  int SendReqMsg(CTransportReqMsg* req_msg) override { return 0; }

  int SendOnly(CTransportReqMsg* req_msg) override { return 0; }

  ConnectionState GetConnectionState() override { return ConnectionState::kConnected; }
};

using TestPipelineConnPool = PipelineConnPool<TestFutureConnector>;

TEST(PipelineConnPool, Test) {
  uint64_t max_conn_num = 2;
  TestPipelineConnPool conn_pool(max_conn_num);

  const auto& connectors = conn_pool.GetConnectors();
  ASSERT_TRUE(connectors.size() == max_conn_num);

  for (size_t i = 0; i < max_conn_num; ++i) {
    uint64_t conn_id = conn_pool.GenAvailConnectorId();
    ASSERT_TRUE(conn_id < max_conn_num);
    bool res = conn_pool.AddConnector(conn_id, std::make_unique<TestFutureConnector>(FutureConnectorOptions{}));
    ASSERT_TRUE(res);
    ASSERT_TRUE(conn_pool.GetConnector(conn_id) != nullptr);
    ASSERT_TRUE(connectors[i] != nullptr);

    auto connector = conn_pool.GetAndDelConnector(conn_id);
    ASSERT_TRUE(connector != nullptr);
    ASSERT_TRUE(connectors[i] == nullptr);
  }
}

}  // namespace trpc::testing
