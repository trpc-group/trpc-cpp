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
DEFINE_string(local_path, "", "local file path");
DEFINE_string(filename, "", "file name to upload");
DEFINE_bool(use_chunked, true, "send request content in chunked mode");

namespace video_demo {

using HttpServiceProxyPtr = std::shared_ptr<::trpc::http::HttpServiceProxy>;

bool UploadWithChunked(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string& local_path) {
    TRPC_FMT_INFO("Uploading file in chunked mode: {}", local_path);

    std::ifstream fin(local_path, std::ios::binary);
    if (!fin.is_open()) {
        TRPC_FMT_ERROR("Failed to open file: {}", local_path);
        return false;
    }

    auto ctx = ::trpc::MakeClientContext(proxy);
    ctx->SetTimeout(5000);
    ctx->SetHttpHeader(::trpc::http::kHeaderTransferEncoding, ::trpc::http::kTransferEncodingChunked);

    auto stream = proxy->Post(ctx, url);
    if (!stream.GetStatus().OK()) {
        TRPC_FMT_ERROR("Failed to create HTTP stream: {}", stream.GetStatus().ToString());
        return false;
    }

    ::trpc::BufferBuilder builder;
    while (fin) {
        fin.read(builder.data(), builder.SizeAvailable());
        std::size_t n = fin.gcount();
        if (n > 0) {
            ::trpc::NoncontiguousBuffer buffer;
            buffer.Append(builder.Seal(n));
            auto status = stream.Write(std::move(buffer));
            if (!status.OK()) {
                TRPC_FMT_ERROR("Failed to write data: {}", status.ToString());
                return false;
            }
        }
    }

    auto status = stream.WriteDone();
    if (!status.OK()) {
        TRPC_FMT_ERROR("Failed to send WriteDone: {}", status.ToString());
        return false;
    }

    int http_status = 0;
    ::trpc::http::HttpHeader header;
    status = stream.ReadHeaders(http_status, header);
    if (!status.OK() || http_status != ::trpc::http::ResponseStatus::kOk) {
        TRPC_FMT_ERROR("Failed to read response header: {}", status.ToString());
        return false;
    }

    TRPC_FMT_INFO("Upload completed successfully.");
    return true;
}

bool UploadWithContentLength(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string& local_path) {
    TRPC_FMT_INFO("Uploading file in content-length mode: {}", local_path);

    std::ifstream fin(local_path, std::ios::binary);
    if (!fin.is_open()) {
        TRPC_FMT_ERROR("Failed to open file: {}", local_path);
        return false;
    }

    fin.seekg(0, std::ios::end);
    size_t file_size = fin.tellg();
    fin.seekg(0, std::ios::beg);

    auto ctx = ::trpc::MakeClientContext(proxy);
    ctx->SetTimeout(5000);
    ctx->SetHttpHeader(::trpc::http::kHeaderContentLength, std::to_string(file_size));

    auto stream = proxy->Post(ctx, url);
    if (!stream.GetStatus().OK()) {
        TRPC_FMT_ERROR("Failed to create HTTP stream: {}", stream.GetStatus().ToString());
        return false;
    }

    ::trpc::BufferBuilder builder;
    while (fin) {
        fin.read(builder.data(), builder.SizeAvailable());
        std::size_t n = fin.gcount();
        if (n > 0) {
            ::trpc::NoncontiguousBuffer buffer;
            buffer.Append(builder.Seal(n));
            auto status = stream.Write(std::move(buffer));
            if (!status.OK()) {
                TRPC_FMT_ERROR("Failed to write data: {}", status.ToString());
                return false;
            }
        }
    }

    auto status = stream.WriteDone();
    if (!status.OK()) {
        TRPC_FMT_ERROR("Failed to send WriteDone: {}", status.ToString());
        return false;
    }

    int http_status = 0;
    ::trpc::http::HttpHeader header;
    status = stream.ReadHeaders(http_status, header);
    if (!status.OK() || http_status != ::trpc::http::ResponseStatus::kOk) {
        TRPC_FMT_ERROR("Failed to read response header: {}", status.ToString());
        return false;
    }

    TRPC_FMT_INFO("Upload completed successfully.");
    return true;
}

int Run() {
    ::trpc::ServiceProxyOption option;
    option.name = FLAGS_service_name;
    option.target = FLAGS_addr;
    option.codec_name = "http";

    auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(
        FLAGS_service_name, option);

    std::string url = "/upload?filename=" + FLAGS_filename;

    if (FLAGS_use_chunked) {
        return UploadWithChunked(proxy, url, FLAGS_local_path) ? 0 : -1;
    } else {
        return UploadWithContentLength(proxy, url, FLAGS_local_path) ? 0 : -1;
    }
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


