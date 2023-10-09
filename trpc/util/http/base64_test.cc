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

#include "trpc/util/http/base64.h"

#include <string>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(Base64Test, Encode) {
  {
    std::string in = "";
    auto out = http::Base64Encode(std::begin(in), std::end(in));
    EXPECT_EQ("", out);
  }
  {
    std::string in = "f";
    auto out = http::Base64Encode(std::begin(in), std::end(in));
    EXPECT_EQ("Zg==", out);
  }
  {
    std::string in = "fo";
    auto out = http::Base64Encode(std::begin(in), std::end(in));
    EXPECT_EQ("Zm8=", out);
  }
  {
    std::string in = "foo";
    auto out = http::Base64Encode(std::begin(in), std::end(in));
    EXPECT_EQ("Zm9v", out);
  }
  {
    std::string in = "foob";
    auto out = http::Base64Encode(std::begin(in), std::end(in));
    EXPECT_EQ("Zm9vYg==", out);
  }
  {
    std::string in = "fooba";
    auto out = http::Base64Encode(std::begin(in), std::end(in));
    EXPECT_EQ("Zm9vYmE=", out);
  }
  {
    std::string in = "foobar";
    auto out = http::Base64Encode(std::begin(in), std::end(in));
    EXPECT_EQ("Zm9vYmFy", out);
  }
}

TEST(Base64Test, Decode) {
  {
    std::string in = "";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("", out);
  }
  {
    std::string in = "Zg==";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("f", out);
  }
  {
    std::string in = "Zm8=";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("fo", out);
  }
  {
    std::string in = "Zm9v";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("foo", out);
  }
  {
    std::string in = "Zm9vYg==";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("foob", out);
  }
  {
    std::string in = "Zm9vYmE=";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("fooba", out);
  }
  {
    std::string in = "Zm9vYmFy";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("foobar", out);
  }
  {
    // we check the number of valid input must be multiples of 4.
    std::string in = "12345";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("", out);
  }
  {
    // ending invalid character at the boundary of multiples of 4 is bad.
    std::string in = "aBcDeFgH\n";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("", out);
  }
  {
    // after seeing '=', subsequent input must be also '='.
    std::string in = "//12/A=B";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("", out);
  }
  {
    // additional '=' at the end is bad.
    std::string in = "Zm9vYg====";
    auto out = http::Base64Decode(std::begin(in), std::end(in));
    EXPECT_EQ("", out);
  }
}
}  // namespace trpc::testing
