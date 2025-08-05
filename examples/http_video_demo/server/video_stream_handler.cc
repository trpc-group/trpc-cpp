#include "examples/http_video_demo/server/video_stream_handler.h"

#include <fstream>

namespace trpc::examples::http_video_demo {

// Handles video downloading
::trpc::Status VideoStreamHandler::Get(const ::trpc::ServerContextPtr& ctx,
                                       const ::trpc::http::RequestPtr& req,
                                       ::trpc::http::Response* rsp) {
  auto fin = std::ifstream(video_src_path_, std::ios::binary);
  if (!fin.is_open()) {
    TRPC_FMT_ERROR("Failed to open video file for download: {}", video_src_path_);
    rsp->SetStatus(::trpc::http::ResponseStatus::kInternalServerError);
    return ::trpc::kSuccStatus;
  }
  rsp->SetStatus(::trpc::http::ResponseStatus::kOk);
  rsp->SetHeader(::trpc::http::kHeaderTransferEncoding, ::trpc::http::kTransferEncodingChunked);
  auto& writer = rsp->GetStream();
  ::trpc::Status status = writer.WriteHeader();
  if (!status.OK()) {
    TRPC_FMT_ERROR("Failed to write response header: {}", status.ToString());
    return ::trpc::kStreamRstStatus;
  }

  std::size_t nwrite{0};
  char buffer[1024 * 1024];
  while (fin.read(buffer, sizeof(buffer)) || fin.gcount() > 0) {
    ::trpc::NoncontiguousBuffer nb;
    nb.Append(buffer, fin.gcount());
    status = writer.Write(std::move(nb));
    if (!status.OK()) {
      TRPC_FMT_ERROR("Failed to stream video data: {}", status.ToString());
      return ::trpc::kStreamRstStatus;
    }
    nwrite += fin.gcount();
  }

  writer.WriteDone();
  TRPC_FMT_INFO("Finished serving video ({} bytes)", nwrite);
  return ::trpc::kSuccStatus;
}

// Handles video uploading
::trpc::Status VideoStreamHandler::Post(const ::trpc::ServerContextPtr& ctx,
                                        const ::trpc::http::RequestPtr& req,
                                        ::trpc::http::Response* rsp) {
  if (req->HasHeader(::trpc::http::kHeaderContentLength)) {
    TRPC_FMT_DEBUG("Content-Length: {}", req->GetHeader(::trpc::http::kHeaderContentLength));
  } else {
    TRPC_FMT_DEBUG("Chunked upload detected.");
  }

  auto fout = std::ofstream(video_dst_path_, std::ios::binary);
  if (!fout.is_open()) {
    TRPC_FMT_ERROR("Failed to open file for upload: {}", video_dst_path_);
    rsp->SetStatus(::trpc::http::ResponseStatus::kInternalServerError);
    return ::trpc::kSuccStatus;
  }

  constexpr std::size_t kBufferSize = 1024 * 1024;
  std::size_t nread{0};
  auto& reader = req->GetStream();
  ::trpc::Status status;

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
    TRPC_FMT_ERROR("Failed to read upload data: {}", status.ToString());
    return ::trpc::kStreamRstStatus;
  }

  rsp->SetStatus(::trpc::http::ResponseStatus::kOk);
  auto& writer = rsp->GetStream();
  status = writer.WriteHeader();
  if (!status.OK()) {
    TRPC_FMT_ERROR("Failed to send response header: {}", status.ToString());
    return ::trpc::kStreamRstStatus;
  }
  writer.WriteDone();

  TRPC_FMT_INFO("Video uploaded successfully ({} bytes)", nread);
  return ::trpc::kSuccStatus;
}

}  // namespace trpc::examples::http_video_demo