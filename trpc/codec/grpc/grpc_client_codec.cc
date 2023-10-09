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

#include "trpc/codec/grpc/grpc_client_codec.h"

#include <limits>
#include <memory>
#include <utility>

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/codec/grpc/http2/response.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/util/http/util.h"

namespace trpc {

bool GrpcClientCodec::ZeroCopyEncode(const ClientContextPtr& context, const ProtocolPtr& in, NoncontiguousBuffer& out) {
  auto grpc_request = static_cast<GrpcUnaryRequestProtocol*>(in.get());
  grpc_request->SetRequestId(context->GetRequestId());

  // Request header.
  const GrpcUnaryRequestProtocol::GrpcRequestHeader& grpc_header = grpc_request->GetHeader();
  const http2::RequestPtr& http2_request = grpc_request->GetHttp2Request();
  http2_request->SetMethod(grpc_header.method);
  http2_request->SetPath(grpc_request->GetFuncName());
  http2_request->SetScheme(grpc_header.scheme);
  http2_request->SetAuthority(grpc_header.authority);
  // Trailers are expected.
  http2_request->AddHeader("te", "trailers");
  http2_request->AddHeader(kGrpcContentTypeName, TrpcContentTypeToGrpcContentType(context->GetReqEncodeType()));
  std::string content_encoding{TrpcContentEncodingToGrpcContentEncoding(context->GetReqCompressType())};
  if (!content_encoding.empty()) {
    http2_request->AddHeader(kGrpcEncodingName, content_encoding);
  }
  http2_request->AddHeader(kGrpcTimeoutName, GetTimeoutText(grpc_request->GetTimeout()));
  // Key-value pairs defined by user.
  for (const auto& [name, value] : grpc_request->GetKVInfos()) {
    http2_request->AddHeader(name, value);
  }

  // Request content.
  http2_request->SetNonContiguousBufferContent(grpc_request->GetNonContiguousProtocolBody());
  // Sets the request identifier to match the response.
  http2_request->SetContentSequenceId(grpc_header.request_id);
  // Since the codec is stateless and grpc is stateful, the stream ID can only be obtained during stateful encoding.
  // Currently, the stateful encoding for stream protocols is done through the EncodeStreamMessage interface in the
  // stream connection handler.
  // In order to call this interface without affecting the normal logic of other protocols' request-response exchanges,
  // a non-zero stream ID is used as a judgment basis for the presence of a stream exchange.
  // Therefore, although the stream ID cannot be obtained here, assigning it a maximum value represents a request
  // for a stream, which is then processed through the EncodeStreamMessage logic.
  context->SetStreamId(std::numeric_limits<uint32_t>::max());
  return true;
}

bool GrpcClientCodec::ZeroCopyDecode(const ClientContextPtr& context, std::any&& in, ProtocolPtr& out) {
  auto http2_response = std::any_cast<http2::ResponsePtr&&>(std::move(in));
  // If the response code is not "200 OK", decoding will not be performed.
  if (TRPC_UNLIKELY(http2_response->GetStatus() != 200)) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR), 0,
                  "decode failed, http2 status: " + std::to_string(http2_response->GetStatus())};
    TRPC_FMT_ERROR("{}", status.ToString());
    return false;
  }

  auto grpc_response = static_cast<GrpcUnaryResponseProtocol*>(out.get());
  grpc_response->SetRequestId(http2_response->GetContentSequenceId());
  grpc_response->SetHttp2Response(http2_response);

  // Both status and message are stored in the trailer.
  GrpcUnaryResponseProtocol::GrpcResponseHeader* grpc_header = grpc_response->GetMutableHeader();
  internal::GetHeaderInteger(http2_response->GetTrailer(), kGrpcStatusName, &grpc_header->status);
  internal::GetHeaderString(http2_response->GetTrailer(), kGrpcMessageName, &grpc_header->error_msg);

  // Key-value pairs are stored in the trailer.
  for (const auto& [name, value] : http2_response->GetTrailer().Pairs()) {
    // Filter out the request headers set by the user, exclude HTTP2-related headers, and exclude gRPC reserved headers.
    int token = http2::LookupToken(reinterpret_cast<const uint8_t*>(name.data()), name.size());
    if (token == -1 && !http::StringStartsWithLiteralsIgnoreCase(name, "grpc-")) {
      grpc_response->SetKVInfo(std::string{name}, std::string{value});
    }
  }

  // Response content.
  grpc_response->SetNonContiguousProtocolBody(std::move(*http2_response->GetMutableNonContiguousBufferContent()));
  return true;
}

ProtocolPtr GrpcClientCodec::CreateRequestPtr() { return std::make_shared<GrpcUnaryRequestProtocol>(); }

ProtocolPtr GrpcClientCodec::CreateResponsePtr() { return std::make_shared<GrpcUnaryResponseProtocol>(); }

uint32_t GrpcClientCodec::GetSequenceId(const ProtocolPtr& rsp) const {
  auto *grpc_rsp_msg = static_cast<GrpcUnaryResponseProtocol*>(rsp.get());
  return grpc_rsp_msg->GetHeader().request_id;
}

