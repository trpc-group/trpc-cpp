#ifndef EXAMPLES_HTTP_VIDEO_DEMO_SERVER_VIDEO_STREAM_HANDLER_H_
#define EXAMPLES_HTTP_VIDEO_DEMO_SERVER_VIDEO_STREAM_HANDLER_H_

#include "trpc/util/http/stream/http_stream_handler.h"

namespace trpc::examples::http_video_demo {

class VideoStreamHandler : public ::trpc::http::HttpStreamHandler {
 public:
  VideoStreamHandler(std::string dst, std::string src)
      : video_dst_path_(std::move(dst)), video_src_path_(std::move(src)) {}

  ~VideoStreamHandler() override = default;

  ::trpc::Status Get(const ::trpc::ServerContextPtr& ctx,
                     const ::trpc::http::RequestPtr& req,
                     ::trpc::http::Response* rsp) override;

  ::trpc::Status Post(const ::trpc::ServerContextPtr& ctx,
                      const ::trpc::http::RequestPtr& req,
                      ::trpc::http::Response* rsp) override;

 private:
  std::string video_dst_path_;
  std::string video_src_path_;
};

}  // namespace trpc::examples::http_video_demo

#endif  // EXAMPLES_HTTP_VIDEO_DEMO_SERVER_VIDEO_STREAM_HANDLER_H_
