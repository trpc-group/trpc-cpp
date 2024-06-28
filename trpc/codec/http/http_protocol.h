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

#pragma once

#include <string>
#include <utility>

#include "trpc/codec/protocol.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc {

/// @brief The codec protocol name for http.
constexpr char kHttpCodecName[] = "http";
/// @brief The codec protocol name for the scenario that accessing tRPC services via HTTP.
constexpr char kTrpcOverHttpCodecName[] = "trpc_over_http";

/// @brief Converts business data encoding type to corresponding MIME type.
std::string_view EncodeTypeToMime(int encode_type);

/// @brief Converts MIME type to business data encoding type.
int MimeToEncodeType(std::string_view mime);

/// @brief HTTP request protocol message.
class HttpRequestProtocol : public Protocol {
 public:
  // Used by server side.
  HttpRequestProtocol() : request(std::make_shared<http::Request>()) {}
  // Used by client side.
  explicit HttpRequestProtocol(http::RequestPtr&& request) : request(std::move(request)) {}
  ~HttpRequestProtocol() override = default;

  /// @brief Decode/encode o protocol message (zero copy).
  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override { return false; }
  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override { return false; }

  /// @brief  Reports whether waits for the full request message content to arrive (For the RPC-Over-HTTP scene).
  bool WaitForFullRequest() override;

  /// @brief Set/Get body payload of protocol message.
  /// @note For "GetBody" function, the value of body will be moved, cannot be called repeatedly.
  void SetNonContiguousProtocolBody(NoncontiguousBuffer&& buff) override {
    request->SetNonContiguousBufferContent(std::move(buff));
  }
  NoncontiguousBuffer GetNonContiguousProtocolBody() override {
    return std::move(*request->GetMutableNonContiguousBufferContent());
  }

  /// @brief Get/Set the unique id of request/response protocol.
  bool GetRequestId(uint32_t& req_id) const override {
    req_id = request_id;
    return true;
  }
  bool SetRequestId(uint32_t req_id) override {
    request_id = req_id;
    return true;
  }

  // @brief A flag indicates whether request send from "HttpServiceProxy" or not.
  void SetFromHttpServiceProxy(bool from_http_service_proxy) { from_http_service_proxy_ = from_http_service_proxy; }
  bool IsFromHttpServiceProxy() { return from_http_service_proxy_; }

  /// @brief Get size of message
  uint32_t GetMessageSize() const override;

 public:
  uint32_t request_id{0};
  http::RequestPtr request{nullptr};
  // Whether the RPC call goes through http_service_proxy affects codec->FillRequest.
  // For calls that go through http_service_proxy, the header is already filled, so there is no need to fill it in
  // codec->FillRequest.
  // For calls that do not go through http_service_proxy, the header is not filled, so it needs to be filled in
  // codec->FillRequest.
  bool from_http_service_proxy_{false};
};

/// @brief HTTP response protocol message.
class HttpResponseProtocol : public Protocol {
 public:
  ~HttpResponseProtocol() override = default;

  /// @brief Decode or encode o protocol message (zero copy).
  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override { return false; }
  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override { return false; }

  /// @brief Sets or Gets body payload of protocol message.
  /// @note For "GetBody" function, the value of body will be moved, cannot be called repeatedly.
  void SetNonContiguousProtocolBody(NoncontiguousBuffer&& buff) override {
    response.SetNonContiguousBufferContent(std::move(buff));
  }
  NoncontiguousBuffer GetNonContiguousProtocolBody() override {
    return std::move(*response.GetMutableNonContiguousBufferContent());
  }

  /// @brief Get size of message
  uint32_t GetMessageSize() const override;

 public:
  http::Response response;
};

namespace internal {
const std::string& GetHeaderString(const http::HeaderPairs& header, const std::string& name);
int GetHeaderInteger(const http::HeaderPairs& header, const std::string& name);
}  // namespace internal

}  // namespace trpc
