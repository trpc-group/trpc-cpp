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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#include "trpc/codec/grpc/grpc_protocol.h"

#include <string.h>

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/common/trpc_version.h"
#include "trpc/util/http/util.h"

namespace trpc {

namespace {
constexpr const char* kGrpcStatusStrings[] = {
    "0 GRPC OK",
    "1 GRPC CANCELLED",
    "2 GRPC UNKNOWN",
    "3 GRPC INVALID_ARGUMENT",
    "4 GRPC DEADLINE_EXCEEDED",
    "5 GRPC NOT_FOUND",
    "6 GRPC ALREADY_EXISTS",
    "7 GRPC PERMISSION_DENIED",
    "8 GRPC RESOURCE_EXHAUSTED",
    "9 GRPC FAILED_PRECONDITION",
    "10 GRPC ABORTED",
    "11 GRPC OUT_OF_RANGE",
    "12 GRPC UNIMPLEMENTED",
    "13 GRPC INTERNAL",
    "14 GRPC UNAVAILABLE",
    "15 GRPC DATA_LOSS",
    "16 GRPC UNAUTHENTICATED",
    "17 UNKNOWN",
};
}  // namespace

const char* GrpcStatusToString(int status) {
  if (status < 0 || status > GrpcStatus::kGrpcMax) {
    status = GrpcStatus::kGrpcMax;
  }

  return kGrpcStatusStrings[status];
}

uint32_t GrpcContentTypeToTrpcContentType(const std::string& content_type) {
  if (http::StringEqualsLiteralsIgnoreCase(kGrpcContentTypeDefault, content_type) ||
      http::StringEqualsLiteralsIgnoreCase(kGrpcContentTypeProtobuf, content_type)) {
    return TrpcContentEncodeType::TRPC_PROTO_ENCODE;
  } else if (http::StringEqualsLiteralsIgnoreCase(kGrpcContentTypeJson, content_type)) {
    return TrpcContentEncodeType::TRPC_JSON_ENCODE;
  }

  TRPC_LOG_ERROR("unsupported grpc content type: " << content_type);
  return TrpcContentEncodeType::TRPC_PROTO_ENCODE;
}

const char* TrpcContentTypeToGrpcContentType(uint32_t content_type) {
  switch (content_type) {
    case TrpcContentEncodeType::TRPC_PROTO_ENCODE:
      return kGrpcContentTypeDefault;
    case TrpcContentEncodeType::TRPC_JSON_ENCODE:
      return kGrpcContentTypeJson;
    default:
      TRPC_LOG_ERROR("unsupported grpc content type: " << std::to_string(content_type));
      return kGrpcContentTypeDefault;
  }
}

uint32_t GrpcContentEncodingToTrpcContentEncoding(const std::string& content_encoding) {
  if (http::StringEqualsLiteralsIgnoreCase(kGrpcContentEncodingGzip, content_encoding)) {
    return TrpcCompressType::TRPC_GZIP_COMPRESS;
  } else if (http::StringEqualsLiteralsIgnoreCase(kGrpcContentEncodingSnappy, content_encoding)) {
    return TrpcCompressType::TRPC_SNAPPY_COMPRESS;
  } else if (http::StringEqualsLiteralsIgnoreCase(kGrpcContentEncodingZlib, content_encoding)) {
    return TrpcCompressType::TRPC_ZLIB_COMPRESS;
  }

  return TrpcCompressType::TRPC_DEFAULT_COMPRESS;
}

const char* TrpcContentEncodingToGrpcContentEncoding(uint32_t content_encoding) {
  switch (content_encoding) {
    case TrpcCompressType::TRPC_GZIP_COMPRESS:
      return kGrpcContentEncodingGzip;
    case TrpcCompressType::TRPC_SNAPPY_COMPRESS:
      return kGrpcContentEncodingSnappy;
    case TrpcCompressType::TRPC_ZLIB_COMPRESS:
      return kGrpcContentEncodingZlib;
    default:
      static const char none[] = "";
      return none;
  }
}

// The following source codes are from seastar.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/brpc/grpc.cpp.
int64_t GrpcTimeoutTextToUS(const std::string& timeout_text) {
  if (timeout_text.empty()) {
    return -1;
  }

  std::size_t end_pos{0};
  int64_t timeout_value{0};
  try {
    timeout_value = static_cast<int64_t>(std::stoll(timeout_text, &end_pos, 10));
  } catch (std::invalid_argument& e) {
    TRPC_LOG_ERROR("invalid argument, " << e.what());
    return -1;
  } catch (std::out_of_range& e) {
    TRPC_LOG_ERROR("out of range, " << e.what());
    return -1;
  }
  // Only the format that the digit number is equal to (timeout_text.size() - 1) is valid.
  // Otherwise the format is not valid and is treated as no deadline.
  // For example:
  // - "1H", "2993S", "82m" is valid.
  // - "30A" is also valid, but the following switch would fall into default case
  //   and return -1 since 'A' is not a valid time unit.
  // - "123ASH" is not valid since the digit number is 3, while the size is 6.
  // - "HHH" is not valid since the digit number is 0, while the size is 3.
  if (end_pos != (timeout_text.size() - 1)) {
    return -1;
  }

  switch (timeout_text.at(end_pos)) {
    case 'H':
      return timeout_value * 3600 * 1000000;
    case 'M':
      return timeout_value * 60 * 1000000;
    case 'S':
      return timeout_value * 1000000;
    case 'm':
      return timeout_value * 1000;
    case 'u':
      return timeout_value;
    case 'n':
      timeout_value = (timeout_value + 500) / 1000;
      return (timeout_value == 0) ? 1 : timeout_value;
    default:
      return -1;
  }
}
// End of source codes that are from incubator-brpc.

const std::string& GetGrpcUserAgent() {
  static const std::string user_agent{std::string("trpc-cpp/") + TRPC_Cpp_Version()};
  return user_agent;
}

uint32_t Uint32BigEndianConvertor(uint32_t value) {
  return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 | (value & 0x00FF0000U) >> 8 |
         (value & 0xFF000000U) >> 24;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool GrpcMessageContent::Check(NoncontiguousBuffer* in, NoncontiguousBuffer* out) {
  if (in->ByteSize() < GrpcMessageContent::kPrefixByteSize) {
    return false;
  }

  const char* ptr{nullptr};
  std::array<char, GrpcMessageContent::kPrefixByteSize> prefix_buffer{0};

  // If the first block meets the header length requirement, use it directly; otherwise copy enough content.
  if (in->FirstContiguous().size() >= prefix_buffer.size()) {
    ptr = in->FirstContiguous().data();
  } else {
    FlattenToSlow(*in, static_cast<void*>(prefix_buffer.data()), prefix_buffer.size());
    ptr = prefix_buffer.data();
  }

  uint32_t bytes = 0;
  memcpy(&bytes, ptr + GrpcMessageContent::kCompressedFieldByteSize, GrpcMessageContent::kLengthFieldByteSize);
  bytes = Uint32BigEndianConvertor(bytes);

  auto expect_bytes = bytes + GrpcMessageContent::kPrefixByteSize;
  if (in->ByteSize() < expect_bytes) {
    return false;
  }

  *out = in->Cut(expect_bytes);

  return true;
}

bool GrpcMessageContent::Decode(NoncontiguousBuffer* buffer) {
  if (TRPC_UNLIKELY(buffer->ByteSize() < GrpcMessageContent::kPrefixByteSize)) {
    TRPC_FMT_ERROR("less bytes to decode message prefix, expect: {}, actual: {}", GrpcMessageContent::kPrefixByteSize,
                   buffer->ByteSize());
    return false;
  }

  const char* ptr{nullptr};
  std::array<char, GrpcMessageContent::kPrefixByteSize> prefix_buffer{0};

  // If the first block meets the header length requirement, use it directly; otherwise copy enough content.
  if (buffer->FirstContiguous().size() >= prefix_buffer.size()) {
    ptr = buffer->FirstContiguous().data();
  } else {
    FlattenToSlow(*buffer, static_cast<void*>(prefix_buffer.data()), prefix_buffer.size());
    ptr = prefix_buffer.data();
  }

  // Set `compressed`
  memcpy(&compressed, ptr, GrpcMessageContent::kCompressedFieldByteSize);

  // Set `length`
  memcpy(&length, ptr + GrpcMessageContent::kCompressedFieldByteSize, GrpcMessageContent::kLengthFieldByteSize);
  length = Uint32BigEndianConvertor(length);

  auto expect_bytes = length + GrpcMessageContent::kPrefixByteSize;

  if (TRPC_UNLIKELY(buffer->ByteSize() < expect_bytes)) {
    TRPC_FMT_ERROR("less bytes to decode full message content, expect: {}, actual: {}", expect_bytes,
                   buffer->ByteSize());
    return false;
  }

  // Set `content`
  buffer->Skip(GrpcMessageContent::kPrefixByteSize);
  content = buffer->Cut(length);

  return true;
}

bool GrpcMessageContent::Encode(NoncontiguousBuffer* buffer) const {
  if (TRPC_UNLIKELY(content.ByteSize() != length)) {
    TRPC_FMT_ERROR("length prefix did not match content length, length prefix: {}, actual: {}", length,
                   content.ByteSize());
    return false;
  }

  NoncontiguousBufferBuilder builder;
  auto* builder_ptr = builder.Reserve(GrpcMessageContent::kPrefixByteSize);

  // Compression flag.
  memcpy(builder_ptr, &compressed, GrpcMessageContent::kCompressedFieldByteSize);
  builder_ptr += GrpcMessageContent::kCompressedFieldByteSize;
  // Content length.
  uint32_t big_endian_length = Uint32BigEndianConvertor(length);
  memcpy(builder_ptr, &big_endian_length, GrpcMessageContent::kLengthFieldByteSize);
  // Content buffer.
  builder.Append(std::move(content));
  *buffer = builder.DestructiveGet();

  return true;
}

size_t GrpcMessageContent::ByteSizeLong() const noexcept { return kPrefixByteSize + content.ByteSize(); }


namespace internal {
// For internal use only.
bool GetHeaderString(const http::HeaderPairs& header, std::string_view name, std::string* value) {
  const auto& v = header.Get(name);
  if (!v.empty()) {
    *value = v;
    return true;
  }
  return false;
}

// For internal use only.
bool GetHeaderInteger(const http::HeaderPairs& header, std::string_view name, int* value) {
  std::string value_text{""};
  if (GetHeaderString(header, name, &value_text)) {
    *value = std::stoi(value_text);
    return true;
  }
  return false;
}
}  // namespace internal

}  // namespace trpc