bool GrpcClientCodec::FillRequest(const ClientContextPtr& context, const ProtocolPtr& in, void* body) {
  auto unary_request = static_cast<GrpcUnaryRequestProtocol*>(in.get());
  NoncontiguousBuffer buffer{};
  GrpcMessageContent grpc_msg_content{};
  if (!Serialize(context, body, &buffer)) {
    return false;
  }

  if (context->GetReqCompressType() != compressor::kNone) {
    if (!Compress(context, &buffer)) {
      return false;
    }
    grpc_msg_content.compressed = 1;
  }

  grpc_msg_content.length = buffer.ByteSize();
  grpc_msg_content.content = std::move(buffer);
  NoncontiguousBuffer request_content{};
  if (TRPC_UNLIKELY(!grpc_msg_content.Encode(&request_content))) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), 0, "encode grpc message content failed"};
    context->SetStatus(std::move(status));
    return false;
  }
  unary_request->SetNonContiguousProtocolBody(std::move(request_content));
  return true;
}

bool GrpcClientCodec::FillResponse(const ClientContextPtr& context, const ProtocolPtr& in, void* body) {
  auto unary_response = static_cast<GrpcUnaryResponseProtocol*>(in.get());
  GrpcUnaryResponseProtocol::GrpcResponseHeader* grpc_header = unary_response->GetMutableHeader();

  // Checks the return code of the framework and the RPC method.
  // If the grpc status code and the framework return code are inconsistent, they will not be converted, and will be
  // temporarily set to func_ret for easy access by the user side.
  if (grpc_header->status != GrpcStatus::kGrpcOk) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR), grpc_header->status,
                  std::string{grpc_header->error_msg}};
    context->SetStatus(std::move(status));
    return false;
  }

  auto http2_response = unary_response->GetHttp2Response();
  // Both content-type and content-encoding are stored in the header.
  std::string content_type;
  if (internal::GetHeaderString(http2_response->GetHeader(), kGrpcContentTypeName, &content_type)) {
    context->SetRspEncodeType(GrpcContentTypeToTrpcContentType(content_type));
  }
  std::string content_encoding;
  if (internal::GetHeaderString(http2_response->GetHeader(), kGrpcEncodingName, &content_encoding)) {
    context->SetRspCompressType(GrpcContentEncodingToTrpcContentEncoding(content_encoding));
  }

  GrpcMessageContent grpc_msg_content{};
  auto response_content = unary_response->GetNonContiguousProtocolBody();
  if (!grpc_msg_content.Decode(&response_content)) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR), 0, "decode grpc message content failed"};
    context->SetStatus(std::move(status));
    return false;
  }

  if (grpc_msg_content.compressed) {
    if (!Decompress(context, &grpc_msg_content.content)) {
      return false;
    }
    grpc_msg_content.length = grpc_msg_content.content.ByteSize();
  }

  if (!Deserialize(context, &grpc_msg_content.content, body)) {
    return false;
  }
  return true;
}

bool GrpcClientCodec::Serialize(const ClientContextPtr& context, void* body, NoncontiguousBuffer* buffer) {
  serialization::SerializationType serialization_type = context->GetReqEncodeType();
  auto serialization = serialization::SerializationFactory::GetInstance()->Get(serialization_type);
  if (TRPC_UNLIKELY(serialization == nullptr)) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), 0,
                  "unsupported serialization type: " + std::to_string(serialization_type)};
    TRPC_LOG_ERROR(status.ToString());
    context->SetStatus(std::move(status));
    return false;
  }

  serialization::DataType data_type = context->GetReqEncodeDataType();
  auto serialize_success = serialization->Serialize(data_type, body, buffer);
  if (TRPC_UNLIKELY(!serialize_success)) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), 0, "encode failed"};
    TRPC_LOG_ERROR(status.ToString());
    context->SetStatus(std::move(status));
  }
  return serialize_success;
}

bool GrpcClientCodec::Deserialize(const ClientContextPtr& context, NoncontiguousBuffer* buffer, void* body) {
  serialization::SerializationType serialization_type = context->GetRspEncodeType();
  auto serialization = serialization::SerializationFactory::GetInstance()->Get(serialization_type);
  if (TRPC_UNLIKELY(serialization == nullptr)) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR), 0,
                  "unsupported deserialization type: " + std::to_string(serialization_type)};
    TRPC_LOG_ERROR(status.ToString());
    context->SetStatus(std::move(status));
    return false;
  }

  serialization::DataType data_type = context->GetRspEncodeDataType();
  auto deserialize_success = serialization->Deserialize(buffer, data_type, body);
  if (TRPC_UNLIKELY(!deserialize_success)) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR), 0, "decode failed"};
    TRPC_LOG_ERROR(status.ToString());
    context->SetStatus(std::move(status));
  }
  return deserialize_success;
}

bool GrpcClientCodec::Compress(const ClientContextPtr& context, NoncontiguousBuffer* buffer) {
  auto compress_success =
      compressor::CompressIfNeeded(context->GetReqCompressType(), *buffer, context->GetReqCompressLevel());
  if (TRPC_UNLIKELY(!compress_success)) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), 0, "compress failed"};
    TRPC_LOG_ERROR(status.ToString());
    context->SetStatus(std::move(status));
  }
  return compress_success;
}

bool GrpcClientCodec::Decompress(const ClientContextPtr& context, NoncontiguousBuffer* buffer) {
  auto decompress_success = compressor::DecompressIfNeeded(context->GetRspCompressType(), *buffer);
  if (TRPC_UNLIKELY(!decompress_success)) {
    Status status{GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR), 0, "decompress failed"};
    TRPC_LOG_ERROR(status.ToString());
    context->SetStatus(std::move(status));
  }
  return decompress_success;
}

std::string GrpcClientCodec::GetTimeoutText(uint32_t timeout) {
  // Sets timeout value to milliseconds.
  return std::to_string(timeout) + "m";
}

}  // namespace trpc
