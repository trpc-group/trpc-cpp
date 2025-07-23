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

#include "trpc/codec/grpc/grpc_protocol.h"

#include <array>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc.pb.h"

namespace trpc::testing {

TEST(GrpcStatusToStringTest, ToString) {
  struct testing_args_t {
    std::string testing_name;
    std::string expect{""};
    int status;
  };

  std::vector<testing_args_t> testings{
      {"OK 0", "OK", GrpcStatus::kGrpcOk},
      {"CANCELLED 1", "CANCELLED", GrpcStatus::kGrpcCancelled},
      {"UNKNOWN 2", "UNKNOWN", GrpcStatus::kGrpcUnknown},
      {"INVALID_ARGUMENT 3", "INVALID_ARGUMENT", GrpcStatus::kGrpcInvalidArgument},
      {"DEADLINE_EXCEEDED 4", "DEADLINE_EXCEEDED", GrpcStatus::kGrpcDeadlineExceeded},
      {"NOT_FOUND 5", "NOT_FOUND", GrpcStatus::kGrpcNotFound},
      {"ALREADY_EXISTS 6", "ALREADY_EXISTS", GrpcStatus::kGrpcAlreadyExists},
      {"PERMISSION_DENIED 7", "PERMISSION_DENIED", GrpcStatus::kGrpcPermissionDenied},
      {"RESOURCE_EXHAUSTED 8", "RESOURCE_EXHAUSTED", GrpcStatus::kGrpcResourceExhausted},
      {"FAILED_PRECONDITION 9", "FAILED_PRECONDITION", GrpcStatus::kGrpcFailedPrecondition},
      {"ABORTED 10", "ABORTED", GrpcStatus::kGrpcAborted},
      {"OUT_OF_RANGE 11", "OUT_OF_RANGE", GrpcStatus::kGrpcOutOfRange},
      {"UNIMPLEMENTED 12", "UNIMPLEMENTED", GrpcStatus::kGrpcUnimplemented},
      {"INTERNAL 13", "INTERNAL", GrpcStatus::kGrpcInternal},
      {"UNAVAILABLE 14", "UNAVAILABLE", GrpcStatus::kGrpcUnavailable},
      {"DATA_LOSS 15", "DATA_LOSS", GrpcStatus::kGrpcDataLoss},
      {" UNAUTHENTICATED 16", "UNAUTHENTICATED", GrpcStatus::kGrpcUnauthenticated},
      {"Not exists 17", "UNKNOWN", GrpcStatus::kGrpcMax},
      {"Not exists -1", "UNKNOWN", -1},
      {"Not exists 18", "UNKNOWN", 18},
  };

  for (const auto& t : testings) {
    auto status_text = GrpcStatusToString(t.status);
    EXPECT_THAT(status_text, ::testing::HasSubstr(t.expect));
  }
}

TEST(GrpcContentTypeConvertionTest, ToTrpcContentTyp) {
  struct testing_args_t {
    std::string testing_name;
    uint32_t expect{0};
    std::string content_type;
  };

  std::vector<testing_args_t> testings{
      {"grpc", TrpcContentEncodeType::TRPC_PROTO_ENCODE, kGrpcContentTypeDefault},
      {"grpc+proto", TrpcContentEncodeType::TRPC_PROTO_ENCODE, kGrpcContentTypeProtobuf},
      {"grpc+json", TrpcContentEncodeType::TRPC_JSON_ENCODE, kGrpcContentTypeJson},
      {"unknown", TrpcContentEncodeType::TRPC_PROTO_ENCODE, "unknown"},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.expect, GrpcContentTypeToTrpcContentType(t.content_type));
  }
}

TEST(GrpcContentTypeConvertionTest, ToGrpcContentType) {
  struct testing_args_t {
    std::string testing_name;
    std::string expect{""};
    uint32_t content_type;
  };

  std::vector<testing_args_t> testings{
      {"grpc", kGrpcContentTypeDefault, TrpcContentEncodeType::TRPC_PROTO_ENCODE},
      {"grpc+json", kGrpcContentTypeJson, TrpcContentEncodeType::TRPC_JSON_ENCODE},
      {"unknown", kGrpcContentTypeDefault, 999},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.expect, TrpcContentTypeToGrpcContentType(t.content_type));
  }
}

TEST(GrpcContentEncodingConvertionTest, ToTrpcContentEncoding) {
  struct testing_args_t {
    std::string testing_name;
    uint32_t expect{0};
    std::string content_encoding;
  };

  std::vector<testing_args_t> testings{
      {"gzip", TrpcCompressType::TRPC_GZIP_COMPRESS, kGrpcContentEncodingGzip},
      {"snappy", TrpcCompressType::TRPC_SNAPPY_COMPRESS, kGrpcContentEncodingSnappy},
      {"zlib", TrpcCompressType::TRPC_ZLIB_COMPRESS, kGrpcContentEncodingZlib},
      {"unknown", TrpcCompressType::TRPC_DEFAULT_COMPRESS, "unknown"},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.expect, GrpcContentEncodingToTrpcContentEncoding(t.content_encoding));
  }
}

