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

#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "trpc/client/client_context.h"
#include "trpc/stream/stream_provider.h"
#include "trpc/stream/http/http_stream_provider.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/util/http/http_header.h"
#include "trpc/common/status.h"

namespace trpc {

/// @brief HTTP SSE stream reader for reading Server-Sent Events from a streaming response.
class HttpSseStreamReader {
 public:
  using SseEventCallback = std::function<void(const trpc::http::sse::SseEvent&)>;

  /// @brief Constructor
  explicit HttpSseStreamReader(const stream::StreamReaderWriterProviderPtr& stream_provider);

  /// @brief Default constructor
  HttpSseStreamReader() = default;

  /// @brief Destructor
  ~HttpSseStreamReader() = default;

  /// @brief Move constructor
  HttpSseStreamReader(HttpSseStreamReader&& rhs) noexcept;

  /// @brief Move assignment operator
  HttpSseStreamReader& operator=(HttpSseStreamReader&& rhs) noexcept;

  /// @brief Reads HTTP response headers.
  /// @param code HTTP response code.
  /// @param http_header HTTP response headers.
  /// @return Status of the read operation. Returns OK on success.
  Status ReadHeaders(int& code, trpc::http::HttpHeader& http_header);

  /// @brief Reads HTTP response headers with timeout.
  /// @param code HTTP response code.
  /// @param http_header HTTP response headers.
  /// @param expiry Timeout time point.
  /// @return Status of the read operation. Returns OK on success.
  template <typename T>
  Status ReadHeaders(int& code, trpc::http::HttpHeader& http_header, const T& expiry);

  /// @brief Reads the next SSE event from the stream.
  /// @param event The SSE event to be filled with data.
  /// @param timeout Timeout in milliseconds. -1 means no timeout.
  /// @return Status of the read operation. Returns OK on success.
  ///         Returns StreamEof if the stream has ended.
  ///         Returns timeout error if the operation times out.
  ///         Returns other errors for network or parsing issues.
  Status Read(trpc::http::sse::SseEvent* event, int timeout = -1);

  /// @brief Reads raw data from the stream.
  /// @param data The buffer to be filled with raw data.
  /// @param max_size Maximum number of bytes to read.
  /// @param timeout Timeout in milliseconds. -1 means no timeout.
  /// @return Status of the read operation. Returns OK on success.
  ///         Returns StreamEof if the stream has ended.
  ///         Returns timeout error if the operation times out.
  Status ReadRaw(NoncontiguousBuffer* data, size_t max_size, int timeout = -1);

  /// @brief Reads all remaining data from the stream.
  /// @param data The buffer to be filled with all remaining data.
  /// @param timeout Timeout in milliseconds. -1 means no timeout.
  /// @return Status of the read operation. Returns OK on success.
  ///         Returns StreamEof if the stream has ended.
  ///         Returns timeout error if the operation times out.
  Status ReadAll(NoncontiguousBuffer* data, int timeout = -1);

  /// @brief Non-blocking streaming read for SSE events.
  /// This method starts a thread that continuously reads raw data and parses SSE events.
  /// @param callback Function to be called for each SSE event received.
  /// @param timeout_ms Timeout for each read operation in milliseconds.
  /// @return Status indicating success or failure to start the streaming loop.
  Status StartStreaming(SseEventCallback callback, int timeout_ms = 30000);

  /// @brief Finishes the stream and returns the final RPC execution result.
  /// @return Status of the finish operation.
  Status Finish();

  /// @brief Returns the inner status of the stream.
  /// @return Status of the stream.
  Status GetStatus() const;

  /// @brief Checks if the stream is valid.
  /// @return true if the stream is valid, false otherwise.
  bool IsValid() const { return stream_provider_ != nullptr; }

 private:
  stream::StreamReaderWriterProviderPtr stream_provider_;
};

}  // namespace trpc