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

#include "trpc/codec/http/http_stream_parser.h"

#include <optional>

#include "trpc/util/buffer/noncontiguous_buffer_view.h"
#include "trpc/util/http/common.h"
#include "trpc/util/http/util.h"
#include "trpc/util/log/logging.h"

#include "picohttpparser.h"

namespace trpc::stream {

int ParseHttpRequestLine(const NoncontiguousBuffer& in, HttpRequestLine* out) {
  TRPC_ASSERT(out && "Nullptr HttpRequestLine");
  if (in.ByteSize() < http::kMinRequestBytes) {  // less than the minimum length of an http request
    return kParseHttpNeedMore;
  }
  std::string buf = FlattenSlowUntil(in, "\n", in.ByteSize() + 1);
  const char* method = nullptr;
  size_t method_len = 0;
  const char* path = nullptr;
  size_t path_len = 0;
  int minor_version = 0;
  size_t num_headers = 0;
  int parsed_bytes = phr_parse_request(buf.data(), buf.length(), &method, &method_len, &path, &path_len, &minor_version,
                                       nullptr, &num_headers, 0);
  if (parsed_bytes == kParseHttpError || minor_version == -1) {
    return parsed_bytes;
  }
  out->method = std::string(method, method_len);
  out->request_uri = std::string(path, path_len);
  out->version = minor_version == 0 ? http::kVersion10 : http::kVersion11;
  return buf.length();
}

int ParseHttpStatusLine(const NoncontiguousBuffer& in, HttpStatusLine* out) {
  TRPC_ASSERT(out && "Nullptr HttpStatusLine");
  if (in.ByteSize() < http::kMinResponseBytes) {  // less than the minimum length of an http response
    return kParseHttpNeedMore;
  }
  std::string buf = FlattenSlowUntil(in, "\n", in.ByteSize() + 1);
  int minor_version = 0;
  int status = 200;
  const char* msg = nullptr;
  size_t msg_len;
  size_t num_headers = 0;
  int parsed_bytes =
      phr_parse_response(buf.data(), buf.length(), &minor_version, &status, &msg, &msg_len, nullptr, &num_headers, 0);
  if (parsed_bytes == kParseHttpError || msg == nullptr) {
    return parsed_bytes;
  }
  out->version = minor_version == 0 ? http::kVersion10 : http::kVersion11;
  out->status_code = status;
  out->status_text = std::string(msg, msg_len);
  return buf.length();
}

int ParseHttpHeader(const NoncontiguousBuffer& in, http::HttpHeader* out, HttpStreamHeader::MetaData* meta) {
  TRPC_ASSERT(out && "Nullptr HttpHeader");

  if (in.ByteSize() < http::kMinHeaderBytes) {  // less than the minimum length of http headers
    return kParseHttpNeedMore;
  }

  phr_header headers[http::kMaxHeaderNum];
  size_t num_headers = http::kMaxHeaderNum;

  std::string buf = FlattenSlowUntil(in, http::kEndOfHeaderMarker, in.ByteSize() + 1);
  int parsed_bytes = phr_parse_headers(buf.data(), buf.length(), headers, &num_headers, 0);
  // phr_parse_headers return -1 when failed, -2 when a request is incomplete
  if (parsed_bytes < 0) {
    return parsed_bytes;
  }
  for (size_t i = 0; i < num_headers; ++i) {
    const auto& header = headers[i];
    if (!meta->is_chunk && header.name_len == http::kHeaderTransferEncodingLen &&
        strncasecmp(header.name, http::kHeaderTransferEncoding, http::kHeaderTransferEncodingLen) == 0) {
      meta->is_chunk = true;
    } else if (!meta->content_length && header.name_len == http::kHeaderContentLengthLen &&
               strncasecmp(header.name, http::kHeaderContentLength, http::kHeaderContentLengthLen) == 0) {
      std::optional<size_t> content_length = http::ParseContentLength(header.value, header.value_len);
      if (!content_length) {
        TRPC_FMT_ERROR("Invalid Content-Length: {}", std::string(header.value, header.value_len));
        return kParseHttpError;
      }
      meta->content_length = content_length.value();
    } else if (header.name_len == http::kTrailerLen &&
               strncasecmp(header.name, http::kTrailer, http::kTrailerLen) == 0) {
      meta->has_trailer = true;
    }
    out->Add(std::string{header.name, header.name_len}, std::string{header.value, header.value_len});
  }
  return parsed_bytes;
}

int ParseHttpTrailer(const NoncontiguousBuffer& in, http::HttpHeader* out) {
  TRPC_ASSERT(out && "Nullptr HttpHeader");

  if (in.ByteSize() < 2) {
    return kParseHttpNeedMore;
  }

  phr_header headers[http::kMaxHeaderNum];
  size_t num_headers = http::kMaxHeaderNum;

  std::string buf = FlattenSlowUntil(in, http::kEndOfHeaderMarker, in.ByteSize() + 1);
  int parsed_bytes = phr_parse_headers(buf.data(), buf.length(), headers, &num_headers, 0);
  if (parsed_bytes < 0) {
    return parsed_bytes;
  }
  for (size_t i = 0; i < num_headers; ++i) {
    const auto& header = headers[i];
    out->Add(std::string{header.name, header.name_len}, std::string{header.value, header.value_len});
  }
  return parsed_bytes;
}

namespace {

enum {
  CHUNKED_IN_CHUNK_SIZE,
  CHUNKED_IN_CHUNK_EXT,
  CHUNKED_IN_CHUNK_DATA,
  CHUNKED_IN_CHUNK_CRLF,
};

static int DecodeHex(int ch) {
  if ('0' <= ch && ch <= '9') {
    return ch - '0';
  } else if ('A' <= ch && ch <= 'F') {
    return ch - 'A' + 0xa;
  } else if ('a' <= ch && ch <= 'f') {
    return ch - 'a' + 0xa;
  } else {
    return -1;
  }
}

}  // namespace

int ParseHttpChunk(NoncontiguousBuffer* in, NoncontiguousBuffer* out) {
  trpc::detail::NoncontiguousBufferView bytes(*in);
  auto buf_start = bytes.begin();
  auto buf_end = bytes.end();
  auto buf = bytes.begin();

  char state = CHUNKED_IN_CHUNK_SIZE;
  int hex_count = 0;
  size_t chunk_size = 0;
  size_t ext_size = 0;

  switch (state) {
    case CHUNKED_IN_CHUNK_SIZE:
      for (;; ++buf) {
        if (buf == buf_end) {
          return kParseHttpNeedMore;
        }
        int v;
        if ((v = DecodeHex(*buf)) == -1) {
          if (hex_count == 0) {
            return kParseHttpError;
          }
          break;
        }
        if (hex_count == sizeof(size_t) * 2) {
          return kParseHttpError;
        }
        chunk_size = chunk_size * 16 + v;
        ++hex_count;
      }
      hex_count = 0;
      state = CHUNKED_IN_CHUNK_EXT;
    // fallthru
    case CHUNKED_IN_CHUNK_EXT:
      // RFC 7230 A.2 "Line folding in chunk extensions is disallowed"
      for (;; ++buf) {
        if (buf == buf_end) {
          return kParseHttpNeedMore;
        }
        if (*buf == '\012') {
          break;
        }
      }
      ++buf;
      ext_size = buf - buf_start;
      if (chunk_size == 0) {
        in->Skip(ext_size);
        return ext_size;
      }
      state = CHUNKED_IN_CHUNK_DATA;
    // fallthru
    case CHUNKED_IN_CHUNK_DATA: {
      size_t avail = buf_end - buf;
      if (avail < chunk_size) {
        return kParseHttpNeedMore;
      }
      buf = buf + chunk_size;
      state = CHUNKED_IN_CHUNK_CRLF;
    }
    // fallthru
    case CHUNKED_IN_CHUNK_CRLF: {
      auto crlf_buf_begin = buf;
      for (;; ++buf) {
        if (buf == buf_end) {
          return kParseHttpNeedMore;
        }
        if (*buf != '\015') {
          break;
        }
      }
      if (*buf != '\012') {
        return kParseHttpError;
      }
      ++buf;
      in->Skip(ext_size);
      *out = in->Cut(chunk_size);
      in->Skip(buf - crlf_buf_begin);
      return buf - buf_start;
    }
    default:
      TRPC_ASSERT(false && "decoder is corrupt");
  }
  return kParseHttpError;
}

}  // namespace trpc::stream