TEST(GrpcContentEncodingConvertionTest, ToGrpcContentEncoding) {
  struct testing_args_t {
    std::string testing_name;
    std::string expect{0};
    uint32_t content_encoding;
  };

  std::vector<testing_args_t> testings{
      {"gzip", kGrpcContentEncodingGzip, TrpcCompressType::TRPC_GZIP_COMPRESS},
      {"snappy", kGrpcContentEncodingSnappy, TrpcCompressType::TRPC_SNAPPY_COMPRESS},
      {"zlib", kGrpcContentEncodingZlib, TrpcCompressType::TRPC_ZLIB_COMPRESS},
      {"unknown", "", TrpcCompressType::TRPC_DEFAULT_COMPRESS},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.expect, TrpcContentEncodingToGrpcContentEncoding(t.content_encoding));
  }
}

TEST(GrpcTimeoutTextToUSTest, GrpcTimeoutTextToUS) {
  struct testing_args_t {
    std::string testing_name;
    int64_t expect{0};
    std::string timeout_text{""};
  };

  std::vector<testing_args_t> testings {
      {"1 Hour", 1 * 3600 * 1000000LL, "1H"},
      {"2 Minutes", 2 * 60 * 1000000LL, "2M"},
      {"3 Seconds", 3 * 1000000LL, "3S"},
      {"4 Milliseconds", 4 * 1000, "4m"},
      {"5 Microseconds", 5, "5u"},
      {"6 Nanoseconds", 1, "6n"},
      {"invalid", -1, "30A"},
      {"invalid", -1, "123ASH"},
      {"invalid argument", -1, "HHH"},
      {"out of range", -1, "99999999999999999999999999999999999999"},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.expect, GrpcTimeoutTextToUS(t.timeout_text)) << t.testing_name;
  }
}

TEST(GetGrpcUserAgentTest, GetUserAgentOk) {
  auto user_agent = GetGrpcUserAgent();
  EXPECT_THAT(user_agent, ::testing::StartsWith("trpc-cpp/"));
}

TEST(Uint32BigEndianConvertor, ConvertOk) {
  uint32_t value = 1234567890, value_in_big_endian = 3523384905;
  ASSERT_EQ(value_in_big_endian, Uint32BigEndianConvertor(value));
  ASSERT_EQ(value, Uint32BigEndianConvertor(value_in_big_endian));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class GrpcMessageContentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    NoncontiguousBufferBuilder buffer_builder{};
    buffer_builder.Append("hello world");
    auto content = buffer_builder.DestructiveGet();
    auto content_length = content.ByteSize();

    message_content_.compressed = 0;
    message_content_.content = std::move(content);
    message_content_.length = content_length;

    ASSERT_EQ(content_length + GrpcMessageContent::kPrefixByteSize,
              message_content_.ByteSizeLong());
  }

  void TearDown() override {}

  void BuildMessagePrefix(uint8_t compressed, uint32_t content_length,
                          std::array<char, 5>* prefix_buffer) {
    memcpy(prefix_buffer->data(), &compressed, 1);
    uint32_t big_endian_length = Uint32BigEndianConvertor(content_length);
    memcpy(prefix_buffer->data() + 1, &big_endian_length, 4);
  }

 protected:
  GrpcMessageContent message_content_;
};

TEST_F(GrpcMessageContentTest, DecodeFailedDueToLessPrefixBytes) {
  NoncontiguousBufferBuilder buffer_builder{};
  buffer_builder.Append("xx");
  auto buffer = buffer_builder.DestructiveGet();
  ASSERT_FALSE(message_content_.Decode(&buffer));
}

TEST_F(GrpcMessageContentTest, DecodeFailedDueToLessContentBytes) {
  std::array<char, 5> prefix_buffer{0};
  uint8_t compressed{1};
  std::string greetings{"hello world"};
  BuildMessagePrefix(compressed, greetings.size(), &prefix_buffer);

  NoncontiguousBufferBuilder buffer_builder{};
  buffer_builder.Append(prefix_buffer.data(), prefix_buffer.size());
  auto buffer = buffer_builder.DestructiveGet();
  ASSERT_FALSE(message_content_.Decode(&buffer));
}

TEST_F(GrpcMessageContentTest, DecodeOk) {
  std::array<char, 5> prefix_buffer{0};
  uint8_t compressed{1};
  std::string greetings{"hello world"};
  BuildMessagePrefix(compressed, greetings.size(), &prefix_buffer);

  NoncontiguousBufferBuilder buffer_builder{};
  buffer_builder.Append(prefix_buffer.data(), prefix_buffer.size());
  buffer_builder.Append(greetings);
  auto buffer = buffer_builder.DestructiveGet();
  ASSERT_TRUE(message_content_.Decode(&buffer));
  ASSERT_EQ(1, message_content_.compressed);
  ASSERT_EQ(greetings.size(), message_content_.length);
  ASSERT_EQ(greetings, FlattenSlow(message_content_.content));
}

TEST_F(GrpcMessageContentTest, EncodeOk) {
  NoncontiguousBuffer buffer{};
  ASSERT_TRUE(message_content_.Encode(&buffer));
}

