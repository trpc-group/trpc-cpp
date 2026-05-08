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

#include "examples/features/http_upload_download/server/file_storage_handler.h"


#include <fstream>
#include <chrono>

#include "examples/features/http_upload_download/common/file_hasher.h"

#include "../common/picosha2.h"

struct FileSegmentHeader {
  char filename[128];           // 文件名（null-terminated）
  uint64_t file_size;           // 文件内容字节数
  char hash_hex[64];            // 文件的 SHA256
};
std::string GetBaseName(const std::string& filepath) {
  std::size_t pos = filepath.find_last_of("/\\");
  return (pos != std::string::npos) ? filepath.substr(pos + 1) : filepath;
}

bool enable_limit = true;
constexpr std::size_t rate_limit_bytes_per_sec = 900 * 1024; // 512KB/s

namespace http::demo {

// Provides file downloading.
::trpc::Status FileStorageHandler::Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                                       ::trpc::http::Response* rsp) {

  const std::string full_url = req->GetUrl();
  std::string path_only;
  std::size_t pos = full_url.find('?');

  if (pos != std::string::npos) {
    path_only = full_url.substr(0, pos);  
  } else {
    path_only = full_url;
  }

  if (path_only == "/multi-download") {
    TRPC_FMT_INFO("[Multi-File Download] Start processing");
    return DownloadMultipleFiles(ctx, req, rsp);
  }


  TRPC_FMT_INFO("[Download Request] Received download signal");
  //auto fin = std::ifstream(download_src_path_, std::ios::binary);
  std::ifstream fin(download_src_path_, std::ios::binary | std::ios::ate);
  if (!fin.is_open()) {
    TRPC_FMT_ERROR("[Download Request] failed to open file: {}", download_src_path_);
    rsp->SetStatus(::trpc::http::ResponseStatus::kInternalServerError);
    return ::trpc::kSuccStatus;
  }
  std::size_t file_size = fin.tellg();  // 获取总字节数
  fin.seekg(0, std::ios::beg);          // 重置文件指针以便后续读取
  rsp->SetHeader("X-File-Length", std::to_string(file_size));


  std::string hash_tex = CalculateSHA256(download_src_path_);
  rsp->SetHeader("X-File-SHA256",hash_tex);
  TRPC_FMT_INFO("[Download Request] Calculated SHA256 of downloaded file: {}", hash_tex);

  // Send response content in chunked.
  rsp->SetHeader(::trpc::http::kHeaderTransferEncoding, ::trpc::http::kTransferEncodingChunked);
  auto& writer = rsp->GetStream();
  ::trpc::Status status = writer.WriteHeader();
  if (!status.OK()) {
    TRPC_FMT_ERROR("[Download Request] failed to send response header: {}", status.ToString());
    return ::trpc::kStreamRstStatus;
  }
 
  std::size_t nwrite{0};
  ::trpc::BufferBuilder buffer_builder;

  std::size_t sent_bytes = 0;
  auto last_send_time = std::chrono::steady_clock::now();
  for (;;) {

    fin.read(buffer_builder.data(), buffer_builder.SizeAvailable()); // 对当前发送内容进行哈希更新
    std::size_t n = fin.gcount();
    if (n > 0) {


      ::trpc::NoncontiguousBuffer buffer;
      buffer.Append(buffer_builder.Seal(n)); 
      status = writer.Write(std::move(buffer));
      if (status.OK()) {
        nwrite += n;
        sent_bytes+=n;
        //limit
        /*std::chrono::milliseconds delay(
          static_cast<int>(1000.0 * n / rate_limit_bytes_per_sec));
        std::this_thread::sleep_for(delay);*/
        // 限速控制：根据实际发送速率动态延迟
        auto now = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_send_time).count();

        if (enable_limit && duration_ms > 0) {
          std::size_t actual_rate = sent_bytes * 1000 / duration_ms; // bytes/sec
          if (actual_rate > rate_limit_bytes_per_sec) {
            std::size_t expected_duration = sent_bytes * 1000 / rate_limit_bytes_per_sec;
            std::size_t extra_delay = expected_duration > duration_ms ? expected_duration - duration_ms : 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(extra_delay));
          }
          last_send_time = std::chrono::steady_clock::now();
          sent_bytes = 0;
        }


        continue;
      }
      TRPC_FMT_ERROR("[Download Request] failed to write content: {}", status.ToString());
      return ::trpc::kStreamRstStatus;
    } else if (fin.eof()) {
      status = writer.WriteDone();
      if (status.OK()) break;
      TRPC_FMT_ERROR("[Download Request] failed to send write-done: {}", status.ToString());
      return ::trpc::kStreamRstStatus;
    }
    TRPC_FMT_ERROR("[Download Request] failed to read file");
    return ::trpc::kStreamRstStatus;
  }
  
  TRPC_FMT_INFO("[Download Request] finish providing file, write size: {}", nwrite);
  return ::trpc::kSuccStatus;
}

