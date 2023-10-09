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

#include "trpc/util/http/http_cookie.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/util/time.h"

namespace trpc::testing  {

class HttpCookieTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(HttpCookieTest, ToStringVersionZeroOk) {
  {
    http::HttpCookie cookie{};
    std::string expect_cookie_text = "";

    EXPECT_EQ(0, cookie.GetVersion());
    EXPECT_EQ(expect_cookie_text, cookie.ToString());
  }
  {
    http::HttpCookie cookie{};
    std::string expect_cookie_text = "NAME=Tom";

    cookie.SetName("NAME");
    cookie.SetValue("Tom");

    EXPECT_EQ(0, cookie.GetVersion());
    EXPECT_EQ(expect_cookie_text, cookie.ToString());
  }
  {
    http::HttpCookie cookie{};

    cookie.SetName("NAME");
    cookie.SetValue("Tom");
    cookie.SetComment("Comment xx yy");
    cookie.SetDomain(".xx.example.com");
    cookie.SetPath("/xx/yy");
    cookie.SetSecure(true);
    cookie.SetMaxAge(80000);
    cookie.SetHttpOnly(true);
    cookie.SetSameSite(http::HttpCookie::kSameSiteStrict);

    EXPECT_EQ(0, cookie.GetVersion());

    // Expected cookie text like below:
    // `NAME=Tom; domain=.xx.example.com; path=/xx/yy; expires=Thu, 08 Jul 2021 11:08:25 GMT;
    // SameSite=Strict; secure; HttpOnly`
    std::string cookie_text = cookie.ToString();
    EXPECT_THAT(cookie_text, ::testing::StartsWith("NAME=Tom"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; domain=.xx.example.com"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; path=/xx/yy"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; expires="));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("GMT"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; SameSite=Strict"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; secure"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; HttpOnly"));
  }
  {
    std::unordered_map<std::string, std::string> name_value_pairs{
        {"NAME", "Tom"},
        {"comment", "Comment xx yy"},
        {"domain", ".xx.example.com"},
        {"path", "/xx/yy"},
        {"expires", "Thu, 08 Jul 2021 11:08:25 GMT"},
        {"SameSite", "Strict"},
        {"secure", ""},
        {"HttpOnly", ""},
    };
    http::HttpCookie cookie{name_value_pairs};

    EXPECT_EQ(0, cookie.GetVersion());

    // Expected cookie text like below:
    // `NAME=Tom; domain=.xx.example.com; path=/xx/yy; expires=Thu, 08 Jul 2021 11:08:25 GMT;
    // SameSite=Strict; secure; HttpOnly`
    std::string cookie_text = cookie.ToString();
    EXPECT_THAT(cookie_text, ::testing::StartsWith("NAME=Tom"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; domain=.xx.example.com"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; path=/xx/yy"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; expires="));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("GMT"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; SameSite=Strict"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; secure"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr("; HttpOnly"));
  }
}

TEST_F(HttpCookieTest, ToStringVersionOneOk) {
  {
    http::HttpCookie cookie{};
    std::string expect_cookie_text = "";
    cookie.SetVersion(1);

    EXPECT_EQ(1, cookie.GetVersion());
    EXPECT_EQ(expect_cookie_text, cookie.ToString());
  }
  {
    http::HttpCookie cookie{};
    std::string expect_cookie_text = R"(NAME="Tom"; Version="1")";
    cookie.SetVersion(1);

    EXPECT_EQ(1, cookie.GetVersion());
    cookie.SetName("NAME");
    cookie.SetValue("Tom");

    EXPECT_EQ(expect_cookie_text, cookie.ToString());
  }
  {
    http::HttpCookie cookie{};

    cookie.SetVersion(1);
    cookie.SetName("NAME");
    cookie.SetValue("Tom");
    cookie.SetComment("Comment xx yy");
    cookie.SetDomain(".xx.example.com");
    cookie.SetPath("/xx/yy");
    cookie.SetSecure(true);
    cookie.SetMaxAge(80000);
    cookie.SetHttpOnly(true);
    cookie.SetSameSite(http::HttpCookie::kSameSiteStrict);

    EXPECT_EQ(1, cookie.GetVersion());

    // Expected cookie text like below:
    // `NAME="Tom"; Comment="Comment xx yy"; Domain=".xx.example.com"; Path="/xx/yy";
    // Max-Age="80000"; SameSite=Strict; Secure; HttpOnly; Version="1"`
    std::string cookie_text = cookie.ToString();
    EXPECT_THAT(cookie_text, ::testing::StartsWith(R"(NAME="Tom")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Comment="Comment xx yy")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Domain=".xx.example.com")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Path="/xx/yy")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Max-Age="80000")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; SameSite=Strict)"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Secure)"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; HttpOnly)"));
  }

  {
    std::unordered_map<std::string, std::string> name_value_pairs{
        {"NAME", "Tom"},
        {"Comment", "Comment xx yy"},
        {"Domain", ".xx.example.com"},
        {"Path", "/xx/yy"},
        {"Max-Age", "80000"},
        {"SameSite", "Strict"},
        {"Secure", ""},
        {"HttpOnly", ""},
        {"Version", "1"},
    };
    http::HttpCookie cookie{name_value_pairs};

    EXPECT_EQ(1, cookie.GetVersion());

    // Expected cookie text like below:
    // `NAME="Tom"; Comment="Comment xx yy"; Domain=".xx.example.com"; Path="/xx/yy";
    // Max-Age="80000"; SameSite=Strict; Secure; HttpOnly; Version="1"`
    std::string cookie_text = cookie.ToString();
    EXPECT_THAT(cookie_text, ::testing::StartsWith(R"(NAME="Tom")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Comment="Comment xx yy")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Domain=".xx.example.com")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Path="/xx/yy")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Max-Age="80000")"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; SameSite=Strict)"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; Secure)"));
    EXPECT_THAT(cookie_text, ::testing::HasSubstr(R"(; HttpOnly)"));
  }
}

TEST_F(HttpCookieTest, ToStringVersionZeroOkWithDifferentSameSite) {
  http::HttpCookie cookie{};
  std::string expect_cookie_text = "NAME=Tom";

  cookie.SetName("NAME");
  cookie.SetValue("Tom");
  cookie.SetSameSite(http::HttpCookie::kSameSiteNotSpecified);

  EXPECT_EQ(0, cookie.GetVersion());
  EXPECT_EQ(expect_cookie_text, cookie.ToString());

  cookie.SetSameSite(http::HttpCookie::kSameSiteStrict);
  EXPECT_THAT(cookie.ToString(), ::testing::HasSubstr("; SameSite=Strict"));

  cookie.SetSameSite(http::HttpCookie::kSameSiteLax);
  EXPECT_THAT(cookie.ToString(), ::testing::HasSubstr("; SameSite=Lax"));

  cookie.SetSameSite(http::HttpCookie::kSameSiteNone);
  EXPECT_THAT(cookie.ToString(), ::testing::HasSubstr("; SameSite=None"));
}

TEST_F(HttpCookieTest, EscapeOk) {
  EXPECT_EQ("%2Ffoo1%2Fbar%3F%26%2F%0A", http::HttpCookie::Escape("/foo1/bar?&/""\x0a"));
}

TEST_F(HttpCookieTest, UnescapeOk) {
  {
    std::string s = "%66%6F%6f%62%61%72";
    EXPECT_EQ("foobar", http::HttpCookie::Unescape(s));
  }
  {
    std::string s = "%66%6";
    EXPECT_EQ("f%6", http::HttpCookie::Unescape(s));
  }
  {
    std::string s = "%66%";
    EXPECT_EQ("f%", http::HttpCookie::Unescape(s));
  }
}

}  // namespace trpc::testing
