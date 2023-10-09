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

#include "trpc/util/http/match_rule.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/util/http/parameter.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc::testing {

class TestHandler : public trpc::http::HandlerBase {
 public:
  trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, trpc::http::RequestPtr req,
                      trpc::http::Response* rsp) {
    rsp->Done("html");
    return kDefaultStatus;
  }
};

TEST(MatchRule, Get) {
  trpc::http::Parameters param;
  auto h = std::make_shared<TestHandler>();

  trpc::http::MatchRule match_rule(h);
  match_rule.AddString("/hello").AddParam("param");
  // It depends on the insertion order.
  ASSERT_NE(h.get(), match_rule.Get("/hello", param));
  ASSERT_EQ(h.get(), match_rule.Get("/hello/", param));
  ASSERT_EQ("", param["param"]);
  ASSERT_EQ(h.get(), match_rule.Get("/hello/val1", param));
  ASSERT_EQ("val1", param["param"]);
  // Not match.
  ASSERT_EQ(nullptr, match_rule.Get("/hell/val1", param));
}

}  // namespace trpc::testing
