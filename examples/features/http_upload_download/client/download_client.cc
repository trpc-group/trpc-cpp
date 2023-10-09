//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "gflags/gflags.h"

#include "trpc/client/http/http_service_proxy.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/util/log/logging.h"

DEFINE_string(service_name, "http_upload_download_client", "callee service name");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "");
DEFINE_string(addr, "127.0.0.1:24858", "ip:port");
DEFINE_string(dst_path, "download_dst.bin", "file path to store the content which will be downloaded from the server");

namespace http::demo {
using HttpServiceProxyPtr = std::shared_ptr<::trpc::http::HttpServiceProxy>;

bool Download(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string dst_path) {
  auto fout = std::ofstream(dst_path, std::ios::binary);
  if (!fout.is_open()) {
    TRPC_FMT_ERROR("failed to open file, file_path:{}", dst_path);
    return false;
  }

  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(5000);
  // Creates HTTP stream.
  auto stream = proxy->Get(ctx, url);
  if (!stream.GetStatus().OK()) {
    TRPC_FMT_ERROR("failed to create client stream");
    return false;
  }

  // Reads response header.
  int http_status = 0;
  ::trpc::http::HttpHeader http_header;
  ::trpc::Status status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    TRPC_FMT_ERROR("failed to read http header: {}", status.ToString());
    return false;
  } else if (http_status != ::trpc::http::ResponseStatus::kOk) {
    TRPC_FMT_ERROR("http response status:{}", http_status);
    return false;
  }

  constexpr std::size_t kBufferSize{1024 * 1024};
  size_t nread{0};
  // Reads response content.
  for (;;) {
    ::trpc::NoncontiguousBuffer buffer;
    status = stream.Read(buffer, kBufferSize);
    if (status.OK()) {
      nread += buffer.ByteSize();
      for (const auto& block : buffer) {
        fout.write(block.data(), block.size());
      }
      continue;
    } else if (status.StreamEof()) {
      break;
    }
    TRPC_FMT_ERROR("failed to read response content: {}", status.ToString());
    return false;
  }

  TRPC_FMT_INFO("finish downloading, read size: {}", nread);
  return true;
}

int Run() {
  bool final_ok{true};

  struct http_calling_args_t {
    std::string calling_name;
    std::function<bool()> calling_executor;
    bool ok;
  };

  ::trpc::ServiceProxyOption option;
  option.name = FLAGS_service_name;
  option.codec_name = "http";
  option.network = "tcp";
  option.conn_type = "long";
  option.timeout = 5000;
  option.selector_name = "direct";
  option.target = FLAGS_addr;

  auto http_client = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(FLAGS_service_name, option);
  std::vector<http_calling_args_t> callings{
      {"download a file from the server",
       [http_client, dst_path = FLAGS_dst_path]() {
         return Download(http_client, "http://example.com/download", dst_path);
       },
       false},
  };

  auto latch_count = static_cast<std::ptrdiff_t>(callings.size());
  ::trpc::FiberLatch callings_latch{latch_count};

  for (auto& c : callings) {
    ::trpc::StartFiberDetached([&callings_latch, &c]() {
      c.ok = c.calling_executor();
      callings_latch.CountDown();
    });
  }

  callings_latch.Wait();

  for (const auto& c : callings) {
    final_ok &= c.ok;
    std::cout << "name: " << c.calling_name << ", ok: " << c.ok << std::endl;
  }

  std::cout << "final result of http calling: " << final_ok << std::endl;
  return final_ok ? 0 : -1;
}

}  // namespace http::demo

bool ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start client with client_config, for example: " << argv[0]
              << " --client_config=/client/client_config/filepath" << std::endl;
    return false;
  }
  std::cout << "FLAGS_service_name: " << FLAGS_service_name << std::endl;
  std::cout << "FLAGS_client_config: " << FLAGS_client_config << std::endl;
  std::cout << "FLAGS_addr: " << FLAGS_addr << std::endl;
  std::cout << "FLAGS_dst_path: " << FLAGS_dst_path << std::endl;
  return true;
}

int main(int argc, char* argv[]) {
  if (!ParseClientConfig(argc, argv)) {
    exit(-1);
  }

  if (::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config) != 0) {
    std::cerr << "load client_config failed." << std::endl;
    exit(-1);
  }

  // If the business code is running in trpc pure client mode, the business code needs to be running in the
  // `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return http::demo::Run(); });
}
