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

#include "trpc/client/sse/http_sse_stream_reader.h"

#include <utility>

#include "trpc/stream/http/http_client_stream.h"
#include "trpc/util/http/sse/sse_parser.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string_helper.h"

namespace trpc {

HttpSseStreamReader::HttpSseStreamReader(const stream::StreamReaderWriterProviderPtr& stream_provider)
    : stream_provider_(stream_provider) {}

HttpSseStreamReader::HttpSseStreamReader(HttpSseStreamReader&& rhs) noexcept
    : stream_provider_(std::move(rhs.stream_provider_)) {}

HttpSseStreamReader& HttpSseStreamReader::operator=(HttpSseStreamReader&& rhs) noexcept {
  if (this != &rhs) {
    stream_provider_ = std::move(rhs.stream_provider_);
  }
  return *this;
}

Status HttpSseStreamReader::ReadHeaders(int& code, trpc::http::HttpHeader& http_header) {
  if (!stream_provider_) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream reader is not valid"};
  }

  // Try to cast to HttpClientStream to access ReadHeaders
  stream::HttpClientStreamPtr http_stream = dynamic_pointer_cast<stream::HttpClientStream>(stream_provider_);
  if (!http_stream) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream provider is not HTTP client stream"};
  }

  // Make sure the HTTP request is sent before trying to read headers
  // This is critical for SSE streams
  // Configure SSE mode first
  Status sse_status = http_stream->ConfigureSseMode();
  if (!sse_status.OK()) {
    TRPC_FMT_ERROR("Failed to configure SSE mode: {}", sse_status.ToString());
    return sse_status;
  }
  
  // Send the HTTP request header
  Status send_status = http_stream->SendRequestHeader();
  if (!send_status.OK()) {
    TRPC_FMT_ERROR("Failed to send HTTP request header: {}", send_status.ToString());
    return send_status;
  }

  // Call ReadHeaders on the HTTP client stream
  return http_stream->ReadHeaders(code, http_header);
}

template <typename T>
Status HttpSseStreamReader::ReadHeaders(int& code, trpc::http::HttpHeader& http_header, const T& expiry) {
  if (!stream_provider_) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream reader is not valid"};
  }

  // Try to cast to HttpClientStream to access ReadHeaders
  stream::HttpClientStreamPtr http_stream = dynamic_pointer_cast<stream::HttpClientStream>(stream_provider_);
  if (!http_stream) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream provider is not HTTP client stream"};
  }

  // Make sure the HTTP request is sent before trying to read headers
  // This is critical for SSE streams
  // Configure SSE mode first
  Status sse_status = http_stream->ConfigureSseMode();
  if (!sse_status.OK()) {
    TRPC_FMT_ERROR("Failed to configure SSE mode: {}", sse_status.ToString());
    return sse_status;
  }
  
  // Send the HTTP request header
  Status send_status = http_stream->SendRequestHeader();
  if (!send_status.OK()) {
    TRPC_FMT_ERROR("Failed to send HTTP request header: {}", send_status.ToString());
    return send_status;
  }

  // Use the fiber-friendly ReadHeadersNonBlocking method
  return http_stream->ReadHeadersNonBlocking(code, http_header, expiry);
}

// Explicit template instantiation for common types
template Status HttpSseStreamReader::ReadHeaders<std::chrono::milliseconds>(int&, trpc::http::HttpHeader&, const std::chrono::milliseconds&);
template Status HttpSseStreamReader::ReadHeaders<std::chrono::steady_clock::time_point>(int&, trpc::http::HttpHeader&, const std::chrono::steady_clock::time_point&);

Status HttpSseStreamReader::Read(trpc::http::sse::SseEvent* event, int timeout) {
  if (!stream_provider_) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream reader is not valid"};
  }

  if (!event) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "event parameter is null"};
  }

  // Read raw data from the stream
  NoncontiguousBuffer data;
  Status status = stream_provider_->Read(&data, timeout);
  if (!status.OK()) {
    return status;
  }

  // Convert buffer to string
  std::string content = FlattenSlow(data);
  
  // Parse SSE event from the content
  try {
    *event = http::sse::SseParser::ParseEvent(content);
    return kDefaultStatus;
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to parse SSE event: " << e.what() << ", content: " << content);
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, std::string("Failed to parse SSE event: ") + e.what()};
  }
}

Status HttpSseStreamReader::ReadRaw(NoncontiguousBuffer* data, size_t max_size, int timeout) {
  if (!stream_provider_) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream reader is not valid"};
  }

  if (!data) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "data parameter is null"};
  }

  return stream_provider_->Read(data, timeout);
}

Status HttpSseStreamReader::ReadAll(NoncontiguousBuffer* data, int timeout) {
  if (!stream_provider_) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream reader is not valid"};
  }

  if (!data) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "data parameter is null"};
  }

  // For SSE streams, we typically read event by event rather than all at once
  // But we provide this method for flexibility
  return stream_provider_->Read(data, timeout);
}

