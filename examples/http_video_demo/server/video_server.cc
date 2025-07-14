#include <utility>

#include "gflags/gflags.h"

#include "trpc/common/trpc_app.h"
#include "trpc/server/http_service.h"
#include "trpc/util/http/routes.h"

#include "examples/http_video_demo/server/video_stream_handler.h"

DEFINE_string(video_src_path, "examples/http_video_demo/server/videos/my_video.mp4", "Path to source video file for download");
DEFINE_string(video_dst_path, "examples/http_video_demo/server/uploaded_videos/uploaded_video.mp4", "Path to store uploaded video file");

namespace trpc::examples::http_video_demo {

class VideoServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    auto video_handler = std::make_shared<VideoStreamHandler>(FLAGS_video_dst_path, FLAGS_video_src_path);

    auto SetHttpRoutes = [video_handler](::trpc::http::HttpRoutes& routes) {
      routes.Add(::trpc::http::MethodType::POST, ::trpc::http::Path("/upload"), video_handler);
      routes.Add(::trpc::http::MethodType::GET, ::trpc::http::Path("/download"), video_handler);
    };

    auto http_service = std::make_shared<::trpc::HttpService>();
    http_service->SetRoutes(SetHttpRoutes);

    RegisterService("video_stream_service", http_service);

    return 0;
  }

  void Destroy() override {}
};

}  // namespace trpc::examples::http_video_demo

int main(int argc, char** argv) {
  trpc::examples::http_video_demo::VideoServer video_server;

  video_server.Main(argc, argv);
  video_server.Wait();

  return 0;
}



  
  

