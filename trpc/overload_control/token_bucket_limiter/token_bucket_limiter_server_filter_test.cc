/*
*
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2023 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/token_bucket_limiter/token_bucket_limiter_server_filter.h"

#include <vector>
#include <thread>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/runtime/common/stats/frame_stats.h"

namespace trpc::overload_control {
namespace testing {

class TokenBucketLimiterServerTestFixture : public ::testing::Test {
public:
  static void SetUpTestCase() {
	trpc::TrpcConfig::GetInstance()->Init("./trpc/overload_control/token_bucket_limiter/token_bucket_overload_ctrl.yaml");
	bool ret = InitTokenBucketLimiterServerFilter();
	ASSERT_TRUE(ret);
  }
  static void TearDownTestCase() {}

  static ServerContextPtr MakeServerContext() {
    auto context = MakeRefCounted<ServerContext>();
    context->SetRequestMsg(std::make_shared<trpc::testing::TestProtocol>());
    return context;
  }

  static bool InitTokenBucketLimiterServerFilter() {
	MessageServerFilterPtr token_bucket_limiter_filter(new overload_control::TokenBucketLimiterServerFilter());
	token_bucket_limiter_filter->Init();
	return FilterManager::GetInstance()->AddMessageServerFilter(token_bucket_limiter_filter);
  }
};

TEST_F(TokenBucketLimiterServerTestFixture, Init) {
  MessageServerFilterPtr filter = FilterManager::GetInstance()->GetMessageServerFilter(kTokenBucketLimiterName);
  ASSERT_NE(filter, nullptr);
  std::vector<FilterPoint> points = filter->GetFilterPoint();
  ASSERT_EQ(points.size(), 2);

}

// Test concurrency does not exceed the limit.
TEST_F(TokenBucketLimiterServerTestFixture, Ok) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kTokenBucketLimiterName);
  TokenBucketLimiterServerFilter* token_bucket_filter = static_cast<TokenBucketLimiterServerFilter*>(filter.get());

  ServerContextPtr context = MakeServerContext();
  FilterStatus status = FilterStatus::CONTINUE;
  token_bucket_filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
  token_bucket_filter->operator()(status, FilterPoint::SERVER_POST_SCHED_RECV_MSG, context);
  ASSERT_EQ(context->GetStatus().OK(), true);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
}

// Test concurrency exceeds the limit.
TEST_F(TokenBucketLimiterServerTestFixture, Overload) {
  MessageServerFilterPtr filter = trpc::FilterManager::GetInstance()->GetMessageServerFilter(kTokenBucketLimiterName);
  TokenBucketLimiterServerFilter* token_bucket_filter = static_cast<TokenBucketLimiterServerFilter*>(filter.get());

  uint32_t reject_count = 0;
  uint32_t concurr = 100;
  std::vector<std::thread> threads;
  for (uint32_t i = 1; i <= concurr; i++) {
    threads.emplace_back([&](){
      ServerContextPtr context = MakeServerContext();
      FilterStatus status = FilterStatus::CONTINUE;
      token_bucket_filter->operator()(status, FilterPoint::SERVER_PRE_SCHED_RECV_MSG, context);
      if (context->GetStatus().OK() == false) {
    	reject_count++;
    	ASSERT_EQ(status, FilterStatus::REJECT);
      }
    });
  }
  ASSERT_TRUE(reject_count >= 30);

}

}	// namespace testing
}	// namespace trpc::overload_control

#endif
