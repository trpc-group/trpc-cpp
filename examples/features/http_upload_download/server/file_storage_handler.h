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

#include "trpc/util/http/stream/http_stream_handler.h"

namespace http::demo {

class FileStorageHandler : public ::trpc::http::HttpStreamHandler {
 public:
  FileStorageHandler(std::string dst, std::string src)
      : upload_dst_path_(std::move(dst)), download_src_path_(std::move(src)) {}

  ~FileStorageHandler() override = default;

  // Provides file downloading.
  ::trpc::Status Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                     ::trpc::http::Response* rsp) override;

  // Provides file uploading.
  ::trpc::Status Post(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                      ::trpc::http::Response* rsp) override;
                      
  ::trpc::Status DownloadMultipleFiles(
    const ::trpc::ServerContextPtr& ctx,
    const ::trpc::http::RequestPtr& req,
    ::trpc::http::Response* rsp);


 private:
  std::string upload_dst_path_{"upload_dst.bin"};
  std::string download_src_path_{"download_src.bin"};
};

}  // namespace http::demo
