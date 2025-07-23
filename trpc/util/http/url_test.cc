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

#include "trpc/util/http/url.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(ParseUrl, ParseOk) {
  struct testing_args_t {
    std::string url;
    std::string err;
    std::string scheme;
    std::string userinfo;
    std::string host;
    std::string port;
    int integer_port;
    std::string path;
    std::string query;
    std::string fragment;
  };

  std::vector<testing_args_t> testings{
      // Contains full fields.
      {
          "  scheme://user:password@example.com:80/static/jquery.js?name=Tom&src=_web_#L10  ",
          "",
          "scheme",
          "user:password",
          "example.com",
          "80",
          80,
          "/static/jquery.js",
          "name=Tom&src=_web_",
          "L10",
      },

      // Contains only host.
      {
          "  foo://example.com/static/jquery.js?name=Tom&src=_web_#L10  ",
          "",
          "foo",
          "",
          "example.com",
          "",
          -1,
          "/static/jquery.js",
          "name=Tom&src=_web_",
          "L10",
      },
      {
          "bar://example-01.com:12345/static/jquery.js?name=Tom&src=_web_#L10  ",
          "",
          "bar",
          "",
          "example-01.com",
          "12345",
          12345,
          "/static/jquery.js",
          "name=Tom&src=_web_",
          "L10",
      },
      {
          "  example.com-02:12345 ",
          "",
          "",
          "",
          "example.com-02",
          "12345",
          12345,
          "",
          "",
          "",
      },
      {
          "  example.com-03 ",
          "",
          "",
          "",
          "example.com-03",
          "",
          -1,
          "",
          "",
          "",
      },

      // Without scheme.
      {
          "  user:password@example.com/static/jquery.js?name=Tom&src=_web_#L10  ",
          "",
          "",
          "user:password",
          "example.com",
          "",
          -1,
          "/static/jquery.js",
          "name=Tom&src=_web_",
          "L10",
      },

      // Without scheme and userinfo.
      {
          "  example.com/static/jquery.js?name=Tom&src=_web_#L10  ",
          "",
          "",
          "",
          "example.com",
          "",
          -1,
          "/static/jquery.js",
          "name=Tom&src=_web_",
          "L10",
      },

      // Without host.
      {
          "  /static/jquery.js?name=Tom&src=_web_#L10  ",
          "",
          "",
          "",
          "",
          "",
          -1,
          "/static/jquery.js",
          "name=Tom&src=_web_",
          "L10",
      },

      // Empty host.
      {
          "foo://",
          "",
          "foo",
          "",
          "",
          "",
          -1,
          "",
          "",
          "",
      },
  };

  for (const auto& t : testings) {
    trpc::http::Url parsed_url;
    std::string err;
    ASSERT_TRUE(trpc::http::ParseUrl(t.url, &parsed_url, &err));
    ASSERT_EQ(t.err, err);
    ASSERT_EQ(t.scheme, parsed_url.Scheme());
    ASSERT_EQ(t.userinfo, parsed_url.Userinfo());
    ASSERT_EQ(t.host, parsed_url.Host());
    ASSERT_EQ(t.port, parsed_url.Port());
    ASSERT_EQ(t.integer_port, parsed_url.IntegerPort());
    ASSERT_EQ(t.path, parsed_url.Path());
    ASSERT_EQ(t.query, parsed_url.Query());
    ASSERT_EQ(t.fragment, parsed_url.Fragment());
  }
}

