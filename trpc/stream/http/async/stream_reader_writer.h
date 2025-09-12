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

#include "trpc/codec/http/http_stream_frame.h"
#include "trpc/stream/http/async/client/stream.h"
#include "trpc/stream/http/async/server/stream.h"
#include "trpc/util/http/common.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::stream {

/// @brief Asynchronous HTTP streaming reader.
class HttpAsyncStreamReader : public RefCounted<HttpAsyncStreamReader> {
 public:
  explicit HttpAsyncStreamReader(HttpAsyncStreamPtr stream);

  virtual ~HttpAsyncStreamReader() = default;

  /// @brief Reads Header
  /// @param timeout time to wait for the header to be ready
  Future<http::HttpHeader> ReadHeader(uint32_t timeout = std::numeric_limits<uint32_t>::max());

  /// @brief Reads a chunk in chunked mode, note that reading in non-chunked mode will fail
  /// @param timeout time to wait for the header to be ready
  Future<NoncontiguousBuffer> ReadChunk(uint32_t timeout = std::numeric_limits<uint32_t>::max());

  /// @brief Reads at most len data.
  /// @param len max size to read
  /// @param timeout time to wait for the header to be ready
  /// @note If the size of the data obtained from the network is smaller than len, it will return size length data.
  ///       If the size of the data obtained from the network is greater than len, it will return len length data.
  ///       An empty buffer means that the end has been read
  ///       Usage scenario 1: Limits the maximum length of each read When the memory is limited.
  ///       Usage scenario 2: Gets part of data in time and send it downstream on route server.
  Future<NoncontiguousBuffer> ReadAtMost(uint64_t len, uint32_t timeout = std::numeric_limits<uint32_t>::max());

  /// @brief Reads data with a fixed length. If eof is read, it will return as much data as there is in the network
  /// @param len size to read
  /// @param timeout time to wait for the header to be ready
  /// @note If the read buffer size is less than the required length, it means that eof has been read.
  ///       Usage scenario 1: The requested data is compressed by a fixed size, and needs to be read and decompressed by
  ///       a fixed size.
  Future<NoncontiguousBuffer> ReadExactly(uint64_t len, uint32_t timeout = std::numeric_limits<uint32_t>::max());

 private:
  HttpAsyncStreamPtr stream_{nullptr};
};
using HttpAsyncStreamReaderPtr = RefPtr<HttpAsyncStreamReader>;

/// @brief Asynchronous HTTP server streaming reader.
class HttpServerAsyncStreamReader : public HttpAsyncStreamReader {
 public:
  explicit HttpServerAsyncStreamReader(HttpServerAsyncStreamPtr stream);

  /// @brief Gets the request line
  const HttpRequestLine& GetRequestLine();

  /// @brief Gets the path parameters (Placeholder in URL path).
  const http::PathParameters& GetParameters() const { return stream_->GetParameters(); }
  http::PathParameters* GetMutableParameters() { return stream_->GetMutableParameters(); }

 private:
  HttpServerAsyncStreamPtr stream_{nullptr};
};
using HttpServerAsyncStreamReaderPtr = RefPtr<HttpServerAsyncStreamReader>;

/// @brief Asynchronous HTTP client streaming reader.
class HttpClientAsyncStreamReader : public HttpAsyncStreamReader {
 public:
  explicit HttpClientAsyncStreamReader(HttpClientAsyncStreamPtr stream)
      : HttpAsyncStreamReader(stream), stream_(stream) {
    TRPC_ASSERT(stream_ && "HttpClientAsyncStreamPtr can't be nullptr");
  }

  /// @brief Reads the status line of response.
  Future<HttpStatusLine> ReadStatusLine(uint32_t timeout = std::numeric_limits<uint32_t>::max()) {
    return stream_->ReadStatusLine(timeout);
  }

 private:
  HttpClientAsyncStreamPtr stream_{nullptr};
};
using HttpClientAsyncStreamReaderPtr = RefPtr<HttpClientAsyncStreamReader>;

/// @brief Asynchronous HTTP streaming writer.
class HttpAsyncStreamWriter : public RefCounted<HttpAsyncStreamWriter> {
 public:
  explicit HttpAsyncStreamWriter(HttpAsyncStreamPtr stream);

  virtual ~HttpAsyncStreamWriter() = default;

  /// @brief Writes header or trailer
  Future<> WriteHeader(http::HttpHeader&& header, bool is_trailer = false);

  /// @brief Writes data
  Future<> WriteData(NoncontiguousBuffer&& data);

