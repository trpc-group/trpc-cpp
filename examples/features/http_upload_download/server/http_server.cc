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

#include <utility>

#include "gflags/gflags.h"

#include "trpc/common/trpc_app.h"
#include "trpc/server/http_service.h"
#include "trpc/util/http/routes.h"

#include "examples/features/http_upload_download/server/file_storage_handler.h"

DEFINE_string(download_src_path, "download_src.bin", "set a path which saves a file will be download by client");
DEFINE_string(upload_dst_path, "upload_dst.bin", "set a path to save a file uploaded by client");

namespace http::demo {

class HttpdServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    auto file_storage_handler = std::make_shared<FileStorageHandler>(FLAGS_upload_dst_path, FLAGS_download_src_path);





    auto SetHttpRoutes = [file_storage_handler](::trpc::http::HttpRoutes& r) -> void {
      // Provides file downloading.
      r.Add(::trpc::http::MethodType::GET, ::trpc::http::Path("/download"), file_storage_handler);
      // Provides file uploading.
      r.Add(::trpc::http::MethodType::POST, ::trpc::http::Path("/upload"), file_storage_handler);

       // 多文件下载的路由注册，注意 lambda 捕获 & 返回类型
  
      r.Add(::trpc::http::MethodType::GET, ::trpc::http::Path("/multi-download"), file_storage_handler);
    };

    auto http_service = std::make_shared<::trpc::HttpService>();
    http_service->SetRoutes(SetHttpRoutes);

    RegisterService("http_upload_download_service", http_service);

    return 0;
  }

  void Destroy() override {}
};

}  // namespace http::demo

int main(int argc, char** argv) {
  http::demo::HttpdServer httpd_server;

  httpd_server.Main(argc, argv);
  httpd_server.Wait();

  return 0;
}
