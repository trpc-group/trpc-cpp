# HTTP streaming calling (Upload/Download).

In HTTP streaming calling, both the client and the server can upload or download a big file.
Runtime requirements: Please use `Fiber` thread model.

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/http_upload_download/
examples/features/http_upload_download/
├── client
│   ├── BUILD
│   ├── download_client.cc
│   ├── trpc_cpp_fiber.yaml
│   └── upload_client.cc
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── file_storage_handler.cc
    ├── file_storage_handler.h
    ├── http_server.cc
    └── trpc_cpp_fiber.yaml
```

We can use the following script to quickly compile and run a program.
```shell
sh examples/features/http_upload_download/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
bazel build  //examples/features/http_upload_download/server:http_upload_download_server
bazel build  //examples/features/http_upload_download/client:download_client
bazel build  //examples/features/http_upload_download/client:upload_client
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/http_upload_download
$ mkdir -p examples/features/http_upload_download/build && cd examples/features/http_upload_download/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

We can run the following command to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
# Prepares some files to store data.
dd if=/dev/urandom of=download_src.bin bs=1M count=10
dd if=/dev/urandom of=upload_dst.bin bs=1M count=0

# Starts the HTTP server.
bazel-bin/examples/features/http_upload_download/server/http_upload_download_server --download_src_path="download_src.bin" --upload_dst_path="upload_dst.bin" --config=examples/features/http_upload_download/server/trpc_cpp_fiber.yaml
```

* Run the client program

We can run the following command to start the client program.

```shell
# Prepares some files to store data.
dd if=/dev/urandom of=download_dst.bin bs=1M count=0
dd if=/dev/urandom of=upload_src.bin bs=1M count=10

# Downloads a file by download_client.
bazel-bin/examples/features/http_upload_download/client/download_client --dst_path="download_dst.bin" --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml 
# Downloads a file by curl (optional).
curl http://127.0.0.1:24858/download -o curl_download_dst.bin

# Uploads a file by upload_client.
bazel-bin/examples/features/http_upload_download/client/upload_client --src_path="upload_src.bin" --use_chunked=true --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml 
bazel-bin/examples/features/http_upload_download/client/upload_client --src_path="upload_src.bin" --use_chunked=false --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml 
# Uploads a file by curl (optional).
curl http://127.0.0.1:24858/upload -v -T upload_src.bin -X POST  -H "Transfer-Encoding: chunked"
curl http://127.0.0.1:24858/upload -v -T upload_src.bin -X POST
```

The content of the output from the client program is as follows:
``` text
// Downlaods a file from the server:
finish downloading, read size: 10485760
name: download a file from the server, ok: 1
final result of http calling: 1

// Uploads a file to the server:
upload a file with chunked
finish uploading, write size: 10485760
name: upload a file to server, ok: 1
final result of http calling: 1

upload a file with content-length
finish uploading, write size: 10485760
name: upload a file to server, ok: 1
final result of http calling: 1
```




代码体验报告：基于 tRPC 的文件传输增强功能开发
一、项目概述
项目名称：tRPC 文件传输增强模块

开发时间：2025年7月

开发工具/语言：C++

项目目标：在学习和使用 tRPC 的基础上，扩展实现文件上传/下载功能，支持进度条显示、SHA256 校验、多文件流处理、限速下载等，提高文件传输的可靠性与用户体验。

二、开发环境
操作系统：ubuntu22.04

编程语言及版本：C++17

使用的框架/库：

tRPC

picosha2.h（用于 C++ 端 SHA256 哈希计算）

其他工具：

VS Code

三、功能实现

✅ 文件上传下载统计可视化功能

*在上传或下载过程中，实时显示进度条，提升用户体验

*在上传完成时自动记录总耗时与平均速率

*使用 C++ 构建控制台进度条，适用于服务端日志或 CLI 工具

``` text
[Download Request] 📏 Content-Length: 10485760 bytes
[Download Request] 📦 Progress: [█████                                             ] 10.00%
[Download Request] 📦 Progress: [██████████                                        ] 20.00%
[Download Request] 📦 Progress: [███████████████                                   ] 30.00%
[Download Request] 📦 Progress: [████████████████████                              ] 40.00%
[Download Request] 📦 Progress: [█████████████████████████                         ] 50.00%
[Download Request] 📦 Progress: [██████████████████████████████                    ] 60.00%
[Download Request] 📦 Progress: [███████████████████████████████████               ] 70.00%
[Download Request] 📦 Progress: [████████████████████████████████████████          ] 80.00%
[Download Request] 📦 Progress: [█████████████████████████████████████████████     ] 90.00%
[Download Request] 📦 Progress: [██████████████████████████████████████████████████] 100.00%
[Download Request] 📦 Progress: [██████████████████████████████████████████████████] 100.00%
[Download Request] 📥 Download complete: 10485760 bytes in 40 ms, avg speed: 250.00 MB/s
```


✅ SHA256 校验（使用 picosha2.h）

*在 C++ 模块中集成 picosha2.h 实现高效哈希计算

*确保文件上传/下载过程中的完整性，防止传输数据被篡改或破损

📥 计算 SHA256（文件端）

``` cpp
#include <fstream>
#include <vector>
#include "picosha2.h"  // 放入你的 include 路径