  /// @brief Notifies that the data has been written completely.
  Future<> WriteDone();

 private:
  HttpAsyncStreamPtr stream_{nullptr};
};
using HttpAsyncStreamWriterPtr = RefPtr<HttpAsyncStreamWriter>;

/// @brief Asynchronous HTTP server streaming writer.
class HttpServerAsyncStreamWriter : public HttpAsyncStreamWriter {
 public:
  explicit HttpServerAsyncStreamWriter(HttpServerAsyncStreamPtr stream);

  /// @brief Writes status line
  Future<> WriteStatusLine(HttpStatusLine&& rsp_line);

 private:
  HttpServerAsyncStreamPtr stream_{nullptr};
};
using HttpServerAsyncStreamWriterPtr = RefPtr<HttpServerAsyncStreamWriter>;

/// @brief Asynchronous HTTP client streaming writer.
class HttpClientAsyncStreamWriter : public HttpAsyncStreamWriter {
 public:
  explicit HttpClientAsyncStreamWriter(HttpClientAsyncStreamPtr stream)
      : HttpAsyncStreamWriter(stream), stream_(stream) {
    TRPC_ASSERT(stream_ && "HttpClientAsyncStreamPtr can't be nullptr");
  }

  /// @brief Writes request line of request.
  Future<> WriteRequestLine(HttpRequestLine&& req_line) {
    auto frame = MakeRefCounted<HttpStreamRequestLine>();
    *frame->GetMutableHttpRequestLine() = std::move(req_line);
    return stream_->PushSendMessage(std::move(frame));
  }

 private:
  HttpClientAsyncStreamPtr stream_{nullptr};
};
using HttpClientAsyncStreamWriterPtr = RefPtr<HttpClientAsyncStreamWriter>;

/// @brief Asynchronous HTTP streaming reader-writer. It uses HttpAsyncStreamReader and HttpAsyncStreamWriter
///        internally to perform the actual logic.
class HttpAsyncStreamReaderWriter : public RefCounted<HttpAsyncStreamReaderWriter> {
 public:
  explicit HttpAsyncStreamReaderWriter(HttpAsyncStreamPtr stream);

  virtual ~HttpAsyncStreamReaderWriter() = default;

  /// @brief Reads Header
  /// @param timeout time to wait for the header to be ready
  Future<http::HttpHeader> ReadHeader(uint32_t timeout = std::numeric_limits<uint32_t>::max());

  /// @brief Reads a chunk in chunked mode, note that reading in non-chunked mode will fail
  /// @param timeout time to wait for the header to be ready
  Future<NoncontiguousBuffer> ReadChunk(uint32_t timeout = std::numeric_limits<uint32_t>::max());

  /// @brief Reads at most len data.
  /// @param len max size to read
  /// @param timeout time to wait for the header to be ready
  /// @note If the size of the data obtained from the network is smaller than len, it will return size length data.
  ///       If the size of the data obtained from the network is greater than len, it will return len length data.
  ///       An empty buffer means that the end has been read
  ///       Usage scenario 1: Limits the maximum length of each read When the memory is limited.
  ///       Usage scenario 2: Gets part of data in time and send it downstream on route server.
  Future<NoncontiguousBuffer> ReadAtMost(uint64_t len, uint32_t timeout = std::numeric_limits<uint32_t>::max());

  /// @brief Reads data with a fixed length. If eof is read, it will return as much data as there is in the network
  /// @param len size to read
  /// @param timeout time to wait for the header to be ready
  /// @note If the read buffer size is less than the required length, it means that eof has been read.
  ///       Usage scenario 1: The requested data is compressed by a fixed size, and needs to be read and decompressed by
  ///       a fixed size.
  Future<NoncontiguousBuffer> ReadExactly(uint64_t len, uint32_t timeout = std::numeric_limits<uint32_t>::max());

  /// @brief Writes header or trailer
  Future<> WriteHeader(http::HttpHeader&& header);

  /// @brief Writes data
  /// @note Writing empty data is not allowed. In chunked mode, empty data represents the end of the chunked data, so
  ///       WriteDone should be explicitly called to send it.
  Future<> WriteData(NoncontiguousBuffer&& data);

  /// @brief Notifies that the data has been written completely.
  Future<> WriteDone();

 protected:
  /// @brief Sets the reader and writer
  void SetReaderWriter(HttpAsyncStreamReaderPtr reader, HttpAsyncStreamWriterPtr writer);

