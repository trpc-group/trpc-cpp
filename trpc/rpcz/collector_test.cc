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
#include "trpc/rpcz/collector.h"

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/rpcz/span.h"
#include "trpc/rpcz/util/rpcz_fixture.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/time.h"

namespace trpc::testing {

class CollectorTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/rpcz/testing/rpcz.yaml"), 0);

    // Pure server span.
    trpc::rpcz::Span* server_span = PackServerSpan(1);
    trpc::rpcz::RpczCollector::GetInstance()->Submit(server_span);

    // Pure client span.
    trpc::rpcz::Span* client_span = PackClientSpan(2);
    trpc::rpcz::RpczCollector::GetInstance()->Submit(client_span);

    // Server span with client sub span in route scenario.
    trpc::rpcz::Span* route_span = PackServerSpan(3);
    trpc::rpcz::Span* sub_span = PackClientSpan(4);
    route_span->AppendSubSpan(sub_span);
    trpc::rpcz::RpczCollector::GetInstance()->Submit(route_span);

    trpc::rpcz::Span* user_span = PackUserSpan(5);
    trpc::rpcz::RpczCollector::GetInstance()->Submit(user_span);

    PeripheryTaskScheduler::GetInstance()->Init();
    PeripheryTaskScheduler::GetInstance()->Start();
    trpc::rpcz::RpczCollector::GetInstance()->Start();
    usleep(300000);
  }

  static void TearDownTestCase() {
    trpc::rpcz::RpczCollector::GetInstance()->Stop();
    PeripheryTaskScheduler::GetInstance()->Stop();
    PeripheryTaskScheduler::GetInstance()->Join();
  }
};

/// @brief Admin query with no parameter.
TEST_F(CollectorTest, TryGetWithNoParams) {
  trpc::rpcz::RpczAdminReqData req_params;
  req_params.params_size = 0;
  std::string rpcz_data;

  trpc::rpcz::RpczCollector::GetInstance()->FillRpczData(req_params, rpcz_data);
  std::string::size_type position =
      rpcz_data.find("cost=180(us) span_type=S span_id=1 trpc.app.server.greater.SayHello request=100 response=0 [ok]");
  ASSERT_NE(position, rpcz_data.npos);
}

/// @brief Admin query with parameters.
TEST_F(CollectorTest, TryGetWithParams) {
  trpc::rpcz::RpczAdminReqData req_params;
  req_params.params_size = 1;
  req_params.span_id = 2;
  std::string rpcz_data;

  trpc::rpcz::RpczCollector::GetInstance()->FillRpczData(req_params, rpcz_data);
  std::string::size_type position =
      rpcz_data.find("start send request(100) to trpc.app.server.service(0.0.0.0:12345) protocol=trpc span_id=2");
  ASSERT_NE(position, rpcz_data.npos);
}

/// @brief Admin query with invalid parameters.
TEST_F(CollectorTest, TryGetWithParamsInvalid) {
  trpc::rpcz::RpczAdminReqData req_params;
  req_params.params_size = 999;
  req_params.span_id = 2;
  std::string rpcz_data;

  trpc::rpcz::RpczCollector::GetInstance()->FillRpczData(req_params, rpcz_data);
  std::cout << "rpcz_data:  " << rpcz_data << std::endl;
  std::string::size_type position =
      rpcz_data.find("start send request(100) to trpc.app.server.service(0.0.0.0:12345) protocol=trpc span_id=2");
  ASSERT_NE(position, rpcz_data.npos);
}

}  // namespace trpc::testing
#endif
