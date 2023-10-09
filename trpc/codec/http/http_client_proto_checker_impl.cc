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

#include "trpc/codec/http/http_proto_checker.h"

#include <algorithm>
#include <any>
#include <memory>
#include <optional>
#include <utility>

#include "picohttpparser.h"

#include "trpc/util/http/response.h"
#include "trpc/util/http/stream/http_client_stream_response.h"

namespace trpc {

namespace {
struct InflightHttpResponse {
  http::HttpResponse response{};
  size_t expect_content_bytes{0};
  size_t header_content_bytes{0};
  size_t max_packet_size{0};
  bool new_request{true};
  bool is_chunked{false};
  phr_chunked_decoder chunk_decoder{};
};
using InflightHttpResponsePtr = std::shared_ptr<InflightHttpResponse>;

constexpr int kParserNeedMore{-2};
constexpr int kParserError{-1};

std::optional<size_t> ParseContentLength(const char* value, size_t len) {
  char* end_ptr;
  ssize_t content_length = std::strtoll(value, &end_ptr, 10);
  return len != 0 && content_length >= 0 && static_cast<size_t>(end_ptr - value) == len
             ? std::make_optional(content_length)
             : std::nullopt;
}

// Parses the http response headers.
// If parsing is successful, it returns the length of the successfully parsed bytes.
// If the response is incomplete, it returns -2 (kParserNeedMore).
// If parsing fails, it returns -1 (kParserError).
int ParseHeader(const ConnectionPtr& conn, InflightHttpResponsePtr& inflight_response, NoncontiguousBuffer& in) {
  if (in.ByteSize() < http::kMinResponseBytes) {
    return kParserNeedMore;
  }

  int minor_version = 0;
  int status = 200;
  const char* msg = nullptr;
  size_t msg_len;
  struct phr_header headers[http::kMaxHeaderNum];
  size_t num_headers = http::kMaxHeaderNum;

  size_t max_packet_size =
      conn->GetMaxPacketSize() > 0 ? conn->GetMaxPacketSize() : (std::numeric_limits<size_t>::max() - 1);
  auto buf = FlattenSlowUntil(in, http::kEndOfHeaderMarker, max_packet_size + 1);
  if (buf.size() > max_packet_size) {
    return kParserError;
  }

  int nparse =
      phr_parse_response(buf.c_str(), buf.size(), &minor_version, &status, &msg, &msg_len, headers, &num_headers, 0);

  // phr_parse_request return -1 when failed, -2 when a request is incomplete
  if (nparse < 0) {
    return nparse;
  }

  http::HttpResponse rsp;

  // HTTP headers.
  std::optional<size_t> content_length;
  bool conn_reusable = minor_version > 0;
  bool is_chunked = false;
  for (size_t i = 0; i < num_headers; i++) {
    const auto& header = headers[i];
    if (!is_chunked && header.name_len == http::kHeaderTransferEncodingLen &&
        strncasecmp(header.name, http::kHeaderTransferEncoding, http::kHeaderTransferEncodingLen) == 0) {
      is_chunked = true;
    } else if (!content_length && header.name_len == http::kHeaderContentLengthLen &&
               strncasecmp(header.name, http::kHeaderContentLength, http::kHeaderContentLengthLen) == 0) {
      content_length = ParseContentLength(header.value, header.value_len);
      if (!content_length) {  // invalid Content-Length value
        return kParserError;
      }
    } else if (header.name_len == http::kHeaderConnectionLen &&
               strncasecmp(header.name, http::kHeaderConnection, http::kHeaderConnectionLen) == 0) {
      conn_reusable = header.value_len != http::kConnectionCloseLen ||
                      strncasecmp(header.value, http::kConnectionClose, http::kConnectionCloseLen) != 0;
    }

    rsp.AddHeader(std::string{header.name, header.name_len}, std::string{header.value, header.value_len});
  }
  if (content_length && is_chunked) {  // chunked encoding must not have Content-Length
    return kParserError;
  }

  // Status line.
  rsp.SetVersion(minor_version == 0 ? http::kVersion10 : http::kVersion11);
  rsp.SetStatus(http::HttpResponse::StatusCode(status));
  rsp.SetReasonPhrase(std::string{msg, msg_len});
  rsp.SetConnectionReusable(conn_reusable);

  inflight_response->response = std::move(rsp);
  inflight_response->expect_content_bytes = content_length.value_or(0);
  inflight_response->header_content_bytes = nparse;
  inflight_response->max_packet_size = max_packet_size;
  inflight_response->new_request = true;
  inflight_response->is_chunked = is_chunked;
  inflight_response->chunk_decoder = {
      .bytes_left_in_chunk = 0,
      .consume_trailer = true,
  };

  return nparse;
}

// Checks whether the received data exceeds the maximum packet length limit
bool CheckBufferSize(const InflightHttpResponsePtr& inflight_response, const NoncontiguousBuffer& in) {
  return inflight_response->header_content_bytes +
             inflight_response->response.GetMutableNonContiguousBufferContent()->ByteSize() + in.ByteSize() <=
         inflight_response->max_packet_size;
}

// Appends the data into stream.
bool AppendBuffer(const ConnectionPtr& conn, NoncontiguousBuffer&& in, bool fully_received) {
  auto p = std::any_cast<stream::HttpClientStreamResponsePtr>(&(conn->GetUserAny()));
  auto& stream = (*p)->GetStream();
  TRPC_ASSERT(stream);

  stream->PushDataToRecvQueue(std::move(in));
  if (fully_received) {
    stream->PushEofToRecvQueue();
  }
  return true;
}

// Parses the http request body.
// If parsing is successful, it returns the length of the successfully parsed bytes.
// If parsing fails, it returns -1 (kParserError).
ssize_t ParseContent(const ConnectionPtr& conn, InflightHttpResponsePtr& inflight_response, NoncontiguousBuffer& in,
                     std::deque<std::any>* out) {
  NoncontiguousBuffer buffer;
  bool fully_received;
  size_t read_size;
  if (!inflight_response->is_chunked) {
    read_size = std::min(inflight_response->expect_content_bytes, in.ByteSize());
    fully_received = (inflight_response->expect_content_bytes -= read_size) == 0;
    buffer = in.Cut(read_size);
  } else {
    phr_chunked_decoder& decoder = inflight_response->chunk_decoder;
    std::string in_buffer = FlattenSlow(in);
    size_t buf_size = in_buffer.size();
    ssize_t left = phr_decode_chunked(&decoder, in_buffer.data(), &buf_size);
    if (left == kParserError) {
      return left;
    }
    fully_received = left != kParserNeedMore;
    read_size = in_buffer.size() - std::max(left, 0l);
    in.Skip(read_size);
    NoncontiguousBufferBuilder builder;
    builder.Append(in_buffer.c_str(), buf_size);
    buffer = builder.DestructiveGet();
  }

  auto p = std::any_cast<stream::HttpClientStreamResponsePtr>(&(conn->GetUserAny()));
  if (p && ((*p)->GetStream())) { // Streaming content.
    AppendBuffer(conn, std::move(buffer), fully_received);
    if (inflight_response->new_request) {  // Response header is coming.
      out->emplace_back(std::move(inflight_response->response));
    }
    inflight_response->new_request = fully_received;
  } else {  // Non-Streaming content.
    if (!CheckBufferSize(inflight_response, buffer)) {
      return kParserError;
    }
    inflight_response->response.GetMutableNonContiguousBufferContent()->Append(std::move(buffer));
    inflight_response->new_request = fully_received;

    if (!fully_received) {
      return kParserNeedMore;
    }
    out->emplace_back(std::move(inflight_response->response));
  }

  return read_size;
}

// Parses http responses from the buffer.
int ParseHttpResponse(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>* out) {
  auto p = std::any_cast<stream::HttpClientStreamResponsePtr>(&(conn->GetUserAny()));
  bool is_stream = p && ((*p)->GetStream());
  auto& data = is_stream ? (*p)->GetData() : conn->GetUserAny();
  if (TRPC_UNLIKELY(data.type() != typeid(InflightHttpResponsePtr))) {
    data = std::make_shared<InflightHttpResponse>();
  }

  auto& inflight_response = std::any_cast<InflightHttpResponsePtr&>(data);
  // Parse header.
  if (inflight_response->new_request) {
    int nparse = ParseHeader(conn, inflight_response, in);
    if (nparse == kParserError) {
      return PacketChecker::PACKET_ERR;
    } else if (nparse == kParserNeedMore) {
      return PacketChecker::PACKET_LESS;
    }
    in.Skip(nparse);
  }

  // Body is not required when request method is HEAD or PATCH.
  // FIXME: Chunk packets need to be handled separately in the subsequent pipeline mode.
  if (conn->GetConnectionHandler()->GetCurrentContextExt() == http::OperationType::HEAD) {
    inflight_response->expect_content_bytes = 0;
    inflight_response->is_chunked = false;
  }

  // Parse content.
  ssize_t nparse = ParseContent(conn, inflight_response, in, out);
  if (nparse == kParserNeedMore) {
    return PacketChecker::PACKET_LESS;
  }

  if (nparse == kParserError) {
    return PacketChecker::PACKET_ERR;
  }

  return PacketChecker::PACKET_FULL;
}

}  // namespace

int HttpZeroCopyCheckResponse(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  TRPC_ASSERT(conn && "HttpZeroCopyCheck(response), connection must not be nullptr");

  // Try to parse responses from the buffer.
  return ParseHttpResponse(conn, in, &out);
}

}  // namespace trpc
