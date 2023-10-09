[English](../en/http_protocol_upload_download_client.md)

[TOC]

# 访问 HTTP 文件上传-下载服务

**主题：如何基于 tRPC-Cpp 访问 HTTP 文件上传-下载服务**

tRPC-Cpp 提供一套 HTTP 流式读取、写入数据分片的接口，可以分片接收、发送大文件。
本文介绍如何基于 tRPC-Cpp （下面简称 tRPC）访问 HTTP 文件上传-下载服务，开发者可以了解到如下内容：

* 如何使用同步流式接口访问文件上传-下载服务。
    * 编程接口。
    * 访问上传-下载服务。
* 如何使用异步流式接口访问文件上传-下载服务。
    * 编程接口。
    * 代码示例。
* FAQ。

# 如何使用同步流式接口访问文件上传-下载服务

访问 HTTP 文件上传、下载服务基于 `HttpServiceProxy`  和 `HttpClientStreamReaderWriter` 实现。
传输数据时，可以指定长度 `Content-Length: $length`，或者使用分块传输 `Transfer-Encoding: chunked` 。

提示：同步流式接口需要运行在 `fiber` 协程环境。

## 编程接口

### 客户端流读写器

通过 `HttpServiceProxy` 的下列接口获得流读写器，对象类型为 `HttpClientStreamReaderWriter`。
接口列表如下：

| 对象类型                         | 接口签名                                                                                       | 功能                                                      | 参数                                                                                              | 返回值          |
|------------------------------|--------------------------------------------------------------------------------------------|---------------------------------------------------------|-------------------------------------------------------------------------------------------------|--------------|
| HttpServiceProxy             | HttpClientStreamReaderWriter Get(const ClientContextPtr& context, const std::string& url)  | HTTP GET 方法，一般用来下载数据                                    | context: 请求上下文; url: HTTP URL                                                                   | HTTP 客户端流读写器 |
| HttpServiceProxy             | HttpClientStreamReaderWriter Post(const ClientContextPtr& context, const std::string& url) | HTTP POST 方法，一般用来上传数据                                   | context: 请求上下文; url: HTTP URL                                                                   | HTTP 客户端流读写器 |
| HttpServiceProxy             | HttpClientStreamReaderWriter Put(const ClientContextPtr& context, const std::string& url)  | HTTP PUT 方法，一般用来上传数据                                    | context: 请求上下文; url: HTTP URL                                                                   | HTTP 客户端流读写器 |
| HttpClientStreamReaderWriter | Status GetStatus()                                                                         | 获得读写器初始化后的状态                                            | -                                                                                               | Status       |
| HttpClientStreamReaderWriter | Status ReadHeaders(int&amp; code, HttpHeader&amp; http_header)                             | 获得响应头，使用 service 配置的 timeout+now 作为超时时间点阻塞该操作           | -                                                                                               | Status       |
| HttpClientStreamReaderWriter | Status ReadHeaders(int&amp; code, HttpHeader&amp; http_header，const T&amp; expiry)         | 获得响应头，阻塞该操作直到指定的时间                                      | expiry，例：trpc::ReadSteadyClock() + std::chrono::milliseconds(3) ，或 std::chrono::milliseconds(3) | Status       |
| HttpClientStreamReaderWriter | Status Read(NoncontiguousBuffer&amp; item, size_t max_bytes)                               | 读取指定长度的内容，使用 service配置的 timeout+now 作为整个读流过程的超时时间点阻塞该操作 | max_bytes，如果收完整包后的剩余内容不足指定长度，会立即返回剩余内容，并通过返回值标识 EOF                                             | Status       |
| HttpClientStreamReaderWriter | Status Read(NoncontiguousBuffer&amp; item, size_t max_bytes, const T&amp; expiry)          | 读取指定长度的内容，阻塞该操作直到指定的时间                                  | expiry，例：trpc::ReadSteadyClock() + std::chrono::milliseconds(3) ，或std::chrono::milliseconds(3)  | Status       |
| HttpClientStreamReaderWriter | Status ReadAll(NoncontiguousBuffer&amp; item)                                              | 读取整包内容，使用 service 配置的 timeout+now 作为整个读流过程的超时时间点阻塞该操作   | -                                                                                               | Status       |
| HttpClientStreamReaderWriter | Status ReadAll(NoncontiguousBuffer&amp; item, const T&amp; expiry)                         | 读取整包内容，阻塞该操作直到指定的时间                                     | expiry，例：trpc::ReadSteadyClock() + std::chrono::milliseconds(3) ，或std::chrono::milliseconds(3)  | Status       |
| HttpClientStreamReaderWriter | Status Write(NoncontiguousBuffer&amp;&amp; item)                                           | 发送内容                                                    | -                                                                                               | Status       |
| HttpClientStreamReaderWriter | Status WriteDone()                                                                         | 发送结束                                                    | -                                                                                               | Status       |
| HttpClientStreamReaderWriter | void Close()                                                                               | 关闭读写器接口                                                 | -                                                                                               | Status       |