namespace http::demo {
    std::string CalculateSHA256(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) return "";

    std::vector<unsigned char> data;
    char buffer[8192];

    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytes_read = file.gcount();
        if (bytes_read > 0) {
        data.insert(data.end(), buffer, buffer + bytes_read);
        }
    }

    return picosha2::hash256_hex_string(data);
    }
}

```

📤 服务端添加 SHA256 校验头

``` cpp
std::string hash_tex = CalculateSHA256(download_src_path_);
rsp->SetHeader("X-File-SHA256", hash_tex);
TRPC_FMT_INFO("[Download Request] Calculated SHA256 of downloaded file: {}", hash_tex);

```

📦 客户端下载后进行校验

用户可选择是否启用本地校验，通过 FLAGS_enable_download_hash 控制

``` cpp
if (FLAGS_enable_download_hash && http_header.Has("X-File-SHA256")) {
    std::string expected_hash = http_header.Get("X-File-SHA256");
    std::string actual_hash = CalculateSHA256(dst_path);
    TRPC_FMT_INFO("downloaded client: {}", actual_hash);

    if (actual_hash != expected_hash) {
        TRPC_FMT_ERROR("❌ SHA256 mismatch! Expected={}, Actual={}", expected_hash, actual_hash);
        // 可触发错误处理流程
    } else {
        TRPC_FMT_INFO("✅ Verified {} with SHA256: {}", dst_path, actual_hash);
    }
}

```

``` text
[2025-07-28 16:11:35.358] [thread 241376] [info] [examples/features/http_upload_download/server/file_storage_handler.cc:392] ✅ Sent download_src.bin, size: 10485760, hash: 0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017
[Multi-File Download] 🔢 Received bytes: 10485760, Expected: 10485760
downloaded client: 0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017
[Download Request] ✅ SHA256 verified successfully

```
✅ 限速下载 

*控制下载速率，避免服务端带宽过载，同时为用户提供平稳的传输体验。

通过记录发送数据的时间和字节数，动态计算实际传输速率，并在必要时引入延迟（sleep）控制发送速率不超过设定阈值。

``` cpp
if (enable_limit && duration_ms > 0) {
    std::size_t actual_rate = sent_bytes * 1000 / duration_ms;  // bytes/sec
    
    if (actual_rate > rate_limit_bytes_per_sec) {
        std::size_t expected_duration = sent_bytes * 1000 / rate_limit_bytes_per_sec;
        std::size_t extra_delay = expected_duration > duration_ms
                                  ? expected_duration - duration_ms
                                  : 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(extra_delay));  // 延迟传输
    }

    last_send_time = std::chrono::steady_clock::now();  // 更新发送时间
    sent_bytes = 0;  // 清空计数器供下一轮使用
}


