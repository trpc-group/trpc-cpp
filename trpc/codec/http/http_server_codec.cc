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

#include "trpc/codec/http/http_server_codec.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/codec/http/http_proto_checker.h"
#include "trpc/util/http/base64.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/string_helper.h"

namespace trpc {

namespace {
// @brief Converts framework code to HTTP status code.
// @param[in] framework_code.
// @param[out] http_status.
// @return Returns true on success, false otherwise.
bool TrpcErrorsToHttpStatus(int framework_code, int* http_status) {
  switch (framework_code) {
    case TrpcRetCode::TRPC_SERVER_DECODE_ERR:
      *http_status = http::HttpResponse::StatusCode::kBadRequest;
      return true;
    case TrpcRetCode::TRPC_SERVER_ENCODE_ERR:
    case TrpcRetCode::TRPC_SERVER_AUTH_ERR:
    case TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR:
    case TrpcRetCode::TRPC_SERVER_LIMITED_ERR:
      *http_status = http::HttpResponse::StatusCode::kInternalServerError;
      return true;
    case TrpcRetCode::TRPC_SERVER_NOFUNC_ERR:
      *http_status = http::HttpResponse::StatusCode::kNotFound;
      return true;
    case TrpcRetCode::TRPC_SERVER_TIMEOUT_ERR:
      *http_status = http::HttpResponse::StatusCode::kGatewayTimeout;
      return true;
    case TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR:
      *http_status = http::HttpResponse::StatusCode::kTooManyRequests;
      return true;
    default:
      return false;
  }
}

bool ContentTypeToSerializationType(std::string_view content_type, uint32_t* serialization_type) {
  // HTTP Content-Type to trpc serialization-type.
  static const std::unordered_map<std::string_view, uint32_t> types{
      {"application/json", TrpcContentEncodeType::TRPC_JSON_ENCODE},
      {"application/protobuf", TrpcContentEncodeType::TRPC_PROTO_ENCODE},
      {"application/pb", TrpcContentEncodeType::TRPC_PROTO_ENCODE},
      {"application/proto", TrpcContentEncodeType::TRPC_PROTO_ENCODE},
      {"application/x-protobuf", TrpcContentEncodeType::TRPC_PROTO_ENCODE},
      {"application/octet-stream", TrpcContentEncodeType::TRPC_NOOP_ENCODE},
  };
  auto iter = types.find(content_type);
  if (iter == types.end()) return false;
  *serialization_type = iter->second;
  return true;
}

bool SerializationTypeToContentType(uint32_t serialization_type, std::string* content_type) {
  // Trpc serialization-type to  HTTP Content-Type.
  static const std::map<uint32_t, std::string> types{
      {TrpcContentEncodeType::TRPC_PROTO_ENCODE, "application/proto"},
      {TrpcContentEncodeType::TRPC_JSON_ENCODE, "application/json"},
  };
  auto iter = types.find(serialization_type);
  if (iter == types.end()) return false;
  *content_type = iter->second;
  return true;
}

void ParseTrpcTransInfo(std::string& value, HttpRequestProtocol* req) {
  rapidjson::Document document;
  document.Parse(value.c_str());
  if (document.IsObject()) {
    for (auto& mem : document.GetObject()) {
      std::string key = mem.name.GetString();
      if (document[key].IsString()) {
#ifdef TRPC_ENABLE_HTTP_TRANSINFO_BASE64
        std::string ori_value = mem.value.GetString();
        std::string decoded = http::Base64Decode(std::begin(ori_value), std::end(ori_value));
        if (decoded.empty()) {
          req->SetKVInfo(key, ori_value);
          TRPC_LOG_TRACE("trpc transinfo key:" << key << ",value:" << ori_value);
        } else {
          req->SetKVInfo(key, decoded);
          TRPC_LOG_TRACE("trpc transinfo key:" << key << ",value:" << decoded);
        }
#else
        req->SetKVInfo(key, mem.value.GetString());
        TRPC_LOG_TRACE("trpc transinfo key:" << key << ",value:" << mem.value.GetString());
#endif
      }
    }
  } else {
    TRPC_LOG_ERROR("HTTP Header[trpc-trans-info] value need to be json format");
  }
}

void SetTrpcTransInfo(const ServerContextPtr& ctx, HttpResponseProtocol* rsp) {
  if (!ctx->GetPbRspTransInfo().empty()) {
    rapidjson::StringBuffer str_buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(str_buf);
    writer.StartObject();
    for (const auto& [name, value] : ctx->GetPbRspTransInfo()) {
      writer.Key(name.c_str());
#ifdef TRPC_ENABLE_HTTP_TRANSINFO_BASE64
      writer.String(http::Base64Encode(std::begin(value), std::end(value)).c_str());
#else
      writer.String(value.c_str());
#endif
    }
    writer.EndObject();
    rsp->response.AddHeader("trpc-trans-info", str_buf.GetString());
  }
}
}  // namespace

int HttpServerCodec::ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  return HttpZeroCopyCheckRequest(conn, in, out);
}

