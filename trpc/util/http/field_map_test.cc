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

#include "trpc/util/http/field_map.h"

#include <sstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace trpc::http::testing {

class FieldMapTest : public ::testing::Test {
 protected:
  void SetUp() override {
    header_.Add("Host", "xx.example.com");
    header_.Add("Content-Length", "1024");
    header_.Add("Content-Type", "application/text");
    header_.Add("Set-Cookie", "user1=Tom; domain=xx.example.com; path=/");
    header_.Add("Set-Cookie", "user2=Jerry; domain=xx.example.com; path=/");
    header_.Add("Set-Cookie", "user3=Duck; domain=xx.example.com; path=/");
    header_.Add("User-Defined-Key01", "user-defined-value01");
    header_.Add(std::string{"User-Defined-Key02"},
                std::string{"user-defined-value02-1"});
    header_.Add(std::string{"user-defined-key02"},
                std::string{"user-defined-value02-2"});
    header_.Add("user-defined-key03", "user-defined-value03");
    header_.Add("user-defined-key04", "user-defined-value04-01");
    header_.Add("user-defined-key04", "user-defined-value04-02");
    header_.Add("user-defined-key04", "user-defined-value04-02");
    header_.Add("user-defined-key04", "USER-DEFINED-VALUE04-02");
    header_.Add("user-defined-key04", "user-defined-value04-03");
    header_.Add("user-defined-key04", "user-defined-value04-03");
    header_.Add("user-defined-key04", "USER-DEFINED-VALUE04-03");

    header_content_ =
        "Host: xx.example.com\r\n"
        "Set-Cookie: user1=Tom; domain=xx.example.com; path=/\r\n"
        "Set-Cookie: user2=Jerry; domain=xx.example.com; path=/\r\n"
        "Set-Cookie: user3=Duck; domain=xx.example.com; path=/\r\n"
        "Content-Type: application/text\r\n"
        "Content-Length: 1024\r\n"
        "User-Defined-Key01: user-defined-value01\r\n"
        "User-Defined-Key02: user-defined-value02-1\r\n"
        "user-defined-key02: user-defined-value02-2\r\n"
        "user-defined-key03: user-defined-value03\r\n"
        "user-defined-key04: user-defined-value04-01\r\n"
        "user-defined-key04: user-defined-value04-02\r\n"
        "user-defined-key04: user-defined-value04-02\r\n"
        "user-defined-key04: USER-DEFINED-VALUE04-02\r\n"
        "user-defined-key04: user-defined-value04-03\r\n"
        "user-defined-key04: user-defined-value04-03\r\n"
        "user-defined-key04: USER-DEFINED-VALUE04-03\r\n";
  }

  void TearDown() override {}

 protected:
  http::detail::FieldMap<http::detail::CaseInsensitiveLess> header_;
  std::string header_content_;
};

TEST_F(FieldMapTest, GetOk) {
  ASSERT_EQ("xx.example.com", header_.Get("Host"));
  ASSERT_EQ("1024", header_.Get("Content-Length"));
  ASSERT_EQ("application/text", header_.Get("Content-Type"));
  ASSERT_EQ("user1=Tom; domain=xx.example.com; path=/", header_.Get("Set-Cookie"));
  ASSERT_EQ("user-defined-value01", header_.Get("User-Defined-Key01"));
  ASSERT_EQ("user-defined-value02-1", header_.Get("user-defined-key02"));
  ASSERT_EQ("user-defined-value03", header_.Get("user-defined-key03"));
}

TEST_F(FieldMapTest, GetValuesOK) {
  using values_t = std::vector<std::string_view>;
  values_t values = header_.Values("Set-Cookie");
  ASSERT_EQ(3, values.size());
  ASSERT_EQ("user1=Tom; domain=xx.example.com; path=/", values[0]);
  ASSERT_EQ("user2=Jerry; domain=xx.example.com; path=/", values[1]);
  ASSERT_EQ("user3=Duck; domain=xx.example.com; path=/", values[2]);

  values = header_.Values("User-Defined-Key02");
  ASSERT_EQ(2, values.size());
  ASSERT_EQ("user-defined-value02-1", values[0]);
  ASSERT_EQ("user-defined-value02-2", values[1]);

  values = header_.Values("not exists header");
  ASSERT_TRUE(values.empty());
}

TEST_F(FieldMapTest, GetValueIteratorOK) {
  auto [iter, end] = header_.ValueIterator("Set-Cookie");
  ASSERT_NE(iter, end);
  ASSERT_EQ("user1=Tom; domain=xx.example.com; path=/", iter->second);
  ASSERT_NE(++iter, end);
  ASSERT_EQ("user2=Jerry; domain=xx.example.com; path=/", iter->second);
  ASSERT_NE(++iter, end);
  ASSERT_EQ("user3=Duck; domain=xx.example.com; path=/", iter->second);
  ASSERT_EQ(++iter, end);

  auto [iter2, end2] = header_.ValueIterator("User-Defined-Key02");
  ASSERT_NE(iter2, end2);
  ASSERT_EQ("user-defined-value02-1", iter2->second);
  ASSERT_NE(++iter2, end2);
  ASSERT_EQ("user-defined-value02-2", iter2->second);
  ASSERT_EQ(++iter2, end2);

  auto [iter3, end3] = header_.ValueIterator("not exists header");
  ASSERT_EQ(iter3, end3);
}

