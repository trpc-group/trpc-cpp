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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include "trpc/metrics/prometheus/prometheus_common.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(PrometheusCommonTest, SplitName) {
  std::vector<std::string> vec;
  vec.reserve(4);

  // 1. testing with str equal to 4 segments
  std::string str1 = "trpc.app.server.service";
  vec.clear();
  trpc::prometheus::detail::SplitName(str1, '.', vec);
  ASSERT_EQ(4, vec.size());
  ASSERT_EQ("app", vec[trpc::prometheus::detail::kAppIndex]);
  ASSERT_EQ("server", vec[trpc::prometheus::detail::kServerIndex]);
  ASSERT_EQ("service", vec[trpc::prometheus::detail::kServiceIndex]);

  // 2. testing with str less than 4 segments
  std::string str2 = "trpc.app.server";
  vec.clear();
  trpc::prometheus::detail::SplitName(str2, '.', vec);
  ASSERT_EQ(3, vec.size());
  ASSERT_EQ("app", vec[trpc::prometheus::detail::kAppIndex]);
  ASSERT_EQ("server", vec[trpc::prometheus::detail::kServerIndex]);

  // 3. testing with str more than 4 segments
  std::string str3 = "trpc.app.server.service.other";
  vec.clear();
  trpc::prometheus::detail::SplitName(str3, '.', vec);
  ASSERT_EQ(4, vec.size());
  ASSERT_EQ("app", vec[trpc::prometheus::detail::kAppIndex]);
  ASSERT_EQ("server", vec[trpc::prometheus::detail::kServerIndex]);
  ASSERT_EQ("service.other", vec[trpc::prometheus::detail::kServiceIndex]);

  // 4. testing boundary
  std::string str4 = ".app.server.service";
  vec.clear();
  trpc::prometheus::detail::SplitName(str4, '.', vec);
  ASSERT_EQ(4, vec.size());
  ASSERT_EQ("app", vec[trpc::prometheus::detail::kAppIndex]);
  ASSERT_EQ("server", vec[trpc::prometheus::detail::kServerIndex]);
  ASSERT_EQ("service", vec[trpc::prometheus::detail::kServiceIndex]);

  std::string str5 = "trpc.app.server.";
  vec.clear();
  trpc::prometheus::detail::SplitName(str5, '.', vec);
  ASSERT_EQ(3, vec.size());
  ASSERT_EQ("app", vec[trpc::prometheus::detail::kAppIndex]);
  ASSERT_EQ("server", vec[trpc::prometheus::detail::kServerIndex]);

  std::string str6 = "...";
  vec.clear();
  trpc::prometheus::detail::SplitName(str6, '.', vec);
  ASSERT_EQ(3, vec.size());
  ASSERT_EQ("", vec[trpc::prometheus::detail::kAppIndex]);
  ASSERT_EQ("", vec[trpc::prometheus::detail::kServerIndex]);
}

TEST(PrometheusCommonTest, GetServiceName) {
  // 1. testing with str equal to 4 segments
  std::string str1 = "trpc.app.server.service";
  ASSERT_EQ("service", trpc::prometheus::detail::GetServiceName(str1, '.'));

  // 2. testing with str less than 4 segments
  std::string str2 = "trpc.app.server";
  ASSERT_EQ("", trpc::prometheus::detail::GetServiceName(str2, '.'));

  // 3. testing with str more than 4 segments
  std::string str3 = "trpc.app.server.service.other";
  ASSERT_EQ("service.other", trpc::prometheus::detail::GetServiceName(str3, '.'));

  // 4. testing boundary
  std::string str4 = ".app.server.service";
  ASSERT_EQ("service", trpc::prometheus::detail::GetServiceName(str4, '.'));

  std::string str5 = "trpc.app.server.";
  ASSERT_EQ("", trpc::prometheus::detail::GetServiceName(str5, '.'));

  std::string str6 = "...";
  ASSERT_EQ("", trpc::prometheus::detail::GetServiceName(str6, '.'));
}