```
设置 constexpr std::size_t rate_limit_bytes_per_sec = 900 * 1024; // 512KB/s
``` text
[Download Request] 📦 Progress: [█████                                             ] 10.00%
[Download Request] 📦 Progress: [██████████                                        ] 20.00%
[Download Request] 📦 Progress: [███████████████                                   ] 30.00%
[Download Request] 📦 Progress: [████████████████████                              ] 40.00%
[Download Request] 📦 Progress: [█████████████████████████                         ] 50.00%
[Download Request] 📦 Progress: [██████████████████████████████                    ] 60.00%
[Download Request] 📦 Progress: [███████████████████████████████████               ] 70.00%
[Download Request] 📦 Progress: [████████████████████████████████████████          ] 80.00%
[Download Request] 📦 Progress: [█████████████████████████████████████████████     ] 90.00%
[Download Request] 📦 Progress: [██████████████████████████████████████████████████] 100.00%
[2025-07-28 16:48:59.654] [thread 244291] [info] [examples/features/http_upload_download/server/file_storage_handler.cc:137] [Download Request] finish providing file, write size: 10485760
[Download Request] 📦 Progress: [██████████████████████████████████████████████████] 100.00%
[Download Request] 📥 Download complete: 10485760 bytes in 11352 ms, avg speed: 0.88 MB/s
```

✅ 文件流式多文件传输（Streaming Multi-File Transfer）

*流式读取：使用 std::ifstream 按块读取，适合大文件处理

*逐个发送文件：每个文件构造独立的 header（包含文件名、大小、hash），确保接收端可以识别和校验

*边读取边发送：不加载整个文件到内存，避免资源占用过高，适合多大文件连续传输

*封装结构体头部：通过 FileSegmentHeader 发送结构化元信息，利于客户端解析与验证

*非阻塞串流发送：利用 writer.Write(...) 实现异步非阻塞写入到流通道

🧩 多文件识别与调度

``` cpp
const std::string full_url = req->GetUrl();
std::string path_only;
std::size_t pos = full_url.find('?');

if (pos != std::string::npos) {
  path_only = full_url.substr(0, pos);  
} else {
  path_only = full_url;
}

if (path_only == "/multi-download") {
  TRPC_FMT_INFO("[Multi-File Download] Start processing");
  return DownloadMultipleFiles(ctx, req, rsp);  // 🌟 调用多文件下载分发逻辑
}


```

🚚 多文件发送核心代码

``` cpp
for (const auto& path : file_paths) {
  std::string hash = CalculateSHA256(path);
  TRPC_FMT_INFO("[Multi-File Download] 📦 SHA256: {}", hash);

  std::ifstream fin(path, std::ios::binary | std::ios::ate);
  if (!fin.is_open()) {
    TRPC_FMT_ERROR("❌ Failed to open: {}", path);
    continue;
  }

  std::size_t file_size = fin.tellg();
  fin.seekg(0);

  FileSegmentHeader header;
  std::memset(&header, 0, sizeof(header));
  std::strncpy(header.filename, GetBaseName(path).c_str(), sizeof(header.filename));
  std::strncpy(header.hash_hex, hash.c_str(), sizeof(header.hash_hex));
  header.file_size = file_size;

  // ✉️ 发送 Header
  ::trpc::NoncontiguousBufferBuilder builder;
  builder.Append(static_cast<const void*>(&header), sizeof(header));
  writer.Write(builder.DestructiveGet());

  // 📤 分块发送文件内容
  while (fin) {[Multi-File Download] 🔍 Verifying download_src.bin, expected=0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017, actual=0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017
[Multi-File Download] ✅ Verified download_src.bin
    ::trpc::BufferBuilder buffer_builder;
    fin.read(buffer_builder.data(), buffer_builder.SizeAvailable());
    std::size_t n = fin.gcount();
    if (n > 0) {
      ::trpc::NoncontiguousBuffer buffer;
      buffer.Append(buffer_builder.Seal(n));
      if (!writer.Write(std::move(buffer)).OK()) {
        TRPC_FMT_ERROR("Write failed on file: {}", path);
        return ::trpc::kStreamRstStatus;
      }
    }
  }

  fin.close();
  TRPC_FMT_INFO("✅ Finished sending file: {}", header.filename);
}

writer.WriteDone();  // 标记传输结束


```

📦 FileSegmentHeader 示例结构说明

``` cpp

struct FileSegmentHeader {
  char filename[128];      // 文件名
  char hash_hex[64];       // SHA256 校验值（HEX格式）
  std::uint64_t file_size; // 文件大小（字节）
};