### 客户端流接口常用返回码

超时类型的错误，业务可以尝试重试，但网络错误就不要再重试了，说明读写器所在的连接存在异常。

| 返回码                                        | 值   | 含义                           |
|--------------------------------------------|-----|------------------------------|
| kSuccStatus                                | 0   | 操作执行成功                       |
| kStreamStatusReadEof                       | -99 | 已读完所有内容                      |
| kStreamStatusClientReadTimeout             | 354 | 客户端读超时                       |
| kStreamStatusClientWriteTimeout            | 334 | 客户端写超时                       |
| kStreamStatusClientNetworkError            | 301 | 客户端网络错误                      |
| kStreamStatusClientWriteContentLengthError | 332 | 写数据长度和设置的 Content-Length 不匹配 |

### 读相关接口的超时时间说明

读相关接口：ReadHeaders，Read，ReadAll。
这里以 Read 接口为例，框架提供两种类型的具体接口形式：

- Read(item, max_bytes)，不带自定义超时时间。
  此类接口，超时时间点在创建读写器后就是一个固定值（由 service 配置或者 context 设置的 timeout 计算出来）。

> 例如 context->SetTimeout(60000)，记用户获得读写器的时间点为 now，那么用户后续无论怎么调用 Read(item, max_bytes)
> ，它的超时时间点一直是 now + 1min 不变，可以理解为整个读流过程的超时时间。

- Read(item, max_bytes, timeout)，带自定义超时时间。
  如果用户的数据很大，比如 10G 文件，网络情况也不确定，这种场景下建议用户使用 Read(item, max_bytes, timeout) 类的接口。

> 这里的 timeout 参数只针对该 Read 操作，timeout 类型可以为时间间隔，如 10s 则表示该次 Read 操作从触发开始阻塞
> 10s，也可以为具体的时间点，则表示该次 Read 操作阻塞到指定的时间点。

## 访问文件上传-下载服务

### 上传文件

示例： [upload_client.cc](../../examples/features/http_upload_download/client/upload_client.cc)

基本的数据上传过程需要经过如下几个步骤：设置长度形式/chunked 形式，发送请求头，读取响应头，写数据，完成写。

- **设置长度形式/chunked 形式**
  在创建 stream (客户端读写器)前，通过 client_context 设置 HTTP 发送头信息。长度形式和 chunked 形式二选一，如果已经完整数据长度，可以采用长度形式
  “Content-Length: 104857600”；如果不知道完整数据有多长，可以采用 chunked 形式 “Transfer-Encoding: chunked”。
- **发送请求头**
  客户端不需要用户进行发送请求头的动作，tRPC 也没有提供该方法。用户在获得 stream 时 tRPC 已经将请求头发送出去。
- **读取响应头**
  如果 ReadHeaders 接口执行成功，说明正常接收到服务端的响应头，从 http_code 参数能拿到 HTTP 状态码（200，404等），这些常量在
  tRPC 中也有定义，比如下面例子中的 ResponseStatus::kOk。从 http_header 参数能获取响应头。
- **写数据**
  通过 Write 接口，用户可以不断地向服务端发送数据分片。如果用户使用的是 chunked 形式，用户也不需要对传输数据做 chunked
  编码，tRPC 会自动处理。如果用户使用的是长度形式，一旦用户发送的数据超过了设置的长度，Write 接口会报
  kStreamStatusClientWriteContentLengthError 错误。
- **完成写**
  通过 WriteDone 接口，用户告知读写器数据全部发送完毕。如果用户使用的是 chunked 形式，框架会向服务端发送chunked结束标志；
  如果用户使用的是长度形式，框架会检查用户已发送的数据长度和设置的长度是否一致，不一致会报
  kStreamStatusClientWriteContentLengthError 错误。
  一旦调用 WriteDone 接口后，用户不应该再尝试使用 Write 接口。

简单的示例代码：