TEST(PrometheusCommonTest, SetCallerServiceInfo) {
  std::map<std::string, std::string> infos;

  // 1. testing with caller service name equal to 4 segments
  std::string str1 = "trpc.app.server.service";
  infos.clear();
  trpc::prometheus::detail::SetCallerServiceInfo(str1, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCallerAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCallerServerKey]);
  ASSERT_EQ("service", infos[trpc::prometheus::kCallerServiceKey]);

  // 2. testing with caller service name less than 4 segments
  std::string str2 = "trpc.app.server";
  infos.clear();
  trpc::prometheus::detail::SetCallerServiceInfo(str2, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCallerAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCallerServerKey]);
  ASSERT_EQ("", infos[trpc::prometheus::kCallerServiceKey]);

  // 3. testing with caller service name more than 4 segments
  std::string str3 = "trpc.app.server.service.other";
  infos.clear();
  trpc::prometheus::detail::SetCallerServiceInfo(str3, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCallerAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCallerServerKey]);
  ASSERT_EQ("service.other", infos[trpc::prometheus::kCallerServiceKey]);

  // 4. testing boundary
  std::string str4 = ".app.server.service";
  infos.clear();
  trpc::prometheus::detail::SetCallerServiceInfo(str4, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCallerAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCallerServerKey]);
  ASSERT_EQ("service", infos[trpc::prometheus::kCallerServiceKey]);

  std::string str5 = "trpc.app.server.";
  infos.clear();
  trpc::prometheus::detail::SetCallerServiceInfo(str5, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCallerAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCallerServerKey]);
  ASSERT_EQ("", infos[trpc::prometheus::kCallerServiceKey]);

  std::string str6 = "...";
  infos.clear();
  trpc::prometheus::detail::SetCallerServiceInfo(str6, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("", infos[trpc::prometheus::kCallerAppKey]);
  ASSERT_EQ("", infos[trpc::prometheus::kCallerServerKey]);
  ASSERT_EQ("", infos[trpc::prometheus::kCallerServiceKey]);
}

TEST(PrometheusCommonTest, SetCalleeServiceInfo) {
  std::map<std::string, std::string> infos;

  // 1. testing with callee service name equal to 4 segments
  std::string str1 = "trpc.app.server.service";
  infos.clear();
  trpc::prometheus::detail::SetCalleeServiceInfo(str1, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCalleeAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCalleeServerKey]);
  ASSERT_EQ("service", infos[trpc::prometheus::kCalleeServiceKey]);

  // 2. testing with callee service name less than 4 segments
  std::string str2 = "trpc.app.server";
  infos.clear();
  trpc::prometheus::detail::SetCalleeServiceInfo(str2, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCalleeAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCalleeServerKey]);
  ASSERT_EQ("", infos[trpc::prometheus::kCalleeServiceKey]);

  // 3. testing with callee service name more than 4 segments
  std::string str3 = "trpc.app.server.service.other";
  infos.clear();
  trpc::prometheus::detail::SetCalleeServiceInfo(str3, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCalleeAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCalleeServerKey]);
  ASSERT_EQ("service.other", infos[trpc::prometheus::kCalleeServiceKey]);

  // 4. testing boundary
  std::string str4 = ".app.server.service";
  infos.clear();
  trpc::prometheus::detail::SetCalleeServiceInfo(str4, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCalleeAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCalleeServerKey]);
  ASSERT_EQ("service", infos[trpc::prometheus::kCalleeServiceKey]);

  std::string str5 = "trpc.app.server.";
  infos.clear();
  trpc::prometheus::detail::SetCalleeServiceInfo(str5, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("app", infos[trpc::prometheus::kCalleeAppKey]);
  ASSERT_EQ("server", infos[trpc::prometheus::kCalleeServerKey]);
  ASSERT_EQ("", infos[trpc::prometheus::kCalleeServiceKey]);

  std::string str6 = "...";
  infos.clear();
  trpc::prometheus::detail::SetCalleeServiceInfo(str6, infos);
  ASSERT_EQ(3, infos.size());
  ASSERT_EQ("", infos[trpc::prometheus::kCalleeAppKey]);
  ASSERT_EQ("", infos[trpc::prometheus::kCalleeServerKey]);
  ASSERT_EQ("", infos[trpc::prometheus::kCalleeServiceKey]);
}

}  // namespace trpc::testing
#endif
