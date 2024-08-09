/*
 *
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2024 THL A29 Limited, a Tencent company.
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

#include "trpc/overload_control/smooth_filter/server_flow_controller_conf.h"

#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(Server_FlowControlLimiterConf, All) {
  Server_FlowControlLimiterConf conf;
  ASSERT_EQ(conf.service_name.empty(), true);
  ASSERT_EQ(conf.service_limiter.empty(), true);
  ASSERT_EQ(conf.is_report, false);
  ASSERT_EQ(conf.window_size, 0);
  ASSERT_EQ(conf.func_limiters.size(), 0);

  YAML::convert<Server_FlowControlLimiterConf> concurr_yaml;

  conf.service_name = "trpc.test.helloworld.Greeter";
  conf.service_limiter = "default(10000)";

  trpc::overload_control::Server_FuncLimiterConfig func_limiter;
  func_limiter.name = "SayHello";
  func_limiter.limiter = "default(200000)";
  func_limiter.window_size = 200;
  conf.func_limiters.push_back(func_limiter);
  conf.is_report = true;

  YAML::Node concurr_node = concurr_yaml.encode(conf);

  Server_FlowControlLimiterConf decode_conf;

  ASSERT_EQ(concurr_yaml.decode(concurr_node, decode_conf), true);

  ASSERT_STREQ(decode_conf.service_name.c_str(), "trpc.test.helloworld.Greeter");
  ASSERT_STREQ(decode_conf.service_limiter.c_str(), "default(10000)");
  ASSERT_EQ(decode_conf.window_size, 0);
  ASSERT_EQ(decode_conf.is_report, true);
  ASSERT_EQ(decode_conf.func_limiters.size(), 1);
  ASSERT_STREQ(decode_conf.func_limiters[0].name.c_str(), "SayHello");
  ASSERT_STREQ(decode_conf.func_limiters[0].limiter.c_str(), "default(200000)");
  ASSERT_EQ(decode_conf.func_limiters[0].window_size, 200);

  decode_conf.Display();
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