```cpp
// 使用 Chunked 方式上传文件
bool UploadWithChunked(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string src_path) {
  // 打开待上传的文件
  auto fin = std::ifstream(src_path, std::ios::binary);
  if (!fin.is_open()) {
    return false;
  }

  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(5000);
  // 设置 "Transfer-Encoding: chunked"
  ctx->SetHttpHeader(::trpc::http::kHeaderTransferEncoding, ::trpc::http::kTransferEncodingChunked);
  // 或者使用 "Content-Length: $length"
  // ctx->SetHttpHeader(::trpc::http::kHeaderContentLength, std::to_string(file_size));
  // 创建 HTTP 同步流对象
  auto stream = proxy->Post(ctx, url);
  if (!stream.GetStatus().OK()) {
    return false;
  }
  
  // 开始上传文件内容
  std::size_t nwrite{0};
  ::trpc::BufferBuilder buffer_builder;
  for (;;) {
    trpc::Status status;
    fin.read(buffer_builder.data(), buffer_builder.SizeAvailable());
    std::size_t n = fin.gcount();
    if (n > 0) {
      ::trpc::NoncontiguousBuffer buffer;
      buffer.Append(buffer_builder.Seal(n));
      // 发送文件内容
      status = stream.Write(std::move(buffer));
      if (status.OK()) {
        nwrite += n;
        continue;
      }
      return false;
    } else if (fin.eof()) {
      // 读取到待发送文件 EOF，结束发送
      status = stream.WriteDone();
      if (status.OK()) break;
      return false;
    }
    return false;
  }

  int http_status = 0;
  ::trpc::http::HttpHeader http_header;
  // 读取响应头部，并检查响应码
  ::trpc::Status status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    return false;
  } else if (http_status != ::trpc::http::ResponseStatus::kOk) {
    return false;
  }
  return true;
}
```

### 下载文件

示例： [download_client.cc](../../examples/features/http_upload_download/client/download_client.cc)

基本的数据下载过程需要经过如下步骤：发送请求头，读取响应头，读数据，完成读等步骤。

- **发送请求头**
  客户端不需要用户进行发送请求头的动作，tRPC 也没有提供该方法。用户在获得 stream 时 tRPC 已经将请求头发送出去。
- **读取响应头**
  如果 ReadHeaders 接口执行成功，说明正常接收到服务端的响应头，从 http_code 参数能拿到 HTTP 状态码（200，404等），这些常量
  tRPC 也有定义，比如下面例子中的 ResponseStatus::kOk。从 http_header 参数能获取到响应头。
- **读数据**
  Read(item, max_bytes)接口，如果服务端的回包数据没有结束，该次 Read 会一直阻塞到读出 max_bytes 长度的数据；如果服务端的回包数据已经结束，该次
  Read 操作读出 max_bytes 长度数据或者读出数据结尾都会立即返回。
- **完成读**
  当读出数据结尾时，Read 接口会返回 kStreamStatusReadEof 返回码，告知用户服务端数据已经全部读完。

简单的示例代码：

```cpp
// 下载文件
bool Download(const HttpServiceProxyPtr& proxy, const std::string& url, const std::string dst_path) {
  // 打开文件来保存待下载的数据
  auto fout = std::ofstream(dst_path, std::ios::binary);
  if (!fout.is_open()) {
    return false;
  }

  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(5000);
  // 创建 HTTP 同步流
  auto stream = proxy->Get(ctx, url);
  if (!stream.GetStatus().OK()) {
    return false;
  }

  // 读取响应头部
  int http_status = 0;
  ::trpc::http::HttpHeader http_header;
  ::trpc::Status status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    return false;
  } else if (http_status != ::trpc::http::ResponseStatus::kOk) {
    return false;
  }

  // 每次读取 1MB 数据
  constexpr std::size_t kBufferSize{1024 * 1024};
  size_t nread{0};
  // 读取响应消息体
  for (;;) {
    ::trpc::NoncontiguousBuffer buffer;
    status = stream.Read(buffer, kBufferSize);
    if (status.OK()) {
      nread += buffer.ByteSize();
      // 将非连续 Buffer 的数据块写入到文件中
      for (const auto& block : buffer) {
        fout.write(block.data(), block.size());
      }
      continue;
    } else if (status.StreamEof()) {
      // 流读取结束: EOF
      break;
    }
    return false;
  }
  return true;
}
```

# 如何使用异步流式接口访问文件上传-下载服务

