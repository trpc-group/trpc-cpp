#include <fstream>
#include <iostream>
#include <string>

#include "gflags/gflags.h"

#include "trpc/client/http/http_service_proxy.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/util/log/logging.h"

DEFINE_string(service_name, "video_stream_service", "callee service name");
DEFINE_string(addr, "127.0.0.1:8080", "server address");
DEFINE_string(local_path, "", "local file path to save downloaded video");
DEFINE_string(filename, "", "file name to download");

namespace video_demo {

using HttpServiceProxyPtr = std::shared_ptr<::trpc::http::HttpServiceProxy>;

bool DownloadVideo(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string& local_path) {
    TRPC_FMT_INFO("Downloading file from server: {}", local_path);

    std::ofstream fout(local_path, std::ios::binary);
    if (!fout.is_open()) {
        TRPC_FMT_ERROR("Failed to open file for writing: {}", local_path);
        return false;
    }

    auto ctx = ::trpc::MakeClientContext(proxy);
    ctx->SetTimeout(5000);

    auto stream = proxy->Get(ctx, url);
    if (!stream.GetStatus().OK()) {
        TRPC_FMT_ERROR("Failed to create HTTP stream: {}", stream.GetStatus().ToString());
        return false;
    }

    int http_status = 0;
    ::trpc::http::HttpHeader header;
    auto status = stream.ReadHeaders(http_status, header);
    if (!status.OK()) {
        TRPC_FMT_ERROR("Failed to read response header: {}", status.ToString());
        return false;
    }

    // if (http_status != ::trpc::http::ResponseStatus::kOk) {
    //     TRPC_FMT_WARN("Unexpected HTTP status code: {}, proceeding anyway", http_status);
    // }

    constexpr size_t kBufferSize = 1024 * 1024;  // Read 1MB chunks
    ::trpc::NoncontiguousBuffer buffer;
    while (stream.Read(buffer, kBufferSize).OK()) {
        for (const auto& block : buffer) {
            fout.write(block.data(), block.size());
        }
    }

    fout.close();
    TRPC_FMT_INFO("Download completed successfully.");
    return true;
}

int Run() {
    ::trpc::ServiceProxyOption option;
    option.name = FLAGS_service_name;
    option.target = FLAGS_addr;
    option.codec_name = "http";

    auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(
        FLAGS_service_name, option);

    std::string url = "/video_preview?filename=" + FLAGS_filename;

    return DownloadVideo(proxy, url, FLAGS_local_path) ? 0 : -1;
}

}  // namespace video_demo

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (::trpc::TrpcConfig::GetInstance()->Init("examples/http_video_demo/client/trpc_cpp_fiber.yaml") != 0) {
        std::cerr << "Failed to load client config." << std::endl;
        return -1;
    }

    return ::trpc::RunInFiberRuntime([]() { return video_demo::Run(); });
}