bool HttpServerCodec::ZeroCopyDecode(const ServerContextPtr& ctx, std::any&& in, ProtocolPtr& out) {
  auto http_req = std::any_cast<http::RequestPtr&&>(std::move(in));
  auto* http_req_msg = static_cast<HttpRequestProtocol*>(out.get());
  http_req_msg->request = std::move(http_req);
  return true;
}

bool HttpServerCodec::ZeroCopyEncode(const ServerContextPtr& ctx, ProtocolPtr& in, NoncontiguousBuffer& out) {
  return true;
}

ProtocolPtr HttpServerCodec::CreateRequestObject() { return std::make_shared<HttpRequestProtocol>(); }

ProtocolPtr HttpServerCodec::CreateResponseObject() { return std::make_shared<HttpResponseProtocol>(); }

bool TrpcOverHttpServerCodec::ZeroCopyDecode(const ServerContextPtr& ctx, std::any&& in, ProtocolPtr& out) {
  try {
    auto http_req = std::any_cast<http::RequestPtr&&>(std::move(in));
    auto* http_req_msg = static_cast<HttpRequestProtocol*>(out.get());
    std::string value;

    value = http_req->GetHeader().Get("trpc-call-type");
    if (!value.empty()) {
      ctx->SetCallType(static_cast<RpcCallType>(std::stoi(value)));
    }

    value = http_req->GetHeader("trpc-request-id");
    if (!value.empty()) {
      auto req_id = std::stoi(value);
      ctx->SetRequestId(req_id);
      http_req_msg->SetRequestId(req_id);
    }

    value = http_req->GetHeader("trpc-timeout");
    if (!value.empty()) {
      auto timeout = std::stoi(value);
      ctx->SetTimeout(timeout);
      http_req_msg->SetTimeout(timeout);
    }

    value = http_req->GetHeader("trpc-caller");
    if (!value.empty()) {
      http_req_msg->SetCallerName(value);
    }

    value = http_req->GetHeader("trpc-callee");
    if (!value.empty()) {
      http_req_msg->SetCalleeName(value);
    }

    http_req_msg->SetFuncName(http_req->GetRouteUrl());

    value = http_req->GetHeader("trpc-message-type");
    if (!value.empty()) {
      ctx->SetMessageType(std::stoi(value));
    }

    // Content-Type.
    const auto& content_type = http_req->GetHeader(http::kHeaderContentType);
    if (!content_type.empty()) {
      // Try to parse string like: content-type:text/html; charset=utf-8
      std::map<std::string_view, uint32_t>::const_iterator it;
      uint32_t req_encode_type{0};
      auto pos = content_type.find(';');
      bool ok = (pos == std::string::npos)
                    ? ContentTypeToSerializationType(content_type, &req_encode_type)
                    : ContentTypeToSerializationType(content_type.substr(0, pos), &req_encode_type);
      if (ok) {
        ctx->SetReqEncodeType(req_encode_type);
        // Async-Response depends on it.
        ctx->SetRspEncodeType(req_encode_type);
      }
    }

    //  Content-Encoding.
    const std::string& content_encoding = http_req->GetHeader(http::kHeaderContentEncoding);
    if (!content_encoding.empty() && content_encoding != "identity") {
      // Set compressor::kMaxType if compress method not found.
      compressor::CompressType compress_type = http::StringToCompressType(content_encoding);
      ctx->SetReqCompressType(compress_type);
    }
    std::vector<std::string_view> accept_encoding_list = Split(http_req->GetHeader(http::kHeaderAcceptEncoding), ",");
    for (const auto& accept_encoding : accept_encoding_list) {
      std::string_view formatted_accept_encoding = Trim(accept_encoding);
      compressor::CompressType compress_type = http::StringToCompressType(formatted_accept_encoding);
      if (compress_type != compressor::kMaxType && compress_type != compressor::kNone) {
        ctx->SetRspCompressType(compress_type);
        break;
      }
    }

    // Key-value pairs for trpc-over-http
    value = http_req->GetHeader("trpc-trans-info");
    if (!value.empty()) {
      ParseTrpcTransInfo(value, http_req_msg);
    }
    http_req_msg->request = std::move(http_req);
  } catch (const std::exception& ex) {
    TRPC_LOG_ERROR("HTTP decode throw execption: " << ex.what());
    ctx->SetResponseWhenDecodeFail(false);
    return false;
  }
  return true;
}

