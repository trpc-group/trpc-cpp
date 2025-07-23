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

#pragma once

#include <memory>
#include <string>
#include <utility>

#include "trpc/codec/grpc/http2/http2.h"
#include "trpc/util/function.h"
#include "trpc/util/http/request.h"

namespace trpc::http2 {

class Response;
using ResponsePtr = std::shared_ptr<Response>;

// @brief HTTP2 request.
class Request : public trpc::http::Request {
 public:
  // @brief Response ready callback method.
  using OnResponseReadyCallback = Function<void(ResponsePtr&&)>;

 public:
  // @brief Processes chunked message content.
  //
  // @param data is the message buffer.
  // @param len is data length (if no more data, set len=0 to indicate EOF).
  void OnData(const uint8_t* data, std::size_t len);

  // @brief Reads the response message content (supports reading in chunks).
  //
  // @param data is message read buffer.
  // @param len is buffer length.
  //
  // @return >0: Successfully read byte count; 0: EOF; -1: error.
  ssize_t ReadContent(uint8_t* data, std::size_t len);

  // @brief Calls the response ready callback method.
  void OnResponseReady(ResponsePtr&& response);

  // @brief Sets URI.
  void SetUriRef(UriRef&& uri_ref) { uri_ref_ = std::move(uri_ref); }
  const UriRef& GetUriRef() const { return uri_ref_; }
  UriRef* GetMutableUriRef() { return &uri_ref_; }

  // @brief Gets or Sets scheme, e.g., http/https
  void SetScheme(std::string scheme) { *uri_ref_.MutableScheme() = std::move(scheme); }
  const std::string& GetScheme() const { return uri_ref_.Scheme(); }

  // @brief Gets or sets URL path, e.g, /index.html
  void SetPath(std::string path) { *uri_ref_.MutablePath() = std::move(path); }
  const std::string& GetPath() const { return uri_ref_.Path(); }

  // @brief Gets of sets authority.
  void SetAuthority(std::string authority) { *uri_ref_.MutableHost() = std::move(authority); }
  const std::string& GetAuthority() const { return uri_ref_.Host(); }

  // @brief Sets callback for response is ready.
  void SetOnResponseReadyCallback(OnResponseReadyCallback&& on_response_ready_cb) {
    on_response_ready_cb_ = std::move(on_response_ready_cb);
  }

  // @brief Gets or sets the sequence number of the request message.
  void SetContentSequenceId(uint32_t sequence_id) { GetMutableContentProvider()->SetSequenceId(sequence_id); }
  uint32_t GetContentSequenceId() const { return GetContentProvider().GetSequenceId(); }

  // @brief Gets or sets the stream identifier; generally, users will not use it, it is used for internal logic
  // request/response correlation of the framework.
  void SetStreamId(uint32_t stream_id) { stream_id_ = stream_id; }
  uint32_t GetStreamId() const { return stream_id_; }

  // @brief Converts request to string.
  std::string ToString() const;

  // @brief overloads `operator<<`
  friend std::ostream& operator<<(std::ostream& output, const Request& request);

  // @brief Gets or sets the flag indicating whether the HEADER frame has been received in full. HEADER information
  // can't be updated during the transmission of DATA frames; otherwise, it will cause read/write problems with
  // multiple threads in ServerContext. Always assume the HEADER frame has been completely received when the first
  // Data frame arrives.
  void SetHeaderRecv(bool is_header_recv) { is_header_recv_ = is_header_recv; }
  bool CheckHeaderRecv() { return is_header_recv_; }

 private:
  // Stream identifier to which the request belongs.
  uint32_t stream_id_{0};
  UriRef uri_ref_{};
  OnResponseReadyCallback on_response_ready_cb_{nullptr};
  // Flag indicating whether the HEADER frame has been received in full.
  bool is_header_recv_{false};
};
using RequestPtr = std::shared_ptr<Request>;

RequestPtr CreateRequest();

}  // namespace trpc::http2