TEST_F(GrpcMessageContentTest, EncodeFailedDueToLengthNotMatch) {
  NoncontiguousBuffer buffer{};
  message_content_.length = 0;
  ASSERT_FALSE(message_content_.Encode(&buffer));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class GrpcUnaryRequestProtocolTest : public ::testing::Test {
 protected:
  void SetUp() override { request_ = std::make_shared<GrpcUnaryRequestProtocol>(); }

  void TearDown() override {}

 protected:
  GrpcUnaryRequestProtocolPtr request_{nullptr};
};

TEST_F(GrpcUnaryRequestProtocolTest, ZeroCopyEncodeAndDecodeFailed) {
  NoncontiguousBuffer buffer{};
  ASSERT_FALSE(request_->ZeroCopyEncode(buffer));
  ASSERT_FALSE(request_->ZeroCopyDecode(buffer));
}

TEST_F(GrpcUnaryRequestProtocolTest, SetAndGetRequestIdOk) {
  uint32_t request_id{0};
  ASSERT_TRUE(request_->SetRequestId(123456));
  ASSERT_TRUE(request_->GetRequestId(request_id));
  ASSERT_EQ(123456, request_id);
}

TEST_F(GrpcUnaryRequestProtocolTest, SetAndGetCallerName) {
  std::string caller_name{"xx.yy.zz.Caller"};
  request_->SetCallerName(caller_name);
  ASSERT_EQ(caller_name, request_->GetCallerName());
}

TEST_F(GrpcUnaryRequestProtocolTest, SetAndGetCalleeName) {
  std::string callee_name{"xx.yy.zz.Callee"};
  request_->SetCalleeName(callee_name);
  ASSERT_EQ(callee_name, request_->GetCalleeName());
}

TEST_F(GrpcUnaryRequestProtocolTest, SetAndGetFuncName) {
  std::string func_name{"xx.yy.zz.Service/SayHello"};
  request_->SetFuncName(func_name);
  ASSERT_EQ(func_name, request_->GetFuncName());
}

TEST_F(GrpcUnaryRequestProtocolTest, SetAndGetKVInfo) {
  const auto& kvs = request_->GetKVInfos();
  ASSERT_TRUE(kvs.empty());

  request_->SetKVInfo("x-user-defined-name", "Tom");
  auto tom_iter = kvs.find("x-user-defined-name");
  ASSERT_NE(tom_iter, kvs.end());
  ASSERT_EQ("Tom", tom_iter->second);

  auto mutable_kvs = request_->GetMutableKVInfos();
  ASSERT_NE(nullptr, mutable_kvs);
}

TEST_F(GrpcUnaryRequestProtocolTest, SetAndGetNonContiguousProtocolBody) {
  NoncontiguousBufferBuilder buffer_builder{};
  std::string greetings{"hello world"};
  buffer_builder.Append(greetings);

  request_->SetNonContiguousProtocolBody(std::move(buffer_builder.DestructiveGet()));
  auto content = request_->GetNonContiguousProtocolBody();

  ASSERT_EQ(greetings, FlattenSlow(content));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class GrpcUnaryResponseProtocolTest : public ::testing::Test {
 protected:
  void SetUp() override { response_ = std::make_shared<GrpcUnaryResponseProtocol>(); }

  void TearDown() override {}

 protected:
  GrpcUnaryResponseProtocolPtr response_{nullptr};
};

TEST_F(GrpcUnaryResponseProtocolTest, ZeroCopyEncodeAndDecodeFailed) {
  NoncontiguousBuffer buffer{};
  ASSERT_FALSE(response_->ZeroCopyEncode(buffer));
  ASSERT_FALSE(response_->ZeroCopyDecode(buffer));
}

TEST_F(GrpcUnaryResponseProtocolTest, SetAndGetRequestIdOk) {
  uint32_t request_id{0};
  ASSERT_TRUE(response_->SetRequestId(123456));
  ASSERT_TRUE(response_->GetRequestId(request_id));
  ASSERT_EQ(123456, request_id);
}

TEST_F(GrpcUnaryResponseProtocolTest, SetAndGetKVInfo) {
  const auto& kvs = response_->GetKVInfos();
  ASSERT_TRUE(kvs.empty());

  response_->SetKVInfo("x-user-defined-name", "Tom");
  auto tom_iter = kvs.find("x-user-defined-name");
  ASSERT_NE(tom_iter, kvs.end());
  ASSERT_EQ("Tom", tom_iter->second);

  auto mutable_kvs = response_->GetMutableKVInfos();
  ASSERT_NE(nullptr, mutable_kvs);
}

TEST_F(GrpcUnaryResponseProtocolTest, SetAndGetNonContiguousProtocolBody) {
  NoncontiguousBufferBuilder buffer_builder{};
  std::string greetings{"hello world"};
  buffer_builder.Append(greetings);

  response_->SetNonContiguousProtocolBody(std::move(buffer_builder.DestructiveGet()));
  auto content = response_->GetNonContiguousProtocolBody();

  ASSERT_EQ(greetings, FlattenSlow(content));
}

}  // namespace trpc::testing
