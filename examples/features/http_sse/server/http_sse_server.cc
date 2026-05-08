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
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "trpc/common/trpc_app.h"
#include "trpc/server/http_service.h"
#include "trpc/util/http/function_handlers.h"
#include "trpc/util/http/http_handler.h"
#include "trpc/util/http/routes.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/stream/http/http_sse_stream.h" // SseStreamWriter

namespace http::sse_demo {

// SSE handler: on GET /sse/test will send multiple SSE events and keep connection open briefly.
class SseTestHandler : public ::trpc::http::HttpHandler {
 public:
  // We implement GET; you could also override Handle(...) generically.
  ::trpc::Status Get(const ::trpc::ServerContextPtr& ctx,
                     const ::trpc::http::RequestPtr& req,
                     ::trpc::http::Response* rsp) override {
    TRPC_LOG_INFO("SSE connect: " << req->SerializeToString());

    // Ensure response enters streaming mode. HttpService::Handle may do this for stream handlers,
    // but calling explicitly is safe.
    rsp->EnableStream(ctx.get());

    // Create our SseStreamWriter wrapper using the ServerContext
    trpc::stream::SseStreamWriter writer(ctx.get());

    // Optional: explicitly write header (WriteEvent will call it implicitly)
    auto status = writer.WriteHeader();
    if (!status.OK()) {
      TRPC_LOG_ERROR("SSE WriteHeader failed: " << status.ToString());
      return status;
    }

    // Send N events, 1s apart
    for (int i = 0; i < 10; ++i) {
      trpc::http::sse::SseEvent ev;
      ev.id = std::to_string(i);
      ev.event_type = "message";
      ev.data = "{\"msg\": \"hello\", \"idx\": " + std::to_string(i) + "}";
      
      // Log the event being sent
      TRPC_LOG_INFO("Sending SSE event: " << ev.ToString());
      
      auto s = writer.WriteEvent(ev);
      if (!s.OK()) {
        TRPC_LOG_WARN("WriteEvent failed (client probably closed): " << s.ToString());
        break;
      }

      // flush / wait a bit to simulate streaming
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Optionally send the chunked end marker (will close chunked stream, client may see EOF)
    writer.WriteDone();

    // Keep connection open for a short while (not required), then close
    // writer.Close(); // not needed here; framework may close after handler returns for non-stream response.
    TRPC_LOG_INFO("SSE handler finished");
    return trpc::kSuccStatus;
  }

  // Make handler explicitly a stream-capable handler (some frameworks use this; if not needed you can omit)
  bool IsStream()  { return true; }
};

void SetRoutes(::trpc::http::HttpRoutes& r) {
  auto sse_handler = std::make_shared<SseTestHandler>();
  // register GET route
  r.Add(::trpc::http::MethodType::GET, ::trpc::http::Path("/sse/test"), sse_handler);
}

class SseServerApp : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    auto http_service = std::make_shared<::trpc::HttpService>();
    http_service->SetRoutes(SetRoutes);
    RegisterService("default_http_sse_service", http_service);
    return 0;
  }

  void Destroy() override {}
};

}  // namespace http::sse_demo

int main(int argc, char** argv) {
  http::sse_demo::SseServerApp app;
  app.Main(argc, argv);
  app.Wait();
  return 0;
}
