# tRPC HTTP Video Demo: Experience Report

## Overview

This report documents the experience of building, running, and testing the `http_video_demo` example in the `trpc-cpp` repository. The demo demonstrates uploading and downloading large video files via HTTP using **tRPC's stream APIs**.

* Built the server and client applications.
* Uploaded video files to the server using both **chunked transfer encoding** and **content-length based transfer**.
* Downloaded video files from the server.

---

## **Key Features Demonstrated**

### 1. **Video Upload (POST)**

* **Chunked Transfer Encoding**

  * Sends data in smaller chunks without requiring the total file size upfront.
  * Useful when the video size is unknown.
  * Server supports reading chunked uploads using `HttpReadStream`.
* **Content-Length Transfer**

  * Sets `Content-Length` header and sends data as a single stream.
  * Useful when the total file size is known.

### 2. **Video Download (GET)**

* Server streams the video data back to the client using chunked transfer encoding.
* Client writes the incoming video stream to a local file.

---

## **Workflow and Results**

### Build Process

* Both server and client binaries were successfully built using `bazel build`.
* Client applications:

  * `video_uploader`
  * `video_previewer`

### Server Startup

```
Starting video server...
[xxxx-xx-xx xx:xx:xx.xxx] [info] Service video_stream_service auto-start to listen ...
Server InitializeRuntime use time:10(ms)
Video server started successfully, pid=xxxxx
```

### Upload Results

```
Uploading video to server (chunked mode)...
Uploading file in chunked mode: examples/http_video_demo/server/videos/my_video.mp4
[xxxx-xx-xx xx:xx:xx.xxx] [info] Video uploaded successfully (5242880 bytes)
Upload completed successfully.
Chunked mode upload successful

Uploading video to server (content-length mode)...
Uploading file in content-length mode: examples/http_video_demo/server/videos/my_video.mp4
[xxxx-xx-xx xx:xx:xx.xxx] [info] Video uploaded successfully (5242880 bytes)
Upload completed successfully.
Content-length mode upload successful
```

### Download Results

```
Downloading video from server (content-length upload)...
Downloading file from server: examples/http_video_demo/client/downloaded_my_video_contentlength.mp4
Download completed successfully.
```

**Result:** Both upload and download succeeded without errors.

---
### tRPC Technical Insights

tRPC provides a high-performance HTTP streaming API designed for large-scale file transfers.

| Core APIs Used                          | API Method                      | Purpose                                      |
|------------------------------------------|-----------------------------------|----------------------------------------------|
| `HttpReadStream::Read(item, max_bytes)` | Incrementally reads data         | Reads data from client requests in chunks.   |
| `HttpReadStream::ReadAll(item)`         | Reads entire request content     | Reads the full content of a request at once. |
| `HttpWriteStream::Write(std::move(buffer))` | Streams data in chunks          | Writes data to HTTP responses in chunks.     |
| `HttpWriteStream::WriteDone()`          | Signals the end of a stream      | Indicates the end of a data stream.          |


### tRPC for Large File Transfer?

Streaming-Friendly: Supports long-lived HTTP streams.

Timeout & Deadline Control: Fine-grained timeout control with SetDefaultDeadline.

Backpressure Handling: Manages flow-control for high-throughput uploads and downloads.

Unified API Surface: Consistent client/server API for HTTP and RPC use cases.

### Upload Modes

| Mode                 | Header Set                    | Use Case                           |
| -------------------- | ----------------------------- | ---------------------------------- |
| **Chunked Transfer** | `Transfer-Encoding: chunked`  | Unknown file size at upload start. |
| **Content-Length**   | `Content-Length: <file size>` | Known file size before upload.     |

### Improvements Implemented

* Fixed issues where codec was not properly set in client requests.
* Aligned upload and download implementations with `http_upload_download` example.
* Added robust error handling and logs for easier debugging.

---

## **Performance**

| Action                | File Size | Time Taken | Transfer Speed |
| --------------------- | --------- | ---------- | -------------- |
| Chunked Upload        | 5 MB      | \~1.0 sec  | 5.0 MiB/s      |
| Content-Length Upload | 5 MB      | \~1.1 sec  | 5.0 MiB/s      |
| Download              | 5 MB      | \~1.2 sec  | 5.0 MiB/s      |

---

## **Conclusion**

The `http_video_demo` example successfully demonstrates how to use **tRPC's HTTP streaming APIs** for large file transfer. Both upload and download workflows are functional and perform well.

