[TOC]

# Instructions for using HttpSseProxy

## 1 HttpSseProxy
`HttpSseProxy` is an extension of `ServiceProxy` for the HTTP Server-Sent Events (SSE) protocol. It enables clients to establish persistent connections with servers to receive real-time updates.

## 2 User interface 
### 2.1 Streaming interface
#### 2.1.1 Get
Method: `Get`. Creates an [HttpSseStreamReader](http_sse_stream_reader.h) for receiving Server-Sent Events from a streaming response.

#### 2.1.2 Start
Method: `Start`. Starts an SSE client in a background thread that automatically reads events and dispatches them to a callback function.

## 3 HttpSseStreamReader
The [HttpSseStreamReader](http_sse_stream_reader.h) provides methods for reading SSE events from a streaming response.

### 3.1 Read
Reads the next SSE event from the stream with an optional timeout.

### 3.2 ReadHeaders
Reads HTTP response headers from the SSE stream.

### 3.3 StartStreaming
Starts a non-blocking streaming loop that continuously reads raw data and parses SSE events, invoking a callback function for each event received.

### 3.4 Finish
Finishes the stream and returns the final RPC execution result.

## 4 Setting Custom Headers
Setting headers is at the request level, and it is achieved through the interface of `ClientContext`.

### 4.1 SetHttpHeader
Users can set custom HTTP headers for SSE requests.

### 4.2 GetHttpHeader
Users can get their own set of custom HTTP headers.

### 4.3 Example
A typical SSE streaming request with custom headers:
```cpp
trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
ctx->SetTimeout(120000); // 120 seconds for SSE streaming
ctx->SetHttpHeader("Authorization", "Bearer token");
ctx->SetHttpHeader("Custom-Header", "custom-value");

// Method 1: Manual reading of SSE events
std::string url = "http://example.com/sse/stream";
trpc::HttpSseStreamReader reader = proxy->Get(ctx, url);
if (reader.IsValid()) {
  // Read headers first
  int response_code;
  trpc::http::HttpHeader headers;
  trpc::Status status = reader.ReadHeaders(response_code, headers);
  
  // Read events manually
  trpc::http::sse::SseEvent event;
  while (reader.Read(&event, 30000).OK()) { // 30s timeout
    std::cout << "Event ID: " << (event.id.has_value() ? event.id.value() : "none") 
              << ", Type: " << event.event_type 
              << ", Data: " << event.data << std::endl;
  }
}

// Method 2: Callback-based streaming
std::string url = "http://example.com/sse/stream";
auto event_callback = [](const trpc::http::sse::SseEvent& event) {
  std::cout << "Received SSE event - ID: " << (event.id.has_value() ? event.id.value() : "none")
            << ", Type: " << event.event_type
            << ", Data: " << event.data << std::endl;
};

bool success = proxy->Start(ctx, url, event_callback);
if (success) {
  std::cout << "SSE streaming started successfully" << std::endl;
}
```

## 5 Configuration Requirements
For proper SSE client operation, configure the ServiceProxyOption with:
- `conn_type='long'` to maintain persistent connections for SSE streams
- Appropriate timeout values:
  - Read timeout: 120 seconds or more to handle streaming events
  - Overall connection timeout: 180 seconds or more

Example configuration:
```cpp
trpc::ServiceProxyOption option;
option.name = "sse_client";
option.codec_name = "http";
option.network = "tcp";
option.conn_type = "long";     // Required for SSE
option.timeout = 180000;       // 180 seconds
option.selector_name = "direct";
option.target = "127.0.0.1:8080";
```