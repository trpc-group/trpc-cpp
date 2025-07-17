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

#include "trpc/serialization/pb/pb_serialization.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "trpc/serialization/testing/test_serialization.pb.h"

namespace trpc::testing {

using namespace trpc::serialization;

using HelloRequest = trpc::test::serialization::HelloRequest;
using HelloReply = trpc::test::serialization::HelloReply;

TEST(PbSerializationTest, PbSerializationTest) {
  std::unique_ptr<PbSerialization> pb_serialization(new PbSerialization());
  ASSERT_TRUE(pb_serialization->Type() == kPbType);

  HelloRequest request{};
  std::string greetings = "Hello World!";
  request.set_msg(greetings);

  void* pb_request = static_cast<void*>(&request);
  auto buffer = CreateBufferSlow("");

  bool ret = pb_serialization->Serialize(kPbMessage, pb_request, &buffer);
  ASSERT_TRUE(ret);

  NoncontiguousBuffer already_serialized_buffer = buffer;
  void* serialized_pb_request = static_cast<void*>(&already_serialized_buffer);
  std::string already_serialized_string = FlattenSlow(already_serialized_buffer);
  NoncontiguousBuffer out_buffer = CreateBufferSlow("");
  ret = pb_serialization->Serialize(kNonContiguousBufferNoop, serialized_pb_request, &out_buffer);
  ASSERT_TRUE(ret);

  ASSERT_EQ(already_serialized_string, FlattenSlow(out_buffer));

  ret = pb_serialization->Serialize(kMaxDataType, pb_request, &out_buffer);
  ASSERT_FALSE(ret);

  HelloRequest request_deserialize{};
  void* pb_request_deserialize = static_cast<void*>(&request_deserialize);

  ret = pb_serialization->Deserialize(&buffer, kPbMessage, pb_request_deserialize);
  ASSERT_TRUE(ret);

  ASSERT_EQ(request.msg(), request_deserialize.msg());
}

}  // namespace trpc::testing
