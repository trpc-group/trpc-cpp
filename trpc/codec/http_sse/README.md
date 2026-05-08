# HTTP SSE Codec

## Overview

The HTTP SSE (Server-Sent Events) codec is a specialized codec implementation for the tRPC-CPP framework that enables real-time, unidirectional communication from server to client using the Server-Sent Events protocol. This codec extends the standard HTTP codec to handle SSE-specific requirements and provides both client-side and server-side implementations.

## Features

- **Real-time Communication**: Enables server-to-client streaming using the SSE protocol
- **Zero-copy Operations**: Efficient memory management with zero-copy encoding/decoding
- **Protocol Validation**: Built-in validation for SSE-specific headers and request/response formats
- **Event Parsing**: Automatic parsing and serialization of SSE events
- **CORS Support**: Built-in CORS headers for web browser compatibility
- **Error Handling**: Comprehensive error handling and logging

## Architecture

### Core Components

#### 1. Protocol Classes

- **`HttpSseRequestProtocol`**: Extends `HttpRequestProtocol` to handle SSE-specific request data
- **`HttpSseResponseProtocol`**: Extends `HttpResponseProtocol` to handle SSE-specific response data

#### 2. Codec Classes

- **`HttpSseClientCodec`**: Client-side codec for encoding requests and decoding responses
- **`HttpSseServerCodec`**: Server-side codec for decoding requests and encoding responses

#### 3. Protocol Checker

- **`HttpSseProtoChecker`**: Validates SSE protocol compliance and handles zero-copy packet checking

### Class Hierarchy

```
HttpRequestProtocol
    └── HttpSseRequestProtocol

HttpResponseProtocol
    └── HttpSseResponseProtocol

HttpClientCodec
    └── HttpSseClientCodec

HttpServerCodec
    └── HttpSseServerCodec
```

## API Reference

### HttpSseRequestProtocol

```cpp
class HttpSseRequestProtocol : public HttpRequestProtocol {
public:
    // Get parsed SSE event from request body
    std::optional<http::sse::SseEvent> GetSseEvent() const;
    
    // Set SSE event as request body
    void SetSseEvent(const http::sse::SseEvent& event);
};
```

### HttpSseResponseProtocol

```cpp
class HttpSseResponseProtocol : public HttpResponseProtocol {
public:
    // Get parsed SSE event from response body
    std::optional<http::sse::SseEvent> GetSseEvent() const;
    
    // Set single SSE event as response body
    void SetSseEvent(const http::sse::SseEvent& event);
    
    // Set multiple SSE events as response body
    void SetSseEvents(const std::vector<http::sse::SseEvent>& events);
};
```

### HttpSseClientCodec

```cpp
class HttpSseClientCodec : public HttpClientCodec {
public:
    // Zero-copy protocol checking
    int ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;
    
    // Zero-copy decoding
    bool ZeroCopyDecode(const ClientContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;
    
    // Zero-copy encoding
    bool ZeroCopyEncode(const ClientContextPtr& ctx, const ProtocolPtr& in, NoncontiguousBuffer& out) override;
    
    // Fill request with SSE data
    bool FillRequest(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;
    
    // Fill response with SSE data
    bool FillResponse(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;
    
    // Create protocol objects
    ProtocolPtr CreateRequestPtr() override;
    ProtocolPtr CreateResponsePtr() override;
};
```

### HttpSseServerCodec

```cpp
class HttpSseServerCodec : public HttpServerCodec {
public:
    // Zero-copy protocol checking
    int ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;
    
    // Zero-copy decoding
    bool ZeroCopyDecode(const ServerContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;
    
    // Zero-copy encoding
    bool ZeroCopyEncode(const ServerContextPtr& ctx, ProtocolPtr& in, NoncontiguousBuffer& out) override;
    
    // Create protocol objects
    ProtocolPtr CreateRequestObject() override;
    ProtocolPtr CreateResponseObject() override;
    
    // Validate SSE request
    bool IsValidSseRequest(const http::Request* request) const;
};
```

## Usage Examples

### Client-Side Usage

```cpp
#include "trpc/codec/http_sse/http_sse_client_codec.h"
#include "trpc/util/http/sse/sse_parser.h"

// Create client codec
HttpSseClientCodec codec;

// Create request protocol
auto request = codec.CreateRequestPtr();
auto sse_request = std::dynamic_pointer_cast<HttpSseRequestProtocol>(request);

// Set SSE event
http::sse::SseEvent event;
event.event_type = "message";
event.data = "Hello Server";
event.id = "123";
sse_request->SetSseEvent(event);

// Fill request with SSE data
codec.FillRequest(ctx, request, &event);

// Encode request
NoncontiguousBuffer buffer;
codec.ZeroCopyEncode(ctx, request, buffer);
```

### Server-Side Usage

