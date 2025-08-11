# HTTP Server-Sent Events (SSE) Codec

This directory contains the HTTP SSE codec implementation for tRPC-Cpp, providing protocol encoding and decoding for Server-Sent Events over HTTP.

## Overview

The HTTP SSE codec extends the existing HTTP codec infrastructure to handle SSE-specific protocol requirements:

- **SSE Protocol Compliance**: Full compliance with [W3C Server-Sent Events specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)
- **HTTP Integration**: Seamless integration with existing HTTP codec infrastructure
- **Bidirectional Support**: Both client-side and server-side SSE handling
- **Streaming Support**: Optimized for long-lived HTTP connections

## Components

### 1. HttpSseRequestProtocol

Extends `HttpRequestProtocol` to handle SSE-specific request processing.

#### Header File
```cpp
#include "trpc/codec/http_sse/http_sse_codec.h"
```

#### Class Definition
```cpp
namespace trpc {

class HttpSseRequestProtocol : public HttpRequestProtocol {
 public:
  HttpSseRequestProtocol();
  explicit HttpSseRequestProtocol(http::RequestPtr&& request);
  ~HttpSseRequestProtocol() override = default;

  // SSE-specific methods
  std::optional<http::sse::SseEvent> GetSseEvent() const;
  void SetSseEvent(const http::sse::SseEvent& event);
};

} // namespace trpc
```

#### Methods

| Method | Parameters | Return | Description |
|--------|------------|--------|-------------|
| `GetSseEvent` | `const` | `std::optional<http::sse::SseEvent>` | Parse SSE event from request body |
| `SetSseEvent` | `const http::sse::SseEvent& event` | `void` | Set SSE event as request body |

### 2. HttpSseResponseProtocol

Extends `HttpResponseProtocol` to handle SSE-specific response processing.

#### Class Definition
```cpp
namespace trpc {

class HttpSseResponseProtocol : public HttpResponseProtocol {
 public:
  HttpSseResponseProtocol();
  ~HttpSseResponseProtocol() override = default;

  // SSE-specific methods
  std::optional<http::sse::SseEvent> GetSseEvent() const;
  void SetSseEvent(const http::sse::SseEvent& event);
  void SetSseEvents(const std::vector<http::sse::SseEvent>& events);
};

} // namespace trpc
```

#### Methods

| Method | Parameters | Return | Description |
|--------|------------|--------|-------------|
| `GetSseEvent` | `const` | `std::optional<http::sse::SseEvent>` | Parse SSE event from response body |
| `SetSseEvent` | `const http::sse::SseEvent& event` | `void` | Set single SSE event as response body |
| `SetSseEvents` | `const std::vector<http::sse::SseEvent>& events` | `void` | Set multiple SSE events as response body |

### 3. HttpSseClientCodec

Client-side codec for encoding SSE requests and decoding SSE responses.

#### Class Definition
```cpp
namespace trpc {

class HttpSseClientCodec : public HttpClientCodec {
 public:
  ~HttpSseClientCodec() override = default;

  // Overridden methods
  std::string Name() const override;
  bool ZeroCopyDecode(const ClientContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;
  bool ZeroCopyEncode(const ClientContextPtr& ctx, const ProtocolPtr& in, NoncontiguousBuffer& out) override;
  bool FillRequest(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;
  bool FillResponse(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;
  ProtocolPtr CreateRequestPtr() override;
  ProtocolPtr CreateResponsePtr() override;

 private:
  void SetSseRequestHeaders(http::Request* request);
  void SetSseResponseHeaders(http::Response* response);
};

} // namespace trpc
```

#### Key Features

- **Automatic Header Management**: Sets SSE-specific headers (`Accept: text/event-stream`, `Cache-Control: no-cache`, etc.)
- **Event Parsing**: Automatically parses SSE events from response bodies
- **Type Safety**: Supports both single `SseEvent` and `std::vector<SseEvent>` in `FillResponse`

### 4. HttpSseServerCodec

Server-side codec for decoding SSE requests and encoding SSE responses.

#### Class Definition
```cpp
namespace trpc {

class HttpSseServerCodec : public HttpServerCodec {
 public:
  ~HttpSseServerCodec() override = default;

  // Overridden methods
  std::string Name() const override;
  int ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;
  bool ZeroCopyDecode(const ServerContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;
  bool ZeroCopyEncode(const ServerContextPtr& ctx, ProtocolPtr& in, NoncontiguousBuffer& out) override;
  ProtocolPtr CreateRequestObject() override;
  ProtocolPtr CreateResponseObject() override;

 private:
  void SetSseResponseHeaders(http::Response* response);
 public:
  bool IsValidSseRequest(const http::Request* request) const;
};

} // namespace trpc
```

#### Key Features

- **Request Validation**: Validates SSE requests (GET method, proper Accept headers)
- **CORS Support**: Automatically sets CORS headers for web browser compatibility
- **Streaming Headers**: Sets appropriate headers for long-lived connections

## Usage Examples

### Client-Side Usage

