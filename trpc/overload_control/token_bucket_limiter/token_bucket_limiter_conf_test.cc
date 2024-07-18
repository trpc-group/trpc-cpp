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

#include "trpc/overload_control/token_bucket_limiter/token_bucket_limiter_conf.h"

#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(TokenBucketLimiterControlConf, All) {
  TokenBucketLimiterControlConf conf;
  ASSERT_EQ(conf.capacity, 1000);
  ASSERT_EQ(conf.initial_token, 0);
  ASSERT_EQ(conf.rate, 10);
  ASSERT_EQ(conf.is_report, false);
  
  YAML::convert<TokenBucketLimiterControlConf> concurr_yaml;
  
  conf.capacity = 20000;
  conf.initial_token = 5000;
  conf.rate = 2000;
  
  YAML::Node concurr_node = concurr_yaml.encode(conf);
  
  TokenBucketLimiterControlConf decode_conf;
  
  ASSERT_EQ(concurr_yaml.decode(concurr_node, decode_conf), true);
  
  ASSERT_EQ(decode_conf.capacity, 20000);
  ASSERT_EQ(decode_conf.initial_token, 5000);
  ASSERT_EQ(decode_conf.rate, 2000);
  
  decode_conf.Display();
}

}	// namespace testing
}	// namespace trpc::overload_control

#endif

