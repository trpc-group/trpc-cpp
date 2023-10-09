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

#include "trpc/codec/http/http_client_codec.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/codec/http/http_proto_checker.h"
#include "trpc/common/status.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc {

int HttpClientCodec::ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  return HttpZeroCopyCheckResponse(conn, in, out);
}

bool HttpClientCodec::ZeroCopyEncode(const ClientContextPtr& ctx, const ProtocolPtr& in, NoncontiguousBuffer& out) {
  auto* http_req_msg = static_cast<HttpRequestProtocol*>(in.get());
  if (!http_req_msg->request->GetHeader().Has("Host")) {
    // Use IP:Port as Host header(e.g. Host: 127.0.0.1:8080)
    std::string host = ctx->GetIp();
    host.append(":").append(std::to_string(ctx->GetPort()));
    http_req_msg->request->SetHeader("Host", std::move(host));
  }

  auto body_buf = http_req_msg->GetNonContiguousProtocolBody();
  if (body_buf.ByteSize() > 0) {
    http_req_msg->request->SetHeader(http::kHeaderContentLength, std::to_string(body_buf.ByteSize()));
  }

  NoncontiguousBufferBuilder builder;
  http_req_msg->request->SerializeToString(builder);
  out = builder.DestructiveGet();
  // Body_buf stores body of RPC-Over-HTTP.
  if (body_buf.ByteSize() > 0) {
    out.Append(std::move(body_buf));
  }
  return true;
}

bool HttpClientCodec::ZeroCopyDecode(const ClientContextPtr& ctx, std::any&& in, ProtocolPtr& out) {
  try {
    auto* http_rsp_msg = static_cast<HttpResponseProtocol*>(out.get());
    http_rsp_msg->response = std::any_cast<http::Response&&>(std::move(in));
    return true;
  } catch (std::exception& e) {
    TRPC_LOG_ERROR("HTTP decode throw exception: " << e.what());
  }
  return false;
}

bool HttpClientCodec::FillRequest(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) {
  serialization::Serialization* serialization =
      serialization::SerializationFactory::GetInstance()->Get(ctx->GetReqEncodeType());
  if (!serialization) {
    ctx->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), "unknown encode type"));
    return false;
  }

  NoncontiguousBuffer data;
  if (TRPC_UNLIKELY(!serialization->Serialize(ctx->GetReqEncodeDataType(), body, &data))) {
    ctx->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), "encode failed."));
    return false;
  }

  auto* http_req_msg = static_cast<HttpRequestProtocol*>(in.get());
  auto& req = http_req_msg->request;
  compressor::CompressType compress_type = ctx->GetReqCompressType();
  if (compress_type != compressor::kNone && data.ByteSize() > 0) {
    bool compress_ret = compressor::CompressIfNeeded(compress_type, data, ctx->GetReqCompressLevel());
    if (TRPC_UNLIKELY(!compress_ret)) {
      ctx->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), "compress failed."));
      return compress_ret;
    }
    req->SetHeader(http::kHeaderContentEncoding, http::CompressTypeToString(compress_type));
    req->SetHeaderIfNotPresent(http::kHeaderAcceptEncoding, http::CompressTypeToString(compress_type));
  }

  auto req_body_size = data.ByteSize();
  req->SetNonContiguousBufferContent(std::move(data));
  if (req_body_size > 0) {
    req->SetHeader(http::kHeaderContentLength, std::to_string(req_body_size));
  }

  if (!http_req_msg->IsFromHttpServiceProxy()) {
    req->SetMethodType(http::OperationType::POST);
    req->SetVersion("1.1");
    req->SetUrl(ctx->GetFuncName());
    for (const auto& kv : ctx->GetHttpHeaders()) {
      req->AddHeader(kv.first, kv.second);
    }
    switch (ctx->GetReqEncodeType()) {
      case serialization::kPbType:
        req->SetHeaderIfNotPresent("Content-Type", "application/pb");
        break;
      case serialization::kJsonType:
        req->SetHeaderIfNotPresent("Content-Type", "application/json");
        req->SetHeaderIfNotPresent("Accept", "application/json");
        break;
      case serialization::kNoopType:
        // Sets octet-stream as default.
        req->SetHeaderIfNotPresent("Content-Type", "application/octet-stream");
        req->SetHeaderIfNotPresent("Accept", "application/octet-stream");
        break;
      default:
        TRPC_LOG_ERROR("unsupported encode_type:" << ctx->GetReqEncodeType());
        return false;
    }
  }
  return true;
}

bool HttpClientCodec::FillResponse(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) {
  Status status;
  auto* http_rsp_msg = static_cast<HttpResponseProtocol*>(in.get());
  int trpc_ret = internal::GetHeaderInteger(http_rsp_msg->response.GetHeader(), "trpc-ret");
  int trpc_func_ret = internal::GetHeaderInteger(http_rsp_msg->response.GetHeader(), "trpc-func-ret");
  // Checking: `trpc-ret: xx`
  // Checking: `trpc-func-ret: xx`
  if (trpc_ret != 0 || trpc_func_ret != 0) {
    status.SetFrameworkRetCode(trpc_ret);
    status.SetFuncRetCode(trpc_func_ret);
    status.SetErrorMessage(internal::GetHeaderString(http_rsp_msg->response.GetHeader(), "trpc-error-msg"));
    TRPC_LOG_ERROR(status.ToString());
    ctx->SetStatus(std::move(status));
    return false;
  }

  // 2xx was required while RPC over HTTP,
  auto rsp_http_status_code = http_rsp_msg->response.GetStatus();
  if (rsp_http_status_code > http::ResponseStatus::kPartialContent) {
    std::string error{fmt::format("bad http rsp status: {}.", rsp_http_status_code)};
    TRPC_LOG_ERROR(error);
    status.SetFuncRetCode(rsp_http_status_code);
    status.SetErrorMessage(std::move(error));
    ctx->SetStatus(std::move(status));
    return false;
  }

  const std::string& compress_type = http_rsp_msg->response.GetHeader().Get(http::kHeaderContentEncoding);
  if (!compress_type.empty()) {
    ctx->SetRspCompressType(http::StringToCompressType(compress_type));
  }

  auto rsp_body_data = http_rsp_msg->GetNonContiguousProtocolBody();
  bool decompress_ok = compressor::DecompressIfNeeded(ctx->GetRspCompressType(), rsp_body_data);
  if (TRPC_UNLIKELY(!decompress_ok)) {
    ctx->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR),
                          "http input buffer which was decompressed failed"));
    return decompress_ok;
  }

  serialization::Serialization* serialization =
      serialization::SerializationFactory::GetInstance()->Get(ctx->GetRspEncodeType());
  if (!serialization) {
    ctx->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::ENCODE_ERROR), "unknown encode type"));
    return false;
  }

  bool decode_ok = serialization->Deserialize(&rsp_body_data, ctx->GetRspEncodeDataType(), body);
  if (!decode_ok) {
    ctx->SetStatus(Status(GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR),
                          "http input response parse from array failed"));
    return decode_ok;
  }
  return decode_ok;
}

ProtocolPtr HttpClientCodec::CreateRequestPtr() {
  return std::make_shared<HttpRequestProtocol>(std::make_shared<http::Request>());
}

ProtocolPtr HttpClientCodec::CreateResponsePtr() { return std::make_shared<HttpResponseProtocol>(); }
}  // namespace trpc
