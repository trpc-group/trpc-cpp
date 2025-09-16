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
//#include <iostream>
#include <string>
#include <memory>
#include <chrono>

#include "gflags/gflags.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/client/sse/http_sse_proxy.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/status.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/http/sse/sse_event.h"

DEFINE_string(service_name, "sse_client", "callee service name");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "path to client config");
DEFINE_string(addr, "127.0.0.1:24856", "server ip:port");
DEFINE_string(path, "/sse/test", "SSE URL path");

namespace http::sse_demo {

using HttpSseProxyPtr = std::shared_ptr<::trpc::HttpSseProxy>;

// Callback-based SSE client using streaming approach
bool StartSseClient(const HttpSseProxyPtr& proxy) {
    std::string url = "http://" + FLAGS_addr + FLAGS_path;
    TRPC_FMT_INFO("StartSseClient connecting to {}", url);

    auto ctx = ::trpc::MakeClientContext(proxy);
    
    // Set very long timeout for the context
    ctx->SetTimeout(120000); // 120 seconds

    // Create the reader
    auto reader = proxy->Get(ctx, url);
    if (!reader.IsValid()) {
        TRPC_FMT_ERROR("Failed to create SSE stream reader");
        return false;
    }

    // Event callback
    auto event_callback = [](const ::trpc::http::sse::SseEvent& event) {
        std::string id_str = event.id.has_value() ? event.id.value() : "";
        TRPC_FMT_INFO("Received SSE event - id: {}, event: {}, data: {}",
                      id_str, event.event_type, event.data);
    };

    // Start streaming with the new non-blocking approach
    ::trpc::Status status = reader.StartStreaming(event_callback, 30000); // 30 second timeout for reads
    if (!status.OK()) {
        TRPC_FMT_ERROR("Failed to start SSE streaming: {}", status.ToString());
        return false;
    }

    TRPC_FMT_INFO("SSE client started successfully with streaming (callback-based)");
    
    // Wait for events to be received
    ::trpc::FiberSleepFor(std::chrono::seconds(15)); // Wait for callback to print events

    return true;
}

// Manual read SSE client - using streaming approach
bool GetSseClient(const HttpSseProxyPtr& proxy) {
    std::string url = "http://" + FLAGS_addr + FLAGS_path;
    TRPC_FMT_INFO("GetSseClient connecting to {}", url);
    TRPC_FMT_DEBUG("Fiber Scheduler running: {}", ::trpc::IsRunningInFiberWorker());

    auto ctx = ::trpc::MakeClientContext(proxy);
    
    // Set very long timeout for the context
    ctx->SetTimeout(120000); // 120 seconds

    // Create the reader
    ::trpc::HttpSseStreamReader reader = proxy->Get(ctx, url);
    if (!reader.IsValid()) {
        TRPC_FMT_ERROR("Failed to create SSE stream reader");
        return false;
    }

    // For manual reading, we'll use the streaming approach but with a different pattern
    TRPC_FMT_INFO("Using streaming approach for manual SSE reading");
    
    // We'll use a flag to control the reading loop
    bool should_continue = true;
    int event_count = 0;
    const int max_events = 10;
    
    // Start streaming with a callback that stores events
    auto event_callback = [&should_continue, &event_count, max_events](const ::trpc::http::sse::SseEvent& event) {
        std::string id_str = event.id.has_value() ? event.id.value() : "";
        TRPC_FMT_INFO("Received SSE event - id: {}, event: {}, data: {}",
                      id_str, event.event_type, event.data);
        
        event_count++;
        if (event_count >= max_events) {
            should_continue = false;
        }
    };

    // Start streaming
    ::trpc::Status status = reader.StartStreaming(event_callback, 30000); // 30 second timeout for reads
    if (!status.OK()) {
        TRPC_FMT_ERROR("Failed to start SSE streaming: {}", status.ToString());
        return false;
    }

    TRPC_FMT_INFO("SSE streaming started successfully (manual reading)");
    
    // Wait for events to be received
    for (int i = 0; i < 15 && should_continue; i++) {
        ::trpc::FiberSleepFor(std::chrono::seconds(1));
    }

    return true;
}

int Run() {
    bool final_ok = true;

    ::trpc::ServiceProxyOption option;
    option.name = FLAGS_service_name;
    option.codec_name = "http";
    option.network = "tcp";
    option.conn_type = "long";  // Long connection
    option.timeout = 180000;    // 180 seconds timeout
    option.selector_name = "direct";
    option.target = FLAGS_addr;

    auto sse_client = ::trpc::GetTrpcClient()->GetProxy<::trpc::HttpSseProxy>(FLAGS_service_name, option);

    TRPC_FMT_INFO("Testing SSE client with Start API (callback-based)");
    if (!StartSseClient(sse_client)) final_ok = false;

    ::trpc::FiberSleepFor(std::chrono::seconds(3));

    TRPC_FMT_INFO("Testing SSE client with Get API (manual reading)");
    if (!GetSseClient(sse_client)) final_ok = false;

    std::cout << "Final SSE result: " << final_ok << std::endl;
    return final_ok ? 0 : -1;
}

}  // namespace http::sse_demo

void ParseClientConfig(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::CommandLineFlagInfo info;
    if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
        std::cerr << "start client with client_config, for example: " << argv[0]
                  << " --client_config=/client/client_config/filepath" << std::endl;
        exit(-1);
    }
    std::cout << "FLAGS_service_name: " << FLAGS_service_name << std::endl;
    std::cout << "FLAGS_client_config: " << FLAGS_client_config << std::endl;
    std::cout << "FLAGS_addr: " << FLAGS_addr << std::endl;
    std::cout << "FLAGS_path: " << FLAGS_path << std::endl;
}

int main(int argc, char* argv[]) {
    ParseClientConfig(argc, argv);

    if (::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config) != 0) {
        std::cerr << "load client_config failed." << std::endl;
        return -1;
    }

    return ::trpc::RunInTrpcRuntime([]() { return http::sse_demo::Run(); });
}