```

``` text 
[2025-07-28 16:11:35.358] [thread 241376] [info] [examples/features/http_upload_download/server/file_storage_handler.cc:392] ✅ Sent download_src.bin, size: 10485760, hash: 0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017
[Multi-File Download] 🔢 Received bytes: 10485760, Expected: 10485760
downloaded client: 0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017
[Multi-File Download] 🔢  File saved: download_src.bin, size: 10485760
[Multi-File Download] 🔍 Verifying download_src.bin, expected=0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017, actual=0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017
[Multi-File Download] ✅ Verified download_src.bin
[2025-07-28 16:11:35.583] [thread 241376] [info] [examples/features/http_upload_download/server/file_storage_handler.cc:392] ✅ Sent download_dst.bin, size: 10485760, hash: 0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017
[Multi-File Download] 🔢 Received bytes: 10485760, Expected: 10485760
[Multi-File Download] 🔢  File saved: download_dst.bin, size: 10485760
[Multi-File Download] 🔍 Verifying download_dst.bin, expected=0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017, actual=0959acfa65ad143b3b27a5a2f7d4f7e3fb2ac005df1d1ac7e27dae8177ab1017
[Multi-File Download] ✅ Verified download_dst.bin
```


四、开发过程体验
🧩 遇到的问题及解决方案

1. SHA256 校验值不一致问题

🧨 问题表现
服务端或客户端写入文件后立即调用 CalculateSHA256() 校验文件内容

获取到的哈希值与期望值不匹配（即使文件名和大小相同）

🎯 原因分析

文件流（如 std::ofstream fout）在写入时使用缓冲机制。如果未主动刷新或关闭，部分数据可能仍停留在缓冲区，尚未写入磁盘 。

调用 CalculateSHA256() 时读取的是磁盘文件内容，如果未 flush，会导致读取数据不完整，从而哈希值错误。

✅ 解决办法

在文件写入完成后，务必执行如下操作：

```cpp
fout.flush();   // 💡 强制刷新缓冲区
fout.close();   // ✅ 自动关闭文件 & 刷新数据

```
调用 CalculateSHA256() 校验之前，确保文件已完全写入磁盘。

2.添加限速下载 客户端出现超时 

🧨 问题表现

客户端下载添加如下代码，定位问题。

```cpp
    else if (status.ToString().find("timeout") != std::string::npos){
      TRPC_FMT_WARN("[Download Request] ⚠️ Read timed out, retrying...");
      continue;
    }
```

🎯 原因分析

在调试过程中，发现客户端在 stream.Read() 时长时间阻塞，怀疑与服务端限速策略有关。为验证和规避这一问题，查阅了 HttpReadStream 的流式同步接口文档，发现其支持以下两种读取方式：
### HttpReadStream 流式同步接口列表

| 对象类型      | 接口签名                                                                                                                                             | 功能说明                                                                                               | 参数说明                                                        | 返回值     |
|---------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------|------------------------------------------------------------------|------------|
| HttpReadStream | `Status Read(NoncontiguousBuffer& item, size_t max_bytes)`                                                                                          | 读取指定长度的数据，整体读取过程受服务端配置的 timeout 控制                                           | `max_bytes`：读取字节数，如果剩余内容不足则立即返回并标识 EOF  | `Status`   |
| HttpReadStream | `Status Read(NoncontiguousBuffer& item, size_t max_bytes, const std::chrono::time_point<Clock, Dur>& expiry)`                                        | 读取指定长度的数据，阻塞直到达到指定的时间点                                                           | `expiry`：如 `trpc::ReadSteadyClock() + std::chrono::milliseconds(3)` | `Status`   |


对比后采用了第二种接口形式，明确设置了读取操作的超时时间点，有效防止了长期阻塞。最终代码调整如下：

```cpp
status = stream.Read(buffer, kBufferSize, std::chrono::seconds(3));
```
通过配合 status.ToString().find("timeout") 的判断，实现了对限速场景的优雅处理，客户端逻辑更加健壮稳定。

✅ 解决办法

添加：status =  stream.Read(buffer, kBufferSize, std::chrono::seconds(3));

```cpp
    if(FLAGS_enable_download_limit)
      status =  stream.Read(buffer, kBufferSize, std::chrono::seconds(3));
    else
      status = stream.Read(buffer, kBufferSize);
```