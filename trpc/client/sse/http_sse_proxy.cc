#include "trpc/client/sse/http_sse_proxy.h"

#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/http/http_client_codec.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/stream/http/http_stream_provider.h"
#include "trpc/stream/http/http_client_stream.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/util/http/util.h"
#include "trpc/util/http/url.h"
#include "trpc/util/log/logging.h"
#include <thread>
#include <chrono>
#include <memory>

namespace trpc {

HttpSseStreamReader HttpSseProxy::Get(const ClientContextPtr& ctx, const std::string& url) {
  return Get(ctx, url, "GET");
}

HttpSseStreamReader HttpSseProxy::Get(const ClientContextPtr& ctx, const std::string& url, const std::string& method) {
  return CreateStreamReader(ctx, url, method);
}

bool HttpSseProxy::Start(const ClientContextPtr& ctx, const std::string& url, EventCallback cb) {
  return Start(ctx, url, "GET", std::move(cb));
}

bool HttpSseProxy::Start(const ClientContextPtr& ctx, const std::string& url, const std::string& method, EventCallback cb) {
  // Launch a std::thread to handle the SSE event loop instead of a fiber
  try {
    std::thread([this, ctx, url, method, cb = std::move(cb)]() mutable {
      // Get the SSE stream reader
      auto reader = this->Get(ctx, url, method);

      // Read HTTP response headers first
      int code;
      trpc::http::HttpHeader headers;
      Status status = reader.ReadHeaders(code, headers);
      if (!status.OK() || code != 200) {
        TRPC_FMT_ERROR("SSE connect failed, code={}, status={}", code, status.ToString());
        return;
      }

      // Check if the response is actually SSE (Content-Type: text/event-stream)
      std::string content_type = headers.Get("Content-Type");
      if (content_type.find("text/event-stream") == std::string::npos) {
        TRPC_FMT_WARN("Response may not be SSE, Content-Type: {}", content_type);
      }

      // Main event reading loop
      while (true) {
        trpc::http::sse::SseEvent evt;
        // Use a reasonable timeout for reading events (30 seconds)
        Status read_status = reader.Read(&evt, 30000); // 30 seconds timeout in milliseconds
        
        if (!read_status.OK()) {
          TRPC_FMT_WARN("SSE read failed: {}", read_status.ToString());
          
          // Check for EOF or network errors to break the loop
          if (read_status.StreamEof() || 
              read_status.GetFrameworkRetCode() == TRPC_STREAM_CLIENT_NETWORK_ERR ||
              read_status.GetFrameworkRetCode() == TRPC_STREAM_SERVER_NETWORK_ERR) {
            TRPC_FMT_INFO("SSE connection closed, stopping event loop");
            break;
          }
          
          // For other errors, we might want to implement reconnection logic
          // For now, we'll sleep a bit and continue
          std::this_thread::sleep_for(std::chrono::seconds(1));
          continue;
        }
        
        // Successfully read an event, invoke the callback
        cb(evt);
      }
    }).detach(); // Detach the thread to allow it to run independently
    return true;
  } catch (const std::exception& e) {
    TRPC_FMT_ERROR("Failed to start SSE client: {}", e.what());
    return false;
  }
}

HttpSseStreamReader HttpSseProxy::CreateStreamReader(const ClientContextPtr& ctx, const std::string& url,
                                                     const std::string& method) {
  // Create HTTP request protocol
  auto codec = ClientCodecFactory::GetInstance()->Get("http");
  if (!codec) {
    TRPC_LOG_ERROR("Failed to get HTTP codec");
    return HttpSseStreamReader{};
  }

  // Make sure the context has a request
  if (!ctx->GetRequest()) {
    ctx->SetRequest(codec->CreateRequestPtr());
  }

  auto req_protocol = ctx->GetRequest();
  auto http_req = std::dynamic_pointer_cast<HttpRequestProtocol>(req_protocol);
  if (!http_req) {
    TRPC_LOG_ERROR("Failed to cast to HttpRequestProtocol");
    return HttpSseStreamReader{};
  }

  // Build request header with SSE-specific settings
  BuildRequestHeader(ctx, url, method, http_req.get());

  // Create stream using SelectStreamProvider directly
  auto stream_provider = SelectStreamProvider(ctx, nullptr);
  if (!stream_provider || !stream_provider->GetStatus().OK()) {
    TRPC_LOG_ERROR("Failed to create stream provider");
    return HttpSseStreamReader{};
  }

  // Cast to HttpClientStream and set the request protocol
  auto http_stream_provider = static_pointer_cast<stream::HttpClientStream>(stream_provider);
  http_stream_provider->SetHttpRequestProtocol(http_req.get());

  return HttpSseStreamReader(stream_provider);
}

void HttpSseProxy::BuildRequestHeader(const ClientContextPtr& ctx, const std::string& url, const std::string& method,
                                      HttpRequestProtocol* req) {
  if (!req) {
    return;
  }

  // Parse URL to extract host, path, etc.
  http::UrlParser url_parser(url);
  if (!url_parser.IsValid()) {
    TRPC_LOG_ERROR("Invalid URL: " << url);
    return;
  }

  // Set HTTP method (typically GET for SSE)
  req->request->SetMethodType(http::OperationType::GET); // SSE typically uses GET
  if (method == "POST") {
    req->request->SetMethodType(http::OperationType::POST);
  } else if (method == "PUT") {
    req->request->SetMethodType(http::OperationType::PUT);
  } else if (method == "DELETE") {
    req->request->SetMethodType(http::OperationType::DELETE);
  }

  // Set URL path and query
  std::string request_url = url_parser.Request();
  req->request->SetUrl(request_url);

  // Set Host header
  std::string host = url_parser.Hostname();
  if (!url_parser.Port().empty()) {
    host.append(":").append(url_parser.Port());
  }
  req->request->SetHeader("Host", host);

  // Set SSE-specific headers
  req->request->SetHeader("Accept", "text/event-stream");
  req->request->SetHeader("Cache-Control", "no-cache");
  req->request->SetHeader("Connection", "keep-alive");

  // Set any additional headers from the context
  const auto& headers = ctx->GetHttpHeaders();
  for (const auto& [key, value] : headers) {
    req->request->SetHeader(key, value);
  }

  // Set protocol version (SSE typically uses HTTP/1.1)
  req->request->SetVersion("1.1");
}

}  // namespace trpc
