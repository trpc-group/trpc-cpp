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

#include "trpc/codec/grpc/http2/request.h"

#include <utility>

namespace trpc::http2 {

void Request::OnData(const uint8_t* data, std::size_t len) { GetMutableContentProvider()->Write(data, len); }

ssize_t Request::ReadContent(uint8_t* data, std::size_t len) { return GetMutableContentProvider()->Read(data, len); }

void Request::OnResponseReady(ResponsePtr&& response) {
  if (on_response_ready_cb_) {
    on_response_ready_cb_(std::move(response));
  }
}

std::string Request::ToString() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::ostream& operator<<(std::ostream& output, const Request& request) {
  // Request line.
  output << request.GetMethod() << " " << request.GetUriRef().RequestUri() << " "
         << "HTTP/2.0\r\n";
  // Request header.
  output << request.GetHeader().ToString();
  output << "\r\n";
  // Request content.
  output << request.GetContentProvider().SerializeToString();
  return output;
}

RequestPtr CreateRequest() { return std::make_shared<Request>(); }

}  // namespace trpc::http2