HTTP 客户端异步流式接口基于 `HttpStreamProxy` 和 `HttpClientAsyncStreamReaderWriterPtr` 实现。
传输数据时，可以指定长度 `Content-Length: $length`，或者使用分块传输 `Transfer-Encoding: chunked` 。

要求：

- 异步流式接口需要运行在 `merge` 线程模型环境。`merge` 线程模型是 tRPC 的一种 runtime，线程角色既做 `IO`
  ，也做业务逻辑 `Handle`。
- 当前仅支持在 tRPC 内部线程调用异步流式接口，暂不支持在用户自定义的外部线程中使用（程序会 crash）。
- 采用 `future/promise` 编程。
- HTTP 消息读、写满足 HTTP 协议规范，也即对于请求和响应的写满足如下顺序：
    - 先写 start_line/status_line + headers。
    - 再写 body。

## 编程接口

### 客户端流读写器

- 调用 `HttpStreamProxy` 的 `GetAsyncStreamReaderWriter` 获得流读写器对象 (HttpClientAsyncStreamReaderWriterPtr）

接口列表如下：

| 接口签名                                                                                                    | 功能                                                                 | 参数                |
|---------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------|-------------------|
| Future&lt;HttpClientAsyncStreamReaderWriterPtr> GetAsyncStreamReaderWriter(const ClientContextPtr& ctx) | 获取异步流读写器                                                           | context: 客户端请求上下文 |
| Future<> WriteRequestLine(HttpRequestLine&& req_line)                                                   | 写入请求起始行                                                            | 参数                |
| Future<> WriteHeader(HttpHeader&& header)                                                               | 写入 header                                                          | header            |
| Future<> WriteData(NoncontiguousBuffer&& data)                                                          | 写入数据</br>将会识别 header 的信息，如果 header 里设置了 chunk，将会把数据组织成 chunk 格式再发送 | 数据                |
| Future<> WriteDone()                                                                                    | 写完数据</br>如果是 chunk 模式，将会写入 chunk 终止标识                              | -                 |

- 客户端从流里读出响应的接口（除了起始行，其余和服务端类似）

| 接口签名                                                                        | 功能                                                                                                                                                                                                   | 参数                      |
|-----------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------|
| Future&lt;HttpStatusLine> ReadStatusLine(int timeout = max)                 | 读出响应状态行                                                                                                                                                                                              | 超时时间（ms）                |
| Future&lt;HttpHeader> ReadHeader(int timeout = max)                         | 从流里读出 header                                                                                                                                                                                         | 超时时间（ms）                |
| Future&lt;NoncontiguousBuffer> ReadChunk(int timeout = max)                 | 从流里读出 chunk，只有确认是 chunk 模式才能调这个接口，否则将会失败（从 header 判断）                                                                                                                                                | 超时时间（ms)                |
| Future&lt;NoncontiguousBuffer> ReadAtMost(uint64_t len, int timeout = max)  | 长度模式/chunk模式均可调用，获得最多 len 长度的数据</br>如果从网络拿到的数据 size 比 len  小，返回 size 长度数据</br>如果从网络拿到的数据 size 比 len大，返回 len 长度的数据</br>buffer为空代表eof</br>场景说明1: 用于内存受限下，每次最多读限制的长度</br>场景说明2: 在中转模式下，能及时地拿到部分数据，发送给下游 | len 字节数，timeout超时时间（ms） |
| Future&lt;NoncontiguousBuffer> ReadExactly(uint64_t len, int timeout = max) | 长度模式/chunk模式均可调用，获得固定len长度的数据，如果读到eof，则网络有多少数据就返回多少</br>如果读出来的buffer大小，小于传入的len，那么代表读到了eof</br>特殊场景说明1: 请求数据按固定大小进行压缩，需要以固定大小读出，进行解压                                                                 | len 字节数，timeout超时时间（ms） |

- 客户端可用的写完整请求，读完整响应的接口

| 接口签名                                                                                                    | 功能        | 参数             |
|---------------------------------------------------------------------------------------------------------|-----------|----------------|
| Future<> WriteFullRequest(HttpClientAsyncStreamWriterPtr rw, HttpRequest&& req)                         | 往流里写入完整请求 | 客户端流读写器，超时（ms） |
| Future&lt;HttpResponsePtr> ReadFullResponse(HttpClientAsyncStreamReaderWriterPtr rw, int timeout = max) | 从流里读出完整响应 | 客户端流读写器，超时（ms） |

## 代码示例

待补充：中转代理 上传下载服务代码。

# FAQ

暂无
