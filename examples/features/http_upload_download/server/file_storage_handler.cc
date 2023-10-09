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

namespace http::demo {

// Provides file downloading.
::trpc::Status FileStorageHandler::Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                                       ::trpc::http::Response* rsp) {
  auto fin = std::ifstream(download_src_path_, std::ios::binary);
  if (!fin.is_open()) {
    TRPC_FMT_ERROR("failed to open file: {}", download_src_path_);
    rsp->SetStatus(::trpc::http::ResponseStatus::kInternalServerError);
    return ::trpc::kSuccStatus;
  }

  // Send response content in chunked.
  rsp->SetHeader(::trpc::http::kHeaderTransferEncoding, ::trpc::http::kTransferEncodingChunked);
  auto& writer = rsp->GetStream();
  ::trpc::Status status = writer.WriteHeader();
  if (!status.OK()) {
    TRPC_FMT_ERROR("failed to send response header: {}", status.ToString());
    return ::trpc::kStreamRstStatus;
  }

  std::size_t nwrite{0};
  ::trpc::BufferBuilder buffer_builder;
  for (;;) {
    fin.read(buffer_builder.data(), buffer_builder.SizeAvailable());
    std::size_t n = fin.gcount();
    if (n > 0) {
      ::trpc::NoncontiguousBuffer buffer;
      buffer.Append(buffer_builder.Seal(n));
      status = writer.Write(std::move(buffer));
      if (status.OK()) {
        nwrite += n;
        continue;
      }
      TRPC_FMT_ERROR("failed to write content: {}", status.ToString());
      return ::trpc::kStreamRstStatus;
    } else if (fin.eof()) {
      status = writer.WriteDone();
      if (status.OK()) break;
      TRPC_FMT_ERROR("failed to send write-done: {}", status.ToString());
      return ::trpc::kStreamRstStatus;
    }
    TRPC_FMT_ERROR("failed to read file");
    return ::trpc::kStreamRstStatus;
  }
  TRPC_FMT_INFO("finish providing file, write size: {}", nwrite);
  return ::trpc::kSuccStatus;
}

// Provides file uploading.
::trpc::Status FileStorageHandler::Post(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                                        ::trpc::http::Response* rsp) {
  if (req->HasHeader(::trpc::http::kHeaderContentLength)) {
    TRPC_FMT_DEBUG("the request has Content-Length: {}", req->GetHeader(::trpc::http::kHeaderContentLength));
  } else {
    TRPC_FMT_DEBUG("the request has no Content-Length, may be chunked");
  }

  auto fout = std::ofstream(upload_dst_path_, std::ios::binary);
  if (!fout.is_open()) {
    TRPC_FMT_ERROR("failed to open file: {}", download_src_path_);
    rsp->SetStatus(::trpc::http::ResponseStatus::kInternalServerError);
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status status;
  auto& reader = req->GetStream();
  constexpr std::size_t kBufferSize{1024 * 1024};
  std::size_t nread{0};
  for (;;) {
    ::trpc::NoncontiguousBuffer buffer;
    status = reader.Read(buffer, kBufferSize);
    if (status.OK()) {
      nread += buffer.ByteSize();
      for (const auto& block : buffer) {
        fout.write(block.data(), block.size());
      }
      continue;
    } else if (status.StreamEof()) {
      break;
    }
    TRPC_FMT_ERROR("failed to read request content: {}", status.ToString());
    return ::trpc::kStreamRstStatus;
  }

  rsp->SetStatus(::trpc::http::ResponseStatus::kOk);
  auto& writer = rsp->GetStream();
  status = writer.WriteHeader();
  if (!status.OK()) {
    TRPC_FMT_ERROR("failed to send response header: {}", status.ToString());
    return ::trpc::kStreamRstStatus;
  }
  TRPC_FMT_INFO("finish storing the file, read size: {}", nread);
  return ::trpc::kSuccStatus;
}

}  // namespace http::demo
