# HTTP Video Upload & Download Demo

This is a demo project built with Tencent tRPC (C++) demonstrating how to upload and download video files via HTTP streaming.

The example contains both a server and client implementation:

* **Server**: Handles video uploads (POST) and video downloads (GET).
* **Client**: Uploads a local video file and downloads it back from the server.

## Features

✅ Supports large file uploads with **chunked transfer encoding** and **content-length encoding**.

✅ Streams files for download without loading the entire file into memory.

✅ Example of tRPC HTTP service integration with streaming.

---

## Directory Structure

```
examples/http_video_demo/
├── client/
│   ├── video_uploader.cc       # Client to upload video files
│   ├── video_previewer.cc      # Client to download video files
│   └── trpc_cpp_fiber.yaml     # Client config
│
├── server/
│   ├── video_server.cc         # HTTP server setup
│   ├── video_stream_handler.cc # Handles upload & download requests
│   ├── video_stream_handler.h  # Header file for request handler
│   └── trpc_cpp_fiber.yaml     # Server config
│
└── run.sh                      # Script to build & run the demo
```

---

## Build Instructions

### 1. Build the Server and Client

Run the following command to build:

```bash
bazel build //examples/http_video_demo/server:video_server
bazel build //examples/http_video_demo/client:video_uploader
bazel build //examples/http_video_demo/client:video_previewer
```

Alternatively, use the provided script:

```bash
./example/http_video_demo/run.sh
```

This will build and run both the server and client.

---

## How It Works

1. **Server**

   * Starts and listens on the default port `8080`.
   * Accepts video uploads at endpoint:

     ```
     POST /video_upload?filename=<filename>
     ```
   * Provides video downloads at endpoint:

     ```
     GET /video_preview?filename=<filename>
     ```

2. **Client**

   * Uploads a video file in either chunked or content-length mode.
   * Downloads the video back from the server.

---

## Example Run

```bash
./examples/http_video_demo/run.sh
```

* This script:

  * Builds the server and client.
  * Starts the server.
  * Uploads a sample video file.
  * Downloads the same video back.

Expected output:

```
Starting video server...
Video server started successfully.
Uploading video to server (chunked mode)...
Upload completed successfully.
Downloading video from server...
Download completed successfully.
Stopping video server...
Done.
```

---

## Configuration

Configuration files are in YAML format:

* **Server**: `examples/http_video_demo/server/trpc_cpp_fiber.yaml`
* **Client**: `examples/http_video_demo/client/trpc_cpp_fiber.yaml`

You can modify them to change ports, logging levels, etc.

---

## Requirements

* [tRPC C++ SDK](https://github.com/Tencent/trpc-cpp)
* Bazel build system
* GCC >= 9

---

## License

Apache 2.0 - See LICENSE file.

---

## Related Examples

* [HTTP Upload/Download Demo](../features/http_upload_download)
* [Basic HTTP Service](../features/http_service)

---
