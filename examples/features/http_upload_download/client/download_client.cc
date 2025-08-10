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
#include "../common/picosha2.h"  
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
DEFINE_string(dst_path, "download_dst.bin", "file path to store the content which will be downloaded from the server");

DEFINE_bool(enable_download_hash, true, "Enable SHA256 hash verification during download");
DEFINE_bool(enable_download_limit, true, "Enable SHA256 hash verification during download");
DEFINE_string(multi_download_dir, "./downloads", "Directory to save multiple downloaded files");


struct FileSegmentHeader {
  char filename[128];           // 文件名（null-terminated）
  uint64_t file_size;           // 文件内容字节数
  char hash_hex[64];            // 文件的 SHA256
};

std::string ByteDump(const void* data, std::size_t len) {
  const unsigned char* bytes = static_cast<const unsigned char*>(data);
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');

  for (std::size_t i = 0; i < len; ++i) {
    oss << std::setw(2) << static_cast<int>(bytes[i]) << " ";
    if ((i + 1) % 16 == 0) oss << "\n"; // 每16字节换行
  }

  return oss.str();
}
namespace http::demo {
using HttpServiceProxyPtr = std::shared_ptr<::trpc::http::HttpServiceProxy>;


bool Download(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string dst_path) {
  // 打开输出文件（保存下载内容）
  auto fout = std::ofstream(dst_path, std::ios::binary);
  if (!fout.is_open()) {
    TRPC_FMT_ERROR("[Download Request]  failed to open file, file_path:{}", dst_path);
    return false;
  }

  // 创建客户端上下文并设置超时时间
  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(50000);

  // 通过 HTTP GET 创建流式连接
  auto stream = proxy->Get(ctx, url);


  if (!stream.GetStatus().OK()) {
    TRPC_FMT_ERROR("[Download Request]  failed to create client stream");
    return false;
  }

  // 读取响应头
  int http_status = 0;
  ::trpc::http::HttpHeader http_header;
  ::trpc::Status status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    TRPC_FMT_ERROR("[Download Request]  failed to read http header: {}", status.ToString());
    return false;
  } else if (http_status != ::trpc::http::ResponseStatus::kOk) {
    TRPC_FMT_ERROR("[Download Request]  http response status:{}", http_status);
    return false;
  }
  
  std::size_t total_size = 0;
  if (http_header.Has("X-File-Length")) {
    total_size = std::stoull(http_header.Get("X-File-Length"));
    TRPC_FMT_INFO("[Download Request] 📏 Content-Length: {} bytes", total_size);
  } else {
    TRPC_FMT_WARN("[Download Request] ⚠️ No Content-Length found, can't compute progress");
  }


  // 定义每次读取的 buffer 大小
  constexpr std::size_t kBufferSize{1024 * 1024};
  size_t nread{0};
  auto start_time = std::chrono::steady_clock::now();

  // 读取响应内容块（直到 EOF）
  for (;;) {
    ::trpc::NoncontiguousBuffer buffer;
    if(FLAGS_enable_download_limit)
      status =  stream.Read(buffer, kBufferSize, std::chrono::seconds(3));
    else
      status = stream.Read(buffer, kBufferSize);
   
    if (status.OK()) {
      nread += buffer.ByteSize();

      if (total_size > 0) {//下载：速率 hasher


/*
1.支持一次上传多个文件（使用 multipart 或多路流）

2.可以在每个文件流中记录独立的上传进度

3.建立文件元信息结构（如文件名、大小、用户ID）


4.按分片序号或 byte range 进行断点续传

5.支持临时存储和标记上传状态（已完成/未完成）

6.与客户端配合实现分布式大文件传输


提供 streaming response（边上传边返回进度）

支持 WebSocket 或 SSE（服务端事件）用于浏览器反馈

可打通到前端 UI 组件显示进度条和速率曲线


集成 Prometheus 导出指标：

上传文件总数

总流量

当前并发上传数

单文件上传耗时分布

可用于 Grafana 进行可视化

限速上传（流控机制）

超时断开连接

大文件分区限流，防止拖慢整个服务

上传后自动移动到分目录（按日期/用户）

增加文件元信息存储（如 SQLite、Redis）

可实现上传后的异步处理队列（如压缩、转码）
*/
        const int bar_width = 50; // 进度条宽度
        double progress = static_cast<double>(nread) / total_size;
        int pos = static_cast<int>(bar_width * progress);

        std::stringstream ss;
        ss << "[Download Request] 📦 Progress: [";
        for (int i = 0; i < bar_width; ++i) {
          ss << (i < pos ? "█" : " ");
        }
        ss << "] " << std::fixed << std::setprecision(2) << (progress * 100.0) << "%";

        TRPC_FMT_INFO("{}", ss.str());
      }

      // 将 buffer 中的每个块写入文件
      for (const auto& block : buffer) {
        fout.write(block.data(), block.size());
      }
      continue;
    } else if (status.StreamEof()) {
      // 读取完毕，跳出循环
      auto end_time = std::chrono::steady_clock::now();
      auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
      double speed_mb = static_cast<double>(nread) / (1024 * 1024) / (duration_ms / 1000.0);

      TRPC_FMT_INFO("[Download Request] 📥 Download complete: {} bytes in {} ms, avg speed: {:.2f} MB/s", nread, duration_ms, speed_mb);

      break;
    }
    else if (status.ToString().find("timeout") != std::string::npos){
      TRPC_FMT_WARN("[Download Request] ⚠️ Read timed out, retrying...");

      continue;
    }
    TRPC_FMT_ERROR("[Download Request]  failed to read response content from client : {}", status.ToString());
    return false;
  }

