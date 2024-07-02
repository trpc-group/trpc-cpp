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

#include "trpc/codec/http/http_protocol.h"

#include "trpc/codec/trpc/trpc.pb.h"

namespace trpc {

namespace {
constexpr char kMimePb[] = "application/pb";
constexpr char kMimeProto[] = "application/proto";
constexpr char kMimeProtobuf[] = "application/protobuf";
constexpr char kMimeJson[] = "application/json";
constexpr char kMimeOctetStream[] = "application/octet-stream";
}  // namespace

std::string_view EncodeTypeToMime(int encode_type) {
  switch (encode_type) {
    case TrpcContentEncodeType::TRPC_PROTO_ENCODE: {
      return kMimeProto;
    }
    case TrpcContentEncodeType::TRPC_JSON_ENCODE: {
      return kMimeJson;
    }
    default: {
      return kMimeOctetStream;
    }
  }
}

int MimeToEncodeType(std::string_view mime) {
  if (mime == kMimeProto || mime == kMimePb || mime == kMimeProtobuf) {
    return TrpcContentEncodeType::TRPC_PROTO_ENCODE;
  } else if (mime == kMimeJson) {
    return TrpcContentEncodeType::TRPC_JSON_ENCODE;
  }
  return TrpcContentEncodeType::TRPC_NOOP_ENCODE;
}

bool HttpRequestProtocol::WaitForFullRequest() {
  auto& read_stream = request->GetStream();
  read_stream.SetDefaultDeadline(trpc::ReadSteadyClock() + std::chrono::milliseconds(GetTimeout()));
  if (read_stream.AppendToRequest(request->ContentLength()).OK()) {
    return true;
  }
  return false;
}

uint32_t HttpRequestProtocol::GetMessageSize() const { return request->ContentLength(); }

uint32_t HttpResponseProtocol::GetMessageSize() const { return response.ContentLength(); }

namespace internal {
const std::string& GetHeaderString(const http::HeaderPairs& header, const std::string& name) {
  return header.Get(name);
}

int GetHeaderInteger(const http::HeaderPairs& header, const std::string& name) {
  const auto& value = GetHeaderString(header, name);
  if (value.empty()) {
    return 0;
  }
  return std::stoi(value);
}
}  // namespace internal

}  // namespace trpc