```cpp
#include "trpc/codec/http_sse/http_sse_codec.h"
#include "trpc/client/service_proxy.h"

using namespace trpc;
using namespace trpc::http::sse;

// Create SSE client codec
auto codec = std::make_shared<HttpSseClientCodec>();

// Create request protocol
auto request_protocol = std::make_shared<HttpSseRequestProtocol>();

// Set up SSE event
SseEvent event("message", "Hello from client", "client_123");
request_protocol->SetSseEvent(event);

// Fill request with SSE data
codec->FillRequest(client_context, request_protocol, &event);

// Encode request
NoncontiguousBuffer encoded_request;
codec->ZeroCopyEncode(client_context, request_protocol, encoded_request);
```

### Server-Side Usage

```cpp
#include "trpc/codec/http_sse/http_sse_codec.h"
#include "trpc/server/http_service.h"

using namespace trpc;
using namespace trpc::http::sse;

// Create SSE server codec
auto codec = std::make_shared<HttpSseServerCodec>();

// Create response protocol
auto response_protocol = std::make_shared<HttpSseResponseProtocol>();

// Set up SSE events
std::vector<SseEvent> events = {
  SseEvent("message", "Event 1", "1"),
  SseEvent("update", "Event 2", "2"),
  SseEvent("notification", "Event 3", "3")
};

response_protocol->SetSseEvents(events);

// Encode response
NoncontiguousBuffer encoded_response;
codec->ZeroCopyEncode(server_context, response_protocol, encoded_response);
```

### Service Integration

```cpp
// In your service implementation
class MySseService : public HttpService {
 public:
  Status HandleSseRequest(ServerContextPtr context, const HttpRequestProtocol& request) {
    auto sse_request = std::dynamic_pointer_cast<HttpSseRequestProtocol>(request);
    if (!sse_request) {
      return Status(trpc::kUnknownError, "Invalid SSE request");
    }

    // Get SSE event from request
    auto event = sse_request->GetSseEvent();
    if (event) {
      // Process the SSE event
      TRPC_LOG_INFO("Received SSE event: " << event->GetData());
    }

    // Create SSE response
    auto response = std::make_shared<HttpSseResponseProtocol>();
    SseEvent response_event("response", "Processed successfully", "resp_123");
    response->SetSseEvent(response_event);

    // Send response
    context->SendResponse(response);
    return Status::OK();
  }
};
```

## Building

### Prerequisites
- Bazel build system
- C++17 or later
- tRPC-Cpp framework
- SSE utilities (`//trpc/util/http/sse:http_sse`)

### Build Commands

#### Build the SSE codec library
```bash
bazel build //trpc/codec/http_sse:http_sse_codec
```

#### Build and run tests
```bash
# Build tests
bazel build //trpc/codec/http_sse:http_sse_codec_test

# Run tests with minimal output
bazel test //trpc/codec/http_sse:http_sse_codec_test

# Run tests with detailed output
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_output=all
```

#### Build specific targets
```bash
# Build with debug symbols
bazel build -c dbg //trpc/codec/http_sse:http_sse_codec

# Build with optimizations
bazel build -c opt //trpc/codec/http_sse:http_sse_codec
```

### Dependencies

The HTTP SSE codec depends on:
- `//trpc/codec/http:http_client_codec` - Base HTTP client codec
- `//trpc/codec/http:http_server_codec` - Base HTTP server codec
- `//trpc/codec/http:http_protocol` - HTTP protocol definitions
- `//trpc/common:status` - Common status types
- `//trpc/log:trpc_log` - Logging utilities
- `//trpc/util/http/sse:http_sse` - SSE utilities

### Integration

To use the HTTP SSE codec in your project:

```cpp
// In your BUILD file
cc_library(
    name = "my_sse_service",
    srcs = ["my_sse_service.cc"],
    deps = [
        "//trpc/codec/http_sse:http_sse_codec",
        "//trpc/util/http/sse:http_sse",
        # ... other dependencies
    ],
)
```

## SSE Headers

The codec automatically manages SSE-specific HTTP headers:

### Request Headers (Client)
- `Accept: text/event-stream`
- `Cache-Control: no-cache`
- `Connection: keep-alive`

### Response Headers (Server)
- `Content-Type: text/event-stream`
- `Cache-Control: no-cache`
- `Connection: keep-alive`
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Headers: Cache-Control`

## Testing

The test suite provides comprehensive coverage for:
- ✅ **Protocol Classes**: Request/response protocol functionality
- ✅ **Codec Operations**: Encoding/decoding operations
- ✅ **Header Management**: SSE-specific header handling
- ✅ **Event Parsing**: SSE event parsing and serialization
- ✅ **Request Validation**: SSE request validation logic

Run tests with:
```bash
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_output=all
```

## Notes

- **Namespace**: All components are in `trpc` namespace
- **Thread Safety**: All methods are thread-safe
- **Error Handling**: Graceful handling of invalid SSE data
- **Performance**: Optimized for streaming scenarios
- **Compatibility**: Fully compatible with existing HTTP infrastructure
- **Constructor Usage**: SseEvent uses a single constructor with optional parameters for flexibility
- **Method Visibility**: `IsValidSseRequest` method is public for testing and validation purposes

## Related Documentation

- [SSE Utilities README](../util/http/sse/README.md) - Core SSE utilities
- [Test Suite README](test/README.md) - Comprehensive test documentation
- [HTTP Codec Documentation](../http/README.md) - Base HTTP codec documentation
- [tRPC-Cpp Framework](https://github.com/trpc-group/trpc-cpp) - Main framework documentation
- [W3C SSE Specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events) - SSE standard reference
