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

#include "trpc/codec/grpc/grpc_server_codec.h"

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/grpc_stream_frame.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/util/http/util.h"

namespace trpc {

bool GrpcServerCodec::ZeroCopyDecode(const ServerContextPtr& context, std::any&& in, ProtocolPtr& out) {
  auto packet = std::any_cast<stream::GrpcRequestPacket>(in);
  if (!packet.req) {
    TRPC_LOG_ERROR("Grpc Streaming RPC error executing in Unary RPC");
    return false;
  }

  http2::RequestPtr&& http2_request = std::move(packet.req);
  auto grpc_request = static_cast<GrpcUnaryRequestProtocol*>(out.get());
  grpc_request->SetHttp2Request(http2_request);

  // Func-Name.
  grpc_request->SetFuncName(http2_request->GetPath());

  // Content-Type.
  std::string content_type{""};
  internal::GetHeaderString(http2_request->GetHeader(), kGrpcContentTypeName, &content_type);
  context->SetReqEncodeType(GrpcContentTypeToTrpcContentType(content_type));

  // Timeout.
  std::string timeout_text{""};
  internal::GetHeaderString(http2_request->GetHeader(), kGrpcTimeoutName, &timeout_text);
  int64_t timeout_us = GrpcTimeoutTextToUS(timeout_text);
  if (timeout_us != -1) {
    // Converts microseconds to milliseconds.
    grpc_request->SetTimeout(static_cast<uint32_t>(timeout_us / 1000));
  }

  // Key-value pairs defined by user.
  for (const auto& [name, value] : http2_request->GetHeaderPairs()) {
    auto token = http2::LookupToken(reinterpret_cast<const uint8_t*>(name.data()), name.size());
    // Filter out the request headers set by the user, exclude HTTP2-related headers, and exclude gRPC reserved headers.
    if (token == -1 && !http::StringStartsWithLiteralsIgnoreCase(name, "grpc-")) {
      grpc_request->SetKVInfo(std::string{name}, std::string{value});
    }
  }

  // Message content.
  GrpcMessageContent grpc_msg_content{};
  auto request_body = http2_request->GetNonContiguousBufferContent();
  if (TRPC_UNLIKELY(!grpc_msg_content.Decode(&request_body))) {
    TRPC_LOG_ERROR("decode grpc message content failed");
    return false;
  }
  grpc_request->SetNonContiguousProtocolBody(std::move(grpc_msg_content.content));

  // Content-Encoding.
  if (grpc_msg_content.compressed) {
    std::string content_encoding{""};
    internal::GetHeaderString(http2_request->GetHeader(), kGrpcEncodingName, &content_encoding);
    context->SetReqCompressType(GrpcContentEncodingToTrpcContentEncoding(content_encoding));
  }

  context->SetRspEncodeType(context->GetReqEncodeType());
  context->SetRspCompressType(context->GetReqCompressType());

  auto grpc_response = static_cast<GrpcUnaryResponseProtocol*>(context->GetResponseMsg().get());
  grpc_response->GetHttp2Response()->SetStreamId(grpc_request->GetHttp2Request()->GetStreamId());
  // Sets stream-related information. The business can obtain the stream ID through the ServerContext and be able to
  // use stream response logic when the framework synchronously or asynchronously replies.
  context->SetStreamId(grpc_request->GetHttp2Request()->GetStreamId());
  return true;
}

bool GrpcServerCodec::ZeroCopyEncode(const ServerContextPtr& context, ProtocolPtr& in, NoncontiguousBuffer& out) {
  auto grpc_response = static_cast<GrpcUnaryResponseProtocol*>(in.get());

  auto http2_response = grpc_response->GetHttp2Response();
  http2_response->SetStatus(200);

  int grpc_status{0};
  const auto& status = context->GetStatus();
  if (!status.OK()) {
    grpc_status = (status.GetFrameworkRetCode() != 0) ? status.GetFrameworkRetCode() : status.GetFuncRetCode();
    http2_response->GetMutableTrailer()->Add(kGrpcMessageName, status.ToString());
  }
  http2_response->GetMutableTrailer()->Add(kGrpcStatusName, std::to_string(grpc_status));

  http2_response->AddHeader(kGrpcContentTypeName, TrpcContentTypeToGrpcContentType(context->GetRspEncodeType()));
  std::string content_encoding = TrpcContentEncodingToGrpcContentEncoding(context->GetRspCompressType());
  if (!content_encoding.empty()) {
    http2_response->AddHeader(kGrpcEncodingName, content_encoding);
  }

  // Key-values defined by user.
  for (const auto& [name, value] : grpc_response->GetKVInfos()) {
    http2_response->GetMutableTrailer()->Add(name, value);
  }

  GrpcMessageContent grpc_msg_content{};
  grpc_msg_content.compressed = !content_encoding.empty() ? 1 : 0;
  grpc_msg_content.content = grpc_response->GetNonContiguousProtocolBody();
  grpc_msg_content.length = grpc_msg_content.content.ByteSize();
  NoncontiguousBuffer response_content{};
  if (TRPC_UNLIKELY(!grpc_msg_content.Encode(&response_content))) {
    TRPC_LOG_ERROR("encode grpc message content failed");
    return false;
  }
  http2_response->SetNonContiguousBufferContent(std::move(response_content));
  return true;
}

ProtocolPtr GrpcServerCodec::CreateRequestObject() { return std::make_shared<GrpcUnaryRequestProtocol>(); }

ProtocolPtr GrpcServerCodec::CreateResponseObject() { return std::make_shared<GrpcUnaryResponseProtocol>(); }

bool GrpcServerCodec::Pick(const std::any& message, std::any& data) const {
  auto& packet = std::any_cast<const stream::GrpcRequestPacket&>(message);
  auto& metadata = std::any_cast<GrpcProtocolMessageMetadata&>(data);
  if (!packet.frame) {
    return true;
  }
  metadata.stream_id = packet.frame->GetStreamId();
  metadata.stream_frame_type = static_cast<uint8_t>(packet.frame->GetFrameType());
  metadata.enable_stream = true;
  return true;
}

int GrpcServerCodec::GetProtocolRetCode(codec::ServerRetCode ret_code) const {
  switch (ret_code) {
    case codec::ServerRetCode::SUCCESS:
      return GrpcStatus::kGrpcOk;
    case codec::ServerRetCode::ENCODE_ERROR:
    case codec::ServerRetCode::DECODE_ERROR:
    case codec::ServerRetCode::INVOKE_UNKNOW_ERROR:
      return GrpcStatus::kGrpcInternal;
    case codec::ServerRetCode::NOT_FUN_ERROR:
      return GrpcStatus::kGrpcUnimplemented;
    case codec::ServerRetCode::TIMEOUT_ERROR:
    case codec::ServerRetCode::FULL_LINK_TIMEOUT_ERROR:
      return GrpcStatus::kGrpcDeadlineExceeded;
    case codec::ServerRetCode::OVERLOAD_ERROR:
    case codec::ServerRetCode::LIMITED_ERROR:
      return GrpcStatus::kGrpcResourceExhausted;
    case codec::ServerRetCode::AUTH_ERROR:
      return GrpcStatus::kGrpcUnauthenticated;
    default:
      return GrpcStatus::kGrpcInternal;
  }
}

}  // namespace trpc
