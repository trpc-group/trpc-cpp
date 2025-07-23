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
#include "trpc/util/http/response.h"

namespace trpc::http2 {

// @brief HTTP2 resposne.
class Response : public trpc::http::Response {
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

  void OnClose(int error_code) {}

  // @brief Gets or sets the sequence number of the response message.
  void SetContentSequenceId(uint32_t sequence_id) { GetMutableContentProvider()->SetSequenceId(sequence_id); }
  uint32_t GetContentSequenceId() const { return GetContentProvider().GetSequenceId(); }

  // @brief Gets or sets the stream identifier; generally, users will not use it, it is used for internal logic
  // request/response correlation of the framework.
  void SetStreamId(uint32_t stream_id) { stream_id_ = stream_id; }
  uint32_t GetStreamId() const { return stream_id_; }

  // @brief Converts response to string.
  std::string ToString() const;

  // @brief overloads `operator<<`
  friend std::ostream& operator<<(std::ostream& output, const Response& response);

 private:
  // Stream identifier to which the response belongs.
  uint32_t stream_id_{0};
};
using ResponsePtr = std::shared_ptr<Response>;

ResponsePtr CreateResponse();

}  // namespace trpc::http2
