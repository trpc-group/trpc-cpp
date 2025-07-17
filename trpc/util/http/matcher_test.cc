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

#include "trpc/util/http/matcher.h"

#include <vector>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(ParamMatcher, Match) {
  struct testing_args_t {
    std::string uri_path;
    std::size_t start_pos;
    std::size_t end_pos;
    std::string matched_path;
    std::string matched_param;
  };

  std::vector<testing_args_t> testings {
      {"/trpc/hello", 0, 5, "/trpc", "trpc"},
      {"/trpc/hello", 5, 11, "/hello", "hello"},
      {"/trpc/hello", 4, 5, "c", "c"},
      {"/trpc/hello", 6, 11, "hello", "hello"},
      {"/trpc/hello", 11, std::string::npos, "hello", "hello"},
  };

  trpc::http::ParamMatcher m("param");
  trpc::http::Parameters param;
  for (const auto& t : testings)  {
    ASSERT_EQ(t.end_pos, m.Match(t.uri_path, t.start_pos, param));
    ASSERT_EQ(t.matched_path, param.Path("param"));
    ASSERT_EQ(t.matched_param, param["param"]);
  }
}

TEST(ParamMatcher, MatchWithEntirePath) {
  trpc::http::ParamMatcher m("param", true);
  trpc::http::Parameters param;
  ASSERT_EQ(5, m.Match("/trpc", 5, param));
  ASSERT_EQ("", param.Path("param"));
  ASSERT_EQ("", param["param"]);

  ASSERT_EQ(5, m.Match("/trpc", 0, param));
  ASSERT_EQ("/trpc", param.Path("param"));
  ASSERT_EQ("trpc", param["param"]);
}

TEST(StringMatcher, Match) {
  trpc::http::StringMatcher m("/hello");
  trpc::http::Parameters param;
  ASSERT_EQ(6, m.Match("/hello", 0, param));
  ASSERT_EQ(6, m.Match("/hello/", 0, param));
  ASSERT_EQ(6, m.Match("/hello/world", 0, param));
  ASSERT_EQ(6, m.Match("/hello/world/x", 0, param));
  ASSERT_EQ(11, m.Match("/trpc/hello", 5, param));
  // Not matched.
  ASSERT_EQ(std::string::npos, m.Match("/trpc/hello", 0, param));
}

TEST(StringMatcher, MatchAll) {
  trpc::http::StringMatcher m("");
  trpc::http::Parameters param;
  ASSERT_EQ(0, m.Match("/", 0, param));
  ASSERT_EQ(0, m.Match("/trpc", 0, param));
  ASSERT_EQ(0, m.Match("/trpc/hello", 0, param));
}

TEST(StringMatcher, RegexMatch) {
  trpc::http::StringMatcher m("<regex(/img/[a-z]_.*)>");
  trpc::http::Parameters param;
  ASSERT_EQ(12, m.Match("/img/p_123/c", 0, param));
  ASSERT_EQ(14, m.Match("/img/p_123/c/d", 0, param));
  // Not match
  ASSERT_EQ(std::string::npos, m.Match("/img/1_123/c/d", 0, param));
}

TEST(StringMatcher, RegexMatchAll) {
  trpc::http::StringMatcher m("<regex(.*)>");
  trpc::http::Parameters param;
  ASSERT_EQ(1, m.Match("/", 0, param));
  ASSERT_EQ(12, m.Match("/img/p_123/c", 0, param));
  ASSERT_EQ(14, m.Match("/img/p_123/c/d", 0, param));
}

TEST(StringMatcher, BothStringAndRegexNotMatch) {
  trpc::http::StringMatcher m("/img/[a-z]_.*");
  trpc::http::Parameters param;
  EXPECT_EQ(std::string::npos, m.Match("/trpc/img/p_123/c", 5, param));
  EXPECT_EQ(std::string::npos, m.Match("/trpc/img/pp_123/c/d", 5, param));
}

TEST(PlaceholderMatcher, Match) {
  trpc::http::PlaceholderMatcher m("/channels/<channel_id>/clients/<client_id>");
  trpc::http::Parameters param;
  ASSERT_EQ(27, m.Match("/channels/x-y-z/clients/123", 0, param));
  ASSERT_EQ("x-y-z", param.Path("channel_id"));
  ASSERT_EQ("123", param.Path("client_id"));
}

TEST(PlaceholderMatcher, NotMatch) {
  trpc::http::PlaceholderMatcher m("/channels/<channel_id>/clients/<client_id>");
  trpc::http::Parameters param;
  EXPECT_NE(22, m.Match("/channels//clients/123", 0, param));
  EXPECT_NE(23, m.Match("/channels/x-y-z/clients", 0, param));
  EXPECT_NE(17, m.Match("/channels/clients", 0, param));
}

TEST(ProxyMatcher, Match) {
  trpc::http::Parameters param;

  trpc::http::StringProxyMatcher m1("/test");
  ASSERT_EQ(5, m1.Match("/test", 0, param));

  trpc::http::StringProxyMatcher m2("<regex(/img/[a-z]_.*)>");
  ASSERT_EQ(12, m2.Match("/img/p_123/c", 0, param));

  trpc::http::StringProxyMatcher m3("<ph(/channels/<channel_id>/clients/<client_id>)>");
  ASSERT_EQ(27, m3.Match("/channels/x-y-z/clients/123", 0, param));
  ASSERT_EQ("x-y-z", param.Path("channel_id"));
  ASSERT_EQ("123", param.Path("client_id"));
}

}  // namespace trpc::testing
