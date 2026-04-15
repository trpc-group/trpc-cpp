#pragma once

#include "trpc/common/status.h"
#include "trpc/server/server_context.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/http/sse/sse_event.h"

namespace trpc::stream {

/// @brief SSE (Server-Sent Events) stream writer for HTTP response.
class SseStreamWriter {
 public:
  explicit SseStreamWriter(ServerContext* ctx) : context_(ctx) {}
  ~SseStreamWriter() { Close(); }

  /// @brief Write the SSE response headers (Content-Type: text/event-stream, etc.)
  Status WriteHeader();

  /// @brief Send a structured SSE event.
  Status WriteEvent(const http::sse::SseEvent& ev);

  /// @brief Send a raw pre-serialized buffer as SSE data (wrapped as chunk).
  Status WriteBuffer(NoncontiguousBuffer&& buf);

  /// @brief Send end-of-chunk marker to finish the stream.
  Status WriteDone();

  /// @brief Close connection (best-effort WriteDone first).
  void Close();

  size_t Capacity() const;
  void SetCapacity(size_t capacity);

 private:
  enum StateFlags {
    kHeaderWritten = 1 << 0,
    kWriteDone     = 1 << 1,
  };

  ServerContext* context_{nullptr};
  uint32_t state_{0};
};

}  // namespace trpc::stream

