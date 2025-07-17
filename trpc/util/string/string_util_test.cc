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

#include "trpc/util/string/string_util.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace trpc::util::testing {

TEST(StringUtilTest, TrimTest) {
  EXPECT_EQ(Trim(""), "");

  EXPECT_EQ(Trim("1"), "1");
  EXPECT_EQ(Trim("  1"), "1");
  EXPECT_EQ(Trim("1  "), "1");
  EXPECT_EQ(Trim("  1  "), "1");

  EXPECT_EQ(Trim("1  1"), "1  1");
  EXPECT_EQ(Trim("  1  1"), "1  1");
  EXPECT_EQ(Trim("1  1  "), "1  1");
  EXPECT_EQ(Trim("  1  1  "), "1  1");
}

TEST(StringUtilTest, TrimStringViewTest) {
  EXPECT_EQ(TrimStringView(""), "");

  EXPECT_EQ(TrimStringView("1"), "1");
  EXPECT_EQ(TrimStringView("  1"), "1");
  EXPECT_EQ(TrimStringView("1  "), "1");
  EXPECT_EQ(TrimStringView("  1  "), "1");

  EXPECT_EQ(TrimStringView("1  1"), "1  1");
  EXPECT_EQ(TrimStringView("  1  1"), "1  1");
  EXPECT_EQ(TrimStringView("1  1  "), "1  1");
  EXPECT_EQ(TrimStringView("  1  1  "), "1  1");
}

TEST(StringUtilTest, TrimPrefixStringViewTest) {
  EXPECT_EQ(TrimPrefixStringView("", ""), "");
  EXPECT_EQ(TrimPrefixStringView("1", "1"), "");
  EXPECT_EQ(TrimPrefixStringView("12", "1"), "2");
  EXPECT_EQ(TrimPrefixStringView("12", "012"), "12");
}

TEST(StringUtilTest, TrimSuffixStringViewTest) {
  EXPECT_EQ(TrimSuffixStringView("", ""), "");
  EXPECT_EQ(TrimSuffixStringView("1", "1"), "");
  EXPECT_EQ(TrimSuffixStringView("12", "2"), "1");
  EXPECT_EQ(TrimSuffixStringView("12", "123"), "12");
}

TEST(StringUtilTest, SplitStringTest) {
  std::string test_str = "127.0.0.1:1001,0.0.0.0:1002,1.1.1.1:2008, ";

  auto vec1 = SplitString(test_str, ',');

  EXPECT_EQ(vec1.size(), 4);
  EXPECT_EQ(vec1[0], "127.0.0.1:1001");
  EXPECT_EQ(vec1[1], "0.0.0.0:1002");
  EXPECT_EQ(vec1[2], "1.1.1.1:2008");
  EXPECT_EQ(vec1[3], " ");

  auto vec2 = SplitString(vec1[1], ':');
  EXPECT_EQ(1002, Convert<uint32_t>(vec2[1]));

  std::string test = "|123|456|";
  auto vec3 = SplitString(test, '|');
  EXPECT_EQ(2, vec3.size());
  EXPECT_EQ("123", vec3[0]);
  EXPECT_EQ("456", vec3[1]);
}

TEST(StringUtilTest, SplitStringViewTest) {
  std::string test_str = "127.0.0.1:1001,0.0.0.0:1002,1.1.1.1:2008, ";

  auto vec1 = SplitStringView(test_str, ',');

  EXPECT_EQ(vec1.size(), 4);
  EXPECT_EQ(vec1[0], "127.0.0.1:1001");
  EXPECT_EQ(vec1[1], "0.0.0.0:1002");
  EXPECT_EQ(vec1[2], "1.1.1.1:2008");
  EXPECT_EQ(vec1[3], " ");

  auto vec2 = SplitStringView(vec1[1], ':');
  EXPECT_EQ(1002, Convert<uint32_t>(vec2[1]));

  std::string test = "|123|456|";
  auto vec3 = SplitStringView(test, '|');
  EXPECT_EQ(2, vec3.size());
  EXPECT_EQ("123", vec3[0]);
  EXPECT_EQ("456", vec3[1]);
}

TEST(StringUtilTest, SplitStringToMapTest) {
  {
    std::string str;
    auto ret = SplitStringToMap(str, '&', '=');
    EXPECT_TRUE(ret.empty());
  }

  {
    std::string str("a=1&b=2&c=3=4&d");
    auto ret = SplitStringToMap(str, '&', '=');
    EXPECT_EQ(ret.size(), 4);
    EXPECT_EQ(ret["a"], "1");
    EXPECT_EQ(ret["b"], "2");
    EXPECT_EQ(ret["c"], "3");
    EXPECT_TRUE(ret["d"].empty());
  }
  // trim
  {
    std::string str(" a = 1 & b = 2 & c = & = 4 ");
    auto ret = SplitStringToMap(str, '&', '=');
    EXPECT_EQ(ret.size(), 3);
    EXPECT_EQ(ret["a"], "1");
    EXPECT_EQ(ret["b"], "2");
    EXPECT_TRUE(ret["c"].empty());
  }
  // no trim
  {
    std::string str(" a = 1 & b = 2 & c = & = 4 ");
    auto ret = SplitStringToMap(str, '&', '=', false);
    EXPECT_EQ(ret.size(), 4);
    EXPECT_EQ(ret[" a "], " 1 ");
    EXPECT_EQ(ret[" b "], " 2 ");
    EXPECT_EQ(ret[" c "], " ");
    EXPECT_EQ(ret[" "], " 4 ");
  }

  {
    std::string str("a&b");
    auto ret = SplitStringToMap(str, '&', '=');
    EXPECT_EQ(ret.size(), 2);
    EXPECT_TRUE(ret["a"].empty());
    EXPECT_TRUE(ret["b"].empty());
  }

  {
    std::string str("a=1");
    auto ret = SplitStringToMap(str, '&', '=');
    EXPECT_EQ(ret.size(), 1);
    EXPECT_EQ(ret["a"], "1");
  }

  {
    std::string str("abc");
    auto ret = SplitStringToMap(str, '&', '=');
    EXPECT_EQ(ret.size(), 1);
    EXPECT_TRUE(ret["abc"].empty());
  }
}

TEST(StringUtilTest, Convert) {
  auto out1 = Convert<std::string, int>(123);
  EXPECT_EQ(out1, "123");
  auto out2 = Convert<int, std::string>("123");
  EXPECT_EQ(out2, 123);
}

TEST(StringUtilTest, Join) {
  auto out = Join(std::vector<int>({1, 2, 3}), ".");
  EXPECT_EQ(out, "1.2.3");
}

TEST(StringUtilTest, ModifyString) {
  auto out = ModifyString(123, "-", ".0");
  EXPECT_EQ(out, "-123.0");
}

TEST(StringUtilTest, FormatString) {
  auto out = FormatString("{}{}{}", 1, 2, 3);
  EXPECT_EQ(out, "123");
}

TEST(StringUtilTest, PrintString) {
  auto out = PrintString("%d%s%d", 1, "2", 3);
  EXPECT_EQ(out, "123");
}
}  // namespace trpc::util::testing
