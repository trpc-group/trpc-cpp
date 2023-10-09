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

#include <deque>
#include <memory>

#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/http/common.h"
#include "trpc/util/http/http_header.h"
#include "trpc/util/http/stream/http_start_line.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::stream {

/// @brief Base class for HTTP stream frames.
class HttpStreamFrame : public RefCounted<HttpStreamFrame> {
 public:
  /// @brief Types of HTTP stream frames.
  enum class HttpStreamFrameType {
    kUnknown = 0,
    kRequestLine = 1,
    kStatusLine = 2,
    kHeader = 3,
    kData = 4,
    kEof = 5,
    kTrailer = 6,
    kFullMessage = 7,
  };

  virtual HttpStreamFrameType GetFrameType() { return HttpStreamFrameType::kUnknown; }
  virtual ~HttpStreamFrame() = default;
};

using HttpStreamFramePtr = RefPtr<HttpStreamFrame>;

/// @brief Request line stream frame.
class HttpStreamRequestLine : public HttpStreamFrame {
 public:
  HttpStreamFrameType GetFrameType() override { return HttpStreamFrameType::kRequestLine; }

  HttpRequestLine* GetMutableHttpRequestLine() { return &start_line_; }

 private:
  HttpRequestLine start_line_;
};

/// @brief Status line stream frame.
class HttpStreamStatusLine : public HttpStreamFrame {
 public:
  HttpStreamFrameType GetFrameType() override { return HttpStreamFrameType::kStatusLine; }

  HttpStatusLine* GetMutableHttpStatusLine() { return &status_line_; }

 private:
  HttpStatusLine status_line_;
};

/// @brief Header stream frame.
class HttpStreamHeader : public HttpStreamFrame {
 public:
  struct MetaData {
    size_t content_length{0};
    bool is_chunk{false};
    bool has_trailer{false};
  };

  HttpStreamFrameType GetFrameType() override { return HttpStreamFrameType::kHeader; }

  http::HttpHeader* GetMutableHeader() { return &header_; }

  MetaData* GetMutableMetaData() { return &meta_; }

 private:
  http::HttpHeader header_;
  MetaData meta_;
};

/// @brief Data stream frame.
class HttpStreamData : public HttpStreamFrame {
 public:
  HttpStreamFrameType GetFrameType() override { return HttpStreamFrameType::kData; }

  NoncontiguousBuffer* GetMutableData() { return &buffer_; }

 private:
  NoncontiguousBuffer buffer_;
};

/// @brief EOF stream frame.
class HttpStreamEof : public HttpStreamFrame {
 public:
  HttpStreamEof() = default;

  HttpStreamFrameType GetFrameType() override { return HttpStreamFrameType::kEof; }
};

/// @brief Trailer stream frame.
class HttpStreamTrailer : public HttpStreamFrame {
 public:
  HttpStreamFrameType GetFrameType() override { return HttpStreamFrameType::kTrailer; }

  http::HttpHeader* GetMutableHeader() { return &header_; }

 private:
  http::HttpHeader header_;
};

/// @brief Full message stream frame. When a single check operation can check out the complete message, submit this
///        stream frame to the stream to avoid unnecessary asynchronous waiting processes and improve performance.
class HttpStreamFullMessage : public HttpStreamFrame {
 public:
  HttpStreamFrameType GetFrameType() override { return HttpStreamFrameType::kFullMessage; }

  std::deque<std::any>* GetMutableFrames() { return &frames_; }

 private:
  std::deque<std::any> frames_;
};

}  // namespace trpc::stream
