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

#include "trpc/serialization/json/json_serialization.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/serialization/testing/test_serialization.pb.h"
#include "trpc/util/pb2json.h"

namespace trpc::testing {

using namespace trpc::serialization;

using HelloRequest = trpc::test::serialization::HelloRequest;
using HelloReply = trpc::test::serialization::HelloReply;

TEST(JsonSerializationTest, JsonSerializationRapidjsonTest) {
  std::unique_ptr<JsonSerialization> json_serialization(new JsonSerialization());
  ASSERT_TRUE(json_serialization->Type() == kJsonType);

  std::string json_str = "{\"age\":\"18\",\"height\":180}";

  rapidjson::Document serialize_document;
  serialize_document.Parse(json_str.c_str());

  ASSERT_TRUE(!serialize_document.HasParseError());

  void* json_request = static_cast<void*>(&serialize_document);
  auto noncontiguous_buffer = CreateBufferSlow("");

  bool ret = json_serialization->Serialize(kRapidJson, json_request, &noncontiguous_buffer);
  ASSERT_TRUE(ret);

  std::string serialize_buffer = FlattenSlow(noncontiguous_buffer);
  ASSERT_EQ(json_str, serialize_buffer);

  rapidjson::Document deserialize_document{};
  void* json_request_deserialize = static_cast<void*>(&deserialize_document);

  ret = json_serialization->Deserialize(&noncontiguous_buffer, kRapidJson, json_request_deserialize);
  ASSERT_TRUE(ret);
}

TEST(JsonSerializationTest, EmptyStringJsonDerializationRapidjsonTest) {
  std::unique_ptr<JsonSerialization> json_serialization(new JsonSerialization());
  ASSERT_TRUE(json_serialization->Type() == kJsonType);

  std::string json_str = "";

  rapidjson::Document serialize_document;
  serialize_document.Parse(json_str.c_str());

  ASSERT_TRUE(serialize_document.HasParseError());

  auto noncontiguous_buffer = CreateBufferSlow("");

  std::string serialize_buffer = FlattenSlow(noncontiguous_buffer);
  ASSERT_EQ(json_str, serialize_buffer);

  rapidjson::Document deserialize_document{};
  void* json_request_deserialize = static_cast<void*>(&deserialize_document);

  auto ret = json_serialization->Deserialize(&noncontiguous_buffer, kRapidJson, json_request_deserialize);
  ASSERT_TRUE(ret);
  ASSERT_TRUE(deserialize_document.HasParseError());
}

TEST(JsonSerializationTest, JsonSerializationPbTest) {
  std::unique_ptr<JsonSerialization> json_serialization(new JsonSerialization());

  ASSERT_TRUE(json_serialization->Type() == kJsonType);

  HelloRequest request{};
  std::string greetings = "Hello World!";
  request.set_msg(greetings);

  std::string buffer("");
  bool ret = trpc::Pb2Json::PbToJson(request, &buffer);
  ASSERT_TRUE(ret);

  void* pb_request = static_cast<void*>(&request);
  auto noncontiguous_buffer = CreateBufferSlow("");

  ret = json_serialization->Serialize(kPbMessage, pb_request, &noncontiguous_buffer);
  ASSERT_TRUE(ret);

  std::string serialize_buffer = FlattenSlow(noncontiguous_buffer);
  ASSERT_EQ(buffer, serialize_buffer);

  HelloRequest request_deserialize{};
  void* pb_request_deserialize = static_cast<void*>(&request_deserialize);

  ret = json_serialization->Deserialize(&noncontiguous_buffer, kPbMessage, pb_request_deserialize);
  ASSERT_TRUE(ret);

  ASSERT_EQ(request.msg(), request_deserialize.msg());
}

}  // namespace trpc::testing