TEST(ParseUrl, InvalidCharacter) {
  struct testing_args_t {
    std::string url;
    std::string err;
  };

  std::vector<testing_args_t> testings{
      {"foo bar:/user:password@example.com/static/jquery.js?name=Tom&src=_web_#L10 ", "invalid character in url"},
      {"foo bar://user:password@example.com/static/jquery.js?name=Tom&src=_web_#L10 ", "invalid character in url"},
      {"foobar://user xx:password@example.com/static/jquery.js?name=Tom&src=_web_#L10 ", "invalid character in url"},
      {"foobar://user:password xx@example.com/static/jquery.js?name=Tom&src=_web_#L10 ", "invalid character in url"},
      {"foobar://user:password@example -01.com/static/jquery.js?name=Tom&src=_web_#L10 ", "invalid character in url"},
      {"foobar://user:password@example.com /static/jquery.js?name=Tom&src=_web_#L10 ", "invalid character in url"},
      {"foobar://user:password@example.com/ static/jquery.js?name=Tom&src=_web_#L10 ", "invalid character in path"},
      {"foobar://user:password@example.com/static/jquery.js ?name=Tom&src=_web_#L10 ", "invalid character in path"},
      {"foobar://user:password@example.com/static/jquery.js? name=Tom&src=_web_#L10 ", "invalid character in query"},
      {"foobar://user:password@example.com/static/jquery.js?na me=Tom&src=_web_#L10 ", "invalid character in query"},
      {"foobar://user:password@example.com/static/jquery.js?name=Tom&src=_web_ #L10 ", "invalid character in query"},
      {"foobar://user:password@example.com/static/jquery.js?name=Tom&src=_web_# L10 ", "invalid character in fragment"},
      {"foobar://user:password@example.com/static/jquery.js?name=Tom&src=_web_#L 10 ", "invalid character in fragment"},
  };

  for (const auto& t : testings) {
    trpc::http::Url parsed_url;
    std::string err;
    ASSERT_FALSE(trpc::http::ParseUrl(t.url, &parsed_url, &err));
    ASSERT_EQ(t.err, err);
  }
}

TEST(UrlParser, Parse) {
  trpc::http::UrlParser parser(
      "https://user:password@example.com/market_library/utils/tree/master/url_parser?123=456#3-todo");
  ASSERT_TRUE(parser.IsValid());
  ASSERT_EQ(parser.Scheme(), "https");
  ASSERT_EQ(parser.Username(), "user");
  ASSERT_EQ(parser.Password(), "password");
  ASSERT_EQ(parser.Hostname(), "example.com");
  EXPECT_TRUE(parser.Port().empty());
  ASSERT_EQ(parser.Path(), "/market_library/utils/tree/master/url_parser");
  ASSERT_EQ(parser.Query(), "123=456");
  ASSERT_EQ(parser.Fragment(), "3-todo");
  ASSERT_EQ(parser.Request(), "/market_library/utils/tree/master/url_parser?123=456#3-todo");
}

TEST(UrlParser, ParseWithNoQuery) {
  trpc::http::UrlParser parser("https://user:password@example.com/market_library/utils/tree/master/url_parser");
  ASSERT_TRUE(parser.IsValid());
  ASSERT_EQ(parser.Scheme(), "https");
  ASSERT_EQ(parser.Username(), "user");
  ASSERT_EQ(parser.Password(), "password");
  ASSERT_EQ(parser.Hostname(), "example.com");
  EXPECT_TRUE(parser.Port().empty());
  ASSERT_EQ(parser.Path(), "/market_library/utils/tree/master/url_parser");
  EXPECT_TRUE(parser.Query().empty());
  EXPECT_TRUE(parser.Fragment().empty());
  ASSERT_EQ(parser.Request(), "/market_library/utils/tree/master/url_parser");
}

TEST(UrlParser, ParseWithNoPath) {
  trpc::http::UrlParser parser("https://example.com");
  ASSERT_TRUE(parser.IsValid());
  ASSERT_EQ(parser.Scheme(), "https");
  ASSERT_TRUE(parser.Username().empty());
  ASSERT_TRUE(parser.Password().empty());
  ASSERT_EQ(parser.Hostname(), "example.com");
  ASSERT_TRUE(parser.Port().empty());
  ASSERT_EQ(parser.Path(), "");
  ASSERT_TRUE(parser.Query().empty());
  ASSERT_TRUE(parser.Fragment().empty());
  ASSERT_EQ(parser.Request(), "");
}

TEST(UrlParser, ParseValid) {
  trpc::http::UrlParser parser;
  // Valid : scheme:path.
  auto ret = parser.Parse("https:/example.com/market_library/utils/tree/master/url_parser?123=456#3-todo");
  ASSERT_TRUE(ret);
  ASSERT_TRUE(parser.IsValid());
}

}  // namespace trpc::testing
