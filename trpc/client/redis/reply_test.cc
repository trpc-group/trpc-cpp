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

#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define private public

#include "trpc/client/redis/reply.h"

namespace trpc {

namespace testing {

TEST(Reply, String) {
  trpc::redis::Reply r(trpc::redis::StringReplyMarker{}, "redis");

  EXPECT_TRUE(r.IsString());
  EXPECT_EQ("redis", r.GetString());
  EXPECT_EQ("string", r.TypeToString());
}

TEST(Reply, Integer) {
  trpc::redis::Reply r(trpc::redis::IntegerReplyMarker{}, 1024);

  EXPECT_TRUE(r.IsInteger());
  EXPECT_EQ(1024, r.GetInteger());
  EXPECT_EQ("integer", r.TypeToString());
}

TEST(Reply, Nil) {
  trpc::redis::Reply r(trpc::redis::NilReplyMarker{});

  EXPECT_TRUE(r.IsNil());
  EXPECT_EQ("nil", r.TypeToString());
}

TEST(Reply, Invalid) {
  trpc::redis::Reply r(trpc::redis::InvalidReplyMarker{});

  EXPECT_TRUE(r.IsInvalid());
  EXPECT_EQ("invalid", r.TypeToString());
}

TEST(Reply, Status) {
  trpc::redis::Reply r(trpc::redis::StatusReplyMarker{}, "OK");

  EXPECT_TRUE(r.IsStatus());
  EXPECT_EQ("OK", r.GetString());
  EXPECT_EQ("status", r.TypeToString());
}

TEST(Reply, Error) {
  trpc::redis::Reply r(trpc::redis::ErrorReplyMarker{}, "ERR");

  EXPECT_TRUE(r.IsError());
  EXPECT_EQ("ERR", r.GetString());
  EXPECT_EQ("error", r.TypeToString());
}

TEST(Reply, Array) {
  trpc::redis::Reply a(trpc::redis::StringReplyMarker{}, "redis");
  trpc::redis::Reply b(trpc::redis::IntegerReplyMarker{}, 1024);
  trpc::redis::Reply c(trpc::redis::NilReplyMarker{});
  trpc::redis::Reply d(trpc::redis::StatusReplyMarker{}, "OK");
  trpc::redis::Reply e(trpc::redis::ErrorReplyMarker{}, "ERR");

  std::vector<trpc::redis::Reply> v{a, b, c, d, e};

  trpc::redis::Reply r(trpc::redis::ArrayReplyMarker{}, std::move(v));

  EXPECT_TRUE(r.IsArray());
  EXPECT_EQ("array", r.TypeToString());

  auto array_ = r.GetArray();

  EXPECT_EQ(5, array_.size());

  EXPECT_TRUE(array_.at(0).IsString());
  EXPECT_TRUE(array_.at(1).IsInteger());
  EXPECT_TRUE(array_.at(2).IsNil());
  EXPECT_TRUE(array_.at(3).IsStatus());
  EXPECT_TRUE(array_.at(4).IsError());
}

TEST(Reply, NestedArray) {
  trpc::redis::Reply a(trpc::redis::StringReplyMarker{}, "redis");
  std::vector<trpc::redis::Reply> v{a};

  trpc::redis::Reply aa(trpc::redis::ArrayReplyMarker{}, std::move(v));

  trpc::redis::Reply b(trpc::redis::StatusReplyMarker{}, "OK");
  std::vector<trpc::redis::Reply> vv{aa, b};

  trpc::redis::Reply r(trpc::redis::ArrayReplyMarker{}, std::move(vv));

  EXPECT_TRUE(r.IsArray());
  EXPECT_EQ("array", r.TypeToString());
  EXPECT_EQ(2, r.GetArray().size());

  EXPECT_TRUE(r.GetArray().at(0).IsArray());
  EXPECT_TRUE(r.GetArray().at(1).IsStatus());
}

TEST(Reply, SetString) {
  trpc::redis::Reply r;
  r.type_ = trpc::redis::Reply::Type::STRING;
  r.Set(trpc::redis::StringReplyMarker{}, "redis", 5);

  EXPECT_TRUE(r.IsString());
  EXPECT_EQ("redis", r.GetString());
  EXPECT_EQ("string", r.TypeToString());
}

TEST(Reply, SetStatus) {
  trpc::redis::Reply r;
  r.type_ = trpc::redis::Reply::Type::STATUS;
  r.Set(trpc::redis::StatusReplyMarker{}, "OK", 2);

  EXPECT_TRUE(r.IsStatus());
  EXPECT_EQ("OK", r.GetString());
  EXPECT_EQ("status", r.TypeToString());
}

TEST(Reply, SetError) {
  trpc::redis::Reply r;
  r.type_ = trpc::redis::Reply::Type::ERROR;
  r.Set(trpc::redis::ErrorReplyMarker{}, "ERR", 3);

  EXPECT_TRUE(r.IsError());
  EXPECT_EQ("ERR", r.GetString());
  EXPECT_EQ("error", r.TypeToString());
}

TEST(Reply, SetInteger) {
  trpc::redis::Reply r;
  r.type_ = trpc::redis::Reply::Type::INTEGER;
  r.Set(trpc::redis::IntegerReplyMarker{}, 1024);

  EXPECT_TRUE(r.IsInteger());
  EXPECT_EQ(1024, r.GetInteger());
  EXPECT_EQ("integer", r.TypeToString());
}

TEST(Reply, SetInvalid) {
  trpc::redis::Reply r;
  r.Set(trpc::redis::InvalidReplyMarker{});

  EXPECT_TRUE(r.IsInvalid());
  EXPECT_EQ("invalid", r.TypeToString());
}

TEST(Reply, SetNil) {
  trpc::redis::Reply r;
  r.Set(trpc::redis::NilReplyMarker{});

  EXPECT_TRUE(r.IsNil());
  EXPECT_EQ("nil", r.TypeToString());
}

TEST(Reply, SetStatusPart) {
  trpc::redis::Reply r;
  r.type_ = trpc::redis::Reply::Type::STATUS;
  r.Set(trpc::redis::StatusReplyMarker{}, "O", 1, 2);
  r.AppendString("K", 1);
  EXPECT_TRUE(r.IsStatus());
  EXPECT_EQ("OK", r.GetString());
  EXPECT_EQ("status", r.TypeToString());
}

TEST(Reply, SetErorPart) {
  trpc::redis::Reply r;
  r.type_ = trpc::redis::Reply::Type::ERROR;
  r.Set(trpc::redis::ErrorReplyMarker{}, "E", 1, 3);
  r.AppendString("RR", 2);
  EXPECT_TRUE(r.IsError());
  EXPECT_EQ("ERR", r.GetString());
  EXPECT_EQ("error", r.TypeToString());
}

TEST(Reply, SetStringPart) {
  trpc::redis::Reply r;
  r.type_ = trpc::redis::Reply::Type::STRING;
  r.Set(trpc::redis::StringReplyMarker{}, "re", 2, 5);
  r.AppendString("dis", 3);
  EXPECT_TRUE(r.IsString());
  EXPECT_EQ("redis", r.GetString());
  EXPECT_EQ("string", r.TypeToString());
}

TEST(Reply, StringMove) {
  trpc::redis::Reply r;
  r.type_ = trpc::redis::Reply::Type::STRING;
  r.Set(trpc::redis::StringReplyMarker{}, "redis", 5);
  EXPECT_TRUE(r.IsString());
  std::string value;
  EXPECT_EQ(0, r.GetString(value));
  EXPECT_EQ("redis", value);
  std::string value2;
  EXPECT_EQ(-1, r.GetString(value2));
  EXPECT_EQ("", value2);
}

TEST(Reply, EmptyStringMove) {
  trpc::redis::Reply r;
  r.type_ = trpc::redis::Reply::Type::STRING;
  r.Set(trpc::redis::StringReplyMarker{}, "", 0);
  EXPECT_TRUE(r.IsString());
  std::string value;
  EXPECT_EQ(0, r.GetString(value));
  EXPECT_EQ("", value);
}

TEST(Reply, ArrayMove) {
  trpc::redis::Reply a(trpc::redis::StringReplyMarker{}, "redis");
  trpc::redis::Reply b(trpc::redis::StatusReplyMarker{}, "OK");
  std::vector<trpc::redis::Reply> vv{a, b};
  trpc::redis::Reply r(trpc::redis::ArrayReplyMarker{}, std::move(vv));
  std::vector<trpc::redis::Reply> value;
  EXPECT_EQ(0, r.GetArray(value));
  EXPECT_EQ(2, value.size());
  EXPECT_TRUE(value.at(0).IsString());
  EXPECT_TRUE(value.at(1).IsStatus());
  std::vector<trpc::redis::Reply> value2;
  EXPECT_EQ(-1, value.at(0).GetArray(value2));
  EXPECT_EQ(0, value2.size());
}

TEST(Reply, EmptyArrayMove) {
  std::vector<trpc::redis::Reply> vv;
  trpc::redis::Reply r(trpc::redis::ArrayReplyMarker{}, std::move(vv));
  std::vector<trpc::redis::Reply> value;
  EXPECT_EQ(0, r.GetArray(value));
  EXPECT_EQ(0, value.size());
}

TEST(Reply, Construct) {
  trpc::redis::Reply r1;
  r1.type_ = trpc::redis::Reply::Type::STRING;
  r1.Set(trpc::redis::StringReplyMarker{}, "redis", 5);

  // Default copy constructor
  trpc::redis::Reply copy_construct_reply(r1);
  EXPECT_EQ("redis", r1.GetString());
  EXPECT_EQ("redis", copy_construct_reply.GetString());

  // Default move constructor
  trpc::redis::Reply move_construct_reply(std::move(r1));
  EXPECT_EQ("", r1.GetString());
  EXPECT_EQ("redis", move_construct_reply.GetString());

  trpc::redis::Reply r2;
  r2.type_ = trpc::redis::Reply::Type::STRING;
  r2.Set(trpc::redis::StringReplyMarker{}, "redis", 5);

  // Default copy assignment
  trpc::redis::Reply copy_assigning_reply = r2;
  EXPECT_EQ("redis", r2.GetString());
  EXPECT_EQ("redis", copy_assigning_reply.GetString());

  // Default move assignment
  trpc::redis::Reply move_assigning_reply(std::move(r2));
  EXPECT_EQ("", r2.GetString());
  EXPECT_EQ("redis", move_assigning_reply.GetString());
}

}  // namespace testing

}  // namespace trpc