// Provides file uploading.
// 服务端文件上传处理逻辑
::trpc::Status FileStorageHandler::Post(const ::trpc::ServerContextPtr& ctx,
                                        const ::trpc::http::RequestPtr& req,
                                        ::trpc::http::Response* rsp) {
  std::size_t total_size = 0;

  // 从请求头中获取 Content-Length，如果有的话
  if (req->HasHeader(::trpc::http::kHeaderContentLength)) {
    total_size = std::stoull(req->GetHeader(::trpc::http::kHeaderContentLength));
    TRPC_FMT_INFO("[Upload Request] Total upload size: {} bytes", total_size);
  } else {
    TRPC_FMT_INFO("[Upload Request] No Content-Length header, possibly chunked upload");
  }

  // 打开目标文件用于接收上传内容
  auto fout = std::ofstream(upload_dst_path_, std::ios::binary);
  if (!fout.is_open()) {
    TRPC_FMT_ERROR("[Upload Request] failed to open file: {}", download_src_path_);
    rsp->SetStatus(::trpc::http::ResponseStatus::kInternalServerError);
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status status;
  auto& reader = req->GetStream();  // 获取 HTTP 请求体的流
  constexpr std::size_t kBufferSize{1024 * 1024};  // 每次读取 1MB
  std::size_t nread{0};
  auto start_time = std::chrono::steady_clock::now();  // 上传开始时间

  // 数据读取循环
  for (;;) {
    ::trpc::NoncontiguousBuffer buffer;
    status = reader.Read(buffer, kBufferSize);
    if (status.OK()) {
      // 更新读取字节数
      nread += buffer.ByteSize();

      // 写入磁盘
      for (const auto& block : buffer) {
        fout.write(block.data(), block.size());
      }

      // 上传进度日志
      TRPC_FMT_INFO_IF(TRPC_EVERY_N(100), "Uploaded {} bytes so far", nread);
      if (total_size > 0) {
          const int bar_width = 50; // 控制进度条宽度
          double progress = static_cast<double>(nread) / total_size;
          int pos = static_cast<int>(bar_width * progress);

          std::stringstream ss;
          ss << "[Upload Request] Progress: [";

          for (int i = 0; i < bar_width; ++i) {
              if (i < pos)
                  ss << "█"; // 已完成部分
              else
                  ss << " "; // 未完成部分
          }

          ss << "] " << std::fixed << std::setprecision(2) << (progress * 100.0) << "%";

          TRPC_FMT_INFO("{}", ss.str());
      }

/*if (total_size > 0) {
        double progress = static_cast<double>(nread) / total_size * 100;
        TRPC_FMT_INFO("[Upload Request] Uploaded progress: {:.2f}%", progress);
      }*/
      continue;
    } else if (status.StreamEof()) {
      // 上传结束时记录速率
      auto end_time = std::chrono::steady_clock::now();
      auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
      double speed_mb = static_cast<double>(nread) / 1024 / 1024 / (duration_ms / 1000.0);
      TRPC_FMT_INFO("[Upload Request] Upload complete: {} bytes in {} ms, avg speed: {:.2f} MB/s", nread, duration_ms, speed_mb);
      break;
    }

    // 出现异常
    TRPC_FMT_ERROR("[Upload Request] failed to read request content: {}", status.ToString());
    return ::trpc::kStreamRstStatus;
  }

  // 提取客户端传来的哈希值用于比对
  std::string expected_hash;
  if (req->HasHeader("X-File-Hash")) {
    expected_hash = req->GetHeader("X-File-Hash");
    TRPC_FMT_INFO("[Upload Request] Expected file hash from client: {}", expected_hash);
  }

  // 重新计算服务端收到的文件的哈希值
  std::string actual_hash = CalculateSHA256(upload_dst_path_);
  TRPC_FMT_INFO("[Upload Request] Calculated SHA256 of uploaded file: {}", actual_hash);

  // 校验结果比对
  if (!expected_hash.empty() && actual_hash != expected_hash) {
    TRPC_FMT_ERROR("[Upload Request] ❌File hash mismatch! Integrity check failed.");
    rsp->SetStatus(::trpc::http::ResponseStatus::kBadRequest);
    return ::trpc::kSuccStatus;
  }
  else{
    TRPC_FMT_INFO("[Upload Request] ✅ SHA256 verified successfully");
  }
  // 响应成功
  rsp->SetStatus(::trpc::http::ResponseStatus::kOk);
  auto& writer = rsp->GetStream();
  status = writer.WriteHeader();
  if (!status.OK()) {
    TRPC_FMT_ERROR("[Upload Request] failed to send response header: {}", status.ToString());
    return ::trpc::kStreamRstStatus;
  }

  TRPC_FMT_INFO("[Upload Request] File upload and hash verification complete, read size: {}", nread);
  return ::trpc::kSuccStatus;
}


#include <string>
#include <sstream>
#include <unordered_map>

std::string ParseQueryParameter(const std::string& url, const std::string& key) {
  // 提取 ? 后面的 query 字符串
  std::size_t qpos = url.find('?');
  if (qpos == std::string::npos || qpos + 1 >= url.size()) return "";

  std::string query = url.substr(qpos + 1);
  std::stringstream qs_stream(query);
  std::string kv;

  while (std::getline(qs_stream, kv, '&')) {
    std::size_t eq_pos = kv.find('=');
    if (eq_pos == std::string::npos) continue;

    std::string param_key = kv.substr(0, eq_pos);
    std::string param_val = kv.substr(eq_pos + 1);

    if (param_key == key) return param_val;
  }

  return "";  // 未找到指定参数
}

std::string ByteDump(const char* data, std::size_t size) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < size; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << (static_cast<int>(data[i]) & 0xff) << " ";
  }
  return oss.str();
}

