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

#include "trpc/serialization/noop/noop_serialization.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"

namespace trpc::testing {

using namespace trpc::serialization;

TEST(NoopSerializationTest, StringSerializationTest) {
  std::unique_ptr<NoopSerialization> noop_serialization(new NoopSerialization());

  ASSERT_TRUE(noop_serialization->Type() == kNoopType);

  std::string str = "Hello World!";

  void* str_request = static_cast<void*>(&str);
  auto buffer = CreateBufferSlow("");

  bool ret = noop_serialization->Serialize(kStringNoop, str_request, &buffer);
  ASSERT_TRUE(ret);

  std::string str_deserialize("");
  void* str_request_deserialize = static_cast<void*>(&str_deserialize);

  ret = noop_serialization->Deserialize(&buffer, kStringNoop, str_request_deserialize);
  ASSERT_TRUE(ret);

  ASSERT_EQ(str, str_deserialize);
}

TEST(NoopSerializationTest, NoncontiguousSerializationTest) {
  std::unique_ptr<NoopSerialization> noop_serialization(new NoopSerialization());

  ASSERT_TRUE(noop_serialization->Type() == kNoopType);

  auto in_buffer = CreateBufferSlow("hello world!");
  void* in_request = static_cast<void*>(&in_buffer);
  auto out_buffer = CreateBufferSlow("");

  bool ret = noop_serialization->Serialize(kNonContiguousBufferNoop, in_request, &out_buffer);
  ASSERT_TRUE(ret);

  ASSERT_EQ(in_buffer.Empty(), true);

  auto out_buffer_deserialize = CreateBufferSlow("");
  void* out_request = static_cast<void*>(&out_buffer_deserialize);

  ret = noop_serialization->Deserialize(&out_buffer, kNonContiguousBufferNoop, out_request);
  ASSERT_TRUE(ret);

  std::string str = FlattenSlow(out_buffer_deserialize);

  ASSERT_EQ(out_buffer.Empty(), true);
  ASSERT_EQ(str, "hello world!");
}

TEST(NoopSerializationTest, InvalidSerializationTest) {
  std::unique_ptr<NoopSerialization> noop_serialization(new NoopSerialization());

  ASSERT_TRUE(noop_serialization->Type() == kNoopType);

  DataType type = 190;
  void* in_request = nullptr;
  auto out_buffer = CreateBufferSlow("");

  bool ret = noop_serialization->Serialize(type, in_request, &out_buffer);
  EXPECT_FALSE(ret);

  auto out_buffer_deserialize = CreateBufferSlow("");
  void* out_request = nullptr;

  ret = noop_serialization->Deserialize(&out_buffer, type, out_request);
  EXPECT_FALSE(ret);
}

}  // namespace trpc::testing
