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

#include <optional>

#include "picohttpparser.h"

#include "trpc/transport/server/fiber/fiber_server_connection_handler_factory.h"
#include "trpc/transport/server/fiber/fiber_server_transport_impl.h"
#include "trpc/util/http/util.h"
#include "trpc/util/log/logging.h"

namespace trpc {

namespace {
// The internal class used to assist in parsing HttpRequest
struct InternalInflightHttpRequest {
  http::RequestPtr request;         // HttpRequest instance
  size_t expect_content_bytes = 0;  // expected incoming content bytes
  size_t max_body_size = 0;         // max content body size
  bool new_request = true;   // whether a new request? A request that has finished parsing the headers would be false.
  bool is_chunked = false;   // whether a chunked request?
  bool is_blocking = false;  // whether in the streaming blocking environment?
  phr_chunked_decoder chunk_decoder;  // chunked decoder
};
using InternalInflightHttpRequestPtr = std::shared_ptr<InternalInflightHttpRequest>;

constexpr int kParserNeedMore = -2;
constexpr int kParserError = -1;

/// @brief Parses the http request headers.
/// @return If parsing is successful, it returns the length of the successfully parsed bytes.
///         If the request is incomplete, it returns -2 (kParserNeedMore).
///         If parsing fails, it returns -1 (kParserError).
int ParseHeader(const ConnectionPtr& conn, InternalInflightHttpRequestPtr& inflight_request, NoncontiguousBuffer& in) {
  if (in.ByteSize() < http::kMinRequestBytes) {  // less than the minimum length required for http request
    return kParserNeedMore;
  }

  size_t max_packet_size =
      conn->GetMaxPacketSize() > 0 ? conn->GetMaxPacketSize() : (std::numeric_limits<size_t>::max() - 1);
  auto buf = FlattenSlowUntil(in, http::kEndOfHeaderMarker, max_packet_size + 1);
  if (buf.size() > max_packet_size) {  // the length of headers exceed the maximum allowed size
    return kParserError;
  }

  // 1: parse the headers
  const char* method = nullptr;
  size_t method_len = 0;
  const char* path = nullptr;
  size_t path_len = 0;
  int minor_version = 0;
  phr_header headers[http::kMaxHeaderNum];
  size_t num_headers = http::kMaxHeaderNum;

  int parsed_bytes = phr_parse_request(buf.c_str(), buf.size(), &method, &method_len, &path, &path_len, &minor_version,
                                       headers, &num_headers, 0);
  // phr_parse_request return -1 when failed, -2 when a request is incomplete
  if (parsed_bytes < 0) {
    return parsed_bytes;
  }

  // 2: convert phr_parse_request result to HttpRequest
  size_t queue_capacity = max_packet_size - parsed_bytes;
  auto req = std::make_shared<http::Request>(queue_capacity, inflight_request->is_blocking);
  std::optional<size_t> content_length;
  bool is_chunked = false;

  for (size_t i = 0; i < num_headers; i++) {
    const auto& header = headers[i];
    if (!is_chunked && header.name_len == http::kHeaderTransferEncodingLen &&
        strncasecmp(header.name, http::kHeaderTransferEncoding, http::kHeaderTransferEncodingLen) == 0) {
      is_chunked = true;
    } else if (!content_length && header.name_len == http::kHeaderContentLengthLen &&
               strncasecmp(header.name, http::kHeaderContentLength, http::kHeaderContentLengthLen) == 0) {
      content_length = http::ParseContentLength(header.value, header.value_len);
      if (!content_length ||  // invalid Content-Length value
          (!inflight_request->is_blocking && content_length.value() > queue_capacity)) {  // rpc request too large
        return kParserError;
      }
    }

    req->AddHeader(std::string{header.name, header.name_len}, std::string{header.value, header.value_len});
  }
  if (content_length && is_chunked) {  // chunked encoding must not have Content-Length
    return kParserError;
  }

  req->SetMethodType(http::StringToType(std::string_view{method, method_len}));
  req->SetUrl(std::string{path, path_len});
  req->SetVersion(minor_version == 0 ? http::kVersion10 : http::kVersion11);
  req->SetContentLength(content_length);

  // 3: setup inflight_request
  inflight_request->request = std::move(req);
  inflight_request->expect_content_bytes = content_length.value_or(0);
  inflight_request->max_body_size =
      is_chunked ? queue_capacity : std::min(queue_capacity, inflight_request->expect_content_bytes);
  inflight_request->request->SetMaxBodySize(inflight_request->max_body_size);
  inflight_request->new_request = true;
  inflight_request->is_chunked = is_chunked;
  inflight_request->chunk_decoder = {
      .bytes_left_in_chunk = 0,
      .consume_trailer = true,
  };

  return parsed_bytes;
}

// Checks whether the received data exceeds the maximum packet length limit
inline bool CheckBufferSize(const InternalInflightHttpRequestPtr& inflight_request, const NoncontiguousBuffer& in) {
  return inflight_request->request->GetStream().Size() + in.ByteSize() <= inflight_request->max_body_size;
}

// Appends the data into stream.
bool AppendBuffer(const ConnectionPtr& conn, InternalInflightHttpRequestPtr& inflight_request, NoncontiguousBuffer&& in,
                  bool fully_received) {
  bool success = false;
  auto& stream = inflight_request->request->GetStream();
  if (stream.IsBlocking() || CheckBufferSize(inflight_request, in)) {
    success = stream.Write(std::move(in));
    if (fully_received) {
      stream.WriteEof();
    }
  }
  return success;
}

// Checks whether push data to the 'out' queue?
bool ShouldEnqueue(const InternalInflightHttpRequestPtr& inflight_request, bool fully_received) {
  bool blocking = inflight_request->request->GetStream().IsBlocking();
  // If allow blocking wait, the new request need to be pushed.
  // If not allow blocking wait, only fully_received request can be pushed.
  return (blocking && inflight_request->new_request) || (!blocking && fully_received);
}

// Parses the http request body.
// If parsing is successful, it returns the length of the successfully parsed bytes.
// If parsing fails, it returns -1 (kParserError).
ssize_t ParseContent(const ConnectionPtr& conn, InternalInflightHttpRequestPtr& inflight_request,
                     NoncontiguousBuffer& in, std::deque<std::any>& out) {
  NoncontiguousBuffer buffer;
  bool fully_received = false;  // whether received completely
  size_t read_size = 0;
  if (!inflight_request->is_chunked) {
    read_size = std::min(inflight_request->expect_content_bytes, in.ByteSize());
    fully_received = (inflight_request->expect_content_bytes -= read_size) == 0;
    buffer = in.Cut(read_size);
  } else {
    phr_chunked_decoder& decoder = inflight_request->chunk_decoder;
    std::string in_buffer = FlattenSlow(in);
    size_t buf_size = in_buffer.size();
    ssize_t left = phr_decode_chunked(&decoder, in_buffer.data(), &buf_size);
    // phr_decode_chunked return -1 when failed, -2 when request is incomplete
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
  if (!AppendBuffer(conn, inflight_request, std::move(buffer), fully_received)) {
    return kParserError;
  } else if (ShouldEnqueue(inflight_request, fully_received)) {
    out.emplace_back(inflight_request->request);
  }
  inflight_request->new_request = fully_received;
  return read_size;
}

void SetConnCloseFunction(const ConnectionPtr& conn) {
  auto* handler = static_cast<FiberServerConnectionHandler*>(conn->GetConnectionHandler());
  auto& bind_info = handler->GetBindAdapter()->GetTransport()->GetBindInfo();
  if (TRPC_UNLIKELY(!bind_info.conn_close_function)) {
    // interrupt any blocking operation on connection close
    bind_info.conn_close_function = [](const Connection* closed_connection) {
      if (auto* inflight_ptr = std::any_cast<InternalInflightHttpRequestPtr>(
              &const_cast<trpc::Connection*>(closed_connection)->GetUserAny())) {
        if (auto inflight = std::move(*inflight_ptr)) {
          if (auto request = std::move(inflight->request)) {
            request->GetStream().Close(stream::HttpReadStream::ReadState::kErrorBit);
          }
        }
      }
    };
  }
}

// Parses http requests from the buffer.
// Parameter |is_blocking| used to indicate whether the operation is being performed in the streaming blocking
// environment.
// Return coded: true: means that the request can be pushed into the 'out' queue without waiting for the complete
// reception of the package, and then wait at the handle point.
// false: means that the request must be received completely before it can be pushed into the 'out'
// queue, and waiting at the handle is not allowed.
int ParseHttpRequest(const ConnectionPtr& conn, NoncontiguousBuffer& in, bool is_blocking, std::deque<std::any>& out) {
  InternalInflightHttpRequestPtr inflight_request;

  // use the self-define field of Connection to store InternalInflightHttpRequestPtr
  auto* p = std::any_cast<InternalInflightHttpRequestPtr>(&conn->GetUserAny());
  if (TRPC_UNLIKELY(!p)) {
    inflight_request = std::make_shared<InternalInflightHttpRequest>();
    conn->SetUserAny(inflight_request);

    if (is_blocking && conn->GetConnectionHandler()) {
      SetConnCloseFunction(conn);
    }
  } else {
    inflight_request = *p;
  }

  while (!in.Empty()) {
    // parse head
    if (inflight_request->new_request) {
      inflight_request->is_blocking = is_blocking;
      int parsed_bytes = ParseHeader(conn, inflight_request, in);
      if (parsed_bytes == kParserError) {
        return kPacketError;
      } else if (parsed_bytes == kParserNeedMore) {
        break;
      }
      in.Skip(parsed_bytes);
    }

    // parse body
    ssize_t parsed_bytes = ParseContent(conn, inflight_request, in, out);
    if (parsed_bytes == kParserError) {
      return kPacketError;
    } else if (inflight_request->new_request) {
      inflight_request->request.reset();
    }
  }

  return out.empty() ? kPacketLess : kPacketFull;
}

}  // namespace

int HttpZeroCopyCheckRequest(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  TRPC_ASSERT(conn && "HttpZeroCopyCheck(request), connection must not be nullptr");

  // parse http requests from the buffer
  return ParseHttpRequest(conn, in, fiber::detail::IsFiberContextPresent(), out);
}

}  // namespace trpc