Status HttpSseStreamReader::StartStreaming(SseEventCallback callback, int timeout_ms) {
  if (!stream_provider_) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream reader is not valid"};
  }

  if (!callback) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "callback is null"};
  }

  // Try to cast to HttpClientStream to access SSE-specific methods
  stream::HttpClientStreamPtr http_stream = dynamic_pointer_cast<stream::HttpClientStream>(stream_provider_);
  if (!http_stream) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream provider is not HTTP client stream"};
  }

  // Start a thread to continuously read and parse SSE events
  // We detach this thread as it will run independently
  std::thread([http_stream, callback, timeout_ms]() {
    TRPC_FMT_DEBUG("Starting SSE streaming loop in thread");
    
    // Configure SSE mode
    Status sse_status = http_stream->ConfigureSseMode();
    if (!sse_status.OK()) {
      TRPC_FMT_ERROR("Failed to configure SSE mode: {}", sse_status.ToString());
      return;
    }

    // Send the HTTP request header
    Status send_status = http_stream->SendRequestHeader();
    if (!send_status.OK()) {
      TRPC_FMT_ERROR("Failed to send HTTP request header: {}", send_status.ToString());
      return;
    }
    
    // First, read HTTP response headers
    int http_status_code = 0;
    trpc::http::HttpHeader http_headers;
    
    TRPC_FMT_DEBUG("Attempting to read HTTP headers with timeout: {}ms", timeout_ms);
    
    // Try to read headers with a reasonable timeout
    auto expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    Status header_status = http_stream->ReadHeaders(http_status_code, http_headers, expiry);
    if (!header_status.OK()) {
      TRPC_FMT_ERROR("Failed to read HTTP headers: {}", header_status.ToString());
      return;
    }
    
    TRPC_FMT_DEBUG("HTTP status code: {}", http_status_code);
    
    // Check if it's a successful response
    if (http_status_code != 200) {
      TRPC_FMT_WARN("HTTP error status: {}", http_status_code);
    }
    
    // Check for Content-Type header
    std::string content_type = http_headers.Get("Content-Type");
    if (content_type.find("text/event-stream") != std::string::npos) {
      TRPC_FMT_INFO("Confirmed SSE stream (text/event-stream)");
    } else {
      TRPC_FMT_WARN("Response may not be SSE stream, Content-Type: {}", content_type);
    }
    
    // Print all headers for debugging
    for (const auto& [key, value] : http_headers.Pairs()) {
      TRPC_FMT_DEBUG("HTTP Header: {}: {}", key, value);
    }
    
    // Now read SSE events using the proper SSE methods
    auto event_expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    
    while (true) {
      trpc::http::sse::SseEvent event;
      Status status = http_stream->ReadSseEvent(event, 8192, event_expiry);
      
      // Handle read status
      if (!status.OK()) {
        // For timeout, just continue reading as SSE is a long-lived connection
        if (status.GetFrameworkRetCode() == TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR) {
          TRPC_FMT_DEBUG("Read timeout, continuing to read (normal for SSE)");
          // Reset expiry for next read
          event_expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
          continue;
        }
        
        // For EOF, break the loop
        if (status.StreamEof()) {
          TRPC_FMT_INFO("SSE stream ended (EOF)");
          break;
        }
        
        // For network errors, break the loop
        if (status.GetFrameworkRetCode() == TRPC_STREAM_CLIENT_NETWORK_ERR ||
            status.GetFrameworkRetCode() == TRPC_STREAM_SERVER_NETWORK_ERR) {
          TRPC_FMT_INFO("SSE stream ended or connection closed: {}", status.ToString());
          break;
        }
        
        // For other errors, log and continue
        TRPC_FMT_WARN("SSE read error: {}, continuing", status.ToString());
        // Reset expiry for next read
        event_expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        continue;
      }
      
      // Successfully read an event, invoke the callback
      TRPC_FMT_DEBUG("Received SSE event - id: {}, event: {}, data: {}", 
                    event.id.has_value() ? event.id.value() : "",
                    event.event_type, 
                    event.data);
      callback(event);
      
      // Reset expiry for next read
      event_expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    }
    
    TRPC_FMT_INFO("SSE streaming loop ended");
  }).detach();

  return Status{}; // Success
}

Status HttpSseStreamReader::Finish() {
  if (!stream_provider_) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream reader is not valid"};
  }

  return stream_provider_->Finish();
}

Status HttpSseStreamReader::GetStatus() const {
  if (!stream_provider_) {
    return Status{TRPC_STREAM_UNKNOWN_ERR, 0, "stream reader is not valid"};
  }

  return stream_provider_->GetStatus();
}

}  // namespace trpc