bool TrpcOverHttpServerCodec::ZeroCopyEncode(const ServerContextPtr& ctx, ProtocolPtr& in, NoncontiguousBuffer& out) {
  try {
    ProtocolPtr http_req_msg = ctx->GetRequestMsg();
    const auto& http_req = *static_cast<HttpRequestProtocol*>(ctx->GetRequestMsg().get())->request;
    auto* http_rsp_msg = static_cast<HttpResponseProtocol*>(in.get());

    if (http_rsp_msg->response.IsOK()) {
      http_rsp_msg->response.SetVersion(http_req.GetVersion());
      http_rsp_msg->response.SetHeader("trpc-call-type", std::to_string(ctx->GetCallType()));
      uint32_t seq_id = 0;
      http_req_msg->GetRequestId(seq_id);
      http_rsp_msg->response.SetHeader("trpc-request-id", std::to_string(seq_id));
      http_rsp_msg->response.SetHeader("trpc-ret", std::to_string(ctx->GetStatus().GetFrameworkRetCode()));
      http_rsp_msg->response.SetHeader("trpc-func-ret", std::to_string(ctx->GetStatus().GetFuncRetCode()));
      http_rsp_msg->response.SetHeader("trpc-error-msg", ctx->GetStatus().ErrorMessage());
      http_rsp_msg->response.SetHeader("trpc-message-type", std::to_string(ctx->GetMessageType()));

      // Explanation: Here, we are addressing the issue of duplicate Content-Type when HttpService responds
      // asynchronously.
      // If it is not set by the business itself, the response type should be consistent with the serialization field
      // of the request, to address the issue of non-trpc clients not recognizing the application/proto field.
      // Example: Clients cannot recognize fields other than application/x-protobuf.
      std::string content_type = http_req.GetHeader(http::kHeaderContentType);
      if (content_type.empty()) {
        (void)SerializationTypeToContentType(ctx->GetRspEncodeType(), &content_type);
      }
      http_rsp_msg->response.SetHeaderIfNotPresent(http::kHeaderContentType, content_type);
      compressor::CompressType compress_type = ctx->GetRspCompressType();
      if (compress_type != compressor::kNone && compress_type < compressor::kMaxType) {
        http_rsp_msg->response.SetHeader(http::kHeaderContentEncoding,
                                         http::CompressTypeToString(ctx->GetRspCompressType()));
      }

      // Set trans info into header
      SetTrpcTransInfo(ctx, http_rsp_msg);
    }

    int http_status;
    if (TrpcErrorsToHttpStatus(ctx->GetStatus().GetFrameworkRetCode(), &http_status)) {
      http_rsp_msg->response.SetStatus(http_status);
    }

    auto body_buf = http_rsp_msg->GetNonContiguousProtocolBody();
    if (!body_buf.Empty()) {
      http_rsp_msg->response.AddHeader(http::kHeaderContentLength, std::to_string(body_buf.ByteSize()));
    }
    http_rsp_msg->response.SerializeToString(out);
    if (!body_buf.Empty()) {
      out.Append(std::move(body_buf));
    }
  } catch (const std::exception& ex) {
    TRPC_LOG_ERROR("HTTP encode throw exception: " << ex.what());
    return false;
  }
  return true;
}

}  // namespace trpc
