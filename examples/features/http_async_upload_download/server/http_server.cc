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
#include <utility>

#include "gflags/gflags.h"

#include "trpc/common/trpc_app.h"
#include "trpc/future/future_utility.h"
#include "trpc/server/http/http_async_stream_service.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/random.h"
#include "trpc/util/string_helper.h"

DEFINE_string(download_src_path, "async_download_src.bin", "set a path which saves a file will be download by client");
DEFINE_string(upload_dst_path, "async_upload_dst.bin", "set a path to save a file uploaded by client");

namespace http::demo {

// Provides file uploading.
::trpc::Future<> AsyncUploadWithChunk(const ::trpc::ServerContextPtr& ctx,
                                      ::trpc::stream::HttpServerAsyncStreamReaderWriterPtr rw) {
  struct State {
    std::size_t counter{0};
    std::size_t bytes{0};
  };
  auto file = std::make_shared<std::ofstream>(FLAGS_upload_dst_path);
  auto state = std::make_shared<State>();
  const ::trpc::stream::HttpRequestLine& line = rw->GetRequestLine();
  TRPC_FMT_INFO("handle http request, method: {}, path: {}, version: {}", line.method, line.request_uri, line.version);
  return rw->ReadHeader().Then([rw, state, file](::trpc::http::HttpHeader&& header) {
    for (const auto& [k, v] : header.Pairs()) {
      TRPC_FMT_INFO("http key-value, {}: {}", k, v);
    }
    return ::trpc::DoUntil([rw, state, file]() {
             return rw->ReadChunk().Then([rw, state, file](::trpc::NoncontiguousBuffer&& data) {
               if (!data.Empty()) {
                 (*file) << trpc::FlattenSlow(data);
                 state->bytes += data.ByteSize();
                 ++state->counter;
                 return ::trpc::MakeReadyFuture<bool>(true);
               } else {
                 TRPC_FMT_INFO("Read eof, chunk num: {}, chunk bytes: {}", state->counter, state->bytes);
                 file->close();
                 return trpc::MakeReadyFuture<bool>(false);
               }
             });
           })
        .Then([rw, state]() {
          ::trpc::http::HttpResponse rsp;
          rsp.SetContent(::trpc::Format("Read eof, chunk num: {}, chunk bytes: {}", state->counter, state->bytes));
          return ::trpc::stream::WriteFullResponse(rw, std::move(rsp));
        });
  });
}

trpc::Future<> AsyncDownloadWithChunk(const ::trpc::ServerContextPtr& ctx,
                                      ::trpc::stream::HttpServerAsyncStreamReaderWriterPtr rw) {
  struct State {
    std::size_t counter{0};
    std::size_t bytes{0};
  };
  auto file = std::make_shared<std::ifstream>(FLAGS_download_src_path, std::ifstream::binary);
  auto state = std::make_shared<State>();
  return ::trpc::stream::ReadFullRequest(rw).Then([rw, file, state](::trpc::http::HttpRequestPtr req) {
    TRPC_FMT_INFO(req->SerializeToString());
    return rw->WriteStatusLine({.status_code = 200, .status_text = "OK", .version = req->GetVersion()})
        .Then([rw, state, file]() {
          ::trpc::http::HttpHeader header;
          header.Add("Transfer-Encoding", "chunked");
          return rw->WriteHeader(std::move(header)).Then([rw, state, file]() {
            return ::trpc::DoUntil([rw, state, file]() {
              if (!file->eof() && file) {
                int random_read_bytes = trpc::Random<int>(1, 10240);
                char* fbuf = new char[random_read_bytes];
                file->read(fbuf, random_read_bytes);
                // |fbuf| will be freed by NoncontiguousBuffer.
                ::trpc::NoncontiguousBuffer data;
                TRPC_ASSERT(file->gcount() != 0);
                data.Append(fbuf, file->gcount());
                std::size_t bytes = data.ByteSize();
                TRPC_FMT_INFO("Current data bytes: {}", data.ByteSize());
                return rw->WriteData(std::move(data)).Then([state, bytes]() {
                  state->bytes += bytes;
                  ++state->counter;
                  return trpc::MakeReadyFuture<bool>(true);
                });
              } else {
                return rw->WriteDone().Then([state, file]() {
                  TRPC_FMT_INFO("Write eof, chunk num: {}, chunk bytes: {}", state->counter, state->bytes);
                  file->close();
                  return trpc::MakeReadyFuture<bool>(false);
                });
              }
            });
          });
        });
  });
}

class HttpdServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    auto http_async_service = std::make_shared<::trpc::stream::HttpAsyncStreamService>();
    http_async_service->RegisterAsyncStreamMethod(::trpc::http::MethodType::POST, ::trpc::http::Path("/upload"),
                                                  AsyncUploadWithChunk);
    http_async_service->RegisterAsyncStreamMethod(::trpc::http::MethodType::GET, ::trpc::http::Path("/download"),
                                                  AsyncDownloadWithChunk);

    RegisterService("http_async_upload_download_service", http_async_service);

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