```cpp
#include "trpc/codec/http_sse/http_sse_server_codec.h"

// Create server codec
HttpSseServerCodec codec;

// Create response protocol
auto response = codec.CreateResponseObject();
auto sse_response = std::dynamic_pointer_cast<HttpSseResponseProtocol>(response);

// Set SSE event
http::sse::SseEvent event;
event.event_type = "notification";
event.data = "User logged in";
event.id = "456";
sse_response->SetSseEvent(event);

// Encode response
NoncontiguousBuffer buffer;
codec.ZeroCopyEncode(ctx, response, buffer);
```

### Multiple Events

```cpp
// Set multiple SSE events
std::vector<http::sse::SseEvent> events = {
    {.event_type = "message", .data = "Event 1", .id = "1"},
    {.event_type = "update", .data = "Event 2", .id = "2"},
    {.data = "Event 3"}  // No event type specified
};

sse_response->SetSseEvents(events);
```

## SSE Event Structure

The SSE event structure follows the Server-Sent Events specification:

```cpp
struct SseEvent {
    std::string event_type;                    // Event type (optional)
    std::string data;                          // Event data (required)
    std::optional<std::string> id;             // Event ID (optional)
    std::optional<uint32_t> retry;             // Retry interval in milliseconds (optional)
};
```

### Event Serialization Format

SSE events are serialized according to the SSE specification:

```
event: message
id: 123
data: Hello World

```

## HTTP Headers

### Request Headers (Set by Client Codec)

- `Accept: text/event-stream`
- `Cache-Control: no-cache`
- `Connection: keep-alive`

### Response Headers (Set by Server Codec)

- `Content-Type: text/event-stream`
- `Cache-Control: no-cache`
- `Connection: keep-alive`
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Headers: Cache-Control`

## Protocol Validation

### SSE Request Validation

A valid SSE request must have:
- HTTP method: `GET`
- `Accept` header containing `text/event-stream`
- `Cache-Control` header containing `no-cache`

### SSE Response Validation

A valid SSE response must have:
- `Content-Type: text/event-stream`
- `Cache-Control` header containing `no-cache`

## Error Handling

The codec provides comprehensive error handling:

- **Invalid SSE Data**: Graceful handling of malformed SSE events
- **Protocol Validation**: Automatic validation of SSE-specific headers
- **Memory Management**: Zero-copy operations with proper error recovery
- **Logging**: Detailed error logging for debugging

## Dependencies

- `trpc/codec/http:http_protocol` - Base HTTP protocol classes
- `trpc/codec/http:http_client_codec` - Base HTTP client codec
- `trpc/codec/http:http_server_codec` - Base HTTP server codec
- `trpc/util/http/sse:http_sse_parser` - SSE event parsing utilities
- `trpc/util/buffer:noncontiguous_buffer` - Buffer management
- `trpc/common:status` - Status handling
- `trpc/log:trpc_log` - Logging utilities

## Building

The HTTP SSE codec is built using Bazel. Include the following targets in your BUILD file:

```python
cc_library(
    name = "my_target",
    deps = [
        "//trpc/codec/http_sse:http_sse_codec",  # Combined library
        # OR individual components:
        # "//trpc/codec/http_sse:http_sse_client_codec",
        # "//trpc/codec/http_sse:http_sse_server_codec",
        # "//trpc/codec/http_sse:http_sse_protocol",
        # "//trpc/codec/http_sse:http_sse_proto_checker",
    ],
)
```

## Performance Considerations

- **Zero-copy Operations**: The codec uses zero-copy operations for optimal performance
- **Memory Efficiency**: Efficient buffer management with `NoncontiguousBuffer`
- **Streaming Support**: Designed for high-throughput streaming scenarios
- **Connection Reuse**: Supports keep-alive connections for better performance

## Browser Compatibility

The codec includes CORS headers to ensure compatibility with web browsers:
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Headers: Cache-Control`

## Security Considerations

- **CORS Headers**: Configurable CORS support for web applications
- **Input Validation**: Comprehensive validation of SSE protocol compliance
- **Error Handling**: Secure error handling without information leakage

## Limitations

- **Unidirectional**: SSE is unidirectional (server to client only)
- **Browser Limits**: Some browsers have connection limits for SSE
- **Proxy Support**: May require special configuration for proxy servers

## Future Enhancements

- **Compression Support**: Add support for gzip compression
- **Authentication**: Enhanced authentication mechanisms
- **Rate Limiting**: Built-in rate limiting capabilities
- **Metrics**: Performance metrics and monitoring

## Related Documentation

- [Server-Sent Events Specification](https://html.spec.whatwg.org/multipage/server-sent-events.html)
- [tRPC-CPP HTTP Codec Documentation](../http/README.md)
- [Testing Documentation](test/README.md)
