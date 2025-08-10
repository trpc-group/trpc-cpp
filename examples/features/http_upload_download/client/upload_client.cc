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

#include "examples/features/http_upload_download/common/file_hasher.h"


DEFINE_string(service_name, "http_upload_download_client", "callee service name");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "");
DEFINE_string(addr, "127.0.0.1:24858", "ip:port");
DEFINE_string(src_path, "upload_src.bin", "file path to store the content which will be upload to the server");
DEFINE_bool(use_chunked, true, "send request content in chunked");

DEFINE_bool(enable_upload_hash, true, "Enable SHA256 hash verification during upload");



namespace http::demo {
using HttpServiceProxyPtr = std::shared_ptr<::trpc::http::HttpServiceProxy>;

bool UploadWithChunked(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string src_path) {
  TRPC_FMT_INFO("[Upload Request] upload a file with chunked");
  auto fin = std::ifstream(src_path, std::ios::binary);
  if (!fin.is_open()) {
    TRPC_FMT_ERROR("[Upload Request] failed to open file, file_path: {}", src_path);
    return false;
  }

  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(5000);
  // Send request content in chunked.
  ctx->SetHttpHeader(::trpc::http::kHeaderTransferEncoding, ::trpc::http::kTransferEncodingChunked);

  if (FLAGS_enable_upload_hash) {
    std::string hash = CalculateSHA256(src_path);

    ctx->SetHttpHeader("X-File-Hash", hash);
  }

  // Creates HTTP stream.
  auto stream = proxy->Post(ctx, url);
  if (!stream.GetStatus().OK()) {
    TRPC_FMT_ERROR("[Upload Request] failed to create client stream");
    return false;
  }

  // Sends request content.
  std::size_t nwrite{0};
  ::trpc::BufferBuilder buffer_builder;
  for (;;) {
    trpc::Status status;
    fin.read(buffer_builder.data(), buffer_builder.SizeAvailable());
    std::size_t n = fin.gcount();
    if (n > 0) {
      ::trpc::NoncontiguousBuffer buffer;
      buffer.Append(buffer_builder.Seal(n));
      status = stream.Write(std::move(buffer));
      if (status.OK()) {
        nwrite += n;
        continue;
      }
      TRPC_FMT_ERROR("[Upload Request] failed to write request content: {}", status.ToString());
      return false;
    } else if (fin.eof()) {
      status = stream.WriteDone();
      if (status.OK()) break;
      TRPC_FMT_ERROR("failed to send write-done: {}", status.ToString());
      return false;
    }
    TRPC_FMT_ERROR("[Upload Request] failed to read file");
    return false;
  }

  int http_status = 0;
  ::trpc::http::HttpHeader http_header;
  // Reads response header.
  ::trpc::Status status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    TRPC_FMT_ERROR("[Upload Request] failed to read http header: {}", status.ToString());
    return false;
  } else if (http_status != ::trpc::http::ResponseStatus::kOk) {
    TRPC_FMT_ERROR("[Upload Request] http response status: {}", http_status);
    return false;
  }

  TRPC_FMT_INFO("[Upload Request] finish uploading, write size: {}", nwrite);
  return true;
}

bool UploadWithContentLength(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string src_path) {
  TRPC_FMT_INFO("[Upload Request] upload a file with content-length");
  auto fin = std::ifstream(src_path, std::ios::binary);
  if (!fin.is_open()) {
    TRPC_FMT_ERROR("failed to open file, file_path: {}", src_path);
    return false;
  }
  // Gets the file size.
  std::size_t fsize = [](std::ifstream& fin) -> std::size_t {
    std::streampos begin_pos{fin.tellg()};
    fin.seekg(0, std::ios::end);
    std::size_t fsize = (fin.tellg() - begin_pos);
    fin.seekg(begin_pos);
    return fsize;
  }(fin);
  if (fsize <= 0) {
    TRPC_FMT_ERROR("[Upload Request] failed to read file size, file_path: {}", src_path);
    return false;
  }

  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(5000);
  // Send request content with content-length.
  ctx->SetHttpHeader(::trpc::http::kHeaderContentLength, std::to_string(fsize));

  if (FLAGS_enable_upload_hash) {
    std::string hash = CalculateSHA256(src_path);
    ctx->SetHttpHeader("X-File-Hash", hash);
  }

  // Creates HTTP stream.
  auto stream = proxy->Post(ctx, url);
  if (!stream.GetStatus().OK()) {
    TRPC_FMT_ERROR("[Upload Request] failed to create client stream");
    return false;
  }

  ::trpc::Status status;
  // Sends request content.
  std::size_t nwrite{0};
  ::trpc::BufferBuilder buffer_builder;
  for (;;) {
    fin.read(buffer_builder.data(), buffer_builder.SizeAvailable());
    std::size_t n = fin.gcount();
    if (n > 0) {
      ::trpc::NoncontiguousBuffer buffer;
      buffer.Append(buffer_builder.Seal(n));
      status = stream.Write(std::move(buffer));
      if (status.OK()) {
        nwrite += n;
        continue;
      }
      TRPC_FMT_ERROR("[Upload Request] failed to write request content: {}", status.ToString());
      return false;
    } else if (fin.eof()) {
      status = stream.WriteDone();
      if (status.OK()) break;
      TRPC_FMT_ERROR("failed to send write-done: {}", status.ToString());
      return false;
    }
    TRPC_FMT_ERROR("[Upload Request] failed to read file");
    return false;
  }

  int http_status = 0;
  ::trpc::http::HttpHeader http_header;
  // Reads response header.
  status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    TRPC_FMT_ERROR("[Upload Request] failed to read http header: {}", status.ToString());
    return false;
  } else if (http_status != ::trpc::http::ResponseStatus::kOk) {
    TRPC_FMT_ERROR("[Upload Request] http response status: {}", http_status);
    return false;
  }

  TRPC_FMT_INFO("[Upload Request] finish uploading, write size: {}", nwrite);
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
      {"upload a file to server",
       [http_client, src_path = FLAGS_src_path, chunked = FLAGS_use_chunked]() {
         if (chunked) {
           return UploadWithChunked(http_client, "http://example.com/upload", src_path);
         } else {
           return UploadWithContentLength(http_client, "http://example.com/upload", src_path);
         }
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
  std::cout << "FLAGS_src_path: " << FLAGS_src_path << std::endl;
  std::cout << "FLAGS_use_chunked: " << FLAGS_use_chunked << std::endl;
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