  TRPC_FMT_INFO("[Download Request]  finish downloading, read size: {}", nread);

  // 确保文件全部写入磁盘（防止哈希不一致）
  fout.close();

  // 如果开启哈希验证并且响应头中包含 hash 字段
  std::string expected_hash;
  if (FLAGS_enable_download_hash && http_header.Has("X-File-SHA256")) {
    expected_hash = http_header.Get("X-File-SHA256");

    // 对本地下载后的文件进行 SHA256 计算
    std::string actual_hash = CalculateSHA256(dst_path);
    TRPC_FMT_INFO("downloaded client: {}", actual_hash);

    // 比较本地哈希与服务端发来的 hash
    if (expected_hash != actual_hash) {
      TRPC_FMT_ERROR("[Download Request] ❌SHA256 mismatch! File may be corrupted.");
      return false;
    } else {
      TRPC_FMT_INFO("[Download Request] ✅ SHA256 verified successfully ");
    }
  } else {
    // 如果未收到 hash，则跳过校验
    TRPC_FMT_WARN("[Download Request]  No SHA256 header received from server, skipping verification...");
  }

  return true;
}



bool DownloadMultipleFiles(const HttpServiceProxyPtr& proxy, const std::string& url) {

  
  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(10000);
  auto stream = proxy->Get(ctx, url);
  if (!stream.GetStatus().OK()) {
    TRPC_FMT_ERROR("[Multi-File Download] failed to create client stream");
    return false;
  }

  int http_status = 0;
  ::trpc::http::HttpHeader http_header;
  if (!stream.ReadHeaders(http_status, http_header).OK()) {
    TRPC_FMT_ERROR("[Multi-File Download] failed to read http header");
    return false;
  }
  int file_index = 0;  // 文件计数器

  for (;;) {


    // 1. 接收 header
    ::trpc::NoncontiguousBuffer header_buf;
    if (!stream.Read(header_buf, sizeof(FileSegmentHeader)).OK()) {
      TRPC_FMT_INFO("[Multi-File Download] 📥 Finished reading {} file(s).", file_index);
      break;
    }

    FileSegmentHeader header;
    if (header_buf.ByteSize() < sizeof(FileSegmentHeader)) {
      TRPC_FMT_ERROR(" [Multi-File Download] Incomplete header received, size = {}", header_buf.ByteSize());
      return false;
    }

    std::memcpy(&header, header_buf.begin()->data(), sizeof(FileSegmentHeader));
    /*
        for (const auto& block : header_buf) {
          TRPC_FMT_INFO("📏 Header block size: {}", block.size());
        }
    */

    file_index++;

    // 2. 准备写入文件
    std::string full_path = "download/" + std::string(header.filename);
    std::ofstream fout(full_path, std::ios::binary);
    if (!fout.is_open()) {
      TRPC_FMT_ERROR("[Multi-File Download] cannot open file {}", header.filename);
      return false;
    }
    //TRPC_FMT_INFO("🧬 Raw header bytes: {}",ByteDump(header_buf.begin()->data(), sizeof(FileSegmentHeader)));
    
    std::size_t received = 0;
    while (received < header.file_size) {
      ::trpc::NoncontiguousBuffer data_buf;
      size_t chunk_size = std::min<size_t>(1024 * 1024, header.file_size - received);
      if (!stream.Read(data_buf, chunk_size).OK()) return false;
      

      for (const auto& block : data_buf) {
        fout.write(block.data(), block.size());
        received += block.size();
      }
    }
    TRPC_ASSERT(received == header.file_size);
    TRPC_FMT_INFO("[Multi-File Download] 🔢 Received bytes: {}, Expected: {}", received, header.file_size);

    fout.flush();  // 强制刷新缓冲区
    fout.close();
    
    TRPC_FMT_INFO("[Multi-File Download] 🔢  File saved: {}, size: {}", header.filename, header.file_size);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // 3. 校验哈希（可选）
    if (FLAGS_enable_download_hash) {
      std::string actual_hash = CalculateSHA256(full_path);
      TRPC_FMT_INFO("[Multi-File Download] 🔍 Verifying {}, expected={}, actual={}", header.filename, header.hash_hex, actual_hash);
      //TRPC_FMT_INFO("🔍 Calculating hash for saved file at path: {}", full_path);

      if (actual_hash != std::string(header.hash_hex)) {
        TRPC_FMT_ERROR("[Multi-File Download] ❌ Hash mismatch for {}", header.filename);
        return false;
      } else {
        TRPC_FMT_INFO("[Multi-File Download] ✅ Verified {}", header.filename);
      }
    }
  }
  
  return true;
}


