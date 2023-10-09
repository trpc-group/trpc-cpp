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

#include "trpc/util/http/util.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(UtilTest, UrlDeocde) {
  std::string out = "";

  EXPECT_EQ(true, trpc::http::UrlDecode("%23%24%23", out));
  EXPECT_EQ("#$#", out);

  EXPECT_EQ(true, trpc::http::UrlDecode("%22%26%22", out));
  EXPECT_EQ("\"&\"", out);
}

TEST(UtilTest, HexStrToChar) {
  std::string_view in = "A1BEC9";

  EXPECT_EQ('\xA1', trpc::http::HexstrToChar(in, 0));
  EXPECT_EQ('\xBE', trpc::http::HexstrToChar(in, 2));
  EXPECT_EQ('\xC9', trpc::http::HexstrToChar(in, 4));
}

TEST(UtilTest, HexToType) {
  EXPECT_EQ('\x0', trpc::http::HexToByte('0'));
  EXPECT_EQ('\xA', trpc::http::HexToByte('a'));
  EXPECT_EQ('\xA', trpc::http::HexToByte('A'));
  EXPECT_EQ('\x23', trpc::http::HexToByte('z'));
  EXPECT_EQ('\x23', trpc::http::HexToByte('Z'));
}

TEST(UtilTest, NormalizeUrl) {
  EXPECT_EQ("a", trpc::http::NormalizeUrl("a"));
  EXPECT_EQ("/abc", trpc::http::NormalizeUrl("/abc"));
  EXPECT_EQ("/abc", trpc::http::NormalizeUrl("/abc/"));
}

namespace {
constexpr std::array<char, 128> kAsciiCodes{
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
    30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
    45,  46,  47,  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 58,  59,
    60,  61,  62,  63,  64,  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
    'Z', 91,  92,  93,  94,  95,  96,  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    'x', 'y', 'z', 123, 124, 125, 126, 127
};
}  // namespace

TEST(InRfc3986UnreservedCharsTest, InRfc3986UnreservedCharsOk) {
  for (size_t i = 0; i < kAsciiCodes.size(); ++i) {
    if ((48 <= i && i <= 57) || (65 <= i && i <= 90) || (97 <= i && i <= 122)) {
      // Alphas + Digits
      EXPECT_TRUE(trpc::http::InRfc3986UnreservedChars(kAsciiCodes[i]));
      continue;
    }
    switch (kAsciiCodes[i]) {
      case '-':
      case '.':
      case '_':
      case '~':
        EXPECT_TRUE(trpc::http::InRfc3986UnreservedChars(kAsciiCodes[i]));
        break;
      default:
        EXPECT_FALSE(trpc::http::InRfc3986UnreservedChars(kAsciiCodes[i]));
        break;
    }
  }
}

TEST(InRfc3986SubDelimitersTest, InRfc3986SubDelimitersOk) {
  for (size_t i = 0; i < kAsciiCodes.size(); ++i) {
    switch (kAsciiCodes[i]) {
      // {'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';',  '='}
      case '!' :
      case '$' :
      case '&' :
      case '\'' :
      case '(' :
      case ')' :
      case '*' :
      case '+' :
      case ',' :
      case ';' :
      case '=' :
        EXPECT_TRUE(trpc::http::InRfc3986SubDelimiters(kAsciiCodes[i]));
        break;
      default:
        EXPECT_FALSE(trpc::http::InRfc3986SubDelimiters(kAsciiCodes[i]));
        break;
    }
  }
}

TEST(InTokenTest, InTokenOk) {
  for (size_t i = 0; i < kAsciiCodes.size(); ++i) {
    if ((48 <= i && i <= 57) || (65 <= i && i <= 90) || (97 <= i && i <= 122)) {
      EXPECT_TRUE(trpc::http::InToken(kAsciiCodes[i]));
      continue;
    }
    switch (kAsciiCodes[i]) {
      // {'!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|',  '~'}
      case '!':
      case '#':
      case '$':
      case '%':
      case '&':
      case '\'':
      case '*':
      case '+':
      case '-':
      case '.':
      case '^':
      case '_':
      case '`':
      case '|':
      case '~':
        EXPECT_TRUE(trpc::http::InToken(kAsciiCodes[i]));
        break;
      default:
        EXPECT_FALSE(trpc::http::InToken(kAsciiCodes[i]));
        break;
    }
  }
}

TEST(InAttrCharTest, InAttrCharOK) {
  for (size_t i = 0; i < kAsciiCodes.size(); ++i) {
    if ((48 <= i && i <= 57) || (65 <= i && i <= 90) || (97 <= i && i <= 122)) {
      EXPECT_TRUE(trpc::http::InToken(kAsciiCodes[i]));
      continue;
    }
    switch (kAsciiCodes[i]) {
      // {'!', '#', '$', '&', '+', '-', '.', '^', '_', '`', '|',  '~'}
      case '!':
      case '#':
      case '$':
      case '&':
      case '+':
      case '-':
      case '.':
      case '^':
      case '_':
      case '`':
      case '|':
      case '~':
        EXPECT_TRUE(trpc::http::InAttrChar(kAsciiCodes[i]));
        break;
      default:
        EXPECT_FALSE(trpc::http::InAttrChar(kAsciiCodes[i]));
        break;
    }
  }
}

