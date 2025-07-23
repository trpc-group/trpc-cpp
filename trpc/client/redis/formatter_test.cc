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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define private public

#include "trpc/client/redis/formatter.h"

namespace trpc {

namespace testing {

class FormatterTest : public ::testing::Test {
 public:
  void SetUp() { r_.params_.clear(); }

 public:
  trpc::redis::Formatter f_;
  trpc::redis::Request r_;
};

TEST_F(FormatterTest, CommandWithoutInterpolation) {
  int r = f_.TFormatCommand(&r_, "SET trpc redis");
  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("trpc", r_.params_.at(1));
  EXPECT_EQ("redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithInterpolation) {
  int r = f_.TFormatCommand(&r_, "SET %s %s", "trpc", "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("trpc", r_.params_.at(1));
  EXPECT_EQ("redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithEmptyString1) {
  int r = f_.TFormatCommand(&r_, "SET %s %s", "", "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("", r_.params_.at(1));
  EXPECT_EQ("redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithEmptyString2) {
  int r = f_.TFormatCommand(&r_, "SET %s %s", "trpc", "");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("trpc", r_.params_.at(1));
  EXPECT_EQ("", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithBinaryString) {
  int r = f_.TFormatCommand(&r_, "SET %b %b", "trpc", 4, "redis\0redis", 11);

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("trpc", r_.params_.at(1));
  EXPECT_EQ(std::string("redis\0redis", 11), r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithBinaryEmptyString) {
  int r = f_.TFormatCommand(&r_, "SET %b %b", "trpc", 4, "", 0);

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("trpc", r_.params_.at(1));
  EXPECT_EQ("", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithLiteral) {
  int r = f_.TFormatCommand(&r_, "SET %% %%");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("%", r_.params_.at(1));
  EXPECT_EQ("%", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithInt) {
  int r = f_.TFormatCommand(&r_, "SET key:%08d v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithChar) {
  int r = f_.TFormatCommand(&r_, "SET key:%08hhd v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithShort) {
  int r = f_.TFormatCommand(&r_, "SET key:%08hd v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithLong) {
  int r = f_.TFormatCommand(&r_, "SET key:%08ld v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithLongLong) {
  int r = f_.TFormatCommand(&r_, "SET key:%08lld v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithUnsignedInt) {
  int r = f_.TFormatCommand(&r_, "SET key:%08u v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithUnsignedChar) {
  int r = f_.TFormatCommand(&r_, "SET key:%08hhu v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithUnsignedShort) {
  int r = f_.TFormatCommand(&r_, "SET key:%08hu v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithUnsignedLong) {
  int r = f_.TFormatCommand(&r_, "SET key:%08lu v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithUnsignedLongLong) {
  int r = f_.TFormatCommand(&r_, "SET key:%08llu v:%s", 127, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:00000127", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithFloat) {
  int r = f_.TFormatCommand(&r_, "SET key:%08.3f v:%s", 127.0, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:0127.000", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, CommandWithDouble) {
  int r = f_.TFormatCommand(&r_, "SET key:%08.3f v:%s", 127.0, "redis");

  EXPECT_EQ(0, r);
  EXPECT_EQ(3, r_.params_.size());
  EXPECT_EQ("SET", r_.params_.at(0));
  EXPECT_EQ("key:0127.000", r_.params_.at(1));
  EXPECT_EQ("v:redis", r_.params_.at(2));
}

TEST_F(FormatterTest, InvalidCommandWithInt) {
  int r = f_.TFormatCommand(&r_, "SET key:%08hhld v:%s", 127, "redis");
  EXPECT_EQ(-1, r);

  r = f_.TFormatCommand(&r_, "SET key:%08hjx v:%s", 127, "redis");
  EXPECT_EQ(-1, r);

  r = f_.TFormatCommand(&r_, "SET key:%08lljd v:%s", 127, "redis");
  EXPECT_EQ(-1, r);

  r = f_.TFormatCommand(&r_, "SET key:%08ljd v:%s", 127, "redis");
  EXPECT_EQ(-1, r);
}

TEST_F(FormatterTest, TVPrintf) {
  std::string t = "trpc: ";
  int r = f_.TVPrintf(t, "%s version %d", "redis", 1);

  EXPECT_EQ(0, r);
  EXPECT_EQ("trpc: redis version 1", t);

  t.clear();
  std::string large_string(10240, 'a');
  r = f_.TVPrintf(t, "%s", large_string.data());

  EXPECT_EQ(0, r);
  EXPECT_EQ(large_string, t);

  t.clear();
  std::string large_format(10240, 'a');
  r = f_.TVPrintf(t, large_format.data());

  EXPECT_EQ(0, r);
  EXPECT_EQ(large_format, t);
}

}  // namespace testing

}  // namespace trpc