TEST_F(FieldMapTest, GetPairsOK) {
  decltype(header_.Pairs()) expected = {
      {"Host", "xx.example.com"},
      {"Set-Cookie", "user1=Tom; domain=xx.example.com; path=/"},
      {"Set-Cookie", "user2=Jerry; domain=xx.example.com; path=/"},
      {"Set-Cookie", "user3=Duck; domain=xx.example.com; path=/"},
      {"Content-Type", "application/text"},
      {"Content-Length", "1024"},
      {"User-Defined-Key01", "user-defined-value01"},
      {"User-Defined-Key02", "user-defined-value02-1"},
      {"user-defined-key02", "user-defined-value02-2"},
      {"user-defined-key03", "user-defined-value03"},
      {"user-defined-key04", "user-defined-value04-01"},
      {"user-defined-key04", "user-defined-value04-02"},
      {"user-defined-key04", "user-defined-value04-02"},
      {"user-defined-key04", "USER-DEFINED-VALUE04-02"},
      {"user-defined-key04", "user-defined-value04-03"},
      {"user-defined-key04", "user-defined-value04-03"},
      {"user-defined-key04", "USER-DEFINED-VALUE04-03"},
  };
  ASSERT_EQ(expected, header_.Pairs());
}

TEST_F(FieldMapTest, SetOk) {
  ASSERT_EQ(1, header_.Values("User-Defined-Key01").size());
  header_.Set("User-Defined-Key01", "user-defined-value01-new");
  ASSERT_EQ(1, header_.Values("User-Defined-Key01").size());
  ASSERT_EQ("user-defined-value01-new", header_.Get("User-Defined-Key01"));

  ASSERT_EQ(2, header_.Values("user-defined-key02").size());
  header_.Set("User-Defined-Key02", "user-defined-value02");
  ASSERT_EQ(1, header_.Values("User-Defined-Key02").size());
}

TEST_F(FieldMapTest, DeleteOk) {
  ASSERT_EQ(1, header_.Values("User-Defined-Key01").size());
  header_.Delete("User-Defined-Key01");
  ASSERT_EQ(0, header_.Values("User-Defined-Key01").size());

  ASSERT_EQ(2, header_.Values("User-Defined-Key02").size());
  header_.Delete("User-Defined-Key02");
  ASSERT_EQ(0, header_.Values("User-Defined-Key02").size());
}

TEST_F(FieldMapTest, DeleteMultiValuesOk) {
  ASSERT_EQ(7, header_.Values("User-Defined-Key04").size());
  header_.Delete("User-Defined-Key04", "user-defined-value04-02");
  ASSERT_EQ(5, header_.Values("User-Defined-Key04").size());

  http::detail::CaseInsensitiveEqualTo value_equal_to;
  header_.Delete("User-Defined-Key04", "user-defined-value04-03", value_equal_to);
  ASSERT_EQ(2, header_.Values("User-Defined-Key04").size());

  decltype(header_.Pairs()) expected = {
      {"Host", "xx.example.com"},
      {"Set-Cookie", "user1=Tom; domain=xx.example.com; path=/"},
      {"Set-Cookie", "user2=Jerry; domain=xx.example.com; path=/"},
      {"Set-Cookie", "user3=Duck; domain=xx.example.com; path=/"},
      {"Content-Type", "application/text"},
      {"Content-Length", "1024"},
      {"User-Defined-Key01", "user-defined-value01"},
      {"User-Defined-Key02", "user-defined-value02-1"},
      {"user-defined-key02", "user-defined-value02-2"},
      {"user-defined-key03", "user-defined-value03"},
      {"user-defined-key04", "user-defined-value04-01"},
      {"user-defined-key04", "USER-DEFINED-VALUE04-02"},
  };

  ASSERT_EQ(expected, header_.Pairs());
}

TEST_F(FieldMapTest, ToStringAndWriteToStringStream) {
  std::stringstream ss;
  header_.WriteToStringStream(ss);

  auto header_content = ss.str();
  ASSERT_EQ(header_content, header_.ToString());
  ASSERT_EQ(header_content, header_content_);
}

TEST_F(FieldMapTest, ByteSizeLong) {
  ASSERT_EQ(header_content_.size(), header_.ByteSizeLong());
}

TEST_F(FieldMapTest, FlatPairsCount) {
  ASSERT_EQ(17, header_.FlatPairsCount());
}

TEST_F(FieldMapTest, SetIfNotPresentOk) {
  EXPECT_EQ(0, header_.Values("User-Defined-Key99").size());
  header_.SetIfNotPresent("User-Defined-Key99", "user-defined-value99");
  EXPECT_EQ(1, header_.Values("User-Defined-Key99").size());
  EXPECT_EQ("user-defined-value99", header_.Get("User-Defined-Key99"));

  EXPECT_EQ(1, header_.Values("User-Defined-Key01").size());
  header_.SetIfNotPresent("User-Defined-Key01", "user-defined-value01-new");
  EXPECT_EQ(1, header_.Values("User-Defined-Key01").size());
  EXPECT_EQ("user-defined-value01", header_.Get("User-Defined-Key01"));

  // case insensitivity test
  EXPECT_EQ(1, header_.Values("user-defined-key01").size());
  header_.SetIfNotPresent("user-defined-key01", "user-defined-value01-new");
  EXPECT_EQ(1, header_.Values("user-defined-key01").size());
  EXPECT_EQ("user-defined-value01", header_.Get("user-defined-key01"));
}

}  // namespace trpc::http::testing