// 主业务入口，执行所有 HTTP 调用任务
int Run() {
  bool final_ok{true};  // 用于汇总所有任务是否执行成功

  // 定义一个结构体，表示每个 HTTP 调用任务
  struct http_calling_args_t {
    std::string calling_name;            // 描述任务名称（用于日志）
    std::function<bool()> calling_executor; // 实际执行的函数（返回 true / false 表示成功与否）
    bool ok;                             // 执行结果（由执行器函数更新）
  };

  // 初始化 HTTP 客户端配置项
  ::trpc::ServiceProxyOption option;
  option.name = FLAGS_service_name;         // 服务名（来自命令行参数）
  option.codec_name = "http";               // 使用 HTTP 协议编码
  option.network = "tcp";                   // 底层传输使用 TCP
  option.conn_type = "long";                // 长连接
  option.timeout = 5000;                    // 请求超时时间（ms）
  option.selector_name = "direct";          // 选择策略：直接连接目标地址
  option.target = FLAGS_addr;               // 服务地址（host:port）

  // 创建 HTTP 客户端代理对象
  auto http_client = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(FLAGS_service_name, option);

  // 任务列表：
  std::vector<http_calling_args_t> callings{

      {
        "download a file from the server",   // 描述任务
        [http_client, dst_path = FLAGS_dst_path]() {
          return Download(http_client, "http://example.com/download", dst_path); // 实际执行器
        },
        false                                // 初始状态未执行
      },
       {"download multiple files from the server",
        [http_client]() {
          return DownloadMultipleFiles(http_client, "http://example.com/multi-download?files=download_src.bin,download_dst.bin");
        },
        false},
  };

  // 初始化 fiber latch 等待器，用于等待所有任务完成
  auto latch_count = static_cast<std::ptrdiff_t>(callings.size());
  ::trpc::FiberLatch callings_latch{latch_count};

  // 启动所有任务，每个任务运行在一个独立的 fiber 中
  for (auto& c : callings) {
    ::trpc::StartFiberDetached([&callings_latch, &c]() {
      c.ok = c.calling_executor();  // 执行任务，更新状态
      callings_latch.CountDown();   // 当前任务完成，通知 latch 减一
    });
  }

  // 等待所有任务完成
  callings_latch.Wait();

  // 输出每个任务的执行结果，并统计整体是否成功
  for (const auto& c : callings) {
    final_ok &= c.ok;  // 只要有一个任务失败，最终结果为 false
    std::cout << "name: " << c.calling_name << ", ok: " << c.ok << std::endl;
  }

  // 输出整体执行结果
  std::cout << "final result of http calling: " << final_ok << std::endl;

  // 返回值决定是否退出异常：0 表示全部成功，-1 表示有失败
  return final_ok;

}  // namespace http::demo
}
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


