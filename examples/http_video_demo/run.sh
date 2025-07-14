#!/bin/bash

set -e

echo "Building server and client..."
bazel build //examples/http_video_demo/server:video_server
bazel build //examples/http_video_demo/client:video_uploader
bazel build //examples/http_video_demo/client:video_previewer

# Kill any existing server process
echo "Killing previous video_server process..."
killall video_server || true

# Prepare dummy video file
echo "Preparing dummy video file..."
dd if=/dev/urandom of=examples/http_video_demo/server/videos/my_video.mp4 bs=1M count=5

# Start video server
echo "Starting video server..."
bazel-bin/examples/http_video_demo/server/video_server --config=examples/http_video_demo/server/trpc_cpp_fiber.yaml &
server_pid=$!
sleep 1

if ps -p $server_pid > /dev/null; then
    echo "Video server started successfully, pid=$server_pid"
else
    echo "Failed to start video server"
    exit 1
fi

# Upload video (chunked mode)
echo "Uploading video to server (chunked mode)..."
if ! bazel-bin/examples/http_video_demo/client/video_uploader \
    --local_path=examples/http_video_demo/server/videos/my_video.mp4 \
    --filename=my_video_chunked.mp4 \
    --use_chunked=true; then
    echo "Chunked mode failed"
else
    echo "Chunked mode upload successful"
fi

# Upload video (content-length mode)
echo "Uploading video to server (content-length mode)..."
if ! bazel-bin/examples/http_video_demo/client/video_uploader \
    --local_path=examples/http_video_demo/server/videos/my_video.mp4 \
    --filename=my_video_contentlength.mp4 \
    --use_chunked=false; then
    echo "Content-length mode failed"
else
    echo "Content-length mode upload successful"
fi

# Download video (content-length upload)
echo "Downloading video from server (content-length upload)..."
bazel-bin/examples/http_video_demo/client/video_previewer \
    --local_path=examples/http_video_demo/client/downloaded_my_video_contentlength.mp4 \
    --filename=my_video_contentlength.mp4
    
# Stop video server
echo "Stopping video server..."
kill $server_pid
wait $server_pid 2>/dev/null || true
echo "Done."