 private:
  HttpAsyncStreamPtr stream_;
  HttpAsyncStreamReaderPtr reader_;
  HttpAsyncStreamWriterPtr writer_;
};

/// @brief Asynchronous server streaming reader-writer.
class HttpServerAsyncStreamReaderWriter : public HttpAsyncStreamReaderWriter {
 public:
  explicit HttpServerAsyncStreamReaderWriter(HttpServerAsyncStreamPtr stream);

  /// @brief Gets the request line
  const HttpRequestLine& GetRequestLine();

  /// @brief Gets the path parameters (Placeholder in URL path).
  const http::PathParameters& GetParameters() const { return reader_->GetParameters(); }
  http::PathParameters* GetMutableParameters() { return reader_->GetMutableParameters(); }

  /// @brief Writes status line
  Future<> WriteStatusLine(HttpStatusLine&& rsp_line);

  /// @brief Gets the stream reader
  HttpServerAsyncStreamReaderPtr GetReader() { return reader_; }

  /// @brief Gets the stream writer
  HttpServerAsyncStreamWriterPtr GetWriter() { return writer_; }

 private:
  HttpServerAsyncStreamReaderPtr reader_;
  HttpServerAsyncStreamWriterPtr writer_;
};
using HttpServerAsyncStreamReaderWriterPtr = RefPtr<HttpServerAsyncStreamReaderWriter>;

/// @brief Asynchronous client streaming reader-writer.
class HttpClientAsyncStreamReaderWriter : public HttpAsyncStreamReaderWriter {
 public:
  explicit HttpClientAsyncStreamReaderWriter(HttpClientAsyncStreamPtr stream)
      : HttpAsyncStreamReaderWriter(stream),
        stream_(stream),
        reader_(MakeRefCounted<HttpClientAsyncStreamReader>(stream)),
        writer_(MakeRefCounted<HttpClientAsyncStreamWriter>(stream)) {
    SetReaderWriter(reader_, writer_);
  }

  /// @brief Writes request line of request.
  Future<> WriteRequestLine(HttpRequestLine&& req_line) { return writer_->WriteRequestLine(std::move(req_line)); }

  /// @brief Reads status line of response.
  Future<HttpStatusLine> ReadStatusLine(uint32_t timeout = std::numeric_limits<uint32_t>::max()) {
    return reader_->ReadStatusLine(timeout);
  }

  /// @brief Returns the stream reader.
  HttpClientAsyncStreamReaderPtr GetReader() { return reader_; }

  /// @brief Returns the stream writer.
  HttpClientAsyncStreamWriterPtr GetWriter() { return writer_; }

  /// @brief Finishes stream.
  void Finish() { stream_->Finish(); }

 private:
  HttpClientAsyncStreamPtr stream_;
  HttpClientAsyncStreamReaderPtr reader_;
  HttpClientAsyncStreamWriterPtr writer_;
};
using HttpClientAsyncStreamReaderWriterPtr = RefPtr<HttpClientAsyncStreamReaderWriter>;

/// Provides easy-to-use utility functions for programming.

/// @brief Writes out a complete request to the stream.
Future<> WriteFullRequest(HttpClientAsyncStreamReaderWriterPtr rw, trpc::http::HttpRequest&& req);
Future<> WriteFullRequest(HttpClientAsyncStreamWriterPtr rw, trpc::http::HttpRequest&& req);

/// @brief Writes out a complete response to the stream.
Future<> WriteFullResponse(HttpServerAsyncStreamReaderWriterPtr rw, http::HttpResponse&& rsp);
Future<> WriteFullResponse(HttpServerAsyncStreamWriterPtr rw, http::HttpResponse&& rsp);

/// @brief Reads a complete request from the stream.
Future<http::HttpRequestPtr> ReadFullRequest(HttpServerAsyncStreamReaderWriterPtr rw,
                                             uint32_t timeout = std::numeric_limits<uint32_t>::max());
Future<http::HttpRequestPtr> ReadFullRequest(HttpServerAsyncStreamReaderPtr rw,
                                             uint32_t timeout = std::numeric_limits<uint32_t>::max());

/// @brief Reads a complete response from the stream.
Future<trpc::http::HttpResponsePtr> ReadFullResponse(HttpClientAsyncStreamReaderWriterPtr rw,
                                                     uint32_t timeout = std::numeric_limits<uint32_t>::max());
Future<trpc::http::HttpResponsePtr> ReadFullResponse(HttpClientAsyncStreamReaderPtr rw,
                                                     uint32_t timeout = std::numeric_limits<uint32_t>::max());

}  // namespace trpc::stream