TEST(HexToUintTest, HexToUintOk) {
  const std::array<char, 6> lowercase_alphas{'a', 'b', 'c', 'd', 'e', 'f'};
  const std::array<char, 6> uppercase_alphas{'A', 'B', 'C', 'D', 'E', 'F'};

  for (size_t i = 0; i < lowercase_alphas.size(); ++i) {
    EXPECT_EQ(i + 10, trpc::http::HexToUint(lowercase_alphas[i]));
    EXPECT_EQ(i + 10, trpc::http::HexToUint(uppercase_alphas[i]));
  }

  const std::array<char, 10> digits{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

  for (size_t i = 0; i < digits.size(); ++i) {
    EXPECT_EQ(i, trpc::http::HexToUint(digits[i]));
  }

  for (size_t i = 0; i < kAsciiCodes.size(); ++i) {
    if (!((48 <= i && i <= 57) || (65 <= i && i <= 90) || (97 <= i && i <= 122))) {
      EXPECT_EQ(256, trpc::http::HexToUint(kAsciiCodes[i]));
    }
  }
}

TEST(ParseUintTest, ParseUintOk) {
  uint8_t value_123[] = {'1', '2', '3'};
  EXPECT_EQ(123, trpc::http::ParseUint(value_123, 3));

  uint8_t value_0[] = {'0'};
  EXPECT_EQ(0, trpc::http::ParseUint(value_0, 1));
  EXPECT_EQ(-1, trpc::http::ParseUint(value_0, 0));

  uint8_t  value_abc[] = {'a', 'b', 'c'};
  EXPECT_EQ(-1, trpc::http::ParseUint(value_abc, 3));

  uint8_t  value_123abc[] = {'1', '2', '3', 'a', 'b', 'c'};
  EXPECT_EQ(-1, trpc::http::ParseUint(value_123abc, 6));

  uint8_t value_too_big[] = {
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 10
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 20
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 30
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 40
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 50
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 60
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 70
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 80
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 90
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  // 100
  };
  EXPECT_EQ(-1, trpc::http::ParseUint(value_too_big, 100));
}

TEST(PercentEncodeTest, PercentEncode) {
  EXPECT_EQ("%2Ffoo1%2Fbar%3F%26%2F%0A", trpc::http::PercentEncode("/foo1/bar?&/""\x0a"));
}

TEST(PercentEncodePathTest, PercentEncodePath) {
  EXPECT_EQ("/foo1/bar%3F&/%0A", trpc::http::PercentEncodePath("/foo1/bar?&/""\x0a"));
}

TEST(PercentDecodeTest, PercentDecode) {
  {
    std::string s = "%66%6F%6f%62%61%72";
    EXPECT_EQ("foobar", trpc::http::PercentDecode(s));
  }
  {
    std::string s = "%66%6";
    EXPECT_EQ("f%6", trpc::http::PercentDecode(s));
  }
  {
    std::string s = "%66%";
    EXPECT_EQ("f%", trpc::http::PercentDecode(s));
  }
}

TEST(StringStartsWithTest, StringStartsWithOk) {
  std::string str_a{"FooBar"};
  std::string str_b{"FooBar"};
  EXPECT_TRUE(trpc::http::StringStartsWith(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringStartsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringStartsWithLiterals(str_a, "FooBar"));
  EXPECT_TRUE(trpc::http::StringStartsWithLiteralsIgnoreCase(str_a, "FooBar"));


  str_a = "FooBar";
  str_b = "Foo";
  EXPECT_TRUE(trpc::http::StringStartsWith(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringStartsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringStartsWithLiterals(str_a, "Foo"));
  EXPECT_TRUE(trpc::http::StringStartsWithLiteralsIgnoreCase(str_a, "Foo"));

  str_a = "FooBar";
  str_b = "fOO";
  EXPECT_TRUE(!trpc::http::StringStartsWith(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringStartsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringStartsWithLiterals(str_a, "fOO"));
  EXPECT_TRUE(trpc::http::StringStartsWithLiteralsIgnoreCase(str_a, "fOO"));

  str_a = "FooBar";
  str_b = "fooBarFoo";
  EXPECT_TRUE(!trpc::http::StringStartsWith(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringStartsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringStartsWithLiterals(str_a, "fooBarFoo"));
  EXPECT_TRUE(!trpc::http::StringStartsWithLiteralsIgnoreCase(str_a, "fooBarFoo"));

  str_a = "";
  str_b = "";
  EXPECT_TRUE(trpc::http::StringStartsWith(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringStartsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringStartsWithLiterals(str_a, ""));
  EXPECT_TRUE(trpc::http::StringStartsWithLiteralsIgnoreCase(str_a, ""));
}

TEST(StringEndsWithTest, StringEndsWithOk) {
  std::string str_a{"FooBar"};
  std::string str_b{"FooBar"};
  EXPECT_TRUE(trpc::http::StringEndsWith(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEndsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEndsWithLiterals(str_a, "FooBar"));
  EXPECT_TRUE(trpc::http::StringEndsWithLiteralsIgnoreCase(str_a, "FooBar"));

  str_a = "FooBar";
  str_b = "Bar";
  EXPECT_TRUE(trpc::http::StringEndsWith(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEndsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEndsWithLiterals(str_a, "Bar"));
  EXPECT_TRUE(trpc::http::StringEndsWithLiteralsIgnoreCase(str_a, "Bar"));

  str_a = "FooBar";
  str_b = "bAR";
  EXPECT_TRUE(!trpc::http::StringEndsWith(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEndsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringEndsWithLiterals(str_a, "bAR"));
  EXPECT_TRUE(trpc::http::StringEndsWithLiteralsIgnoreCase(str_a, "bAR"));

  str_a = "FooBar";
  str_b = "fooBarFoo";
  EXPECT_TRUE(!trpc::http::StringEndsWith(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringEndsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringEndsWithLiterals(str_a, "fooBarFoo"));
  EXPECT_TRUE(!trpc::http::StringEndsWithLiteralsIgnoreCase(str_a, "fooBarFoo"));

  str_a = "";
  str_b = "";
  EXPECT_TRUE(trpc::http::StringEndsWith(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEndsWithIgnoreCase(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEndsWithLiterals(str_a, ""));
  EXPECT_TRUE(trpc::http::StringEndsWithLiteralsIgnoreCase(str_a, ""));
}

TEST(StringEqualsTest, StringEqualsOk) {
  std::string str_a{"FooBar"};
  std::string str_b{"FooBar"};
  EXPECT_TRUE(trpc::http::StringEquals(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEqualsIgnoreCase(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEqualsLiterals("FooBar", str_a));
  EXPECT_TRUE(trpc::http::StringEqualsLiteralsIgnoreCase("FooBar", str_a));
  EXPECT_TRUE(trpc::http::StringEqualsLiterals("FooBar", str_a.data(), 6));

  str_a = "FooBar";
  str_b = "fOObAR";
  EXPECT_TRUE(!trpc::http::StringEquals(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEqualsIgnoreCase(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringEqualsLiterals("fOObAR", str_a));
  EXPECT_TRUE(trpc::http::StringEqualsLiteralsIgnoreCase("fOObAR", str_a));
  EXPECT_TRUE(!trpc::http::StringEqualsLiterals("fOObAR", str_a.data(), 6));

  str_a = "FooBar";
  str_b = "Foo";
  EXPECT_TRUE(!trpc::http::StringEquals(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringEqualsIgnoreCase(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringEqualsLiterals("Foo", str_a));
  EXPECT_TRUE(!trpc::http::StringEqualsLiteralsIgnoreCase("Foo", str_a));
  EXPECT_TRUE(trpc::http::StringEqualsLiterals("Foo", str_a.data(), 3));

  str_a = "fooBar";
  str_b = "FooBarFoo";
  EXPECT_TRUE(!trpc::http::StringEquals(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringEqualsIgnoreCase(str_a, str_b));
  EXPECT_TRUE(!trpc::http::StringEqualsLiterals("FooBarFoo", str_a));
  EXPECT_TRUE(!trpc::http::StringEqualsLiteralsIgnoreCase("FooBarFoo", str_a));
  EXPECT_TRUE(!trpc::http::StringEqualsLiterals("FooBarFoo", str_a.data(), 6));

  str_a = "";
  str_b = "";
  EXPECT_TRUE(trpc::http::StringEquals(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEqualsIgnoreCase(str_a, str_b));
  EXPECT_TRUE(trpc::http::StringEqualsLiterals("", str_a));
  EXPECT_TRUE(trpc::http::StringEqualsLiteralsIgnoreCase("", str_a));
  EXPECT_TRUE(trpc::http::StringEqualsLiterals("", str_a.data(), 0));
}

TEST(UtilTest, ParseContentLength) {
  ASSERT_FALSE(trpc::http::ParseContentLength(""));
  ASSERT_FALSE(trpc::http::ParseContentLength("1234abcd"));
  ASSERT_FALSE(trpc::http::ParseContentLength("-12345678"));
  ASSERT_EQ(12345678, trpc::http::ParseContentLength("12345678").value_or(0));
}

}  // namespace trpc::testing
