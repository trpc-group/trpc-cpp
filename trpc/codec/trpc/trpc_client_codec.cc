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

#include "trpc/codec/trpc/trpc_client_codec.h"

#include <memory>
#include <utility>

#include "trpc/codec/codec_helper.h"
#include "trpc/codec/trpc/trpc_proto_checker.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/util/buffer/zero_copy_stream.h"
#include "trpc/util/log/logging.h"

namespace trpc {

int TrpcClientCodec::ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  return CheckTrpcProtocolMessage(conn, in, out);
}

bool TrpcClientCodec::ZeroCopyDecode(const ClientContextPtr& context, std::any&& in, ProtocolPtr& out) {
  auto buf = std::any_cast<NoncontiguousBuffer&&>(std::move(in));
  return out->ZeroCopyDecode(buf);
}

bool TrpcClientCodec::ZeroCopyEncode(const ClientContextPtr& context, const ProtocolPtr& in, NoncontiguousBuffer& out) {
  auto* trpc_req = static_cast<TrpcRequestProtocol*>(in.get());
  FillTrpcRequestHeader(context, trpc_req);
  return trpc_req->ZeroCopyEncode(out);
}

void TrpcClientCodec::FillTrpcRequestHeader(const ClientContextPtr& context, TrpcRequestProtocol* req) {
  req->req_header.set_version(0);
  req->req_header.set_call_type(context->GetCallType());
  req->req_header.set_request_id(context->GetRequestId());
  req->req_header.set_timeout(context->GetTimeout());
  req->req_header.set_caller(context->GetCallerName());
  req->req_header.set_callee(context->GetCalleeName());
  req->req_header.set_message_type(context->GetMessageType());
  req->req_header.set_content_type(context->GetReqEncodeType());
  req->req_header.set_content_encoding(context->GetReqCompressType());
}

bool TrpcClientCodec::FillRequest(const ClientContextPtr& context, const ProtocolPtr& in, void* body) {
  TRPC_ASSERT(body);

  auto* trpc_req_protocol = static_cast<TrpcRequestProtocol*>(in.get());
  TRPC_ASSERT(trpc_req_protocol);

  if (TRPC_UNLIKELY(context->IsTransparent())) {
    return ProcessTransparentReq(trpc_req_protocol, body);
  }

  serialization::SerializationType serialization_type = context->GetReqEncodeType();
  serialization::SerializationFactory* serializationfactory = serialization::SerializationFactory::GetInstance();
  auto serialization = serializationfactory->Get(serialization_type);
  if (serialization == nullptr) {
    std::string error_msg = "not support serialization_type:";
    error_msg += std::to_string(serialization_type);

    TRPC_LOG_ERROR(error_msg);

    context->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), "unknown encode type"));
    return false;
  }

  serialization::DataType type = context->GetReqEncodeDataType();

  NoncontiguousBuffer data;
  bool encode_ret = serialization->Serialize(type, body, &data);
  if (TRPC_UNLIKELY(!encode_ret)) {
    context->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), "encode failed."));
    return encode_ret;
  }

  auto compress_type = context->GetReqCompressType();
  bool compress_ret = compressor::CompressIfNeeded(compress_type, data, context->GetReqCompressLevel());
  if (TRPC_UNLIKELY(!compress_ret)) {
    context->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), "compress failed."));
    return compress_ret;
  }
  trpc_req_protocol->SetNonContiguousProtocolBody(std::move(data));

  return true;
}

void TrpcClientCodec::FillResponseContext(const ClientContextPtr& context, TrpcResponseProtocol* rsp) {
  context->SetRspEncodeType(rsp->rsp_header.content_type());
  context->SetRspCompressType(rsp->rsp_header.content_encoding());
}

bool TrpcClientCodec::FillResponse(const ClientContextPtr& context, const ProtocolPtr& in, void* body) {
  TRPC_ASSERT(body);

  auto* trpc_rsp_protocol = static_cast<TrpcResponseProtocol*>(in.get());
  TRPC_ASSERT(trpc_rsp_protocol);

  FillResponseContext(context, trpc_rsp_protocol);

  const auto& rsp_header = trpc_rsp_protocol->rsp_header;

  if (TRPC_UNLIKELY(rsp_header.ret() != 0 || rsp_header.func_ret() != 0)) {
    context->SetStatus(Status(rsp_header.ret(), rsp_header.func_ret(), rsp_header.error_msg()));
    return false;
  }

  context->SetResponseAttachment(trpc_rsp_protocol->GetProtocolAttachment());

  if (TRPC_UNLIKELY(context->IsTransparent())) {
    return ProcessTransparentRsp(trpc_rsp_protocol, body);
  }

  auto rsp_body_data = trpc_rsp_protocol->GetNonContiguousProtocolBody();

  auto compress_type = context->GetRspCompressType();
  bool decompress_ret = compressor::DecompressIfNeeded(compress_type, rsp_body_data);

  if (TRPC_UNLIKELY(!decompress_ret)) {
    std::string error("trpc input buffer which was decompressed failed");
    context->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR), 0, error));
    return decompress_ret;
  }

  serialization::SerializationType serialization_type = context->GetRspEncodeType();
  serialization::SerializationFactory* serializationfactory = serialization::SerializationFactory::GetInstance();
  auto serialization = serializationfactory->Get(serialization_type);

  serialization::DataType type = context->GetRspEncodeDataType();

  bool decode_ret = serialization->Deserialize(&rsp_body_data, type, body);
  if (!decode_ret) {
    std::string error("trpc input pb response parese from array failed");
    context->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR), 0, error));
    return decode_ret;
  }

  return true;
}

ProtocolPtr TrpcClientCodec::CreateRequestPtr() { return std::make_shared<TrpcRequestProtocol>(); }

ProtocolPtr TrpcClientCodec::CreateResponsePtr() { return std::make_shared<TrpcResponseProtocol>(); }

uint32_t TrpcClientCodec::GetSequenceId(const ProtocolPtr& rsp) const {
  auto* trpc_rsp_msg = static_cast<TrpcResponseProtocol*>(rsp.get());
  return trpc_rsp_msg->rsp_header.request_id();
}

bool TrpcClientCodec::ProcessTransparentReq(TrpcRequestProtocol* req_protocol, void* body) {
  auto buf = *reinterpret_cast<NoncontiguousBuffer*>(body);
  req_protocol->SetNonContiguousProtocolBody(std::move(buf));
  return true;
}

bool TrpcClientCodec::ProcessTransparentRsp(TrpcResponseProtocol* rsp_protocol, void* body) {
  auto* rsp_bin = reinterpret_cast<NoncontiguousBuffer*>(body);
  *rsp_bin = rsp_protocol->GetNonContiguousProtocolBody();
  return true;
}

bool TrpcClientCodec::Pick(const std::any& message, std::any& data) const {
  return PickTrpcProtocolMessageMetadata(message, data);
}

}  // namespace trpc
