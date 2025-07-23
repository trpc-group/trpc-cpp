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

#include "trpc/codec/grpc/http2/response.h"

#include <utility>

namespace trpc::http2 {

void Response::OnData(const uint8_t* data, std::size_t len) { GetMutableContentProvider()->Write(data, len); }

ssize_t Response::ReadContent(uint8_t* data, std::size_t len) { return GetMutableContentProvider()->Read(data, len); }

std::string Response::ToString() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::ostream& operator<<(std::ostream& output, const Response& response) {
  // Status line.
  output << ":status: " << response.GetStatus() << "\r\n";
  // Response headers.
  output << response.GetHeader().ToString() << "\r\n";
  output << "\r\n";
  // Response content.
  output << response.GetContentProvider().SerializeToString();
  // Response trailers.
  if (!response.GetTrailer().Empty()) {
    output << "\r\n\r\n";
  }
  return output;
}

ResponsePtr CreateResponse() { return std::make_shared<Response>(); }
}  // namespace trpc::http2