::trpc::Status FileStorageHandler::DownloadMultipleFiles(
    const ::trpc::ServerContextPtr& ctx,
    const ::trpc::http::RequestPtr& req,
    ::trpc::http::Response* rsp) {

  // 设置为 chunked 编码响应
  rsp->SetHeader(::trpc::http::kHeaderTransferEncoding, ::trpc::http::kTransferEncodingChunked);
  auto& writer = rsp->GetStream();

  ::trpc::Status status = writer.WriteHeader();
  if (!status.OK()) {
    TRPC_FMT_ERROR("[Multi-File Download] failed to write response header");
    return ::trpc::kStreamRstStatus;
  }


  std::string files_param = ParseQueryParameter(req->GetUrl(), "files");
  std::vector<std::string> file_paths;

  std::stringstream ss(files_param);
  std::string item;
  while (std::getline(ss, item, ',')) {
    if (!item.empty()) {
      file_paths.push_back( item);
      TRPC_FMT_INFO("{}", item);
    }
  }

  for (const auto& path : file_paths) {
    std::string hash_before = CalculateSHA256(path);
    TRPC_FMT_INFO("[Multi-File Download] 📦 Pre-read hash of {}: {}", path, hash_before);

    std::ifstream fin;
    fin.open(path, std::ios::binary | std::ios::ate);
 
    //std::ifstream fin(path, std::ios::binary | std::ios::ate);
    if (!fin.is_open()) {
      TRPC_FMT_ERROR("[Multi-File Download] failed to open file: {}", path);
      continue; // 可跳过或直接返回错误
    }

    std::size_t file_size = fin.tellg();
    fin.clear();            // 清除状态
    fin.seekg(0, std::ios::beg); // 保证从头读取

    // 构造 header
    FileSegmentHeader header;
    std::memset(&header, 0, sizeof(header));
    //TRPC_FMT_INFO("📦 filename before strncpy: {}", GetBaseName(path).c_str());
    std::strncpy(header.filename, GetBaseName(path).c_str(), sizeof(header.filename));
    //TRPC_FMT_INFO("📦 header.filername after strncpy: {}", std::string(header.filename));
    std::string hash_hex = CalculateSHA256(path);
    //TRPC_FMT_INFO("📦 hash_hex before strncpy: {}", hash_hex);
    std::strncpy(header.hash_hex, hash_hex.c_str(), sizeof(header.hash_hex));
    //std::snprintf(header.hash_hex, sizeof(header.hash_hex), "%s", hash_hex.c_str());

    //TRPC_FMT_INFO("📦 header.hash_hex after strncpy: {}", std::string(header.hash_hex));
    //TRPC_FMT_INFO("🧬 header.hash_hex raw dump: {}", ByteDump(header.hash_hex, sizeof(header.hash_hex)));

    header.file_size = file_size;

    // 发送 header
    ::trpc::NoncontiguousBufferBuilder builder;


    builder.Append(static_cast<const void*>(&header), sizeof(header));
    writer.Write(builder.DestructiveGet());




    for (;;) {

      

      ::trpc::BufferBuilder buffer_builder;
      std::size_t nwrite{0};
      fin.read(buffer_builder.data(), buffer_builder.SizeAvailable());
      std::size_t n = fin.gcount();
      if (n > 0) {
        ::trpc::NoncontiguousBuffer buffer;
        buffer.Append(buffer_builder.Seal(n));
        //TRPC_FMT_INFO("📦 First 16 bytes of chunk: {}", ByteDump(buffer.begin()->data(), std::min<size_t>(16, buffer.begin()->size())));

        status = writer.Write(std::move(buffer));
        if (!status.OK()) {
          TRPC_FMT_ERROR("failed to write content: {}", status.ToString());
          return ::trpc::kStreamRstStatus;
        }
        nwrite += n;
      } else if (fin.eof()) {
        break;
      } else {
        TRPC_FMT_ERROR("failed to read file: {}", path);
        return ::trpc::kStreamRstStatus;
      }
    }

    fin.close();
    TRPC_FMT_INFO("✅ Sent {}, size: {}, hash: {}", header.filename, header.file_size, hash_hex);
  }

  writer.WriteDone();
  return ::trpc::kSuccStatus;
}

}


