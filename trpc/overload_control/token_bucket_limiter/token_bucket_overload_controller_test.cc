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

#include <chrono>

#include "gtest/gtest.h"

#include "trpc/overload_control/token_bucket_limiter/token_bucket_overload_controller.h"
#include "trpc/overload_control/token_bucket_limiter/token_bucket_limiter_conf.h"
#include "trpc/codec/testing/protocol_testing.h"

namespace trpc::overload_control {
namespace testing {
	
TEST(TokenBucketOverloadController, All) {
  TokenBucketLimiterControlConf tb_conf;
  tb_conf.initial_token = 3;
  tb_conf.rate = 0;
  tb_conf.capacity = 100;
  TokenBucketOverloadController tb_controller(tb_conf);
  ServerOverloadControllerPtr tb = std::make_shared<TokenBucketOverloadController>();
  tb->Register(tb_conf);
  ASSERT_TRUE(tb->Init() == true);
  auto context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(std::make_shared<trpc::testing::TestProtocol>());
  ASSERT_TRUE(tb->BeforeSchedule(context));
  ASSERT_TRUE(tb->BeforeSchedule(context));
  ASSERT_TRUE(tb->BeforeSchedule(context));
  ASSERT_FALSE(tb->BeforeSchedule(context));

}

}	// namespace testing
}	// namespace trpc::overload_control

#endif
